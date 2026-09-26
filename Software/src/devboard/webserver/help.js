// Help buttons for the web interface.
//
// Served gzipped from flash as /help.js (see tools/gen_help_js.py, which strips the comments and
// indentation of this file and writes help_js.h - re-run it after editing). Pages that want help
// buttons only carry HELP_SCRIPT from index_html.h.
//
// The texts come from web_data/help/help.json on GitHub, cached in the browser for an hour.
// Devices with enough flash also carry the copy the firmware was built with as /help.json
// (tools/embed_help_json.py), used while there is no cached copy, and the only one offline.
// An element gets a help button when the file has an entry for it:
//   - a form field, keyed by its name attribute; the button goes on the <label> in front of it
//   - any element with a data-h attribute, keyed by that value; the button goes inside it
// Pressing the button opens the text right below the element, pressing it again closes it. It
// works the same with a mouse and a finger. Pages that reload themselves reopen what was open.
// When the browser rejects a field's value on save, the field's text opens by itself, so the
// format rules for validated fields live here too.
//
// Texts are inserted as plain text, never as HTML. The only markup is [label](path), which
// becomes a link to the "wiki" address of the file with the path appended, opening in a new tab.
//
// The script is loaded async near the top of the page, so the texts are fetched while the page
// itself still streams in, and buttons are added as their elements arrive instead of when the
// whole page has loaded (the settings page takes seconds to stream from the ESP32).
(function () {
  var url = 'https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/web_data/help/help.json';
  var store, cached, cachedAt, open = [], help, wiki;
  try {
    store = localStorage;
    cached = store.beHelp;
    cachedAt = +store.beHelpT;
    open = (sessionStorage.beHelpO || '').split(' ');
  } catch (e) {}

  var style = document.createElement('style');
  style.textContent =
    '.hi,.hi:hover{background:none;border:0;margin:0 0 0 4px;padding:0 2px;color:#8ab4f8;font:inherit;cursor:pointer}' +
    '.hb{grid-column:1/-1;margin:0 0 6px;padding:8px 10px;border-radius:8px;background:#26343c;color:#ddd;' +
    'font-size:.9em;font-weight:400;line-height:1.4;text-align:left}.hb a{color:#8ab4f8}';
  document.head.appendChild(style);

  // Remembers which boxes are open for the rest of the browser session.
  function remember(key, isOpen) {
    open = open.filter(function (k) { return k != key; });
    if (isOpen) open.push(key);
    try { sessionStorage.beHelpO = open.join(' '); } catch (e) {}
  }

  // [label](path) becomes a link to wiki + path, everything else stays plain text.
  function render(box, text, wiki) {
    var re = /\[([^\]]+)\]\(([^)\s]+)\)/g, last = 0, m, a;
    while ((m = re.exec(text))) {
      box.append(text.slice(last, m.index));
      if (wiki) {
        a = document.createElement('a');
        a.href = wiki + m[2].replace(/^\/+/, '');
        a.target = '_blank';
        a.rel = 'noopener';
        a.textContent = m[1];
        box.append(a);
      } else {
        box.append(m[1]);
      }
      last = re.lastIndex;
    }
    box.append(text.slice(last));
  }

  // Adds the buttons for everything that has arrived so far; safe to call again and again.
  function scan() {
    if (!help) return;
    var loading = document.readyState == 'loading';
    document.querySelectorAll('form [name],[data-h]').forEach(function (el) {
      if (el.hasHelp) return;
      var key = el.dataset.h || el.name, text = help[key];
      var host = el.dataset.h ? el : el.previousElementSibling;
      if (!text || !host || (!el.dataset.h && host.tagName != 'LABEL')) return;
      // The button goes inside a data-h element: wait until its content has arrived, which is
      // certain once something follows it.
      if (el.dataset.h && loading && !el.nextSibling) return;
      el.hasHelp = 1;
      var btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'hi';
      btn.textContent = '\u24D8';
      btn.setAttribute('aria-label', 'Help');
      btn.setAttribute('aria-expanded', false);
      function toggle(show) {
        var box = el.nextElementSibling;
        if (box && box.className == 'hb') box.remove();
        if (show) {
          box = document.createElement('div');
          box.className = 'hb';
          render(box, text, wiki);
          el.after(box);
        }
        btn.setAttribute('aria-expanded', show);
        remember(key, show);
      }
      btn.onclick = function () { toggle(btn.getAttribute('aria-expanded') != 'true'); };
      el.addEventListener('invalid', function () { toggle(true); });
      host.appendChild(btn);
      if (open.indexOf(key) >= 0) toggle(true);
    });
  }

  function apply(json) {
    if (help) return true;
    try {
      var data = JSON.parse(json);
      help = data.help || {};
      // Only an https address is used for links, anything else leaves the link labels as text.
      wiki = /^https:\/\//.test(data.wiki) ? data.wiki : '';
    } catch (e) {
      return false;
    }
    scan();
    return true;
  }

  // Keep adding buttons while the page streams in, and once more when it is complete.
  if (document.readyState == 'loading') {
    var watch = new MutationObserver(scan);
    watch.observe(document.documentElement, { childList: true, subtree: true });
    document.addEventListener('DOMContentLoaded', function () {
      watch.disconnect();
      scan();
    });
  }

  function get(u) {
    return fetch(u).then(function (r) { if (!r.ok) throw 0; return r.text(); });
  }

  // Show the cached copy straight away, and refresh it in the background once it is an hour old.
  // With nothing cached, whichever of the device's copy and GitHub's answers first is shown; only
  // GitHub's is cached. Without either, the page simply has no help buttons.
  var shown = cached && apply(cached);
  if (!shown || Date.now() - cachedAt > 36e5) {
    if (!shown) get('/help.json').then(apply).catch(function () {});
    get(url)
      .then(function (t) {
        try { store.beHelp = t; store.beHelpT = Date.now(); } catch (e) {}
        apply(t);
      })
      .catch(function () {});
  }
})();
