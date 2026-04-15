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

class TwsJsonGetFunc : public TwsRequestHandler {
public:
    TwsJsonGetFunc(void (*respond)(TwsRequest& request, JsonDocument& doc)) : respond(respond) {}

    void handleRequest(TwsRequest &request) override;

    void (*respond)(TwsRequest& request, JsonDocument& doc);
};

class TwsRawPostFunc : public TwsPostBodyHandler {
public:
    TwsRawPostFunc(int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len)) : handle(handle) {}
    
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;

    int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len);
};

class TwsJsonRestHandler : public TwsStatefulMiddleware<PostBufferingRequestHandlerState> {
public:
    TwsJsonRestHandler(size_t max_size = 16384) : _max_size(max_size) {}

    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;

    // 2. Dispatch to the right logic safely
    void handleRequest(TwsRequest &request) override;

protected:
    // Return true to cascade into handleJsonGet (success). Return false to abort.
    virtual bool handleJsonPost(TwsRequest& request, uint8_t* data, size_t len);

    // Populate the GET response
    virtual void handleJsonGet(TwsRequest& request, JsonDocument& doc);

private:
    size_t _max_size;
};

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
