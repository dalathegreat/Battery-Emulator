// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include "ESPAsyncWebServer.h"
#include "WebResponseImpl.h"

using namespace asyncsrv;

// Since ESP8266 does not link memchr by default, here's its implementation.
void *memchr(void *ptr, int ch, size_t count) {
  unsigned char *p = static_cast<unsigned char *>(ptr);
  while (count--) {
    if (*p++ == static_cast<unsigned char>(ch)) {
      return --p;
    }
  }
  return nullptr;
}

/*
 * Abstract Response
 *
 */

const char *AsyncWebServerResponse::responseCodeToString(int code) {
  switch (code) {
    case 100: return T_HTTP_CODE_100;
    case 101: return T_HTTP_CODE_101;
    case 200: return T_HTTP_CODE_200;
    case 201: return T_HTTP_CODE_201;
    case 202: return T_HTTP_CODE_202;
    case 203: return T_HTTP_CODE_203;
    case 204: return T_HTTP_CODE_204;
    case 205: return T_HTTP_CODE_205;
    case 206: return T_HTTP_CODE_206;
    case 300: return T_HTTP_CODE_300;
    case 301: return T_HTTP_CODE_301;
    case 302: return T_HTTP_CODE_302;
    case 303: return T_HTTP_CODE_303;
    case 304: return T_HTTP_CODE_304;
    case 305: return T_HTTP_CODE_305;
    case 307: return T_HTTP_CODE_307;
    case 400: return T_HTTP_CODE_400;
    case 401: return T_HTTP_CODE_401;
    case 402: return T_HTTP_CODE_402;
    case 403: return T_HTTP_CODE_403;
    case 404: return T_HTTP_CODE_404;
    case 405: return T_HTTP_CODE_405;
    case 406: return T_HTTP_CODE_406;
    case 407: return T_HTTP_CODE_407;
    case 408: return T_HTTP_CODE_408;
    case 409: return T_HTTP_CODE_409;
    case 410: return T_HTTP_CODE_410;
    case 411: return T_HTTP_CODE_411;
    case 412: return T_HTTP_CODE_412;
    case 413: return T_HTTP_CODE_413;
    case 414: return T_HTTP_CODE_414;
    case 415: return T_HTTP_CODE_415;
    case 416: return T_HTTP_CODE_416;
    case 417: return T_HTTP_CODE_417;
    case 429: return T_HTTP_CODE_429;
    case 500: return T_HTTP_CODE_500;
    case 501: return T_HTTP_CODE_501;
    case 502: return T_HTTP_CODE_502;
    case 503: return T_HTTP_CODE_503;
    case 504: return T_HTTP_CODE_504;
    case 505: return T_HTTP_CODE_505;
    default:  return T_HTTP_CODE_ANY;
  }
}

AsyncWebServerResponse::AsyncWebServerResponse()
  : _code(0), _contentType(), _contentLength(0), _sendContentLength(true), _chunked(false), _headLength(0), _sentLength(0), _ackedLength(0), _writtenLength(0),
    _state(RESPONSE_SETUP) {
  for (const auto &header : DefaultHeaders::Instance()) {
    _headers.emplace_back(header);
  }
}

void AsyncWebServerResponse::setCode(int code) {
  if (_state == RESPONSE_SETUP) {
    _code = code;
  }
}

void AsyncWebServerResponse::setContentLength(size_t len) {
  if (_state == RESPONSE_SETUP && addHeader(T_Content_Length, len, true)) {
    _contentLength = len;
  }
}

void AsyncWebServerResponse::setContentType(const char *type) {
  if (_state == RESPONSE_SETUP && addHeader(T_Content_Type, type, true)) {
    _contentType = type;
  }
}

bool AsyncWebServerResponse::removeHeader(const char *name) {
  bool h_erased = false;
  for (auto i = _headers.begin(); i != _headers.end();) {
    if (i->name().equalsIgnoreCase(name)) {
      _headers.erase(i);
      h_erased = true;
    } else {
      ++i;
    }
  }
  return h_erased;
}

