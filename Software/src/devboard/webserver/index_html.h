#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML_HEADER \
  R"rawliteral(<!doctype html><html><head><title>Battery Emulator</title><meta content="width=device-width"name=viewport><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style><body>)rawliteral"
#define INDEX_HTML_FOOTER R"rawliteral(</body></html>)rawliteral";

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
