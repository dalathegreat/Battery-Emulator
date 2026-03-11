// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#ifndef ASYNCWEBSERVERHANDLERIMPL_H_
#define ASYNCWEBSERVERHANDLERIMPL_H_

#include <string>
#include "stddef.h"

class AsyncStaticWebHandler : public AsyncWebHandler {
  using File = fs::File;
  using FS = fs::FS;

protected:
  FS _fs;
  String _uri;
  String _path;
  String _default_file;
  String _cache_control;
  String _last_modified;
  AwsTemplateProcessor _callback;
  bool _isDir;

public:
  AsyncStaticWebHandler(const char *uri, FS &fs, const char *path, const char *cache_control);

};

class AsyncCallbackWebHandler : public AsyncWebHandler {
private:
protected:
  String _uri;
  WebRequestMethodComposite _method;
  ArRequestHandlerFunction _onRequest;
  ArUploadHandlerFunction _onUpload;
  ArBodyHandlerFunction _onBody;
  bool _isRegex;

public:
  AsyncCallbackWebHandler() : _uri(), _method(HTTP_ANY), _onRequest(NULL), _onUpload(NULL), _onBody(NULL), _isRegex(false) {}
  void setUri(const String &uri);
  void setMethod(WebRequestMethodComposite method) {
    _method = method;
  }
  void onRequest(ArRequestHandlerFunction fn) {
    _onRequest = fn;
  }
  void onUpload(ArUploadHandlerFunction fn) {
    _onUpload = fn;
  }
  void onBody(ArBodyHandlerFunction fn) {
    _onBody = fn;
  }

  bool canHandle(AsyncWebServerRequest *request) const override final;
  void handleRequest(AsyncWebServerRequest *request) override final;
  void handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) override final;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override final;

};

#endif /* ASYNCWEBSERVERHANDLERIMPL_H_ */