bool AsyncWebServerResponse::removeHeader(const char *name, const char *value) {
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(name) && i->value().equalsIgnoreCase(value)) {
      _headers.erase(i);
      return true;
    }
  }
  return false;
}

const AsyncWebHeader *AsyncWebServerResponse::getHeader(const char *name) const {
  auto iter = std::find_if(std::begin(_headers), std::end(_headers), [&name](const AsyncWebHeader &header) {
    return header.name().equalsIgnoreCase(name);
  });
  return (iter == std::end(_headers)) ? nullptr : &(*iter);
}

bool AsyncWebServerResponse::headerMustBePresentOnce(const String &name) {
  for (uint8_t i = 0; i < T_only_once_headers_len; i++) {
    if (name.equalsIgnoreCase(T_only_once_headers[i])) {
      return true;
    }
  }
  return false;
}

bool AsyncWebServerResponse::addHeader(AsyncWebHeader &&header, bool replaceExisting) {
  if (!header) {
    return false;  // invalid header
  }
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(header.name())) {
      // header already set
      if (replaceExisting) {
        // remove, break and add the new one
        _headers.erase(i);
        break;
      } else if (headerMustBePresentOnce(i->name())) {  // we can have only one header with that name
        // do not update
        return false;
      } else {
        break;  // accept multiple headers with the same name
      }
    }
  }
  // header was not found found, or existing one was removed
  _headers.emplace_back(std::move(header));
  return true;
}

bool AsyncWebServerResponse::addHeader(const char *name, const char *value, bool replaceExisting) {
  for (auto i = _headers.begin(); i != _headers.end(); ++i) {
    if (i->name().equalsIgnoreCase(name)) {
      // header already set
      if (replaceExisting) {
        // remove, break and add the new one
        _headers.erase(i);
        break;
      } else if (headerMustBePresentOnce(i->name())) {  // we can have only one header with that name
        // do not update
        return false;
      } else {
        break;  // accept multiple headers with the same name
      }
    }
  }
  // header was not found found, or existing one was removed
  _headers.emplace_back(name, value);
  return true;
}

void AsyncWebServerResponse::_assembleHead(String &buffer, uint8_t version) {
  if (version) {
    addHeader(T_Accept_Ranges, T_none, false);
    if (_chunked) {
      addHeader(T_Transfer_Encoding, T_chunked, false);
    }
  }

  if (_sendContentLength) {
    addHeader(T_Content_Length, String(_contentLength), false);
  }

  if (_contentType.length()) {
    addHeader(T_Content_Type, _contentType.c_str(), false);
  }

  // precompute buffer size to avoid reallocations by String class
  size_t len = 0;
  len += 50;  // HTTP/1.1 200 <reason>\r\n
  for (const auto &header : _headers) {
    len += header.name().length() + header.value().length() + 4;
  }

  // prepare buffer
  buffer.reserve(len);

  // HTTP header
#ifdef ESP8266
  buffer.concat(PSTR("HTTP/1."));
#else
  buffer.concat("HTTP/1.");
#endif
  buffer.concat(version);
  buffer.concat(' ');
  buffer.concat(_code);
  buffer.concat(' ');
  buffer.concat(responseCodeToString(_code));
  buffer.concat(T_rn);

  // Add headers
  for (const auto &header : _headers) {
    buffer.concat(header.name());
#ifdef ESP8266
    buffer.concat(PSTR(": "));
#else
    buffer.concat(": ");
#endif
    buffer.concat(header.value());
    buffer.concat(T_rn);
  }

  buffer.concat(T_rn);
  _headLength = buffer.length();
}

bool AsyncWebServerResponse::_started() const {
  return _state > RESPONSE_SETUP;
}
bool AsyncWebServerResponse::_finished() const {
  return _state > RESPONSE_WAIT_ACK;
}
bool AsyncWebServerResponse::_failed() const {
  return _state == RESPONSE_FAILED;
}
bool AsyncWebServerResponse::_sourceValid() const {
  return false;
}
void AsyncWebServerResponse::_respond(AsyncWebServerRequest *request) {
  _state = RESPONSE_END;
  request->client()->close();
}
size_t AsyncWebServerResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time) {
  (void)request;
  (void)len;
  (void)time;
  return 0;
}

