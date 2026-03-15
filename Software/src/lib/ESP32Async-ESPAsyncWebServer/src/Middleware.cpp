// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include "ESPAsyncWebServer.h"

AsyncMiddlewareChain::~AsyncMiddlewareChain() {
  for (AsyncMiddleware *m : _middlewares) {
    if (m->_freeOnRemoval) {
      delete m;
    }
  }
}

void AsyncMiddlewareChain::addMiddleware(ArMiddlewareCallback fn) {
  AsyncMiddlewareFunction *m = new AsyncMiddlewareFunction(fn);
  m->_freeOnRemoval = true;
  _middlewares.emplace_back(m);
}

void AsyncMiddlewareChain::_runChain(AsyncWebServerRequest *request, ArMiddlewareNext finalizer) {
  if (!_middlewares.size()) {
    return finalizer();
  }
  ArMiddlewareNext next;
  std::list<AsyncMiddleware *>::iterator it = _middlewares.begin();
  next = [this, &next, &it, request, finalizer]() {
    if (it == _middlewares.end()) {
      return finalizer();
    }
    AsyncMiddleware *m = *it;
    it++;
    return m->run(request, next);
  };
  return next();
}

