// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#ifndef _ESPAsyncWebServer_H_
#define _ESPAsyncWebServer_H_

#include <Arduino.h>
#include <FS.h>
#include <lwip/tcpbase.h>

#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

#if defined(ESP32) || defined(LIBRETINY)
#include "../../mathieucarbou-AsyncTCPSock/src/AsyncTCP.h"
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <RPAsyncTCP.h>
#include <HTTP_Method.h>
#include <http_parser.h>
#else
#error Platform not supported
#endif

#include "literals.h"

#include "AsyncWebServerVersion.h"
#define ASYNCWEBSERVER_FORK_ESP32Async

#ifdef ASYNCWEBSERVER_REGEX
#define ASYNCWEBSERVER_REGEX_ATTRIBUTE
#else
#define ASYNCWEBSERVER_REGEX_ATTRIBUTE __attribute__((warning("ASYNCWEBSERVER_REGEX not defined")))
#endif

// See https://github.com/ESP32Async/ESPAsyncWebServer/commit/3d3456e9e81502a477f6498c44d0691499dda8f9#diff-646b25b11691c11dce25529e3abce843f0ba4bd07ab75ec9eee7e72b06dbf13fR388-R392
// This setting slowdown chunk serving but avoids crashing or deadlocks in the case where slow chunk responses are created, like file serving form SD Card
#ifndef ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
#define ASYNCWEBSERVER_USE_CHUNK_INFLIGHT 1
#endif

class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncWebHeader;
class AsyncWebParameter;
class AsyncWebRewrite;
class AsyncWebHandler;
class AsyncStaticWebHandler;
class AsyncCallbackWebHandler;
class AsyncResponseStream;
class AsyncMiddlewareChain;

typedef enum {
  HTTP_GET = 0b00000001,
  HTTP_POST = 0b00000010,
  HTTP_DELETE = 0b00000100,
  HTTP_PUT = 0b00001000,
  HTTP_PATCH = 0b00010000,
  HTTP_HEAD = 0b00100000,
  HTTP_OPTIONS = 0b01000000,
  HTTP_ANY = 0b01111111,
} WebRequestMethod;

#ifndef HAVE_FS_FILE_OPEN_MODE
namespace fs {
class FileOpenMode {
public:
  static const char *read;
  static const char *write;
  static const char *append;
};
};  // namespace fs
#else
#include "FileOpenMode.h"
#endif

// if this value is returned when asked for data, packet will not be sent and you will be asked for data again
#define RESPONSE_TRY_AGAIN          0xFFFFFFFF
#define RESPONSE_STREAM_BUFFER_SIZE 1460

typedef uint8_t WebRequestMethodComposite;
typedef std::function<void(void)> ArDisconnectHandler;

/*
 * PARAMETER :: Chainable object to hold GET/POST and FILE parameters
 * */

class AsyncWebParameter {
private:
  String _name;
  String _value;
  size_t _size;
  bool _isForm;
  bool _isFile;

public:
  AsyncWebParameter(const String &name, const String &value, bool form = false, bool file = false, size_t size = 0)
    : _name(name), _value(value), _size(size), _isForm(form), _isFile(file) {}
  const String &name() const {
    return _name;
  }
  const String &value() const {
    return _value;
  }
  size_t size() const {
    return _size;
  }
  bool isPost() const {
    return _isForm;
  }
  bool isFile() const {
    return _isFile;
  }
};

/*
 * HEADER :: Chainable object to hold the headers
 * */

class AsyncWebHeader {
private:
  String _name;
  String _value;

public:
  AsyncWebHeader() {}
  AsyncWebHeader(const AsyncWebHeader &) = default;
  AsyncWebHeader(AsyncWebHeader &&) = default;
  AsyncWebHeader(const char *name, const char *value) : _name(name), _value(value) {}
  AsyncWebHeader(const String &name, const String &value) : _name(name), _value(value) {}

#ifndef ESP8266
  [[deprecated("Use AsyncWebHeader::parse(data) instead")]]
#endif
  AsyncWebHeader(const String &data)
    : AsyncWebHeader(parse(data)){};

  AsyncWebHeader &operator=(const AsyncWebHeader &) = default;
  AsyncWebHeader &operator=(AsyncWebHeader &&other) = default;

  const String &name() const {
    return _name;
  }
  const String &value() const {
    return _value;
  }

  String toString() const;

  // returns true if the header is valid
  operator bool() const {
    return _name.length();
  }

  static const AsyncWebHeader parse(const String &data) {
    return parse(data.c_str());
  }
  static const AsyncWebHeader parse(const char *data);
};

/*
 * REQUEST :: Each incoming Client is wrapped inside a Request and both live together until disconnect
 * */

typedef enum {
  RCT_NOT_USED = -1,
  RCT_DEFAULT = 0,
  RCT_HTTP,
  RCT_WS,
  RCT_EVENT,
  RCT_MAX
} RequestedConnectionType;

