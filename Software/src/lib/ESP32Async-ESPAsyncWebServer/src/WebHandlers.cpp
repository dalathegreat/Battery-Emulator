// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include "ESPAsyncWebServer.h"
#include "WebHandlerImpl.h"

using namespace asyncsrv;

AsyncStaticWebHandler::AsyncStaticWebHandler(const char *uri, FS &fs, const char *path, const char *cache_control)
  : _fs(fs), _uri(uri), _path(path), _default_file(F("index.htm")), _cache_control(cache_control), _last_modified(), _callback(nullptr) {
  // Ensure leading '/'
  if (_uri.length() == 0 || _uri[0] != '/') {
    _uri = String('/') + _uri;
  }
  if (_path.length() == 0 || _path[0] != '/') {
    _path = String('/') + _path;
  }

  // If path ends with '/' we assume a hint that this is a directory to improve performance.
  // However - if it does not end with '/' we, can't assume a file, path can still be a directory.
  _isDir = _path[_path.length() - 1] == '/';

  // Remove the trailing '/' so we can handle default file
  // Notice that root will be "" not "/"
  if (_uri[_uri.length() - 1] == '/') {
    _uri = _uri.substring(0, _uri.length() - 1);
  }
  if (_path[_path.length() - 1] == '/') {
    _path = _path.substring(0, _path.length() - 1);
  }
}

void AsyncCallbackWebHandler::setUri(const String &uri) {
  _uri = uri;
  _isRegex = uri.startsWith("^") && uri.endsWith("$");
}

bool AsyncCallbackWebHandler::canHandle(AsyncWebServerRequest *request) const {
  if (!_onRequest || !request->isHTTP() || !(_method & request->method())) {
    return false;
  }

    if (_uri.length() && _uri.startsWith("/*.")) {
    String uriTemplate = String(_uri);
    uriTemplate = uriTemplate.substring(uriTemplate.lastIndexOf("."));
    if (!request->url().endsWith(uriTemplate)) {
      return false;
    }
  } else if (_uri.length() && _uri.endsWith("*")) {
    String uriTemplate = String(_uri);
    uriTemplate = uriTemplate.substring(0, uriTemplate.length() - 1);
    if (!request->url().startsWith(uriTemplate)) {
      return false;
    }
  } else if (_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri + "/"))) {
    return false;
  }

  return true;
}

void AsyncCallbackWebHandler::handleRequest(AsyncWebServerRequest *request) {
  if (_onRequest) {
    _onRequest(request);
  } else {
    request->send(404, T_text_plain, "Not found");
  }
}
void AsyncCallbackWebHandler::handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (_onUpload) {
    _onUpload(request, filename, index, data, len, final);
  }
}
void AsyncCallbackWebHandler::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (_onBody) {
    _onBody(request, data, len, index, total);
  }
}
