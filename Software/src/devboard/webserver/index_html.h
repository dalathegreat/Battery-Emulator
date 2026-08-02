#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML_HEADER \
  R"rawliteral(<!doctype html><html><head><meta charset="utf-8"><title>Battery Emulator</title><link rel="icon" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgd2lkdGg9IjI0IiBoZWlnaHQ9IjI0Ij48cmVjdCB4PSIyIiB5PSI2IiB3aWR0aD0iMTciIGhlaWdodD0iMTIiIHJ4PSIyLjUiIGZpbGw9IiNmZmZmZmYiIHN0cm9rZT0iIzJlOWU1YiIgc3Ryb2tlLXdpZHRoPSIyIi8+PHJlY3QgeD0iMTkuNCIgeT0iOS40IiB3aWR0aD0iMi42IiBoZWlnaHQ9IjUuMiIgcng9IjEiIGZpbGw9IiMyZTllNWIiLz48cGF0aCBkPSJNNi41LDEyIEwxNC41LDEyIiBmaWxsPSJub25lIiBzdHJva2U9IiMyNTYzZWIiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIi8+PHBhdGggZD0iTTguNSw5LjggTDYuMiwxMiBMOC41LDE0LjIiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzI1NjNlYiIgc3Ryb2tlLXdpZHRoPSIyIiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz48cGF0aCBkPSJNMTIuNSw5LjggTDE0LjgsMTIgTDEyLjUsMTQuMiIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjMjU2M2ViIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCIvPjwvc3ZnPg=="><meta content="width=device-width"name=viewport><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style><body>)rawliteral"
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
