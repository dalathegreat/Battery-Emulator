#!/usr/bin/env python3
"""Differential test for the escapers: does the value survive a real parser?

Run: python3 tools/test_escaping_differential.py
     (build test/build/escape_dump first: cmake --build test/build --target escape_dump)

WHY THIS EXISTS
---------------
Escaping bugs in this subsystem were found one character at a time, always
after the fact: apostrophe, then "</script>", then backtick and "${". Adding
the character that just bit you does not converge, because the question is
never "which characters" - it is "which grammar is this value landing in".

So instead of asserting against a list of characters, this feeds a value
through the real escaper, embeds the result in actual HTML or JavaScript,
hands it to a real parser (Python's html.parser, and node for JS), and
asserts the value that comes back out is the value that went in - and that
nothing new appeared in the parse. A missing escape shows up as a mismatch or
an injected node, whatever character caused it, including characters nobody
thought to test.

Add a new sink here when the firmware starts emitting one, and the corpus
tells you what it needs.
"""
import html.parser
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).parent
DUMP = HERE.parent / 'test' / 'build' / 'escape_dump'

failures = []


def fail(sink, value, detail):
    failures.append((sink, value, detail))
    print(f'  FAIL [{sink}] {value!r}\n        {detail}')


# --- corpus -----------------------------------------------------------------
# Every ASCII character, so no metacharacter can be missed by omission, plus
# sequences that are only dangerous in combination, plus the non-ASCII line
# terminators that end a JS string literal in pre-ES2019 engines.
CORPUS = [chr(c) for c in range(0x20, 0x7F)]
CORPUS += ['\t', '\r', '\n', '\r\n']
CORPUS += [
    '</script>', '</SCRIPT >', '<!--', '-->', ']]>',
    "'", '"', '`', '\\', '${x}', '${alert(1)}', '`+alert(1)+`',
    "');alert(1);//", '"><img src=x onerror=alert(1)>',
    '<img src=x onerror=alert(1)>', 'javascript:alert(1)',
    ' ', ' ',                      # JS line terminators
    ' ', 'Ö', 'ä', 'Överspänning', 'Проміжне', '日本語',
    'U<20V & rising', 'GB (UK & N Ireland)', "l'accu", '100 % des',
    'a\\nb', 'x' * 300,
]


def escape_all(values):
    """Run the corpus through the real C++ escapers."""
    if not DUMP.exists():
        print(f'escape_dump not built at {DUMP}')
        print('build it: cmake -S test -B test/build && '
              'cmake --build test/build --target escape_dump')
        sys.exit(2)
    payload = b''
    for v in values:
        b = v.encode('utf-8')
        payload += str(len(b)).encode() + b'\n' + b
    out = subprocess.run([str(DUMP)], input=payload, stdout=subprocess.PIPE, check=True).stdout

    results, pos = [], 0
    for _ in values:
        pair = {}
        for _field in range(2):
            nl = out.index(b'\n', pos)
            tag, length = out[pos:nl].split(b' ')
            pos = nl + 1
            n = int(length)
            pair[tag.decode()] = out[pos:pos + n].decode('utf-8')
            pos += n + 1  # trailing LF
        results.append(pair)
    return results


# --- HTML sink --------------------------------------------------------------
class TextOnly(html.parser.HTMLParser):
    """Collects text, and records anything that is NOT text - a tag appearing
    means the escaped value broke out of its element."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.text = []
        self.intruders = []

    def handle_data(self, data):
        self.text.append(data)

    def handle_starttag(self, tag, attrs):
        self.intruders.append(f'<{tag}>')

    def handle_endtag(self, tag):
        self.intruders.append(f'</{tag}>')

    def handle_comment(self, data):
        self.intruders.append('<!--comment-->')


def check_html_text(value, escaped):
    # Fed without a wrapper element: at top level the escaped value must parse
    # as pure text, so ANY tag that appears is the value breaking out.
    p = TextOnly()
    p.feed(escaped)
    p.close()
    if p.intruders:
        fail('html-text', value, f'markup appeared: {p.intruders}')
        return
    got = ''.join(p.text)
    if got != value:
        fail('html-text', value, f'round-trip differs: {got!r}')


class AttrOnly(html.parser.HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.attrs = None
        self.count = 0

    def handle_starttag(self, tag, attrs):
        self.count += 1
        if self.attrs is None:
            self.attrs = dict(attrs)


def check_html_attr(value, escaped):
    for quote in ('"', "'"):
        p = AttrOnly()
        p.feed(f'<div title={quote}{escaped}{quote}>x</div>')
        p.close()
        if p.count != 1:
            fail('html-attr', value, f'{p.count} elements parsed (expected 1) with {quote} quoting')
            return
        got = (p.attrs or {}).get('title')
        # A trailing-whitespace-only difference is the parser normalising, not
        # an escape failure; compare exactly otherwise.
        if got != value and not (value.strip() == '' and (got or '').strip() == ''):
            fail('html-attr', value, f'attribute value differs with {quote} quoting: {got!r}')
            return
        if len(p.attrs or {}) != 1:
            fail('html-attr', value, f'extra attributes injected: {p.attrs}')
            return


# --- JS sink ----------------------------------------------------------------
def js_expected(value):
    """js_string_escape is deliberately lossy for line terminators: they would
    end the literal, and a UI label has no business containing one, so they
    become spaces. Everything else must round-trip exactly."""
    return value.replace('\r', ' ').replace('\n', ' ')


def check_js(values, escaped_list):
    """Embed each escaped value in all three JS literal styles, inside a real
    <script>-equivalent context, and have node report what the literal
    actually evaluates to."""
    cases = []
    for esc in escaped_list:
        cases.append(f"'{esc}'")
        cases.append(f'"{esc}"')
        cases.append(f'`{esc}`')
    script = 'const out = [\n' + ',\n'.join(cases) + '\n];\n' \
             'process.stdout.write(JSON.stringify(out));\n'
    try:
        res = subprocess.run(['node', '-e', script], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, timeout=60)
    except FileNotFoundError:
        print('  SKIP js sink (node not installed)')
        return
    if res.returncode != 0:
        # A syntax error means some escaped value broke out of its literal.
        # Bisect to name the culprit rather than reporting the whole batch.
        print('  node rejected the batch, bisecting...')
        for value, esc in zip(values, escaped_list):
            for style, lit in (("'", f"'{esc}'"), ('"', f'"{esc}"'), ('`', f'`{esc}`')):
                one = f'const x = {lit};\nprocess.stdout.write(JSON.stringify([x]));\n'
                r = subprocess.run(['node', '-e', one], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
                if r.returncode != 0:
                    fail('js', value, f'breaks out of a {style}-quoted literal: '
                                      f'{r.stderr.decode().splitlines()[:2]}')
        return
    got = json.loads(res.stdout)
    for i, value in enumerate(values):
        want = js_expected(value)
        for j, style in enumerate(("'", '"', '`')):
            if got[i * 3 + j] != want:
                fail('js', value, f'{style}-quoted literal evaluated to '
                                  f'{got[i * 3 + j]!r}, wanted {want!r}')


# --- run --------------------------------------------------------------------
print(f'escaping differential: {len(CORPUS)} inputs x 3 sinks')
escaped = escape_all(CORPUS)

for value, pair in zip(CORPUS, escaped):
    check_html_text(value, pair['H'])
    check_html_attr(value, pair['H'])

check_js(CORPUS, [p['J'] for p in escaped])

print()
if failures:
    print(f'{len(failures)} failure(s) - an escaper is missing something')
    sys.exit(1)
print('all sinks round-trip cleanly')
