// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include "ESPAsyncWebServer.h"
#include "WebHandlerImpl.h"

using namespace asyncsrv;

bool ON_STA_FILTER(AsyncWebServerRequest *request) {
#ifndef CONFIG_IDF_TARGET_ESP32H2
  return WiFi.localIP() == request->client()->localIP();
#else
  return false;
#endif
}

bool ON_AP_FILTER(AsyncWebServerRequest *request) {
#ifndef CONFIG_IDF_TARGET_ESP32H2
  return WiFi.localIP() != request->client()->localIP();
#else
  return false;
#endif
}

#ifndef HAVE_FS_FILE_OPEN_MODE
const char *fs::FileOpenMode::read = "r";
const char *fs::FileOpenMode::write = "w";
const char *fs::FileOpenMode::append = "a";
#endif

AsyncWebServer::AsyncWebServer(uint16_t port) : _server(port) {
  _catchAllHandler = new AsyncCallbackWebHandler();
  _server.onClient(
    [](void *s, AsyncClient *c) {
      if (c == NULL) {
        return;
      }
      c->setRxTimeout(3);
      AsyncWebServerRequest *r = new AsyncWebServerRequest((AsyncWebServer *)s, c);
      if (r == NULL) {
        c->abort();
        delete c;
      }
    },
    this
  );
}

AsyncWebServer::~AsyncWebServer() {
  reset();
  end();
  delete _catchAllHandler;
  _catchAllHandler = nullptr;  // Prevent potential use-after-free
}

AsyncWebRewrite &AsyncWebServer::addRewrite(std::shared_ptr<AsyncWebRewrite> rewrite) {
  _rewrites.emplace_back(rewrite);
  return *_rewrites.back().get();
}

AsyncWebRewrite &AsyncWebServer::addRewrite(AsyncWebRewrite *rewrite) {
  _rewrites.emplace_back(rewrite);
  return *_rewrites.back().get();
}

bool AsyncWebServer::removeRewrite(AsyncWebRewrite *rewrite) {
  return removeRewrite(rewrite->from().c_str(), rewrite->toUrl().c_str());
}

bool AsyncWebServer::removeRewrite(const char *from, const char *to) {
  for (auto r = _rewrites.begin(); r != _rewrites.end(); ++r) {
    if (r->get()->from() == from && r->get()->toUrl() == to) {
      _rewrites.erase(r);
      return true;
    }
  }
  return false;
}

AsyncWebRewrite &AsyncWebServer::rewrite(const char *from, const char *to) {
  _rewrites.emplace_back(std::make_shared<AsyncWebRewrite>(from, to));
  return *_rewrites.back().get();
}

AsyncWebHandler &AsyncWebServer::addHandler(AsyncWebHandler *handler) {
  _handlers.emplace_back(handler);
  return *(_handlers.back().get());
}

bool AsyncWebServer::removeHandler(AsyncWebHandler *handler) {
  for (auto i = _handlers.begin(); i != _handlers.end(); ++i) {
    if (i->get() == handler) {
      _handlers.erase(i);
      return true;
    }
  }
  return false;
}

void AsyncWebServer::begin() {
  _server.setNoDelay(true);
  _server.begin();
}

void AsyncWebServer::end() {
  _server.end();
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncWebServer::onSslFileRequest(AcSSlFileHandler cb, void *arg) {
  _server.onSslFileRequest(cb, arg);
}

void AsyncWebServer::beginSecure(const char *cert, const char *key, const char *password) {
  _server.beginSecure(cert, key, password);
}
#endif

void AsyncWebServer::_handleDisconnect(AsyncWebServerRequest *request) {
  delete request;
}

void AsyncWebServer::_rewriteRequest(AsyncWebServerRequest *request) {
  // the last rewrite that matches the request will be used
  // we do not break the loop to allow for multiple rewrites to be applied and only the last one to be used (allows overriding)
  for (const auto &r : _rewrites) {
    if (r->match(request)) {
      request->_url = r->toUrl();
      request->_addGetParams(r->params());
    }
  }
}

void AsyncWebServer::_attachHandler(AsyncWebServerRequest *request) {
  for (auto &h : _handlers) {
    if (h->filter(request) && h->canHandle(request)) {
      request->setHandler(h.get());
      return;
    }
  }
  // ESP_LOGD("AsyncWebServer", "No handler found for %s, using _catchAllHandler pointer: %p", request->url().c_str(), _catchAllHandler);
  request->setHandler(_catchAllHandler);
}

AsyncCallbackWebHandler &AsyncWebServer::on(
  const char *uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody
) {
  AsyncCallbackWebHandler *handler = new AsyncCallbackWebHandler();
  handler->setUri(uri);
  handler->setMethod(method);
  handler->onRequest(onRequest);
  handler->onUpload(onUpload);
  handler->onBody(onBody);
  addHandler(handler);
  return *handler;
}

AsyncStaticWebHandler &AsyncWebServer::serveStatic(const char *uri, fs::FS &fs, const char *path, const char *cache_control) {
  AsyncStaticWebHandler *handler = new AsyncStaticWebHandler(uri, fs, path, cache_control);
  addHandler(handler);
  return *handler;
}

void AsyncWebServer::onNotFound(ArRequestHandlerFunction fn) {
  _catchAllHandler->onRequest(fn);
}

void AsyncWebServer::onFileUpload(ArUploadHandlerFunction fn) {
  _catchAllHandler->onUpload(fn);
}

void AsyncWebServer::onRequestBody(ArBodyHandlerFunction fn) {
  _catchAllHandler->onBody(fn);
}

AsyncWebHandler &AsyncWebServer::catchAllHandler() const {
  return *_catchAllHandler;
}

void AsyncWebServer::reset() {
  _rewrites.clear();
  _handlers.clear();

  _catchAllHandler->onRequest(NULL);
  _catchAllHandler->onUpload(NULL);
  _catchAllHandler->onBody(NULL);
}