/*
 * String/Code Response
 * */
AsyncBasicResponse::AsyncBasicResponse(int code, const char *contentType, const char *content) {
  _code = code;
  _content = content;
  _contentType = contentType;
  if (_content.length()) {
    _contentLength = _content.length();
    if (!_contentType.length()) {
      _contentType = T_text_plain;
    }
  }
  addHeader(T_Connection, T_close, false);
}

void AsyncBasicResponse::_respond(AsyncWebServerRequest *request) {
  _state = RESPONSE_HEADERS;
  String out;
  _assembleHead(out, request->version());
  size_t outLen = out.length();
  size_t space = request->client()->space();
  if (!_contentLength && space >= outLen) {
    _writtenLength += request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_WAIT_ACK;
  } else if (_contentLength && space >= outLen + _contentLength) {
    out += _content;
    outLen += _contentLength;
    _writtenLength += request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_WAIT_ACK;
  } else if (space && space < outLen) {
    String partial = out.substring(0, space);
    _content = out.substring(space) + _content;
    _contentLength += outLen - space;
    _writtenLength += request->client()->write(partial.c_str(), partial.length());
    _state = RESPONSE_CONTENT;
  } else if (space > outLen && space < (outLen + _contentLength)) {
    size_t shift = space - outLen;
    outLen += shift;
    _sentLength += shift;
    out += _content.substring(0, shift);
    _content = _content.substring(shift);
    _writtenLength += request->client()->write(out.c_str(), outLen);
    _state = RESPONSE_CONTENT;
  } else {
    _content = out + _content;
    _contentLength += outLen;
    _state = RESPONSE_CONTENT;
  }
}

size_t AsyncBasicResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time) {
  (void)time;
  _ackedLength += len;
  if (_state == RESPONSE_CONTENT) {
    size_t available = _contentLength - _sentLength;
    size_t space = request->client()->space();
    // we can fit in this packet
    if (space > available) {
      _writtenLength += request->client()->write(_content.c_str(), available);
      _content = emptyString;
      _state = RESPONSE_WAIT_ACK;
      return available;
    }
    // send some data, the rest on ack
    String out = _content.substring(0, space);
    _content = _content.substring(space);
    _sentLength += space;
    _writtenLength += request->client()->write(out.c_str(), space);
    return space;
  } else if (_state == RESPONSE_WAIT_ACK) {
    if (_ackedLength >= _writtenLength) {
      _state = RESPONSE_END;
    }
  }
  return 0;
}

/*
 * Abstract Response
 * */

AsyncAbstractResponse::AsyncAbstractResponse(AwsTemplateProcessor callback) : _callback(callback) {
  // In case of template processing, we're unable to determine real response size
  if (callback) {
    _contentLength = 0;
    _sendContentLength = false;
    _chunked = true;
  }
}

void AsyncAbstractResponse::_respond(AsyncWebServerRequest *request) {
  addHeader(T_Connection, T_close, false);
  _assembleHead(_head, request->version());
  _state = RESPONSE_HEADERS;
  _ack(request, 0, 0);
}

