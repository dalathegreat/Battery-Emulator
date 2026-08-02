#!/usr/bin/env python3
"""Build a factory image of the i18n slot store (see i18n_store.h).

Emits a .bin to flash at the spiffs partition offset so devices ship with
language catalogs pre-installed. Layout must byte-match I18nStore:
directory block A at 0 (magic 'I18S', version, sequence, count, 36-byte
entries, CRC-32 over the payload), block B at 4096 left erased (0xFF),
data extents at 4 KB granularity from 8192.

Packs are complete by construction (the generator fills untranslated cells
with English), so the shipping form is a SINGLE-language image per the
single-pack model: `--langs <lang>` (en not required). Multi-language images
remain an option for the 8/16 MB boards.

Usage: python3 i18n_fs_image.py [--langs sv] [--blp-only] [--out i18n_factory.bin]
Run after i18n_gen.py has produced catalogs/.
"""
import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

SECTOR = 4096
DATA_START = 8192
MAX_ENTRIES = 16
def _format_version_from_header():
    """Read I18nStore::FORMAT_VERSION out of the firmware header.

    Derived rather than duplicated: this constant was hardcoded to 1 while the
    firmware moved to 2, so every image this tool built mounted as an
    unformatted store - and verify_image() compared against the same stale
    copy, which is why it self-checked green. A value that cannot drift is
    better than a value that is checked.
    """
    header = Path(__file__).parent.parent / 'Software' / 'src' / 'devboard' / 'i18n' / 'i18n_store.h'
    m = re.search(r'FORMAT_VERSION\s*=\s*(\d+)', header.read_text(encoding='utf-8'))
    if not m:
        raise SystemExit(f'cannot find FORMAT_VERSION in {header}')
    return int(m.group(1))


FORMAT_VERSION = _format_version_from_header()
ENTRY_SIZE = 36  # char name[24] + u32 offset + u32 length + u32 crc32
HINT_SIZE = 8
DIR_HEADER_SIZE = 12 + HINT_SIZE


def round_up_sector(n):
    return (n + SECTOR - 1) & ~(SECTOR - 1)


def build_image(files, hint=''):
    """files: list of (store_name, bytes). Returns the image as bytes."""
    if len(hint.encode()) >= HINT_SIZE:
        sys.exit(f'hint too long: {hint}')
    if len(files) > MAX_ENTRIES:
        sys.exit(f'too many files: {len(files)} > {MAX_ENTRIES}')
    entries = []
    offset = DATA_START
    for name, data in files:
        if len(name.encode()) >= 24:
            sys.exit(f'name too long: {name}')
        entries.append((name, offset, len(data), zlib.crc32(data) & 0xFFFFFFFF))
        offset += round_up_sector(len(data))

    image = bytearray(b'\xff' * offset)

    # Directory block A, sequence 1; block B stays erased so the device's
    # first commit lands there with sequence 2
    payload = bytearray()
    payload += b'I18S'
    payload += struct.pack('<HIH', FORMAT_VERSION, 1, len(entries))
    payload += struct.pack(f'<{HINT_SIZE}s', hint.encode())
    assert len(payload) == DIR_HEADER_SIZE
    for name, off, length, crc in entries:
        payload += struct.pack('<24sIII', name.encode(), off, length, crc)
    payload += struct.pack('<I', zlib.crc32(payload) & 0xFFFFFFFF)
    image[0:len(payload)] = payload

    for (name, off, length, crc), (_, data) in zip(entries, files):
        image[off:off + length] = data
    return bytes(image)


def verify_image(image, files, hint=''):
    """Re-parse the image the way I18nStore::mount does."""
    assert image[0:4] == b'I18S'
    version, sequence, count = struct.unpack_from('<HIH', image, 4)
    assert version == FORMAT_VERSION and sequence == 1 and count == len(files)
    assert image[12:12 + HINT_SIZE].rstrip(b'\x00').decode() == hint
    payload_len = DIR_HEADER_SIZE + count * ENTRY_SIZE
    (stored_crc,) = struct.unpack_from('<I', image, payload_len)
    assert zlib.crc32(image[:payload_len]) & 0xFFFFFFFF == stored_crc, 'directory CRC'
    for i, (name, data) in enumerate(files):
        raw_name, off, length, crc = struct.unpack_from('<24sIII', image, DIR_HEADER_SIZE + i * ENTRY_SIZE)
        assert raw_name.rstrip(b'\x00').decode() == name
        assert length == len(data) and image[off:off + length] == data
        assert zlib.crc32(data) & 0xFFFFFFFF == crc, f'{name} CRC'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--langs', default='en',
                        help='comma-separated; single language is the shipping default')
    parser.add_argument('--hint', default=None,
                        help='preferred-language hint adopted on first boot '
                             '(default: first non-en language in --langs)')
    parser.add_argument('--with-json', action='store_true',
                        help='also include .json.gz catalogs (build artifacts '
                             'for a future keyed frontend; .blp is THE '
                             'language file and the default image content)')
    parser.add_argument('--out', default='i18n_factory.bin')
    args = parser.parse_args()

    langs = args.langs.split(',')
    hint = args.hint
    if hint is None:
        hint = next((l for l in langs if l != 'en'), '')  # en-only image: no hint

    here = Path(__file__).parent
    files = []
    exts = ('.json.gz', '.blp') if args.with_json else ('.blp',)
    for lang in langs:
        for ext in exts:
            path = here / 'catalogs' / f'{lang}{ext}'
            if not path.exists():
                sys.exit(f'missing {path} - run i18n_gen.py first')
            files.append((f'{lang}{ext}', path.read_bytes()))

    image = build_image(files, hint)
    verify_image(image, files, hint)
    Path(args.out).write_bytes(image)
    print(f'{args.out}: {len(image)} bytes, hint={hint!r}, {len(files)} files '
          f'({", ".join(n for n, _ in files)})')


if __name__ == '__main__':
    main()
