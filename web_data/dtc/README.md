# DTC description files

This folder holds the **DTC (Diagnostic Trouble Code) description** JSON files used by the
Battery-Emulator web UI. When a battery page shows a DTC table, a small JavaScript loader fetches the
matching file from server (GitHub) and fills in a human-readable description for each code.

- **Fetched from:** `https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/web_data/dtc/`
  (the `GITHUB_RAW_BASE_URL` constant in
  [`Software/src/devboard/webserver/BatteryHtmlRenderer.h`](../../Software/src/devboard/webserver/BatteryHtmlRenderer.h)).
- **Loader/renderer code:** `get_dtc_json_loader_html()` in the same header.
- **Validator:** [`tools/validate_dtc_json.py`](../../tools/validate_dtc_json.py).
- **JSON Schema:** [`dtc.schema.json`](dtc.schema.json) (editor autocomplete / inline validation).

> The loader fetches files from GitHub `main`. A new or edited file only takes effect for end users
> **after it is merged to `main`**. Until then, test it locally with the page's file picker (see below).

---

## 1. JSON file format

Each file is a **JSON array of objects**. Every object describes one DTC:

```json
[
  { "code": 41848, "dtc": "P0C9500", "s_dsc": "Diag_S_Temp_01CM11_ORH", "l_dsc": "Hybrid/EV Battery Temperature Sensor \"K\" Circuit High" },
  { "code": 41849, "dtc": "P0C9400", "s_dsc": "Diag_S_Temp_01CM11_ORL", "l_dsc": "Hybrid/EV Battery Temperature Sensor \"K\" Circuit Low" }
]
```

### Fields

| Field   | Type    | Required | Purpose |
|---------|---------|----------|---------|
| `code`  | integer | one of `code`/`dtc` | Decimal DTC code. Used to match cells that carry the decimal code. |
| `dtc`   | string  | one of `code`/`dtc` | DTC string (e.g. `"P0C9500"`). Used to match cells that carry the string. |
| `s_dsc` | string  | optional | Short/internal description, shown in grey italics under the long one. |
| `l_dsc` | string  | **yes**  | Long human-readable description. This is the text that gets displayed. |

An entry must have **at least one** identifier (`code` or `dtc`) so the loader can match it to a table
cell, and it must have a non-empty **`l_dsc`** so there is something to show. `s_dsc` is optional — when
empty it is simply omitted from the output (no empty line).

### How matching works

The loader builds a lookup keyed by **whichever identifier each entry has** and matches table cells by
their `data-dtc-code` attribute value:

```js
arr.forEach(function(e){ if (e.code != null) m[e.code] = e; if (e.dtc) m[e.dtc] = e; });
// cell lookup: m[ td.getAttribute('data-dtc-code') ]
```

So a single file can mix entries keyed by decimal code, by string, or by both. A renderer that emits
`data-dtc-code='41848'` matches by `code`; one that emits `data-dtc-code='P0C9500'` matches by `dtc`.

### Rules and gotchas

- **`l_dsc` is mandatory** and must be non-empty.
- **Provide `code`, `dtc`, or both.** If you only have the string code, omit `code` entirely.
- **Do not use `"code": ""`.** An empty string is *not* the same as absent — it would create a junk
  `m[""]` lookup entry. To mean "no code", **omit the key** (preferred) or use `"code": null`.
- **`code` must be an integer** (not a quoted string, float, or boolean).
- **Duplicate identifiers = last wins.** If two entries share the same `code` (or the same `dtc` when
  `dtc` is the only key), the later one overwrites the earlier in the lookup. Decimal `code` is the
  unambiguous key; the same `dtc` string can legitimately map to several distinct codes (different
  sensors), so prefer matching by `code` whenever codes are available.
- **Empty `s_dsc` is fine** — it is just skipped in the rendered output.
- Files are **UTF-8**; escape `"` inside strings as `\"` (standard JSON).
- These files are fetched and cached by the browser, so keep them lean: drop fields you don't use
  (e.g. a string-only file can be just `{ "dtc": "...", "l_dsc": "..." }`).

### Caching / refreshing

The loader caches each file in the browser's `localStorage` (key `dtcJson:<url>`). After you update a
file on `main`, users see the change once their cache expires or they click the **Refresh** link shown
in the DTC status line. If GitHub is unreachable, the page reveals a **file picker** so a local copy
can be loaded manually — handy for testing an unmerged file.

---

## 2. Validating a file

Run the validator before committing. It encodes all the rules above and checks JSON validity, missing
or empty `l_dsc`, unusable/duplicate keys, `code` typing, the empty-string-`code` trap, unknown keys,
and more.