size_t AsyncAbstractResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time) {
  (void)time;
  if (!_sourceValid()) {
    _state = RESPONSE_FAILED;
    request->client()->close();
    return 0;
  }

#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
  // return a credit for each chunk of acked data (polls does not give any credits)
  if (len) {
    ++_in_flight_credit;
  }

  // for chunked responses ignore acks if there are no _in_flight_credits left
  if (_chunked && !_in_flight_credit) {
#ifdef ESP32
    log_d("(chunk) out of in-flight credits");
#endif
    return 0;
  }

  _in_flight -= (_in_flight > len) ? len : _in_flight;
  // get the size of available sock space
#endif

  _ackedLength += len;
  size_t space = request->client()->space();

  size_t headLen = _head.length();
  if (_state == RESPONSE_HEADERS) {
    if (space >= headLen) {
      _state = RESPONSE_CONTENT;
      space -= headLen;
    } else {
      String out = _head.substring(0, space);
      _head = _head.substring(space);
      _writtenLength += request->client()->write(out.c_str(), out.length());
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
      _in_flight += out.length();
      --_in_flight_credit;  // take a credit
#endif
      return out.length();
    }
  }

  if (_state == RESPONSE_CONTENT) {
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
    // for response data we need to control the queue and in-flight fragmentation. Sending small chunks could give low latency,
    // but flood asynctcp's queue and fragment socket buffer space for large responses.
    // Let's ignore polled acks and acks in case when we have more in-flight data then the available socket buff space.
    // That way we could balance on having half the buffer in-flight while another half is filling up, while minimizing events in asynctcp q
    if (_in_flight > space) {
      // log_d("defer user call %u/%u", _in_flight, space);
      //  take the credit back since we are ignoring this ack and rely on other inflight data
      if (len) {
        --_in_flight_credit;
      }
      return 0;
    }
#endif

    size_t outLen;
    if (_chunked) {
      if (space <= 8) {
        return 0;
      }

      outLen = space;
    } else if (!_sendContentLength) {
      outLen = space;
    } else {
      outLen = ((_contentLength - _sentLength) > space) ? space : (_contentLength - _sentLength);
    }

    uint8_t *buf = (uint8_t *)malloc(outLen + headLen);
    if (!buf) {
#ifdef ESP32
      log_e("Failed to allocate");
#endif
      request->abort();
      return 0;
    }

    if (headLen) {
      memcpy(buf, _head.c_str(), _head.length());
    }

    size_t readLen = 0;

    if (_chunked) {
      // HTTP 1.1 allows leading zeros in chunk length. Or spaces may be added.
      // See RFC2616 sections 2, 3.6.1.
      readLen = _fillBufferAndProcessTemplates(buf + headLen + 6, outLen - 8);
      if (readLen == RESPONSE_TRY_AGAIN) {
        free(buf);
        return 0;
      }
      outLen = sprintf((char *)buf + headLen, "%04x", readLen) + headLen;
      buf[outLen++] = '\r';
      buf[outLen++] = '\n';
      outLen += readLen;
      buf[outLen++] = '\r';
      buf[outLen++] = '\n';
    } else {
      readLen = _fillBufferAndProcessTemplates(buf + headLen, outLen);
      if (readLen == RESPONSE_TRY_AGAIN) {
        free(buf);
        return 0;
      }
      outLen = readLen + headLen;
    }

    if (headLen) {
      _head = emptyString;
    }

    if (outLen) {
      _writtenLength += request->client()->write((const char *)buf, outLen);
#if ASYNCWEBSERVER_USE_CHUNK_INFLIGHT
      _in_flight += outLen;
      --_in_flight_credit;  // take a credit
#endif
    }

    if (_chunked) {
      _sentLength += readLen;
    } else {
      _sentLength += outLen - headLen;
    }

    free(buf);

    if ((_chunked && readLen == 0) || (!_sendContentLength && outLen == 0) || (!_chunked && _sentLength == _contentLength)) {
      _state = RESPONSE_WAIT_ACK;
    }
    return outLen;

  } else if (_state == RESPONSE_WAIT_ACK) {
    if (!_sendContentLength || _ackedLength >= _writtenLength) {
      _state = RESPONSE_END;
      if (!_chunked && !_sendContentLength) {
        request->client()->close(true);
      }
    }
  }
  return 0;
}

size_t AsyncAbstractResponse::_readDataFromCacheOrContent(uint8_t *data, const size_t len) {
  // If we have something in cache, copy it to buffer
  const size_t readFromCache = std::min(len, _cache.size());
  if (readFromCache) {
    memcpy(data, _cache.data(), readFromCache);
    _cache.erase(_cache.begin(), _cache.begin() + readFromCache);
  }
  // If we need to read more...
  const size_t needFromFile = len - readFromCache;
  const size_t readFromContent = _fillBuffer(data + readFromCache, needFromFile);
  return readFromCache + readFromContent;
}