// this enum is similar to Arduino WebServer's AsyncAuthType and PsychicHttp
typedef enum {
  AUTH_NONE = 0,  // always allow
  AUTH_BASIC = 1,
  AUTH_DIGEST = 2,
  AUTH_BEARER = 3,
  AUTH_OTHER = 4,
  AUTH_DENIED = 255,  // always returns 401
} AsyncAuthType;

typedef std::function<size_t(uint8_t *, size_t, size_t)> AwsResponseFiller;
typedef std::function<String(const String &)> AwsTemplateProcessor;

using AsyncWebServerRequestPtr = std::weak_ptr<AsyncWebServerRequest>;

class AsyncWebServerRequest {
  using File = fs::File;
  using FS = fs::FS;
  friend class AsyncWebServer;
  friend class AsyncCallbackWebHandler;
  friend class AsyncFileResponse;

private:
  AsyncClient *_client;
  AsyncWebServer *_server;
  AsyncWebHandler *_handler;
  AsyncWebServerResponse *_response;
  ArDisconnectHandler _onDisconnectfn;

  bool _sent = false;                            // response is sent
  bool _paused = false;                          // request is paused (request continuation)
  std::shared_ptr<AsyncWebServerRequest> _this;  // shared pointer to this request

  String _temp;
  uint8_t _parseState;

  uint8_t _version;
  WebRequestMethodComposite _method;
  String _url;
  String _host;
  String _contentType;
  String _boundary;
  String _authorization;
  RequestedConnectionType _reqconntype;
  AsyncAuthType _authMethod = AsyncAuthType::AUTH_NONE;
  bool _isMultipart;
  bool _isPlainPost;
  bool _expectingContinue;
  size_t _contentLength;
  size_t _parsedLength;

  std::list<AsyncWebHeader> _headers;
  std::list<AsyncWebParameter> _params;
  std::list<String> _pathParams;

  std::unordered_map<const char *, String, std::hash<const char *>, std::equal_to<const char *>> _attributes;

  uint8_t _multiParseState;
  uint8_t _boundaryPosition;
  size_t _itemStartIndex;
  size_t _itemSize;
  String _itemName;
  String _itemFilename;
  String _itemType;
  String _itemValue;
  uint8_t *_itemBuffer;
  size_t _itemBufferIndex;
  bool _itemIsFile;

  void _onPoll();
  void _onAck(size_t len, uint32_t time);
  void _onError(int8_t error);
  void _onTimeout(uint32_t time);
  void _onDisconnect();
  void _onData(void *buf, size_t len);

  void _addPathParam(const char *param);

  bool _parseReqHead();
  bool _parseReqHeader();
  void _parseLine();
  void _parsePlainPostChar(uint8_t data);
  void _parseMultipartPostByte(uint8_t data, bool last);
  void _addGetParams(const String &params);

  void _handleUploadStart();
  void _handleUploadByte(uint8_t data, bool last);
  void _handleUploadEnd();

  void _send();
  void _runMiddlewareChain();

  static void _getEtag(uint8_t trailer[4], char *serverETag);

public:
  File _tempFile;
  void *_tempObject;

  AsyncWebServerRequest(AsyncWebServer *, AsyncClient *);
  ~AsyncWebServerRequest();

  AsyncClient *client() {
    return _client;
  }
  uint8_t version() const {
    return _version;
  }
  WebRequestMethodComposite method() const {
    return _method;
  }
  const String &url() const {
    return _url;
  }
  const String &host() const {
    return _host;
  }
  const String &contentType() const {
    return _contentType;
  }
  size_t contentLength() const {
    return _contentLength;
  }
  bool multipart() const {
    return _isMultipart;
  }

  const char *methodToString() const;
  const char *requestedConnTypeToString() const;

  RequestedConnectionType requestedConnType() const {
    return _reqconntype;
  }
  bool isExpectedRequestedConnType(RequestedConnectionType erct1, RequestedConnectionType erct2 = RCT_NOT_USED, RequestedConnectionType erct3 = RCT_NOT_USED)
    const;
  bool isWebSocketUpgrade() const {
    return _method == HTTP_GET && isExpectedRequestedConnType(RCT_WS);
  }
  bool isSSE() const {
    return _method == HTTP_GET && isExpectedRequestedConnType(RCT_EVENT);
  }
  bool isHTTP() const {
    return isExpectedRequestedConnType(RCT_DEFAULT, RCT_HTTP);
  }
  void onDisconnect(ArDisconnectHandler fn);

  // hash is the string representation of:
  //  base64(user:pass) for basic or
  //  user:realm:md5(user:realm:pass) for digest
  bool authenticate(const char *hash) const;
  bool authenticate(const char *username, const char *credentials, const char *realm = NULL, bool isHash = false) const;
  void requestAuthentication(const char *realm = nullptr, bool isDigest = true) {
    requestAuthentication(isDigest ? AsyncAuthType::AUTH_DIGEST : AsyncAuthType::AUTH_BASIC, realm);
  }
  void requestAuthentication(AsyncAuthType method, const char *realm = nullptr, const char *_authFailMsg = nullptr);

