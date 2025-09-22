#include "TinyWebServer.h"

#include "Update.h"

EOtaStart::EOtaStart(TwsHandler *handler) : TwsStatefulHandler<EOtaStartState>(handler) {
    nextQueryParam = handler->onQueryParam;

    handler->onRequest = this;
    handler->onQueryParam = this;
}

void EOtaStart::handleRequest(TwsRequest &request) {
    auto &state = get_state(request);
    
    #ifndef LOCAL

    if(!Update.begin(UPDATE_SIZE_UNKNOWN, state.mode == 1 ? U_SPIFFS : U_FLASH)) {
        request.write_fully("HTTP/1.1 400 Bad Request\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\nNot ok");
    } else {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "\r\nOK");
    }

    #else
    request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "\r\nOK");
    #endif

    request.finish();
}

void EOtaStart::handleQueryParam(TwsRequest &request, const char *param, int len, bool final) {
    auto &state = get_state(request);
    //printf("got query param %s\n", param);
    if(strcmp(param, "mode=fs") == 0) {
        state.mode = 1; // filesystem mode
    }
    // if param starts with hash=
    if(strncmp(param, "hash=", 5) == 0) {
        //printf("hash: %s\n", param + 5);
        #ifndef LOCAL
        if (!Update.setMD5(param + 5)) {
            //printf("Update.setMD5 failed!\n");
            request.send(400, "text/plain", "MD5 parameter invalid.");
            request.finish();
        }
        #endif
    }

    if(nextQueryParam) {
        nextQueryParam->handleQueryParam(request, param, len, final);
    }
}


EOtaUpload::EOtaUpload(TwsHandler *handler) : TwsStatefulHandler<EOtaUploadState>(handler) {
    nextRequest = handler->onRequest;
    nextHeader = handler->onHeader;

    handler->onRequest = this;
    handler->onHeader = this;
    handler->onPostBody = this;
}

void EOtaUpload::handleRequest(TwsRequest &request) {
    if(nextRequest) {
        nextRequest->handleRequest(request);
    }
}

int EOtaUpload::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    auto &state = get_state(request);

   if(Update.write(data, len) != len) {
        //printf("Update.write failed: %s\n", Update.errorString());
        request.send(400, "text/plain", "Update write failed.");
        request.finish();
        return -1;
    }

    if((index + len) >= state.content_length) {
        if(Update.end(true)) {
            //printf("Update completed successfully\n");
            request.write_fully("HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nOK");
        } else {
            //printf("Update failed to end: %s\n", Update.errorString());
            request.write_fully("HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nUpdate failed.");
        }
        request.finish();
        return -1;
    }
    return len;
}

void EOtaUpload::handleHeader(TwsRequest &request, const char *line, int len) {
    auto &state = get_state(request);

    if(strncasecmp(line, "Content-Length:", 15) == 0) {
        char *endptr;
        int content_length = strtol(line + 15, &endptr, 10);
        if (endptr != line + 15 && content_length > 0) {
            state.content_length = content_length;
        }
    }

    if(nextHeader) {
        nextHeader->handleHeader(request, line, len);
    }
}

/*
void EOtaUpload::handleUpload(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) {
    auto &state = get_state(request);

    #ifdef LOCAL

    FILE *file = fopen("/tmp/upload.bin", "a");
    fwrite(data, 1, len, file);
    fclose(file);

    if(final) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\nOK");
    }

    #else
    
    //unsigned long start = micros();
    if(Update.write(data, len) != len) {
        //printf("Update.write failed: %s\n", Update.errorString());
        request.send(400, "text/plain", "Update write failed.");
        request.finish();
        return;
    }
    //tws_log_printf("Update.write %d took %lu us\n", len, micros() - start);

    if(final) {
        if(Update.end(true)) {
            //printf("Update completed successfully\n");
            request.write_fully("HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nOK");
        } else {
            //printf("Update failed to end: %s\n", Update.errorString());
            request.write_fully("HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nUpdate failed.");
        }
        request.finish();
    }
    #endif
}*/