size_t AsyncAbstractResponse::_fillBufferAndProcessTemplates(uint8_t *data, size_t len) {
  if (!_callback) {
    return _fillBuffer(data, len);
  }

  const size_t originalLen = len;
  len = _readDataFromCacheOrContent(data, len);
  // Now we've read 'len' bytes, either from cache or from file
  // Search for template placeholders
  uint8_t *pTemplateStart = data;
  while ((pTemplateStart < &data[len]) && (pTemplateStart = (uint8_t *)memchr(pTemplateStart, TEMPLATE_PLACEHOLDER, &data[len - 1] - pTemplateStart + 1))
  ) {  // data[0] ... data[len - 1]
    uint8_t *pTemplateEnd =
      (pTemplateStart < &data[len - 1]) ? (uint8_t *)memchr(pTemplateStart + 1, TEMPLATE_PLACEHOLDER, &data[len - 1] - pTemplateStart) : nullptr;
    // temporary buffer to hold parameter name
    uint8_t buf[TEMPLATE_PARAM_NAME_LENGTH + 1];
    String paramName;
    // If closing placeholder is found:
    if (pTemplateEnd) {
      // prepare argument to callback
      const size_t paramNameLength = std::min((size_t)sizeof(buf) - 1, (size_t)(pTemplateEnd - pTemplateStart - 1));
      if (paramNameLength) {
        memcpy(buf, pTemplateStart + 1, paramNameLength);
        buf[paramNameLength] = 0;
        paramName = String(reinterpret_cast<char *>(buf));
      } else {  // double percent sign encountered, this is single percent sign escaped.
        // remove the 2nd percent sign
        memmove(pTemplateEnd, pTemplateEnd + 1, &data[len] - pTemplateEnd - 1);
        len += _readDataFromCacheOrContent(&data[len - 1], 1) - 1;
        ++pTemplateStart;
      }
    } else if (&data[len - 1] - pTemplateStart + 1
               < TEMPLATE_PARAM_NAME_LENGTH + 2) {  // closing placeholder not found, check if it's in the remaining file data
      memcpy(buf, pTemplateStart + 1, &data[len - 1] - pTemplateStart);
      const size_t readFromCacheOrContent =
        _readDataFromCacheOrContent(buf + (&data[len - 1] - pTemplateStart), TEMPLATE_PARAM_NAME_LENGTH + 2 - (&data[len - 1] - pTemplateStart + 1));
      if (readFromCacheOrContent) {
        pTemplateEnd = (uint8_t *)memchr(buf + (&data[len - 1] - pTemplateStart), TEMPLATE_PLACEHOLDER, readFromCacheOrContent);
        if (pTemplateEnd) {
          // prepare argument to callback
          *pTemplateEnd = 0;
          paramName = String(reinterpret_cast<char *>(buf));
          // Copy remaining read-ahead data into cache
          _cache.insert(_cache.begin(), pTemplateEnd + 1, buf + (&data[len - 1] - pTemplateStart) + readFromCacheOrContent);
          pTemplateEnd = &data[len - 1];
        } else  // closing placeholder not found in file data, store found percent symbol as is and advance to the next position
        {
          // but first, store read file data in cache
          _cache.insert(_cache.begin(), buf + (&data[len - 1] - pTemplateStart), buf + (&data[len - 1] - pTemplateStart) + readFromCacheOrContent);
          ++pTemplateStart;
        }
      } else {  // closing placeholder not found in content data, store found percent symbol as is and advance to the next position
        ++pTemplateStart;
      }
    } else {  // closing placeholder not found in content data, store found percent symbol as is and advance to the next position
      ++pTemplateStart;
    }
    if (paramName.length()) {
      // call callback and replace with result.
      // Everything in range [pTemplateStart, pTemplateEnd] can be safely replaced with parameter value.
      // Data after pTemplateEnd may need to be moved.
      // The first byte of data after placeholder is located at pTemplateEnd + 1.
      // It should be located at pTemplateStart + numBytesCopied (to begin right after inserted parameter value).
      const String paramValue(_callback(paramName));
      const char *pvstr = paramValue.c_str();
      const unsigned int pvlen = paramValue.length();
      const size_t numBytesCopied = std::min(pvlen, static_cast<unsigned int>(&data[originalLen - 1] - pTemplateStart + 1));
      // make room for param value
      // 1. move extra data to cache if parameter value is longer than placeholder AND if there is no room to store
      if ((pTemplateEnd + 1 < pTemplateStart + numBytesCopied) && (originalLen - (pTemplateStart + numBytesCopied - pTemplateEnd - 1) < len)) {
        _cache.insert(_cache.begin(), &data[originalLen - (pTemplateStart + numBytesCopied - pTemplateEnd - 1)], &data[len]);
        // 2. parameter value is longer than placeholder text, push the data after placeholder which not saved into cache further to the end
        memmove(pTemplateStart + numBytesCopied, pTemplateEnd + 1, &data[originalLen] - pTemplateStart - numBytesCopied);
        len = originalLen;  // fix issue with truncated data, not sure if it has any side effects
      } else if (pTemplateEnd + 1 != pTemplateStart + numBytesCopied) {
        // 2. Either parameter value is shorter than placeholder text OR there is enough free space in buffer to fit.
        //    Move the entire data after the placeholder
        memmove(pTemplateStart + numBytesCopied, pTemplateEnd + 1, &data[len] - pTemplateEnd - 1);
      }
      // 3. replace placeholder with actual value
      memcpy(pTemplateStart, pvstr, numBytesCopied);
      // If result is longer than buffer, copy the remainder into cache (this could happen only if placeholder text itself did not fit entirely in buffer)
      if (numBytesCopied < pvlen) {
        _cache.insert(_cache.begin(), pvstr + numBytesCopied, pvstr + pvlen);
      } else if (pTemplateStart + numBytesCopied < pTemplateEnd + 1) {  // result is copied fully; if result is shorter than placeholder text...
        // there is some free room, fill it from cache
        const size_t roomFreed = pTemplateEnd + 1 - pTemplateStart - numBytesCopied;
        const size_t totalFreeRoom = originalLen - len + roomFreed;
        len += _readDataFromCacheOrContent(&data[len - roomFreed], totalFreeRoom) - roomFreed;
      } else {  // result is copied fully; it is longer than placeholder text
        const size_t roomTaken = pTemplateStart + numBytesCopied - pTemplateEnd - 1;
        len = std::min(len + roomTaken, originalLen);
      }
    }
  }  // while(pTemplateStart)
  return len;
}

