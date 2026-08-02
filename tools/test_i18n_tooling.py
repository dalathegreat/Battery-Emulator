#!/usr/bin/env python3
"""Host tests for the i18n tooling: the parts the C++ suite cannot reach.

Run: python3 tools/test_i18n_tooling.py

These cover gaps that let real defects through:
  - The factory-image tool stamped a directory version the firmware rejects,
    and its own verifier compared against the same stale constant, so it
    self-checked green while producing unmountable images.
  - The generator's promised "no code in translation cells" scan did not
    exist, and seven cells held entire alert('...') statements.
  - The append-only id check compared a regeneration against the same
    checkout, so a branch that renumbered and regenerated consistently passed.
"""
import json
import re
import struct
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))

import i18n_fs_image  # noqa: E402
import i18n_gen  # noqa: E402

failures = []


def check(name, condition, detail=''):
    if condition:
        print(f'  ok   {name}')
    else:
        print(f'  FAIL {name} {detail}')
        failures.append(name)


def store_constant(name):
    header = HERE.parent / 'Software' / 'src' / 'devboard' / 'i18n' / 'i18n_store.h'
    m = re.search(rf'{name}\s*=\s*(\d+)', header.read_text(encoding='utf-8'))
    return int(m.group(1)) if m else None


print('factory image')

# The tool must agree with the firmware about the directory version. mount()
# rejects a mismatch outright, so disagreement means every image it builds
# comes up as an unformatted store.
check('image version matches I18nStore::FORMAT_VERSION',
      i18n_fs_image.FORMAT_VERSION == store_constant('FORMAT_VERSION'),
      f'(tool {i18n_fs_image.FORMAT_VERSION} vs header {store_constant("FORMAT_VERSION")})')

# Parse the built directory the way I18nStore::mount() does, rather than
# calling the tool's own verify_image(), which shares the tool's constants.
catalogs = HERE / 'catalogs'
packs = sorted(catalogs.glob('*.blp'))
if not packs:
    print('  SKIP image build (run i18n_gen.py first to produce catalogs/)')
else:
    files = [(packs[0].name, packs[0].read_bytes())]
    image = i18n_fs_image.build_image(files, hint='sv')

    magic = image[0:4]
    version, sequence, count = struct.unpack_from('<HIH', image, 4)
    hint = image[12:12 + i18n_fs_image.HINT_SIZE].rstrip(b'\x00').decode()
    header_size = i18n_fs_image.DIR_HEADER_SIZE
    entry_size = 36
    payload_len = header_size + count * entry_size
    stored_crc = struct.unpack_from('<I', image, payload_len)[0]

    check('directory magic', magic == b'I18S', magic)
    check('directory version is what mount() accepts',
          version == store_constant('FORMAT_VERSION'), version)
    check('sequence is non-zero', sequence >= 1, sequence)
    check('entry count', count == len(files), count)
    check('language hint round-trips', hint == 'sv', hint)
    check('directory CRC over payload', zlib.crc32(image[:payload_len]) == stored_crc)
    check('header size matches firmware', header_size == 12 + i18n_fs_image.HINT_SIZE)

    # Extent must land inside the image and hold the bytes we asked for
    raw_name, off, length, crc = struct.unpack_from('<24sIII', image, header_size)
    name = raw_name.rstrip(b'\x00').decode()
    check('entry name', name == files[0][0], name)
    check('extent starts at or after DATA_START', off >= i18n_fs_image.DATA_START, off)
    check('extent inside image', off + length <= len(image))
    check('extent content matches source', image[off:off + length] == files[0][1])
    check('entry crc', zlib.crc32(files[0][1]) == crc)

print('generator content scan')

_orig_langs = i18n_gen.LANGS


def entry(key, en, **rest):
    # 'en' is itself in LANGS, so fill the others without clobbering it
    e = {'key': key, 'en': en}
    for lang in _orig_langs:
        if lang != 'en':
            e[lang] = rest.get(lang, '')
    return e


sidecar = HERE / 'i18n_ids.json'
code_errors = i18n_gen.run_checks([entry('UI_X', "alert('boom');")], sidecar)
check('rejects a code-bearing English cell',
      any('code-like' in e for e in code_errors), code_errors)

code_errors_sv = i18n_gen.run_checks([entry('UI_X', 'Saved', sv="document.cookie")], sidecar)
check('rejects code in a translated cell',
      any('code-like' in e for e in code_errors_sv), code_errors_sv)

clean = i18n_gen.run_checks([entry('UI_X', 'Update successful!', sv='Uppdateringen lyckades!')], sidecar)
check('accepts ordinary message text',
      not any('code-like' in e for e in clean), clean)

# The real catalogs contain "100 % des Vollstroms" and Hungarian "100%-nál";
# the printf scan must not fire on those.
prose = i18n_gen.run_checks(
    [entry('UI_X', 'Full power until 95%', de='100 % des Vollstroms', hu='100%-nál')], sidecar)
check('printf scan tolerates percent in prose',
      not any('printf' in e for e in prose), prose)
fmt = i18n_gen.run_checks([entry('UI_X', 'Value %.2f volts')], sidecar)
check('printf scan still catches a real specifier',
      any('printf' in e for e in fmt), fmt)

print('append-only against the base branch')

base = {'next_id': 3, 'ids': {'A': 0, 'B': 1}, 'tombstones': {'C': 2}}
same = i18n_gen.check_append_only_against(base, base)
check('identical sidecar passes', same == [], same)

renumbered = {'next_id': 3, 'ids': {'A': 1, 'B': 0}, 'tombstones': {'C': 2}}
check('renumbered live keys rejected',
      i18n_gen.check_append_only_against(base, renumbered) != [])

reused = {'next_id': 3, 'ids': {'A': 0, 'B': 1, 'D': 2}, 'tombstones': {}}
check('reusing a tombstoned id rejected',
      i18n_gen.check_append_only_against(base, reused) != [])

rewound = {'next_id': 2, 'ids': {'A': 0, 'B': 1}, 'tombstones': {'C': 2}}
check('next_id going backwards rejected',
      i18n_gen.check_append_only_against(base, rewound) != [])

added = {'next_id': 4, 'ids': {'A': 0, 'B': 1, 'E': 3}, 'tombstones': {'C': 2}}
check('appending a new key passes',
      i18n_gen.check_append_only_against(base, added) == [])

dup = {'next_id': 4, 'ids': {'A': 0, 'B': 0}, 'tombstones': {}}
check('duplicate ids in the sidecar rejected',
      i18n_gen.check_sidecar_ids_unique(dup) != [])

print()
if failures:
    print(f'{len(failures)} failure(s): {failures}')
    sys.exit(1)
print('all tooling tests passed')
