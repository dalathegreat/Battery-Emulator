#include "TinyWebServer.h"

void BasicAuth::handleHeader(TwsRequest &request, const char *line, int len) {
    auto &state = get_state(request);
    if(strcmp(line, "Authorization: Basic YXV0aDphdXRo") == 0) {
        state.authed = true;
    }
    // Automatically forward to next via base class
    TwsMiddleware::handleHeader(request, line, len);
}

bool BasicAuth::denyIfUnauthed(TwsRequest &request) {
    if (request.is_options()) return false;
    auto &state = get_state(request);
    if(!state.authed) {
        request.write_fully("HTTP/1.1 401 Unauthorized\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "WWW-Authenticate: Basic realm=\"TinyWebServer\"\r\n"
                    "\r\n"
                    "This is a protected resource.\n");
        request.finish();
        return true;
    }
    return false;
}

int BasicAuth::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    if(denyIfUnauthed(request)) return -1;
    return TwsMiddleware::handlePostBody(request, index, data, len);
}

void BasicAuth::handleRequest(TwsRequest &request) {
    if(denyIfUnauthed(request)) return;
    TwsMiddleware::handleRequest(request);
}