  // IMPORTANT: this method is for internal use ONLY
  // Please do not use it!
  // It can be removed or modified at any time without notice
  void setHandler(AsyncWebHandler *handler) {
    _handler = handler;
  }

#ifndef ESP8266
  [[deprecated("All headers are now collected. Use removeHeader(name) or AsyncHeaderFreeMiddleware if you really need to free some headers.")]]
#endif
  void addInterestingHeader(__unused const char *name) {
  }
#ifndef ESP8266
  [[deprecated("All headers are now collected. Use removeHeader(name) or AsyncHeaderFreeMiddleware if you really need to free some headers.")]]
#endif
  void addInterestingHeader(__unused const String &name) {
  }

  /**
     * @brief issue HTTP redirect response with Location header
     *
     * @param url - url to redirect to
     * @param code - response code, default is 302 : temporary redirect
     */
  void redirect(const char *url, int code = 302);
  void redirect(const String &url, int code = 302) {
    return redirect(url.c_str(), code);
  };

  void send(AsyncWebServerResponse *response);
  AsyncWebServerResponse *getResponse() const {
    return _response;
  }

  void send(int code, const char *contentType = asyncsrv::empty, const char *content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(code, contentType, content, callback));
  }
  void send(int code, const String &contentType, const char *content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(code, contentType.c_str(), content, callback));
  }
  void send(int code, const String &contentType, const String &content, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(code, contentType.c_str(), content.c_str(), callback));
  }

  void send(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(code, contentType, content, len, callback));
  }
  void send(int code, const String &contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(code, contentType, content, len, callback));
  }

  void send(FS &fs, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
  void send(FS &fs, const String &path, const String &contentType, bool download = false, AwsTemplateProcessor callback = nullptr) {
    send(fs, path, contentType.c_str(), download, callback);
  }

  void send(File content, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr) {
    if (content) {
      send(beginResponse(content, path, contentType, download, callback));
    } else {
      send(404);
    }
  }
  void send(File content, const String &path, const String &contentType, bool download = false, AwsTemplateProcessor callback = nullptr) {
    send(content, path, contentType.c_str(), download, callback);
  }

  void send(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(stream, contentType, len, callback));
  }
  void send(Stream &stream, const String &contentType, size_t len, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse(stream, contentType, len, callback));
  }

  void send(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    send(beginResponse(contentType, len, callback, templateCallback));
  }
  void send(const String &contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    send(beginResponse(contentType, len, callback, templateCallback));
  }

  void sendChunked(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    send(beginChunkedResponse(contentType, callback, templateCallback));
  }
  void sendChunked(const String &contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    send(beginChunkedResponse(contentType, callback, templateCallback));
  }

#ifndef ESP8266
  [[deprecated("Replaced by send(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr)")]]
#endif
  void send_P(int code, const String &contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr) {
    send(code, contentType, content, len, callback);
  }
#ifndef ESP8266
  [[deprecated("Replaced by send(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr)")]]
  void send_P(int code, const String &contentType, PGM_P content, AwsTemplateProcessor callback = nullptr) {
    send(code, contentType, content, callback);
  }
#else
  void send_P(int code, const String &contentType, PGM_P content, AwsTemplateProcessor callback = nullptr) {
    send(beginResponse_P(code, contentType, content, callback));
  }