/*
 * File Response
 * */

/**
 * @brief Sets the content type based on the file path extension
 *
 * This method determines the appropriate MIME content type for a file based on its
 * file extension. It supports both external content type functions (if available)
 * and an internal mapping of common file extensions to their corresponding MIME types.
 *
 * @param path The file path string from which to extract the extension
 * @note The method modifies the internal _contentType member variable
 */
void AsyncFileResponse::_setContentTypeFromPath(const String &path) {
#if HAVE_EXTERN_GET_Content_Type_FUNCTION
#ifndef ESP8266
  extern const char *getContentType(const String &path);
#else
  extern const __FlashStringHelper *getContentType(const String &path);
#endif
  _contentType = getContentType(path);
#else
  const char *cpath = path.c_str();
  const char *dot = strrchr(cpath, '.');

  if (!dot) {
    _contentType = T_text_plain;
    return;
  }

  if (strcmp(dot, T__html) == 0 || strcmp(dot, T__htm) == 0) {
    _contentType = T_text_html;
  } else if (strcmp(dot, T__css) == 0) {
    _contentType = T_text_css;
  } else if (strcmp(dot, T__js) == 0) {
    _contentType = T_application_javascript;
  } else if (strcmp(dot, T__json) == 0) {
    _contentType = T_application_json;
  } else if (strcmp(dot, T__png) == 0) {
    _contentType = T_image_png;
  } else if (strcmp(dot, T__ico) == 0) {
    _contentType = T_image_x_icon;
  } else if (strcmp(dot, T__svg) == 0) {
    _contentType = T_image_svg_xml;
  } else if (strcmp(dot, T__jpg) == 0) {
    _contentType = T_image_jpeg;
  } else if (strcmp(dot, T__gif) == 0) {
    _contentType = T_image_gif;
  } else if (strcmp(dot, T__woff2) == 0) {
    _contentType = T_font_woff2;
  } else if (strcmp(dot, T__woff) == 0) {
    _contentType = T_font_woff;
  } else if (strcmp(dot, T__ttf) == 0) {
    _contentType = T_font_ttf;
  } else if (strcmp(dot, T__eot) == 0) {
    _contentType = T_font_eot;
  } else if (strcmp(dot, T__xml) == 0) {
    _contentType = T_text_xml;
  } else if (strcmp(dot, T__pdf) == 0) {
    _contentType = T_application_pdf;
  } else if (strcmp(dot, T__zip) == 0) {
    _contentType = T_application_zip;
  } else if (strcmp(dot, T__gz) == 0) {
    _contentType = T_application_x_gzip;
  } else {
    _contentType = T_text_plain;
  }
#endif
}

