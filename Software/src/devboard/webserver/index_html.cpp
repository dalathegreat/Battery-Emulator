const char index_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head><title>Battery Emulator</title><meta name="viewport" content="width=device-width"><style>html{font-family:Arial;display:inline-block;text-align:center}h2{font-size:3rem}body{max-width:800px;margin:0 auto}</style></head><body><h2>Battery Emulator</h2>%ABC%</body></html>
)rawliteral";

/* The above code is minified to increase performance. Here is the full HTML function:
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
  <h2>Battery Emulator</h2>
  %ABC%
</body>
</html>
*/
