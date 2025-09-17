#include "TinyWebServer.h"

BasicAuth::BasicAuth(TwsHandler *handler) : TwsStatefulHandler<BasicAuthState>(handler) {
    nextPostBody = handler->onPostBody;
    nextHeader = handler->onHeader;
    nextRequest = handler->onRequest;

    handler->onPostBody = this;
    handler->onHeader = this;
    handler->onRequest = this;
}

void BasicAuth::handleHeader(TwsRequest &request, const char *line, int len) {
    auto &state = get_state(request);

    if(strcmp(line, "Authorization: Basic YXV0aDphdXRo")==0) {
        state.authed = true;
    }
    if(nextHeader) {
        nextHeader->handleHeader(request, line, len);
    }
}

bool BasicAuth::denyIfUnauthed(TwsRequest &request) {
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

bool BasicAuth::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    // Handle post body if needed
    if(denyIfUnauthed(request)) {
        return true;
    }

    if(nextPostBody) {
        return nextPostBody->handlePostBody(request, index, data, len);
    }
    return true;
}

void BasicAuth::handleRequest(TwsRequest &request) {
    if(denyIfUnauthed(request)) {
        return;
    }

    if(nextRequest) {
        nextRequest->handleRequest(request);
    }
}