/**
 * @brief Constructor for AsyncFileResponse that handles file serving with compression support
 *
 * This constructor creates an AsyncFileResponse object that can serve files from a filesystem,
 * with automatic fallback to gzip-compressed versions if the original file is not found.
 * It also handles ETag generation for caching and supports both inline and download modes.
 *
 * @param fs Reference to the filesystem object used to open files
 * @param path Path to the file to be served (without compression extension)
 * @param contentType MIME type of the file content (empty string for auto-detection)
 * @param download If true, file will be served as download attachment; if false, as inline content
 * @param callback Template processor callback for dynamic content processing
 */
AsyncFileResponse::AsyncFileResponse(FS &fs, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback) {
  // Try to open the uncompressed version first
  _content = fs.open(path, fs::FileOpenMode::read);
  if (_content.available()) {
    _path = path;
    _contentLength = _content.size();
  } else {
    // Try to open the compressed version (.gz)
    _path = path + asyncsrv::T__gz;
    _content = fs.open(_path, fs::FileOpenMode::read);
    _contentLength = _content.size();

    if (_content.seek(_contentLength - 8)) {
      addHeader(T_Content_Encoding, T_gzip, false);
      _callback = nullptr;  // Unable to process zipped templates
      _sendContentLength = true;
      _chunked = false;

      // Add ETag and cache headers
      uint8_t crcInTrailer[4];
      _content.read(crcInTrailer, sizeof(crcInTrailer));
      char serverETag[9];
      AsyncWebServerRequest::_getEtag(crcInTrailer, serverETag);
      addHeader(T_ETag, serverETag, true);
      addHeader(T_Cache_Control, T_no_cache, true);

      _content.seek(0);
    } else {
      // File is corrupted or invalid
      _code = 404;
      return;
    }
  }

  if (*contentType != '\0') {
    _setContentTypeFromPath(path);
  } else {
    _contentType = contentType;
  }

  if (download) {
    // Extract filename from path and set as download attachment
    int filenameStart = path.lastIndexOf('/') + 1;
    char buf[26 + path.length() - filenameStart];
    char *filename = (char *)path.c_str() + filenameStart;
    snprintf_P(buf, sizeof(buf), PSTR("attachment; filename=\"%s\""), filename);
    addHeader(T_Content_Disposition, buf, false);
  } else {
    // Serve file inline (display in browser)
    addHeader(T_Content_Disposition, PSTR("inline"), false);
  }

  _code = 200;
}

AsyncFileResponse::AsyncFileResponse(File content, const String &path, const char *contentType, bool download, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback) {
  _code = 200;
  _path = path;

  if (!download && String(content.name()).endsWith(T__gz) && !path.endsWith(T__gz)) {
    addHeader(T_Content_Encoding, T_gzip, false);
    _callback = nullptr;  // Unable to process gzipped templates
    _sendContentLength = true;
    _chunked = false;
  }

  _content = content;
  _contentLength = _content.size();

  if (strlen(contentType) == 0) {
    _setContentTypeFromPath(path);
  } else {
    _contentType = contentType;
  }

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26 + path.length() - filenameStart];
  char *filename = (char *)path.c_str() + filenameStart;

  if (download) {
    snprintf_P(buf, sizeof(buf), PSTR("attachment; filename=\"%s\""), filename);
  } else {
    snprintf_P(buf, sizeof(buf), PSTR("inline"));
  }
  addHeader(T_Content_Disposition, buf, false);
}

