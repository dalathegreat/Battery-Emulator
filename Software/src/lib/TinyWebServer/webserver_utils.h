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
    
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        return handle(request, index, data, len);
    }

    int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len);
};

class TwsJsonRestHandler : public TwsStatefulMiddleware<PostBufferingRequestHandlerState> {
public:
    TwsJsonRestHandler(size_t max_size = 16384) : _max_size(max_size) {}

    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        auto &state = get_state(request);
        if(!state.post_body) {
            if (state.content_length > _max_size) {
                request.send(413, "text/plain", "Payload Too Large");
                request.finish();
                return -1;
            }
            state.post_body = std::make_shared<PostBody>(state.content_length);
            // Allocation error handling omitted for brevity, but same as your original
        }
        
        state.post_body->set(data, index, len);

        if(index + len >= state.content_length) {
            return -1; // Upload complete, wait for handleRequest to fire
        }
        return len;
    }

    // 2. Dispatch to the right logic safely
    void handleRequest(TwsRequest &request) override {
        // If it's a POST/PUT, execute the POST logic first
        if (request.is_post()) { // Assuming TwsRequest has method helpers
            auto &state = get_state(request);
            uint8_t* payload = state.post_body ? state.post_body->data : nullptr;
            
            // If handleJsonPost returns false, it means an error was encountered 
            // (e.g., 400 Bad Request) and the GET response should be aborted.
            if (!handleJsonPost(request, payload, state.content_length)) {
                return; 
            }
        }

        // If it's a GET request, OR if the POST request succeeded and we want to 
        // return the updated state, we drop down into the GET logic.
        JsonDocument doc;
        handleJsonGet(request, doc);

        auto response = std::make_shared<String>();
        serializeJson(doc, *response);
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Content-Type: application/json\r\n"
                            "Access-Control-Allow-Origin: *\r\n\r\n");
        request.set_writer_callback(StringWriter(response));
    }

protected:
    // Return true to cascade into handleJsonGet (success). Return false to abort.
    virtual bool handleJsonPost(TwsRequest& request, uint8_t* data, size_t len) {
        return true; 
    }

    // Populate the GET response
    virtual void handleJsonGet(TwsRequest& request, JsonDocument& doc) {}

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
    bool handleJsonPost(TwsRequest& request, uint8_t* data, size_t len) override {
        if (_onPost) {
            return _onPost(request, data, len);
        }
        // Reject POSTs if no handler was provided
        request.write_fully(HTTP_405);
        request.finish();
        return false; 
    }

    void handleJsonGet(TwsRequest& request, JsonDocument& doc) override {
        if (_onGet) {
            _onGet(request, doc);
        }
    }

private:
    JsonGetCallback _onGet;
    JsonPostCallback _onPost;
};

#endif
