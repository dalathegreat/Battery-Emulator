#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// The icon itself is served from /favicon.svg (webserver.cpp) with a long
// cache lifetime, so every page carries only this link tag. Small-flash
// devices omit the icon entirely.
#ifndef SMALL_FLASH_DEVICE
#define INDEX_HTML_FAVICON_LINK "<link rel=\"icon\" href=\"/favicon.svg\">"
#else
#define INDEX_HTML_FAVICON_LINK ""
#endif  // SMALL_FLASH_DEVICE

#define INDEX_HTML_HEADER                                                                                                           \
  R"rawliteral(<!doctype html><html><head><meta charset="utf-8"><title>Battery Emulator</title>)rawliteral" INDEX_HTML_FAVICON_LINK \
  R"rawliteral(<meta content="width=device-width"name=viewport><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style><body>)rawliteral"
#define INDEX_HTML_FOOTER R"rawliteral(</body></html>)rawliteral";

// Help buttons (help.js). A page carries only this tag; the loader is cached by the browser and
// the texts are fetched from web_data/help/help.json, so neither costs flash per page. A form
// field gets a button when help.json has an entry for its name, any other element when it has a
// data-h attribute that help.json has an entry for. Put it early in the page: it loads async and
// adds the buttons while the rest of the page streams in.
#define HELP_SCRIPT "<script src=/help.js async></script>"

// The navigation row at the top of every sub page, so the buttons look the same everywhere: one
// centered row, the same distance from the top of the page, the same fixed pixel height whatever
// the viewport or the page's own button rules (hence the .subnav prefix and the px units), wrapping
// onto further centered rows on a narrow screen. INDEX_HTML_NAV_CSS goes into the page's style
// block, after its own button rules; the page body starts with INDEX_HTML_NAV_START, then the
// page's own buttons (the first is always Back to main page) and INDEX_HTML_NAV_END.
#define INDEX_HTML_NAV_CSS                                                                                      \
  ".subnav{display:flex;flex-wrap:wrap;justify-content:center;gap:10px;margin:0;padding:10px 0 20px}"          \
  ".subnav button{box-sizing:border-box;height:36px;margin:0;padding:0 20px;background:#505E67;color:#fff;"   \
  "border:0;border-radius:10px;font:13.33px Arial,sans-serif;line-height:1;cursor:pointer}"                    \
  ".subnav button:hover{background:#3A4A52}"
#define INDEX_HTML_NAV_START "<div class=subnav><button onclick=\"location.href='/'\">Back to main page</button>"
#define INDEX_HTML_NAV_END "</div>"

// Shared chrome for the battery sub pages (More Battery Info, Cellmonitor): dark page, pill
// buttons and the battery tab strip. Opens the style block only - the page appends its own rules
// and the closing </style>, so both pages carry one styling idea instead of two that drift apart.
#define INDEX_HTML_SUBPAGE_STYLE \
  R"rawliteral(<style>body{background:#000;color:#fff}
button,.battery-tab{background:#505E67;color:#fff;border:0;padding:10px 20px;margin:5px;cursor:pointer;border-radius:10px;display:inline-block;text-decoration:none;font:inherit}
button:hover,.battery-tab:hover{background:#3A4A52}
)rawliteral" INDEX_HTML_NAV_CSS R"rawliteral(
.battery-tab[aria-current=page]{background:#287c58;outline:2px solid #69c999}
.battery-panel{background:#303E47;padding:10px;margin-bottom:10px;border-radius:24px}
)rawliteral"

#define COMMON_JAVASCRIPT \
  R"rawliteral(
<script>
function askReboot() {
  if (window.confirm('Are you sure you want to reboot the emulator? NOTE: If emulator is handling contactors, they will open during reboot!')) {
    reboot();
  }
}
function reboot() {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', '/reboot', true);
  xhr.send();
  setTimeout(function() {
    window.location = "/";
  }, 3000);
}
</script>
)rawliteral"

extern const char index_html[];
extern const char index_html_header[];
extern const char index_html_footer[];

#endif  // INDEX_HTML_H
