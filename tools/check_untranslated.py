#!/usr/bin/env python3
"""Refuse hardcoded user-facing English in the web/render sources.

The ~850-site conversion sweep wrapped strings that were already being built
as C++ expressions, but missed literals baked directly into markup and into
JavaScript - button text and alert/prompt/confirm bodies especially. Those
were never TR-wrapped, so no catalog could translate them, and the gap only
surfaced when a Swedish device rendered English buttons.

Nothing checked for it, so it would come straight back with the next
contributor. This is that check.

Shapes flagged:
  >Literal</button>                     element text
  alert('Literal'  /  confirm(  /  prompt(     JS dialog bodies
  innerText = 'Literal'  /  textContent  /  innerHTML

Run: python3 tools/check_untranslated.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent / 'Software' / 'src'

FILES = sorted(
    list((ROOT / 'devboard' / 'webserver').glob('*.cpp'))
    # webserver/*.h too: index_html.h holds COMMON_JAVASCRIPT, whose reboot
    # confirm slipped through because only .cpp was scanned here.
    + list((ROOT / 'devboard' / 'webserver').glob('*.h'))
    + [p for p in (ROOT / 'battery').glob('*HTML*.h')]
    + [p for p in (ROOT / 'inverter').glob('*HTML*.h')]
)

# Genuine non-prose: placeholders overwritten by script before display, units,
# and single symbols. Keep this list short and justified - it is the escape
# hatch, and a long one means the check is not doing its job.
ALLOW = {
    'Value: ...',   # cellmonitor placeholder, replaced by JS before it is seen
    'Value: ',
}

# Quote style is captured and back-referenced: the JS in these files uses both
# ' and ", and scanning only for ' let four confirm dialogs through.
PATTERNS = [
    (re.compile(r'>([A-Za-z][A-Za-z0-9 /&:._-]{1,})</button>'), 'button text', 'TR()'),
    (re.compile(r'''(?:alert|confirm|prompt)\((['"])([A-Za-z](?:(?!\1).){3,})\1'''), 'JS dialog', 'TR_JS()'),
    (re.compile(r'''(?:innerText|textContent|innerHTML)\s*=\s*(['"])([A-Za-z](?:(?!\1).){2,})\1'''),
     'JS status text', 'TR_JS()'),
]


def join_adjacent_literals(text):
    """Collapse runs of adjacent C string literals into one.

    The compiler concatenates `"a" "b"` into `ab`, so a dialog assembled from
    literals across several source lines is a single string at runtime - but
    invisible to a per-line scan. Three of the four missed dialogs hid exactly
    there. Newlines removed from a run are re-appended after it, so every
    later line number stays correct and the run reports at its first line.
    """
    run = re.compile(r'("(?:[^"\\\n]|\\.)*"\s*){2,}')

    def merge(m):
        chunk = m.group(0)
        parts = re.findall(r'"((?:[^"\\\n]|\\.)*)"', chunk)
        return '"' + ''.join(parts) + '"' + '\n' * chunk.count('\n')

    return run.sub(merge, text)

# No "line already uses an accessor" skip: after literals are joined, one
# logical line can hold both a converted string and an unconverted one, and
# skipping the line hid three dialogs. It is not needed anyway - a converted
# site reads confirm('" + TR_JS(...) + "'), so the character after the quote
# is '"', which the patterns' leading [A-Za-z] already rejects.

problems = []
for path in FILES:
    rel = path.relative_to(ROOT).as_posix()
    in_block_comment = False
    text_all = join_adjacent_literals(path.read_text(encoding='utf-8', errors='replace'))
    for n, line in enumerate(text_all.splitlines(), 1):
        # Block comments matter here: several of these files keep an
        # un-minified copy of their JavaScript in a trailing /* ... */ block
        # for readability, and its markup is not live code.
        was_comment = in_block_comment
        # Detect comment delimiters on a version of the line with string
        # literals and // tails removed. Without that, a line comment
        # mentioning "/api/i18n/*" opens a block comment that never closes -
        # which silently skipped ~970 lines of webserver.cpp, including two of
        # the dialogs this check exists to find.
        bare = re.sub(r'"(?:[^"\\]|\\.)*"', '""', line)
        bare = re.sub(r"'(?:[^'\\]|\\.)*'", "''", bare)
        bare = re.sub(r'//.*$', '', bare)
        if '/*' in bare and '*/' not in bare:
            in_block_comment = True
        elif '*/' in bare:
            in_block_comment = False
        if was_comment or line.lstrip().startswith(('//', '*')):
            continue
        for pattern, what, accessor in PATTERNS:
            for m in pattern.finditer(line):
                text = m.group(m.lastindex).strip()
                if text in ALLOW or text.endswith('%') and len(text) < 4:
                    continue
                problems.append(f'{rel}:{n}: untranslated {what} "{text}" - wrap in {accessor}')

if problems:
    print('\n'.join(problems))
    print(f'\n{len(problems)} untranslated user-facing string(s)')
    sys.exit(1)
print('untranslated strings: none')