size_t AsyncFileResponse::_fillBuffer(uint8_t *data, size_t len) {
  return _content.read(data, len);
}

/*
 * Stream Response
 * */

AsyncStreamResponse::AsyncStreamResponse(Stream &stream, const char *contentType, size_t len, AwsTemplateProcessor callback) : AsyncAbstractResponse(callback) {
  _code = 200;
  _content = &stream;
  _contentLength = len;
  _contentType = contentType;
}

size_t AsyncStreamResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t available = _content->available();
  size_t outLen = (available > len) ? len : available;
  size_t i;
  for (i = 0; i < outLen; i++) {
    data[i] = _content->read();
  }
  return outLen;
}

/*
 * Callback Response
 * */

AsyncCallbackResponse::AsyncCallbackResponse(const char *contentType, size_t len, AwsResponseFiller callback, AwsTemplateProcessor templateCallback)
  : AsyncAbstractResponse(templateCallback) {
  _code = 200;
  _content = callback;
  _contentLength = len;
  if (!len) {
    _sendContentLength = false;
  }
  _contentType = contentType;
  _filledLength = 0;
}

size_t AsyncCallbackResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t ret = _content(data, len, _filledLength);
  if (ret != RESPONSE_TRY_AGAIN) {
    _filledLength += ret;
  }
  return ret;
}

/*
 * Chunked Response
 * */

AsyncChunkedResponse::AsyncChunkedResponse(const char *contentType, AwsResponseFiller callback, AwsTemplateProcessor processorCallback)
  : AsyncAbstractResponse(processorCallback) {
  _code = 200;
  _content = callback;
  _contentLength = 0;
  _contentType = contentType;
  _sendContentLength = false;
  _chunked = true;
  _filledLength = 0;
}

size_t AsyncChunkedResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t ret = _content(data, len, _filledLength);
  if (ret != RESPONSE_TRY_AGAIN) {
    _filledLength += ret;
  }
  return ret;
}

/*
 * Progmem Response
 * */

AsyncProgmemResponse::AsyncProgmemResponse(int code, const char *contentType, const uint8_t *content, size_t len, AwsTemplateProcessor callback)
  : AsyncAbstractResponse(callback) {
  _code = code;
  _content = content;
  _contentType = contentType;
  _contentLength = len;
  _readLength = 0;
}

size_t AsyncProgmemResponse::_fillBuffer(uint8_t *data, size_t len) {
  size_t left = _contentLength - _readLength;
  if (left > len) {
    memcpy_P(data, _content + _readLength, len);
    _readLength += len;
    return len;
  }
  memcpy_P(data, _content + _readLength, left);
  _readLength += left;
  return left;
}

/*
 * Response Stream (You can print/write/printf to it, up to the contentLen bytes)
 * */

AsyncResponseStream::AsyncResponseStream(const char *contentType, size_t bufferSize) {
  _code = 200;
  _contentLength = 0;
  _contentType = contentType;
  // internal buffer will be null on allocation failure
  _content = std::unique_ptr<cbuf>(new cbuf(bufferSize));
  if (bufferSize && _content->size() < bufferSize) {
#ifdef ESP32
    log_e("Failed to allocate");
#endif
  }
}

size_t AsyncResponseStream::_fillBuffer(uint8_t *buf, size_t maxLen) {
  return _content->read((char *)buf, maxLen);
}

size_t AsyncResponseStream::write(const uint8_t *data, size_t len) {
  if (_started()) {
    return 0;
  }
  if (len > _content->room()) {
    size_t needed = len - _content->room();
    _content->resizeAdd(needed);
    // log a warning if allocation failed, but do not return: keep writing the bytes we can
    // with _content->write: if len is more than the available size in the buffer, only
    // the available size will be written
    if (len > _content->room()) {
#ifdef ESP32
      log_e("Failed to allocate");
#endif
    }
  }
  size_t written = _content->write((const char *)data, len);
  _contentLength += written;
  return written;
}

size_t AsyncResponseStream::write(uint8_t data) {
  return write(&data, 1);
}
