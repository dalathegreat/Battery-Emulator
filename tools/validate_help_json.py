#!/usr/bin/env python3
"""Validate web_data/help/help.json, the texts behind the web interface's help buttons.

Checks that
  - the file is {"wiki": "https://.../", "help": {key: text}}
  - every key belongs to something in the firmware: a settings form field (its name attribute in
    settings_html.cpp) or a data-h attribute somewhere under Software/src
  - every data-h in the firmware has a text, so no page carries markup that shows nothing
  - texts are plain text, and links use the [label](path) form with a path relative to "wiki"

    python tools/validate_help_json.py
    python tools/validate_help_json.py --wiki ../Battery-Emulator-Wiki   # also check pages and anchors

With --wiki pointing at a checkout of the wiki repository, every link must name an existing page
(docs/<path>.md or docs/<path>/index.md) and, if it has one, an anchor that is a heading on it.
"""
import argparse
import json
import pathlib
import re
import sys
import unicodedata

HERE = pathlib.Path(__file__).resolve().parent.parent
HELP = HERE / "web_data" / "help" / "help.json"
SRC = HERE / "Software" / "src"
SETTINGS = SRC / "devboard" / "webserver" / "settings_html.cpp"

LINK = re.compile(r"\[([^\]]+)\]\(([^)\s]+)\)")
FIELD = re.compile(r"<(?:input|select|textarea)\b[^>]*?\bname=['\"]([A-Za-z0-9_]+)['\"]", re.S)
DATA_H = re.compile(r"\bdata-h=['\"]?([A-Za-z0-9_]+)")


def firmware_keys():
    fields = set(FIELD.findall(SETTINGS.read_text(encoding="utf-8")))
    hooks = {}
    for path in sorted(SRC.rglob("*")):
        if path.suffix not in (".cpp", ".h") or "lib" in path.relative_to(SRC).parts[:1]:
            continue
        for key in DATA_H.findall(path.read_text(encoding="utf-8", errors="replace")):
            hooks.setdefault(key, path.relative_to(HERE))
    return fields, hooks


def slugify(text):
    # Python-Markdown's default toc slugify, which the wiki uses.
    text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode("ascii")
    text = re.sub(r"[^\w\s-]", "", text).strip().lower()
    return re.sub(r"[-\s]+", "-", text)


def anchors(page):
    found = set()
    for line in page.read_text(encoding="utf-8").splitlines():
        m = re.match(r"#{1,6}\s+(.*?)\s*#*\s*$", line)
        if not m:
            continue
        heading = m.group(1)
        explicit = re.search(r"\{[^}]*#([\w-]+)[^}]*\}\s*$", heading)
        if explicit:
            found.add(explicit.group(1))
            heading = heading[: explicit.start()]
        heading = LINK.sub(r"\1", heading)
        heading = re.sub(r":[a-z0-9_+-]+:", "", heading)  # emoji shortcodes render as images
        found.add(slugify(heading))
    return found


def check_link(wiki_docs, path):
    page, _, anchor = path.partition("#")
    page = page.strip("/")
    candidates = [wiki_docs / (page + ".md"), wiki_docs / page / "index.md"] if page else [wiki_docs / "index.md"]
    target = next((c for c in candidates if c.is_file()), None)
    if target is None:
        return f"no wiki page for '{path}'"
    if anchor and anchor not in anchors(target):
        return f"no heading '#{anchor}' on {target.relative_to(wiki_docs)}"
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--wiki", type=pathlib.Path, help="checkout of the Battery-Emulator-Wiki repository")
    args = parser.parse_args()

    errors = []
    try:
        data = json.loads(HELP.read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        print(f"{HELP.relative_to(HERE)}: {e}", file=sys.stderr)
        return 1

    wiki = data.get("wiki") if isinstance(data, dict) else None
    help_texts = data.get("help") if isinstance(data, dict) else None
    if not isinstance(wiki, str) or not wiki.startswith("https://") or not wiki.endswith("/"):
        errors.append('"wiki" must be an https:// address ending in /')
    if not isinstance(help_texts, dict):
        errors.append('"help" must be an object of key: text')
        help_texts = {}

    fields, hooks = firmware_keys()
    wiki_docs = args.wiki / "docs" if args.wiki else None
    if wiki_docs and not wiki_docs.is_dir():
        errors.append(f"--wiki: no docs folder in {args.wiki}")
        wiki_docs = None

    for key, text in help_texts.items():
        where = f"help[{key!r}]"
        if key not in fields and key not in hooks:
            errors.append(f"{where}: no settings field named {key} and no data-h={key} in the firmware")
        if not isinstance(text, str) or not text.strip():
            errors.append(f"{where}: text must be a non-empty string")
            continue
        if "<" in text or ">" in text:
            errors.append(f"{where}: texts are shown as plain text, HTML tags would show up literally")
        for label, path in LINK.findall(text):
            if re.match(r"[a-z][a-z0-9+.-]*:|/", path, re.I):
                errors.append(f"{where}: link '{path}' must be a path relative to the wiki address")
            elif wiki_docs:
                problem = check_link(wiki_docs, path)
                if problem:
                    errors.append(f"{where}: {problem}")
        if "](" in LINK.sub("", text):
            errors.append(f"{where}: malformed link, use [label](path) with no spaces in the path")

    for key, path in sorted(hooks.items()):
        if key not in help_texts:
            errors.append(f"{path}: data-h={key} has no text in {HELP.relative_to(HERE)}")

    for error in errors:
        print(f"{HELP.relative_to(HERE)}: {error}", file=sys.stderr)
    if not errors:
        print(f"{HELP.relative_to(HERE)}: {len(help_texts)} texts OK"
              + (", wiki links checked" if wiki_docs else ""))
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
