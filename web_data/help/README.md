# Web interface help texts

`help.json` holds the texts behind the ⓘ buttons of the web interface. They are not in the
firmware: the browser fetches this file from GitHub and caches it for an hour, so a text can be
written, fixed or extended without a firmware release and without costing flash.

- **Fetched from:** `https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/web_data/help/help.json`
  (in [`Software/src/devboard/webserver/help.js`](../../Software/src/devboard/webserver/help.js)).
  A change reaches users only **after it is merged to `main`**.
- **Validator:** [`tools/validate_help_json.py`](../../tools/validate_help_json.py), also run by pre-commit.

## Format

```json
{
  "wiki": "https://dalathegreat.github.io/Battery-Emulator-Wiki/",
  "help": {
    "LOWPASSFILTER": "Smooths sudden increases ... See [Inverter config](setup/software/webserver_guide/#inverter-config).",
    "nocntctrl": "This means you are either running CAN controlled contactors ..."
  }
}
```

- A **settings field** gets a button when its `name` attribute is a key here (`LOWPASSFILTER`).
  The button goes on the field's label, the text opens below the field.
- **Any other element** gets a button when it has a `data-h` attribute that is a key here, for
  example `<h4 data-h=nocntctrl>`. The page must also carry `HELP_SCRIPT` (from `index_html.h`);
  the settings, main, CAN tools and More Battery Info pages do, so battery renderers can use
  `data-h` in their panels right away.
- Texts are **plain text**. The only markup is a link, `[label](path)`: the path is appended to
  `wiki` and opens in a new tab. Write paths as the wiki's page addresses, e.g.
  `setup/software/mqtt/#enabling-mqtt`.

## Adding or changing a text

1. Add the entry (and, for something that is not a settings field, the `data-h` attribute in the
   firmware).
2. Run `python tools/validate_help_json.py`. With a checkout of the wiki repository,
   `python tools/validate_help_json.py --wiki ../Battery-Emulator-Wiki` also checks that every link
   points to an existing page and heading.

To try a text before it is merged, paste it into the browser console on a page of the emulator:

```js
localStorage.beHelp = JSON.stringify({wiki: 'https://dalathegreat.github.io/Battery-Emulator-Wiki/', help: {DNS: 'My text'}});
localStorage.beHelpT = Date.now();
```

and reload. It is replaced by the file from GitHub an hour later, or after `localStorage.clear()`.

## Fields with a format rule

Settings fields with a `pattern` carry no `title`: their format rule (for example *Printable
ASCII only*) is part of their text here. When the browser rejects a value on save, the field's
text opens by itself next to the browser's own message. Without internet the browser's generic
message is all there is.