#endif

  AsyncWebServerResponse *
    beginResponse(int code, const char *contentType = asyncsrv::empty, const char *content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr);
  AsyncWebServerResponse *beginResponse(int code, const String &contentType, const char *content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(code, contentType.c_str(), content, callback);
  }
  AsyncWebServerResponse *beginResponse(int code, const String &contentType, const String &content, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(code, contentType.c_str(), content.c_str(), callback);
  }

  AsyncWebServerResponse *beginResponse(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr);
  AsyncWebServerResponse *beginResponse(int code, const String &contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(code, contentType.c_str(), content, len, callback);
  }

  AsyncWebServerResponse *
    beginResponse(FS &fs, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
  AsyncWebServerResponse *
    beginResponse(FS &fs, const String &path, const String &contentType = emptyString, bool download = false, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(fs, path, contentType.c_str(), download, callback);
  }

  AsyncWebServerResponse *
    beginResponse(File content, const String &path, const char *contentType = asyncsrv::empty, bool download = false, AwsTemplateProcessor callback = nullptr);
  AsyncWebServerResponse *
    beginResponse(File content, const String &path, const String &contentType = emptyString, bool download = false, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(content, path, contentType.c_str(), download, callback);
  }

  AsyncWebServerResponse *beginResponse(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback = nullptr);
  AsyncWebServerResponse *beginResponse(Stream &stream, const String &contentType, size_t len, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(stream, contentType.c_str(), len, callback);
  }

  AsyncWebServerResponse *beginResponse(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
  AsyncWebServerResponse *beginResponse(const String &contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    return beginResponse(contentType.c_str(), len, callback, templateCallback);
  }

  AsyncWebServerResponse *beginChunkedResponse(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr);
  AsyncWebServerResponse *beginChunkedResponse(const String &contentType, AwsResponseFiller callback, AwsTemplateProcessor templateCallback = nullptr) {
    return beginChunkedResponse(contentType.c_str(), callback, templateCallback);
  }

  AsyncResponseStream *beginResponseStream(const char *contentType, size_t bufferSize = RESPONSE_STREAM_BUFFER_SIZE);
  AsyncResponseStream *beginResponseStream(const String &contentType, size_t bufferSize = RESPONSE_STREAM_BUFFER_SIZE) {
    return beginResponseStream(contentType.c_str(), bufferSize);
  }

#ifndef ESP8266
  [[deprecated("Replaced by beginResponse(int code, const String& contentType, const uint8_t* content, size_t len, AwsTemplateProcessor callback = nullptr)")]]
#endif
  AsyncWebServerResponse *beginResponse_P(int code, const String &contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback = nullptr) {
    return beginResponse(code, contentType.c_str(), content, len, callback);
  }
#ifndef ESP8266
  [[deprecated("Replaced by beginResponse(int code, const String& contentType, const char* content = asyncsrv::empty, AwsTemplateProcessor callback = nullptr)"
  )]]
#endif
  AsyncWebServerResponse *beginResponse_P(int code, const String &contentType, PGM_P content, AwsTemplateProcessor callback = nullptr);

  /**
   * @brief Request Continuation: this function pauses the current request and returns a weak pointer (AsyncWebServerRequestPtr is a std::weak_ptr) to the request in order to reuse it later on.
   * The middelware chain will continue to be processed until the end, but no response will be sent.
   * To resume operations (send the request), the request must be retrieved from the weak pointer and a send() function must be called.
   * AsyncWebServerRequestPtr is the only object allowed to exist the scope of the request handler.
   * @warning This function should be called from within the context of a request (in a handler or middleware for example).
   * @warning While the request is paused, if the client aborts the request, the latter will be disconnected and deleted.
   * So it is the responsibility of the user to check the validity of the request pointer (AsyncWebServerRequestPtr) before using it by calling lock() and/or expired().
   */
  AsyncWebServerRequestPtr pause();

  bool isPaused() const {
    return _paused;
  }

  /**
   * @brief Aborts the request and close the client (RST).
   * Mark the request as sent.
   * If it was paused, it will be unpaused and it won't be possible to resume it.
   */
  void abort();

  bool isSent() const {
    return _sent;
  }

  /**
     * @brief Get the Request parameter by name
     *
     * @param name
     * @param post
     * @param file
     * @return const AsyncWebParameter*
     */
  const AsyncWebParameter *getParam(const char *name, bool post = false, bool file = false) const;

  const AsyncWebParameter *getParam(const String &name, bool post = false, bool file = false) const {
    return getParam(name.c_str(), post, file);
  };
#ifdef ESP8266
  const AsyncWebParameter *getParam(const __FlashStringHelper *data, bool post, bool file) const;
#endif

  /**
     * @brief Get request parameter by number
     * i.e., n-th parameter
     * @param num
     * @return const AsyncWebParameter*
     */
  const AsyncWebParameter *getParam(size_t num) const;
  const AsyncWebParameter *getParam(int num) const {
    return num < 0 ? nullptr : getParam((size_t)num);
  }

  size_t args() const {
    return params();
  }  // get arguments count

  // get request argument value by name
  const String &arg(const char *name) const;
  // get request argument value by name
  const String &arg(const String &name) const {
    return arg(name.c_str());
  };
#ifdef ESP8266
  const String &arg(const __FlashStringHelper *data) const;  // get request argument value by F(name)
#endif
  const String &arg(size_t i) const;  // get request argument value by number
  const String &arg(int i) const {
    return i < 0 ? emptyString : arg((size_t)i);
  };
  const String &argName(size_t i) const;  // get request argument name by number
  const String &argName(int i) const {
    return i < 0 ? emptyString : argName((size_t)i);
  };
  bool hasArg(const char *name) const;  // check if argument exists
  bool hasArg(const String &name) const {
    return hasArg(name.c_str());
  };
#ifdef ESP8266
  bool hasArg(const __FlashStringHelper *data) const;  // check if F(argument) exists
#endif

  const String &ASYNCWEBSERVER_REGEX_ATTRIBUTE pathArg(size_t i) const;
  const String &ASYNCWEBSERVER_REGEX_ATTRIBUTE pathArg(int i) const {
    return i < 0 ? emptyString : pathArg((size_t)i);
  }

  // get request header value by name
  const String &header(const char *name) const;
  const String &header(const String &name) const {
    return header(name.c_str());
  };

#ifdef ESP8266
  const String &header(const __FlashStringHelper *data) const;  // get request header value by F(name)
#endif

  const String &header(size_t i) const;  // get request header value by number
  const String &header(int i) const {
    return i < 0 ? emptyString : header((size_t)i);
  };
  const String &headerName(size_t i) const;  // get request header name by number
  const String &headerName(int i) const {
    return i < 0 ? emptyString : headerName((size_t)i);
  };

  size_t headers() const;  // get header count

  // check if header exists
  bool hasHeader(const char *name) const;
  bool hasHeader(const String &name) const {
    return hasHeader(name.c_str());
  };
#ifdef ESP8266
  bool hasHeader(const __FlashStringHelper *data) const;  // check if header exists
#endif

  const AsyncWebHeader *getHeader(const char *name) const;
  const AsyncWebHeader *getHeader(const String &name) const {
    return getHeader(name.c_str());
  };
#ifdef ESP8266
  const AsyncWebHeader *getHeader(const __FlashStringHelper *data) const;
#endif

  const AsyncWebHeader *getHeader(size_t num) const;
  const AsyncWebHeader *getHeader(int num) const {
    return num < 0 ? nullptr : getHeader((size_t)num);
  };

  const std::list<AsyncWebHeader> &getHeaders() const {
    return _headers;
  }

  size_t getHeaderNames(std::vector<const char *> &names) const;

  // Remove a header from the request.
  // It will free the memory and prevent the header to be seen during request processing.
  bool removeHeader(const char *name);
  // Remove all request headers.
  void removeHeaders() {
    _headers.clear();
  }

  size_t params() const;  // get arguments count
  bool hasParam(const char *name, bool post = false, bool file = false) const;
  bool hasParam(const String &name, bool post = false, bool file = false) const {
    return hasParam(name.c_str(), post, file);
  };
#ifdef ESP8266
  bool hasParam(const __FlashStringHelper *data, bool post = false, bool file = false) const {
    return hasParam(String(data).c_str(), post, file);
  };
#endif

  // REQUEST ATTRIBUTES

  void setAttribute(const char *name, const char *value) {
    _attributes[name] = value;
  }
  void setAttribute(const char *name, bool value) {
    _attributes[name] = value ? "1" : emptyString;
  }
  void setAttribute(const char *name, long value) {
    _attributes[name] = String(value);
  }
  void setAttribute(const char *name, float value, unsigned int decimalPlaces = 2) {
    _attributes[name] = String(value, decimalPlaces);
  }
  void setAttribute(const char *name, double value, unsigned int decimalPlaces = 2) {
    _attributes[name] = String(value, decimalPlaces);
  }

  bool hasAttribute(const char *name) const {
    return _attributes.find(name) != _attributes.end();
  }

  const String &getAttribute(const char *name, const String &defaultValue = emptyString) const;
  bool getAttribute(const char *name, bool defaultValue) const;
  long getAttribute(const char *name, long defaultValue) const;
  float getAttribute(const char *name, float defaultValue) const;
  double getAttribute(const char *name, double defaultValue) const;

  String urlDecode(const String &text) const;
};

/*
 * FILTER :: Callback to filter AsyncWebRewrite and AsyncWebHandler (done by the Server)
 * */

using ArRequestFilterFunction = std::function<bool(AsyncWebServerRequest *request)>;

bool ON_STA_FILTER(AsyncWebServerRequest *request);

bool ON_AP_FILTER(AsyncWebServerRequest *request);

/*
 * MIDDLEWARE :: Request interceptor, assigned to a AsyncWebHandler (or the server), which can be used:
 * 1. to run some code before the final handler is executed (e.g. check authentication)
 * 2. decide whether to proceed or not with the next handler
 * */

using ArMiddlewareNext = std::function<void(void)>;
using ArMiddlewareCallback = std::function<void(AsyncWebServerRequest *request, ArMiddlewareNext next)>;

// Middleware is a base class for all middleware
class AsyncMiddleware {
public:
  virtual ~AsyncMiddleware() {}
  virtual void run(__unused AsyncWebServerRequest *request, __unused ArMiddlewareNext next) {
    return next();
  };

private:
  friend class AsyncWebHandler;
  friend class AsyncEventSource;
  friend class AsyncMiddlewareChain;
  bool _freeOnRemoval = false;
};

// Create a custom middleware by providing an anonymous callback function
class AsyncMiddlewareFunction : public AsyncMiddleware {
public:
  AsyncMiddlewareFunction(ArMiddlewareCallback fn) : _fn(fn) {}
  void run(AsyncWebServerRequest *request, ArMiddlewareNext next) override {
    return _fn(request, next);
  };

private:
  ArMiddlewareCallback _fn;
};

// For internal use only: super class to add/remove middleware to server or handlers
class AsyncMiddlewareChain {
public:
  ~AsyncMiddlewareChain();

  void addMiddleware(ArMiddlewareCallback fn);
  void addMiddleware(AsyncMiddleware *middleware);
  void addMiddlewares(std::vector<AsyncMiddleware *> middlewares);
  bool removeMiddleware(AsyncMiddleware *middleware);

  // For internal use only
  void _runChain(AsyncWebServerRequest *request, ArMiddlewareNext finalizer);

protected:
  std::list<AsyncMiddleware *> _middlewares;
};

// AsyncAuthenticationMiddleware is a middleware that checks if the request is authenticated
class AsyncAuthenticationMiddleware : public AsyncMiddleware {
public:
  void setUsername(const char *username);
  void setPassword(const char *password);
  void setPasswordHash(const char *hash);

  void setRealm(const char *realm) {
    _realm = realm;
  }
  void setAuthFailureMessage(const char *message) {
    _authFailMsg = message;
  }

  // set the authentication method to use
  // default is AUTH_NONE: no authentication required
  // AUTH_BASIC: basic authentication
  // AUTH_DIGEST: digest authentication
  // AUTH_BEARER: bearer token authentication
  // AUTH_OTHER: other authentication method
  // AUTH_DENIED: always return 401 Unauthorized
  // if a method is set but no username or password is set, authentication will be ignored
  void setAuthType(AsyncAuthType authMethod) {
    _authMethod = authMethod;
  }

  // precompute and store the hash value based on the username, password, realm.
  // can be used for DIGEST and BASIC to avoid recomputing the hash for each request.
  // returns true if the hash was successfully generated and replaced
  bool generateHash();

  // returns true if the username and password (or hash) are set
  bool hasCredentials() const {
    return _hasCreds;
  }

  bool allowed(AsyncWebServerRequest *request) const;

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  String _username;
  String _credentials;
  bool _hash = false;

  String _realm = asyncsrv::T_LOGIN_REQ;
  AsyncAuthType _authMethod = AsyncAuthType::AUTH_NONE;
  String _authFailMsg;
  bool _hasCreds = false;
};

using ArAuthorizeFunction = std::function<bool(AsyncWebServerRequest *request)>;
// AsyncAuthorizationMiddleware is a middleware that checks if the request is authorized
class AsyncAuthorizationMiddleware : public AsyncMiddleware {
public:
  AsyncAuthorizationMiddleware(ArAuthorizeFunction authorizeConnectHandler) : _code(403), _authz(authorizeConnectHandler) {}
  AsyncAuthorizationMiddleware(int code, ArAuthorizeFunction authorizeConnectHandler) : _code(code), _authz(authorizeConnectHandler) {}

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next) {
    return _authz && !_authz(request) ? request->send(_code) : next();
  }

private:
  int _code;
  ArAuthorizeFunction _authz;
};

