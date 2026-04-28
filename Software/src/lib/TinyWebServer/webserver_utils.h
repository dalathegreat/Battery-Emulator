#ifndef WEBSERVER_UTILS_H
#define WEBSERVER_UTILS_H

#include <vector>
#include <memory>
#include <Arduino.h>
#include "TinyWebServer.h"
#include "TwsBuffering.h"
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

// A simple handler that responds to GET requests with a JSON document populated
// by a user-supplied callback

class TwsJsonGetFunc : public TwsRequestHandler {
public:
    TwsJsonGetFunc(void (*respond)(TwsRequest& request, JsonDocument& doc)) : respond(respond) {}

    void handleRequest(TwsRequest &request) override;

    void (*respond)(TwsRequest& request, JsonDocument& doc);
};

// A simple handler that responds to POST requests, repeatedly calling a
// user-supplied callback with the raw POST body data as it is received.

struct RawPostFuncState {
    size_t content_length = 0;
};

class TwsRawPostFunc : public TwsStatefulMiddleware<RawPostFuncState> {
public:
    TwsRawPostFunc(int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len, size_t total)) : handle(handle) {}
    
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleHeader(TwsRequest &request, const char *line, int len) override;

    int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len, size_t total);
};

// A handler superclass for JSON REST endpoint handlers, supporting both GET and
// POST. The POST body is buffered in memory up to a configurable maximum size. 

class TwsJsonRestHandler : public TwsPostBufferingRequestHandler {
public:
    TwsJsonRestHandler(size_t max_size = 16384) : TwsPostBufferingRequestHandler(nullptr, max_size)
    {}

    void handleRequest(TwsRequest &request) override;

protected:
    // Return false to cascade into handleJsonGet. Return true to abort.
    virtual bool handleJsonPost(TwsRequest& request, uint8_t* data, size_t len);

    // Populate the GET response
    virtual void handleJsonGet(TwsRequest& request, JsonDocument& doc);
};


// A handler for JSON REST endpoints, where the request handling logic is
// implemented by user-supplied callbacks.

// For POST requests the second callback will called first, with the entire
// buffered POST body. The callback can end processing at this point by
// returning true, otherwise the GET handler will then be called.

// For GET requests (and continuing POSTs) the first callback is called, with an
// empty JsonDocument to populate, which will be sent as the response body after
// the callback returns.

typedef void (*JsonGetCallback)(TwsRequest& request, JsonDocument& doc);
typedef bool (*JsonPostCallback)(TwsRequest& request, uint8_t* data, size_t len);

class TwsJsonRestFunc : public TwsJsonRestHandler {
public:
    TwsJsonRestFunc(JsonGetCallback onGet, JsonPostCallback onPost = nullptr, size_t max_size = 16384)
        : TwsJsonRestHandler(max_size), _onGet(onGet), _onPost(onPost) {}

protected:
    bool handleJsonPost(TwsRequest& request, uint8_t* data, size_t len) override;

    void handleJsonGet(TwsRequest& request, JsonDocument& doc) override;

private:
    JsonGetCallback _onGet;
    JsonPostCallback _onPost;
};

#endif
