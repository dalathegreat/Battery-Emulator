#ifndef WEBSERVER_UTILS_H
#define WEBSERVER_UTILS_H

#include <vector>
#include <memory>
#include <Arduino.h>
#include "TinyWebServer.h"
#include <src/lib/bblanchon-ArduinoJson/ArduinoJson.h>

extern const char* HTTP_RESPONSE;
extern const char* HTTP_204;
extern const char *HTTP_405;

TwsRequestWriterCallbackFunction StringWriter(std::shared_ptr<String> &response);

class StringLike {
public:
    StringLike(String str) : _str(str), _length(_str.length()) {}
    StringLike(String &str) : _str(str), _length(_str.length()) {}
    StringLike(const char *chr) : _chr(chr), _length(strlen(chr)) {}
    StringLike(const char *chr, int length) : _chr(chr) , _length(length) {}

    const char* c_str() const {
        return _chr ? _chr : _str.c_str();
    }
    int length() const {
        return _length;
    }
private:
    String _str;
    const char *_chr = nullptr;
    int _length = 0;
};

TwsRequestWriterCallbackFunction StringListWriter(std::shared_ptr<std::vector<StringLike>> &response);
TwsRequestWriterCallbackFunction CharBufWriter(const char* buf, int len);

class TwsJsonGetFunc : public TwsRequestHandler {
public:
    TwsJsonGetFunc(void (*respond)(TwsRequest& request, JsonDocument& doc)) : respond(respond) {}

    void handleRequest(TwsRequest &request) override;

    void (*respond)(TwsRequest& request, JsonDocument& doc);
};

class TwsRawPostFunc : public TwsPostBodyHandler {
public:
    TwsRawPostFunc(int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len)) : handle(handle) {}
    
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        return handle(request, index, data, len);
    }

    int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len);
};

#endif