// remove all headers from the incoming request except the ones provided in the constructor
class AsyncHeaderFreeMiddleware : public AsyncMiddleware {
public:
  void keep(const char *name) {
    _toKeep.push_back(name);
  }
  void unKeep(const char *name) {
    _toKeep.remove(name);
  }

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  std::list<const char *> _toKeep;
};

// filter out specific headers from the incoming request
class AsyncHeaderFilterMiddleware : public AsyncMiddleware {
public:
  void filter(const char *name) {
    _toRemove.push_back(name);
  }
  void unFilter(const char *name) {
    _toRemove.remove(name);
  }

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  std::list<const char *> _toRemove;
};

// curl-like logging of incoming requests
class AsyncLoggingMiddleware : public AsyncMiddleware {
public:
  void setOutput(Print &output) {
    _out = &output;
  }
  void setEnabled(bool enabled) {
    _enabled = enabled;
  }
  bool isEnabled() const {
    return _enabled && _out;
  }

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  Print *_out = nullptr;
  bool _enabled = true;
};

// CORS Middleware
class AsyncCorsMiddleware : public AsyncMiddleware {
public:
  void setOrigin(const char *origin) {
    _origin = origin;
  }
  void setMethods(const char *methods) {
    _methods = methods;
  }
  void setHeaders(const char *headers) {
    _headers = headers;
  }
  void setAllowCredentials(bool credentials) {
    _credentials = credentials;
  }
  void setMaxAge(uint32_t seconds) {
    _maxAge = seconds;
  }

