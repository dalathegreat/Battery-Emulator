// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#ifndef ASYNCWEBSERVERHANDLERIMPL_H_
#define ASYNCWEBSERVERHANDLERIMPL_H_

#include <string>
#ifdef ASYNCWEBSERVER_REGEX
#include <regex>
#endif

#include "stddef.h"
#include <time.h>

class AsyncStaticWebHandler : public AsyncWebHandler {
  using File = fs::File;
  using FS = fs::FS;

private:
  bool _getFile(AsyncWebServerRequest *request) const;
  bool _searchFile(AsyncWebServerRequest *request, const String &path);
  uint8_t _countBits(const uint8_t value) const;

protected:
  FS _fs;
  String _uri;
  String _path;
  String _default_file;
  String _cache_control;
  String _last_modified;
  AwsTemplateProcessor _callback;
  bool _isDir;
  bool _tryGzipFirst = true;

public:
  AsyncStaticWebHandler(const char *uri, FS &fs, const char *path, const char *cache_control);
  bool canHandle(AsyncWebServerRequest *request) const override final;
  void handleRequest(AsyncWebServerRequest *request) override final;
  AsyncStaticWebHandler &setTryGzipFirst(bool value);
  AsyncStaticWebHandler &setIsDir(bool isDir);
  AsyncStaticWebHandler &setDefaultFile(const char *filename);
  AsyncStaticWebHandler &setCacheControl(const char *cache_control);

  /**
     * @brief Set the Last-Modified time for the object
     *
     * @param last_modified
     * @return AsyncStaticWebHandler&
     */
  AsyncStaticWebHandler &setLastModified(const char *last_modified);
  AsyncStaticWebHandler &setLastModified(struct tm *last_modified);
  AsyncStaticWebHandler &setLastModified(time_t last_modified);
  // sets to current time. Make sure sntp is running and time is updated
  AsyncStaticWebHandler &setLastModified();

  AsyncStaticWebHandler &setTemplateProcessor(AwsTemplateProcessor newCallback);
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
  bool isRequestHandlerTrivial() const override final {
    return !_onRequest;
  }
};

#endif /* ASYNCWEBSERVERHANDLERIMPL_H_ */
