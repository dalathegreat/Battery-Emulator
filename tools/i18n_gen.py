#!/usr/bin/env python3
"""i18n generator: translations.fods -> i18n_keys.h + <lang>.json.gz + <lang>.blp.

Maintains the machine-owned, append-only id sidecar (tools/i18n_ids.json):
ids are allocated once, never reused; keys removed from the sheet become
tombstones. CI mode (--check) verifies sheet ordering, key uniqueness, token
integrity against English, absence of printf specifiers, and that the sidecar
only grew relative to its committed state.

Binary pack (.blp) layout, all little-endian:
  magic 'BLP1' | u16 version | u16 count | u16 max_id | u16 reserved
  count x (u16 id, u32 offset)   -- sorted by id, offsets into the blob
  blob: NUL-terminated UTF-8 strings, laid out in index order
"""
import gzip
import json
import re
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

NS = {
    'table': 'urn:oasis:names:tc:opendocument:xmlns:table:1.0',
    'text': 'urn:oasis:names:tc:opendocument:xmlns:text:1.0',
}
LANGS = ['en', 'de', 'fi', 'fr', 'hu', 'nl', 'sv', 'uk']  # Must match sheet column order
LANGUAGE_NAMES = {'en': 'English', 'de': 'Deutsch', 'fi': 'Suomi', 'fr': 'Français',
                  'hu': 'Magyar', 'nl': 'Nederlands', 'sv': 'Svenska', 'uk': 'Українська'}
TOKEN_RE = re.compile(r'\{\d\}')
# A '%' directly followed by a printf conversion char is forbidden in
# translated text ("50%" and "% drift" are fine; TR text never reaches printf
# anyway - expand() substitutes literally - this is defense in depth)
# Width/precision/length are covered (%.2f, %5d, %ld, %08x). The space and
# '-' flags are deliberately NOT: they collide with ordinary prose in the
# real catalogs ("100 % des Vollstroms" -> "% d", Hungarian "100%-nál"
# -> "%-n"), and a left-justified specifier in a UI label is not a thing.
PRINTF_RE = re.compile(r'%[+#0]*\d*(?:\.\d+)?(?:hh|h|ll|l|j|z|t|L)?[diouxXeEfFgGaAcspn]')
# Catalog cells hold message text, never code. Seven entries once held whole
# alert('...') statements, which meant a language pack could run script in
# the admin session by construction - no escaping fixes a value that IS code.
CODE_RE = re.compile(r'alert\(|confirm\(|prompt\(|eval\(|xhr\.|document\.|location\.|'
                     r'<\s*script|javascript:|function\s*\(|=>', re.I)

# Bootstrap subset: keys the recovery/AP path needs compiled in.
# Curated list, small and reviewable (see i18n-implementation.md item 3).
# Fail-safe strings compiled in (user scope principle 2026-07-31): WiFi/network
# panel + Languages panel + save/reboot + upload flow + minimal navigation.
# Web-auth panel excluded - the browser shows its native login prompt anyway.
BOOTSTRAP_KEYS = [
    'UI_ACCESS_POINT_ACTIVE', 'UI_ACCESS_POINT_PASSWORD', 'UI_ACTIVE_LANGUAGE',
    'UI_BACK_MAIN_PAGE', 'UI_BROADCAST_WIFI_AP', 'UI_CHANGE_SETTINGS', 'UI_DELETE',
    'UI_ENGLISH_BUILT_IN', 'UI_FILE_UPLOADED_SUCCESSFULLY', 'UI_FORMAT_FAILED',
    'UI_FORMAT_LANGUAGE_STORAGE', 'UI_HOSTNAME', 'UI_LANGUAGES', 'UI_NETWORK_CONFIG',
    'UI_NO_SSID_AVAILABLE', 'UI_NO_SUCH_LANGUAGE', 'UI_PASSWORD', 'UI_READY_RECEIVE_FILE',
    'UI_REBOOT', 'UI_REBOOT_EMULATOR', 'UI_SETTINGS_SAVED_REBOOT', 'UI_SSID',
    'UI_UPLOAD', 'UI_UPLOAD_REJECTED', 'UI_WIFI_STATE',
]