  void addCORSHeaders(AsyncWebServerResponse *response);

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  String _origin = "*";
  String _methods = "*";
  String _headers = "*";
  bool _credentials = true;
  uint32_t _maxAge = 86400;
};

// Rate limit Middleware
class AsyncRateLimitMiddleware : public AsyncMiddleware {
public:
  void setMaxRequests(size_t maxRequests) {
    _maxRequests = maxRequests;
  }
  void setWindowSize(uint32_t seconds) {
    _windowSizeMillis = seconds * 1000;
  }

  bool isRequestAllowed(uint32_t &retryAfterSeconds);

  void run(AsyncWebServerRequest *request, ArMiddlewareNext next);

private:
  size_t _maxRequests = 0;
  uint32_t _windowSizeMillis = 0;
  std::list<uint32_t> _requestTimes;
};

/*
 * REWRITE :: One instance can be handle any Request (done by the Server)
 * */

class AsyncWebRewrite {
protected:
  String _from;
  String _toUrl;
  String _params;
  ArRequestFilterFunction _filter{nullptr};

public:
  AsyncWebRewrite(const char *from, const char *to) : _from(from), _toUrl(to) {
    int index = _toUrl.indexOf('?');
    if (index > 0) {
      _params = _toUrl.substring(index + 1);
      _toUrl = _toUrl.substring(0, index);
    }
  }
  virtual ~AsyncWebRewrite() {}
  AsyncWebRewrite &setFilter(ArRequestFilterFunction fn) {
    _filter = fn;
    return *this;
  }
  bool filter(AsyncWebServerRequest *request) const {
    return _filter == NULL || _filter(request);
  }
  const String &from(void) const {
    return _from;
  }
  const String &toUrl(void) const {
    return _toUrl;
  }
  const String &params(void) const {
    return _params;
  }
  virtual bool match(AsyncWebServerRequest *request) {
    return from() == request->url() && filter(request);
  }
};

