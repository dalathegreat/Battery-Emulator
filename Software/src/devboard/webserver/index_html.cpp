const char index_html[] = R"rawliteral(
<!doctypehtml><title>Battery Emulator</title><meta content="width=device-width"name=viewport><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style>%X%
)rawliteral";

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