def parse_fods(path):
    root = ET.parse(path).getroot()
    table = root.find('.//table:table', NS)
    rows = []
    for row in table.findall('table:table-row', NS):
        cells = []
        for cell in row.findall('table:table-cell', NS):
            repeat = int(cell.get('{%s}number-columns-repeated' % NS['table'], '1'))
            texts = cell.findall('text:p', NS)
            value = '\n'.join(''.join(p.itertext()) for p in texts) if texts else ''
            cells.extend([value] * min(repeat, 16))
        while cells and cells[-1] == '':
            cells.pop()
        if cells:
            rows.append(cells)
    header, data = rows[0], rows[1:]
    expected = ['key', 'comment'] + LANGS
    if header[:len(expected)] != expected:
        sys.exit(f'sheet header mismatch: {header}')
    entries = []
    for cells in data:
        cells += [''] * (len(expected) - len(cells))
        entries.append({'key': cells[0], 'comment': cells[1],
                        **{lang: cells[2 + i] for i, lang in enumerate(LANGS)}})
    return entries


def run_checks(entries, sidecar_path):
    errors = []
    keys = [e['key'] for e in entries]
    if keys != sorted(keys):
        errors.append('sheet is not sorted by key')
    if len(keys) != len(set(keys)):
        errors.append('duplicate keys in sheet')
    for e in entries:
        if not e['en']:
            errors.append(f"{e['key']}: empty English")
        en_tokens = set(TOKEN_RE.findall(e['en']))
        for lang in LANGS:
            text = e[lang]
            if text and set(TOKEN_RE.findall(text)) != en_tokens:
                errors.append(f"{e['key']}: token mismatch in {lang}")
            if text and PRINTF_RE.search(text):
                errors.append(f"{e['key']}: printf specifier in {lang}")
            if text and CODE_RE.search(text):
                errors.append(f"{e['key']}: code-like content in {lang}")
    # A bootstrap key that is renamed or dropped in the sheet silently
    # shrinks the generated table: the firmware still compiles, and the key
    # renders as its [T<id>] marker in the recovery path - the one state
    # where a readable UI matters most.
    key_set = set(keys)
    for key in BOOTSTRAP_KEYS:
        if key not in key_set:
            errors.append(f'bootstrap key {key} is missing from the sheet')
    return errors


def check_sidecar_ids_unique(sidecar):
    """An id shared between a live key and a tombstone, or between two live
    keys, silently makes every installed pack serve the wrong string."""
    errors = []
    seen = {}
    for scope in ('ids', 'tombstones'):
        for key, value in sidecar.get(scope, {}).items():
            if value in seen:
                errors.append(f'id {value} used by both {seen[value]} and {key}')
            seen[value] = key
    return errors


def check_append_only_against(base_sidecar, sidecar):
    """Append-only vs the BASE branch, not vs a regeneration of the same
    checkout. Regenerating consistently after renumbering is self-consistent
    and would otherwise pass."""
    errors = []
    base_ids = dict(base_sidecar.get('ids', {}))
    base_ids.update(base_sidecar.get('tombstones', {}))
    now_ids = dict(sidecar.get('ids', {}))
    now_ids.update(sidecar.get('tombstones', {}))
    for key, old_id in base_ids.items():
        if key in now_ids and now_ids[key] != old_id:
            errors.append(f'{key}: id changed {old_id} -> {now_ids[key]}')
    used_now = {v: k for k, v in now_ids.items()}
    for key, old_id in base_ids.items():
        holder = used_now.get(old_id)
        if holder is not None and holder != key:
            errors.append(f'id {old_id} (was {key}) reused by {holder}')
    if sidecar.get('next_id', 0) < base_sidecar.get('next_id', 0):
        errors.append('next_id decreased')
    return errors


def load_sidecar(path):
    if path.exists():
        return json.loads(path.read_text())
    return {'next_id': 0, 'ids': {}, 'tombstones': {}}


def update_sidecar(sidecar, keys):
    for key in keys:
        if key in sidecar['tombstones']:
            sidecar['ids'][key] = sidecar['tombstones'].pop(key)
        elif key not in sidecar['ids']:
            sidecar['ids'][key] = sidecar['next_id']
            sidecar['next_id'] += 1
    for key in list(sidecar['ids']):
        if key not in keys:
            sidecar['tombstones'][key] = sidecar['ids'].pop(key)
    return sidecar


def check_append_only(old, new):
    errors = []
    for key, kid in old['ids'].items():
        if new['ids'].get(key, new['tombstones'].get(key)) != kid:
            errors.append(f'id changed or lost for {key}')
    for key, kid in old['tombstones'].items():
        if new['tombstones'].get(key, new['ids'].get(key)) != kid:
            errors.append(f'tombstone id changed for {key}')
    if new['next_id'] < old['next_id']:
        errors.append('next_id went backwards')
    return errors