/*
 * HANDLER :: One instance can be attached to any Request (done by the Server)
 * */

class AsyncWebHandler : public AsyncMiddlewareChain {
protected:
  ArRequestFilterFunction _filter = nullptr;
  AsyncAuthenticationMiddleware *_authMiddleware = nullptr;
  bool _skipServerMiddlewares = false;

public:
  AsyncWebHandler() {}
  virtual ~AsyncWebHandler() {}
  AsyncWebHandler &setFilter(ArRequestFilterFunction fn);
  AsyncWebHandler &setAuthentication(const char *username, const char *password, AsyncAuthType authMethod = AsyncAuthType::AUTH_DIGEST);
  AsyncWebHandler &setAuthentication(const String &username, const String &password, AsyncAuthType authMethod = AsyncAuthType::AUTH_DIGEST) {
    return setAuthentication(username.c_str(), password.c_str(), authMethod);
  };
  AsyncWebHandler &setSkipServerMiddlewares(bool state) {
    _skipServerMiddlewares = state;
    return *this;
  }
  // skip all globally defined server middlewares for this handler and only execute those defined for this handler specifically
  AsyncWebHandler &skipServerMiddlewares() {
    return setSkipServerMiddlewares(true);
  }
  bool mustSkipServerMiddlewares() const {
    return _skipServerMiddlewares;
  }
  bool filter(AsyncWebServerRequest *request) {
    return _filter == NULL || _filter(request);
  }
  virtual bool canHandle(AsyncWebServerRequest *request __attribute__((unused))) const {
    return false;
  }
  virtual void handleRequest(__unused AsyncWebServerRequest *request) {}
  virtual void handleUpload(
    __unused AsyncWebServerRequest *request, __unused const String &filename, __unused size_t index, __unused uint8_t *data, __unused size_t len,
    __unused bool final
  ) {}
  virtual void handleBody(__unused AsyncWebServerRequest *request, __unused uint8_t *data, __unused size_t len, __unused size_t index, __unused size_t total) {}
  virtual bool isRequestHandlerTrivial() const {
    return true;
  }
};

/*
 * RESPONSE :: One instance is created for each Request (attached by the Handler)
 * */

typedef enum {
  RESPONSE_SETUP,
  RESPONSE_HEADERS,
  RESPONSE_CONTENT,
  RESPONSE_WAIT_ACK,
  RESPONSE_END,
  RESPONSE_FAILED
} WebResponseState;

class AsyncWebServerResponse {
protected:
  int _code;
  std::list<AsyncWebHeader> _headers;
  String _contentType;
  size_t _contentLength;
  bool _sendContentLength;
  bool _chunked;
  size_t _headLength;
  size_t _sentLength;
  size_t _ackedLength;
  size_t _writtenLength;
  WebResponseState _state;

  static bool headerMustBePresentOnce(const String &name);

public:
  static const char *responseCodeToString(int code);

public:
  AsyncWebServerResponse();
  virtual ~AsyncWebServerResponse() {}
  void setCode(int code);
  int code() const {
    return _code;
  }
  void setContentLength(size_t len);
  void setContentType(const String &type) {
    setContentType(type.c_str());
  }
  void setContentType(const char *type);
  bool addHeader(AsyncWebHeader &&header, bool replaceExisting = true);
  bool addHeader(const AsyncWebHeader &header, bool replaceExisting = true) {
    return header && addHeader(header.name(), header.value(), replaceExisting);
  }
  bool addHeader(const char *name, const char *value, bool replaceExisting = true);
  bool addHeader(const String &name, const String &value, bool replaceExisting = true) {
    return addHeader(name.c_str(), value.c_str(), replaceExisting);
  }
  bool addHeader(const char *name, long value, bool replaceExisting = true) {
    return addHeader(name, String(value), replaceExisting);
  }
  bool addHeader(const String &name, long value, bool replaceExisting = true) {
    return addHeader(name.c_str(), value, replaceExisting);
  }
  bool removeHeader(const char *name);
  bool removeHeader(const char *name, const char *value);
  const AsyncWebHeader *getHeader(const char *name) const;
  const std::list<AsyncWebHeader> &getHeaders() const {
    return _headers;
  }

#ifndef ESP8266
  [[deprecated("Use instead: _assembleHead(String& buffer, uint8_t version)")]]
#endif
  String _assembleHead(uint8_t version) {
    String buffer;
    _assembleHead(buffer, version);
    return buffer;
  }
  void _assembleHead(String &buffer, uint8_t version);

