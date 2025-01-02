#include "index_html.h"

#define INDEX_HTML_HEADER \
  R"rawliteral(<!doctype html><html><head><title>Battery Emulator</title><meta content="width=device-width"name=viewport><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style><body>)rawliteral"
#define INDEX_HTML_FOOTER R"rawliteral(</body></html>)rawliteral";

const char index_html[] = INDEX_HTML_HEADER "%X%" INDEX_HTML_FOOTER;
const char index_html_header[] = INDEX_HTML_HEADER;
const char index_html_footer[] = INDEX_HTML_FOOTER;

/* The above code is minified (https://kangax.github.io/html-minifier/) to increase performance. Here is the full HTML function:
<!DOCTYPE HTML><html>
<head>
  <title>Battery Emulator</title>
  <meta name="viewport" content="width=device-width">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    body {max-width: 800px; margin:0px auto;}
  </style>
</head>
<body>
  %X%
</body>
</html>
*/