def emit_keys_header(entries, ids, path):
    lines = ['// GENERATED by tools/i18n_gen.py - do not edit', '#ifndef _I18N_KEYS_H_',
             '#define _I18N_KEYS_H_', '', '#include <stdint.h>', '',
             '// clang-format off',
             'enum class TrKey : uint16_t {']
    for e in entries:
        lines.append(f"  {e['key']} = {ids[e['key']]},")
    max_id = max(ids[e['key']] for e in entries) if entries else 0
    lines += ['};', '// clang-format on', '', f'constexpr uint16_t TR_MAX_ID = {max_id};',
              f'constexpr uint16_t TR_KEY_COUNT = {len(entries)};', '', '#endif', '']
    path.write_text('\n'.join(lines), encoding='utf-8', newline='\n')


def emit_bootstrap(entries, ids, path):
    subset = [e for e in entries if e['key'] in BOOTSTRAP_KEYS]
    lines = ['// GENERATED by tools/i18n_gen.py - do not edit', '#ifndef _I18N_BOOTSTRAP_H_',
             '#define _I18N_BOOTSTRAP_H_', '', '#include "i18n_keys.h"', '',
             'struct TrBootstrapEntry {', '  uint16_t id;', '  const char* text;', '};', '',
             '// Recovery-path strings that must exist without any language pack',
             'inline constexpr TrBootstrapEntry TR_BOOTSTRAP[] = {']
    for e in sorted(subset, key=lambda x: ids[x['key']]):
        text = e['en'].replace('\\', '\\\\').replace('"', '\\"')
        lines.append(f'    {{{ids[e["key"]]}, "{text}"}},')
    if not subset:
        lines.append('    {0xFFFF, ""},  // sentinel: empty array is not valid C++')
    lines += ['};', '',
              f'constexpr uint16_t TR_BOOTSTRAP_COUNT = {len(subset)};', '', '#endif', '']
    path.write_text('\n'.join(lines), encoding='utf-8', newline='\n')


def emit_catalogs(entries, ids, outdir):
    for lang in LANGS:
        # English-keyed json.gz for keyed frontends
        catalog = {'_language_name': LANGUAGE_NAMES[lang]}
        for e in entries:
            if e[lang]:
                catalog[e['en']] = e[lang]
        data = json.dumps(catalog, ensure_ascii=False, separators=(',', ':')).encode()
        (outdir / f'{lang}.json.gz').write_bytes(gzip.compress(data, 9, mtime=0))
        # Binary pack for the firmware TR() runtime
        pairs = sorted((ids[e['key']], (e[lang] or e['en'])) for e in entries)
        blob = b''
        index = b''
        for kid, text in pairs:
            index += struct.pack('<HI', kid, len(blob))
            blob += text.encode() + b'\0'
        max_id = pairs[-1][0] if pairs else 0
        header = b'BLP1' + struct.pack('<HHHH', 1, len(pairs), max_id, 0)
        (outdir / f'{lang}.blp').write_bytes(header + index + blob)


def main():
    here = Path(__file__).parent
    check_only = '--check' in sys.argv
    entries = parse_fods(here / 'translations.fods')
    errors = run_checks(entries, here / 'i18n_ids.json')
    old_sidecar = load_sidecar(here / 'i18n_ids.json')
    import copy
    new_sidecar = update_sidecar(copy.deepcopy(old_sidecar), [e['key'] for e in entries])
    errors += check_append_only(old_sidecar, new_sidecar)
    errors += check_sidecar_ids_unique(new_sidecar)
    # Append-only has to be judged against the BASE branch: comparing a
    # regeneration to the committed sidecar in the same checkout passes for a
    # branch that renumbered ids and regenerated consistently.
    for arg in sys.argv:
        if arg.startswith('--check-append-only-against='):
            base_path = Path(arg.split('=', 1)[1])
            errors += check_append_only_against(json.loads(base_path.read_text(encoding='utf-8')), new_sidecar)
    if errors:
        print('\n'.join(errors))
        sys.exit(1)
    if check_only:
        print(f'OK: {len(entries)} keys, next_id {new_sidecar["next_id"]}')
        return
    (here / 'i18n_ids.json').write_text(json.dumps(new_sidecar, indent=1, sort_keys=True),
                                        encoding='utf-8', newline='\n')
    ids = new_sidecar['ids']
    src = here.parent / 'Software' / 'src' / 'devboard' / 'i18n'
    emit_keys_header(entries, ids, src / 'i18n_keys.h')
    emit_bootstrap(entries, ids, src / 'i18n_bootstrap.h')
    outdir = here / 'catalogs'
    outdir.mkdir(exist_ok=True)
    emit_catalogs(entries, ids, outdir)
    print(f'generated: {len(entries)} keys, max_id {max(ids.values())}')


if __name__ == '__main__':
    main()