  virtual bool _started() const;
  virtual bool _finished() const;
  virtual bool _failed() const;
  virtual bool _sourceValid() const;
  virtual void _respond(AsyncWebServerRequest *request);
  virtual size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
};

/*
 * SERVER :: One instance
 * */

typedef std::function<void(AsyncWebServerRequest *request)> ArRequestHandlerFunction;
typedef std::function<void(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)>
  ArUploadHandlerFunction;
typedef std::function<void(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)> ArBodyHandlerFunction;

class AsyncWebServer : public AsyncMiddlewareChain {
protected:
  AsyncServer _server;
  std::list<std::shared_ptr<AsyncWebRewrite>> _rewrites;
  std::list<std::unique_ptr<AsyncWebHandler>> _handlers;
  AsyncCallbackWebHandler *_catchAllHandler;

public:
  AsyncWebServer(uint16_t port);
  ~AsyncWebServer();

  void begin();
  void end();

  tcp_state state() const {
#ifdef ESP8266
    // ESPAsyncTCP and RPAsyncTCP methods are not corrected declared with const for immutable ones.
    return static_cast<tcp_state>(const_cast<AsyncWebServer *>(this)->_server.status());
#else
    return static_cast<tcp_state>(_server.status());
#endif
  }

#if ASYNC_TCP_SSL_ENABLED
  void onSslFileRequest(AcSSlFileHandler cb, void *arg);
  void beginSecure(const char *cert, const char *private_key_file, const char *password);
#endif

  AsyncWebRewrite &addRewrite(AsyncWebRewrite *rewrite);

  /**
     * @brief (compat) Add url rewrite rule by pointer
     * a deep copy of the pointer object will be created,
     * it is up to user to manage further lifetime of the object in argument
     *
     * @param rewrite pointer to rewrite object to copy setting from
     * @return AsyncWebRewrite& reference to a newly created rewrite rule
     */
  AsyncWebRewrite &addRewrite(std::shared_ptr<AsyncWebRewrite> rewrite);

  /**
     * @brief add url rewrite rule
     *
     * @param from
     * @param to
     * @return AsyncWebRewrite&
     */
  AsyncWebRewrite &rewrite(const char *from, const char *to);

  /**
     * @brief (compat) remove rewrite rule via referenced object
     * this will NOT deallocate pointed object itself, internal rule with same from/to urls will be removed if any
     * it's a compat method, better use `removeRewrite(const char* from, const char* to)`
     * @param rewrite
     * @return true
     * @return false
     */
  bool removeRewrite(AsyncWebRewrite *rewrite);

  /**
     * @brief remove rewrite rule
     *
     * @param from
     * @param to
     * @return true
     * @return false
     */
  bool removeRewrite(const char *from, const char *to);

  AsyncWebHandler &addHandler(AsyncWebHandler *handler);
  bool removeHandler(AsyncWebHandler *handler);

  AsyncCallbackWebHandler &on(const char *uri, ArRequestHandlerFunction onRequest) {
    return on(uri, HTTP_ANY, onRequest);
  }
  AsyncCallbackWebHandler &on(
    const char *uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload = nullptr,
    ArBodyHandlerFunction onBody = nullptr
  );

  AsyncStaticWebHandler &serveStatic(const char *uri, fs::FS &fs, const char *path, const char *cache_control = NULL);

  void onNotFound(ArRequestHandlerFunction fn);   // called when handler is not assigned
  void onFileUpload(ArUploadHandlerFunction fn);  // handle file uploads
  void onRequestBody(ArBodyHandlerFunction fn);   // handle posts with plain body content (JSON often transmitted this way as a request)
  // give access to the handler used to catch all requests, so that middleware can be added to it
  AsyncWebHandler &catchAllHandler() const;

  void reset();  // remove all writers and handlers, with onNotFound/onFileUpload/onRequestBody

  void _handleDisconnect(AsyncWebServerRequest *request);
  void _attachHandler(AsyncWebServerRequest *request);
  void _rewriteRequest(AsyncWebServerRequest *request);
};

class DefaultHeaders {
  using headers_t = std::list<AsyncWebHeader>;
  headers_t _headers;

public:
  DefaultHeaders() = default;

  using ConstIterator = headers_t::const_iterator;

  void addHeader(const String &name, const String &value) {
    _headers.emplace_back(name, value);
  }

  ConstIterator begin() const {
    return _headers.begin();
  }
  ConstIterator end() const {
    return _headers.end();
  }

  DefaultHeaders(DefaultHeaders const &) = delete;
  DefaultHeaders &operator=(DefaultHeaders const &) = delete;

  static DefaultHeaders &Instance() {
    static DefaultHeaders instance;
    return instance;
  }
};

#include "AsyncEventSource.h"
#include "AsyncWebSocket.h"
#include "WebHandlerImpl.h"
#include "WebResponseImpl.h"

#endif /* _AsyncWebServer_H_ */