```bash
# one file
python tools/validate_dtc_json.py web_data/dtc/bmw_phev_dtc.json

# all of them
python tools/validate_dtc_json.py web_data/dtc/*.json

# treat warnings as failures (good for CI / pre-commit)
python tools/validate_dtc_json.py --strict web_data/dtc/*.json

# only print files that have problems
python tools/validate_dtc_json.py -q web_data/dtc/*.json
```

Exit code is `0` when clean and `1` when any file has errors (or warnings under `--strict`), so it
drops straight into CI or a git hook. Errors point at the offending entry by index plus its
`code`/`dtc`/`l_dsc`, e.g.:

```
ERROR  : #8 (code=5): missing or empty 'l_dsc'
WARNING: duplicate 'code' 100 in entries [0, 1] (later overwrites earlier in lookup)
```

### JSON Schema (editor support)

[`dtc.schema.json`](dtc.schema.json) describes the format for editors and external tooling. It gives
you autocomplete and red-squiggle validation while typing (required `l_dsc`, integer-only `code`,
"at least one identifier", no unknown keys, etc.).

Because each file's root is a JSON **array**, you can't point at the schema with an inline `$schema`
key — map it in your editor instead. For VS Code, add to `.vscode/settings.json`:

```json
{
  "json.schemas": [
    {
      "fileMatch": ["web_data/dtc/*_dtc.json"],
      "url": "./web_data/dtc/dtc.schema.json"
    }
  ]
}
```

You can also validate against it from the command line (the schema gives quick structural feedback;
`validate_dtc_json.py` adds the cross-entry and "last-wins" checks the schema can't express):

```bash
python -c "import json,sys,glob; from jsonschema import Draft202012Validator as V; \
s=json.load(open('web_data/dtc/dtc.schema.json')); v=V(s); \
[print(f,'OK' if not list(v.iter_errors(json.load(open(f)))) else 'INVALID') \
 for f in glob.glob('web_data/dtc/*_dtc.json')]"
```

> Note: the schema flags unknown keys as **errors** (stricter than the validator, which treats them as
> warnings) to catch typos early while authoring.

---

## 3. Using the loader in a battery renderer

A battery's HTML renderer subclasses `BatteryHtmlRenderer`
([`Software/src/devboard/webserver/BatteryHtmlRenderer.h`](../../Software/src/devboard/webserver/BatteryHtmlRenderer.h))
and implements `get_status_html()`. To get DTC descriptions:

1. **Emit each DTC row with a `data-dtc-code` attribute** carrying the value to match — the decimal
   code or the DTC string, matching what your JSON is keyed by.
2. **Call `get_dtc_json_loader_html(base_url, filename)` once**, after the table, to inject the
   status line, file picker, and matching script.

Example from the BMW iX renderer
([`Software/src/battery/BMW-IX-HTML.cpp`](../../Software/src/battery/BMW-IX-HTML.cpp)), which matches by
decimal `code`:

```cpp
// one row per DTC; the description cell carries the decimal code
content += "<td data-dtc-code='" + String(code) +
           "' style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-size: 0.95em; color: #ddd;'>Unknown</td>";
// ...after the table:
content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "bmw_ix_dtc.json");
```

To match by **DTC string** instead (for a file that has only `dtc`/`l_dsc`), put the string in the
attribute:

```cpp
content += "<td class='dc dd' data-dtc-code='" + String(dtcStr) + "'>Unknown</td>";
content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "my_battery_dtc.json");
```

Notes:
- The cell's inner text (`Unknown` above) is the fallback shown when no description is found.
- Pass `GITHUB_RAW_BASE_URL` to fetch from this folder on `main`, or pass `""` to skip the GitHub
  fetch and only offer the local file picker.
- The MEB renderer stores the filename in a member (`dtc_json_filename`) so each platform can override
  it in `setup()` (e.g. MQB Evo → `"vag_mqb_dtc.json"`); a simpler renderer can just pass a literal.

---

## 4. Adding DTC descriptions for a new battery

1. **Create the JSON file** in this folder, e.g. `web_data/dtc/<battery>_dtc.json`, following the
   format in section 1. Decide whether your DTC table cells will carry the decimal `code` or the `dtc`
   string, and key the JSON accordingly (or include both).
2. **Validate it:** `python tools/validate_dtc_json.py web_data/dtc/<battery>_dtc.json` (fix all
   errors; review warnings).
3. **Wire up the renderer** (section 3): emit `data-dtc-code` on each row and call
   `get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "<battery>_dtc.json")`.
4. **Test locally before merge:** flash the firmware, open the battery page, and use the **file
   picker** to load your local JSON (GitHub `main` won't have it yet). Confirm the status line reports
   `n/Y DTCs matched` with the count you expect.
5. **Commit both** the JSON file and the renderer change, and merge to `main` so the GitHub fetch
   works for everyone.
