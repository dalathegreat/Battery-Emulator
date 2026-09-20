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

// Shared chrome for the battery sub pages (More Battery Info, Cellmonitor): dark page, pill
// buttons and the battery tab strip. Opens the style block only - the page appends its own rules
// and the closing </style>, so both pages carry one styling idea instead of two that drift apart.
#define INDEX_HTML_SUBPAGE_STYLE \
  R"rawliteral(<style>body{background:#000;color:#fff}
button,.battery-tab{background:#505E67;color:#fff;border:0;padding:10px 20px;margin:5px;cursor:pointer;border-radius:10px;display:inline-block;text-decoration:none;font:inherit}
button:hover,.battery-tab:hover{background:#3A4A52}
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
