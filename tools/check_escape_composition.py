#!/usr/bin/env python3
"""Refuse the escaper/lookup compositions that have already caused bugs.

TR() is escaped for HTML, TR_JS() for a JavaScript literal, TR_RAW() for
neither. Composing an escaper with a lookup by hand reintroduces the two
mistakes this subsystem has already shipped:

  js_string_escape(TR(...))   double-escapes; French renders "l\\&#39;accu"
  html_escape(TR(...))        the same, in the other direction
  js_string_escape(TR_RAW())  correct, but it is TR_JS()'s job - spelling it
                              out at a call site means the next one can spell
                              it wrong, and nothing would notice

Run: python3 tools/check_escape_composition.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent / 'Software' / 'src'
ALLOWED = {'devboard/i18n/tr.cpp', 'devboard/i18n/tr.h', 'devboard/utils/escape.h',
           'devboard/utils/escape.cpp'}

BAD = [
    (re.compile(r'js_string_escape\s*\(\s*TR\s*\('), 'js_string_escape(TR(...)) double-escapes; use TR_JS()'),
    (re.compile(r'html_escape\s*\(\s*TR\s*\('), 'html_escape(TR(...)) double-escapes; TR() is already HTML-escaped'),
    (re.compile(r'js_string_escape\s*\(\s*TR_RAW\s*\('), 'use TR_JS(key) instead of composing by hand'),
    (re.compile(r'js_string_escape\s*\(\s*tr_expand\s*\('), 'use TR_JS(key, arg) instead of composing by hand'),
]

def check_response_sinks(path, rel, text):
    """TR() in a non-markup response body.

    Deliberately parses whole send(...) calls rather than scanning lines: the
    M2 regression survived a line-oriented grep because the call had been
    wrapped across two lines, so both the sed that was meant to fix it and the
    grep that was meant to verify it matched nothing - and agreed.
    """
    found = []
    for m in re.finditer(r'send\s*\((.{0,400}?)\)\s*;', text, re.S):
        body = m.group(1)
        if not re.search(r'\bTR\s*\(', body):
            continue
        for ctype in ('text/plain', 'application/json', 'text/csv', 'application/octet-stream'):
            if f'"{ctype}"' in body:
                line = text[:m.start()].count('\n') + 1
                found.append(f'{rel}:{line}: TR() in a {ctype} body - entities render '
                             f'literally there; use TR_RAW()\n    '
                             f'{" ".join(body.split())[:110]}')
                break
    return found


def check_raw_literal_expressions(rel, text):
    """C++ expressions stranded inside R"rawliteral(...)" blocks.

    A raw literal is emitted verbatim, so `" + TR(...) + "` inside one reaches
    the browser as that text rather than as a translated string. It compiles
    cleanly and no test renders these pages, so nothing else catches it - this
    shipped once already. Raw literals take %PLACEHOLDER% substitution.
    """
    found = []
    # Any raw-string delimiter, not just "rawliteral": C++ allows any run of
    # characters except parens, backslash and whitespace, and this codebase
    # also uses R"=====(...)=====". Matching only one spelling would leave the
    # other silently unguarded.
    for m in re.finditer(r'R"([^()\\\s]*)\(.*?\)\1"', text, re.S):
        block = m.group(0)
        for hit in re.finditer(r'"\s*\+\s*(TR|TR_JS|TR_RAW|tr_expand)\s*\(', block):
            line = text[:m.start() + hit.start()].count('\n') + 1
            found.append(f'{rel}:{line}: C++ expression inside a raw string literal - '
                         f'it will be emitted as literal text; use a %PLACEHOLDER% instead')
    return found


# AsyncWebServer truncates template token names at this length
# (TEMPLATE_PARAM_NAME_LENGTH in WebResponseImpl.h). A longer name never
# matches the processor, so the substitution silently yields an empty string -
# and when one straddles a response chunk boundary the raw name leaks and
# desyncs %-pairing for everything after it, killing the whole <script>. Both
# shipped once, invisible to every other check.
TEMPLATE_PARAM_NAME_LENGTH = 32


def check_placeholder_length(rel, text):
    found = []
    for m in re.finditer(r'%([A-Za-z][A-Za-z0-9_.]*)%', text):
        name = m.group(1)
        if len(name) > TEMPLATE_PARAM_NAME_LENGTH:
            line = text[:m.start()].count('\n') + 1
            found.append(f'{rel}:{line}: template placeholder "%{name}%" is '
                         f'{len(name)} chars; the processor truncates at '
                         f'{TEMPLATE_PARAM_NAME_LENGTH} and the lookup will never match')
    return found


problems = []
for path in sorted(ROOT.rglob('*')):
    if path.suffix not in ('.cpp', '.h') or not path.is_file():
        continue
    rel = path.relative_to(ROOT).as_posix()
    if rel in ALLOWED:
        continue
    text = path.read_text(encoding='utf-8', errors='replace')
    problems.extend(check_response_sinks(path, rel, text))
    problems.extend(check_raw_literal_expressions(rel, text))
    problems.extend(check_placeholder_length(rel, text))
    for n, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith(('*', '//')):
            continue  # prose about the rule is fine
        for pattern, why in BAD:
            if pattern.search(line):
                problems.append(f'{rel}:{n}: {why}\n    {line.strip()}')

if problems:
    print('\n'.join(problems))
    print(f'\n{len(problems)} escaping problem(s)')
    sys.exit(1)
print('escaper composition: clean')
