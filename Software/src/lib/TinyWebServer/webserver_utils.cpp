#include "webserver_utils.h"

const char* HTTP_RESPONSE = "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "\r\n";
const char* HTTP_204 = "HTTP/1.1 204 n\r\n"
                       "Connection: close\r\n"
                       "\r\n";
const char *HTTP_405 = "HTTP/1.1 405 x\r\n"
                       "Connection: close\r\n"
                       "\r\n";

TwsRequestWriterCallbackFunction StringWriter(std::shared_ptr<String> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        const int remaining = response->length() - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); 
            return;
        }
        req.write_direct(response->c_str() + alreadyWritten, remaining);
    };
}

TwsRequestWriterCallbackFunction StringListWriter(std::shared_ptr<std::vector<StringLike>> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        for(auto &str : *response) {
            int length = str.length();
            if(alreadyWritten >= length) {
                alreadyWritten -= length;
                continue; 
            }

            const int remaining = length - alreadyWritten;
            if(req.write_direct(str.c_str() + alreadyWritten, remaining) < remaining) {
                return;
            }
            alreadyWritten = 0; 
        }  

        req.finish();
    };
}

TwsRequestWriterCallbackFunction CharBufWriter(const char* buf, int len) {
    return [buf, len](TwsRequest &req, int alreadyWritten) {
        const int remaining = len - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); 
            return;
        }
        req.write_direct(buf + alreadyWritten, remaining);
    };
}

void TwsJsonGetFunc::handleRequest(TwsRequest &request) {
    JsonDocument doc;
    respond(request, doc);
    auto response = std::make_shared<String>();
    serializeJson(doc, *response);
    request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "\r\n");
    request.set_writer_callback(StringWriter(response));
}

int TwsRawPostFunc::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    return handle(request, index, data, len);
}

int TwsJsonRestHandler::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    auto &state = get_state(request);
    if(!state.post_body) {
        if (state.content_length > _max_size) {
            request.send(413, "text/plain", "Payload Too Large");
            request.finish();
            return -1;
        }
        state.post_body = std::make_shared<PostBody>(state.content_length);
    }
    
    state.post_body->set(data, index, len);

    if(index + len >= state.content_length) {
        return -1; // Upload complete, wait for handleRequest to fire
    }
    return len;
}

void TwsJsonRestHandler::handleRequest(TwsRequest &request) {
    // If it's a POST, execute the POST logic first
    if (request.is_post()) {
        auto &state = get_state(request);
        uint8_t* payload = state.post_body ? state.post_body->data : nullptr;
        
        // If handleJsonPost returns false, it means the POST completed the
        // response (eg, due to an error), so we should finish.
        if (!handleJsonPost(request, payload, state.content_length)) {
            return; 
        }
    }

    // If it's a GET request, OR if the POST handler returned true, we drop down
    // into the GET logic.
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

bool TwsJsonRestHandler::handleJsonPost(TwsRequest& request, uint8_t* data, size_t len) {
    return true; 
}

void TwsJsonRestHandler::handleJsonGet(TwsRequest& request, JsonDocument& doc) {}

bool TwsJsonRestFunc::handleJsonPost(TwsRequest& request, uint8_t* data, size_t len) {
    if (_onPost) {
        return _onPost(request, data, len);
    }
    // Reject POSTs if no handler was provided
    request.write_fully(HTTP_405);
    request.finish();
    return false; 
}

void TwsJsonRestFunc::handleJsonGet(TwsRequest& request, JsonDocument& doc) {
    if (_onGet) {
        _onGet(request, doc);
    }
}
