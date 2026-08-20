#!/usr/bin/env python3
"""Insert rows into translations.fods, keeping the sheet sorted by key.

The generator asserts the sheet is key-sorted and that ids are append-only, so
rows go in at their sorted position and never get renumbered. Reads a JSON
list on stdin: [{"key":..., "comment":..., "en":..., "de":..., ...}, ...]

Usage: python3 tools/add_i18n_key.py < newkeys.json
"""
import json
import re
import sys
from pathlib import Path

LANGS = ['en', 'de', 'fi', 'fr', 'hu', 'nl', 'sv', 'uk']
FODS = Path(__file__).parent / 'translations.fods'


def esc(t):
    return (t.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;'))


def cell(text):
    if not text:
        return '<table:table-cell/>'
    return f'<table:table-cell office:value-type="string"><text:p>{esc(text)}</text:p></table:table-cell>'


def make_row(entry):
    cells = [cell(entry['key']), cell(entry.get('comment', ''))]
    cells += [cell(entry.get(l, '')) for l in LANGS]
    return '<table:table-row>' + ''.join(cells) + '</table:table-row>'


def main():
    entries = json.load(sys.stdin)
    s = FODS.read_text(encoding='utf-8')

    row_re = re.compile(r'<table:table-row[^>]*>.*?</table:table-row>', re.S)
    rows = [(m.start(), m.end(), m.group(0)) for m in row_re.finditer(s)]
    key_of = lambda row: (re.search(r'<text:p>(.*?)</text:p>', row) or [None, ''])[1]

    existing = {key_of(r[2]) for r in rows}
    added = 0
    for entry in sorted(entries, key=lambda e: e['key']):
        if entry['key'] in existing:
            print(f'skip (already present): {entry["key"]}')
            continue
        # Re-scan each time: every insert shifts the offsets after it
        rows = [(m.start(), m.end(), m.group(0)) for m in row_re.finditer(s)]
        insert_at = None
        for start, end, row in rows[1:]:  # row 0 is the header
            if key_of(row) > entry['key']:
                insert_at = start
                break
        if insert_at is None:
            insert_at = rows[-1][1]
        s = s[:insert_at] + make_row(entry) + s[insert_at:]
        existing.add(entry['key'])
        added += 1

    FODS.write_text(s, encoding='utf-8')
    print(f'added {added} key(s)')


if __name__ == '__main__':
    main()
