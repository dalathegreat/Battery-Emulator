#ifndef TWS_BUFFERING_H
#define TWS_BUFFERING_H

#include <memory>
#include "TwsHandler.h"

class PostBody {
public:
    PostBody(size_t size) : size(size) {
        if(size>0) {
            DEBUG_PRINTF("Allocating PostBody of size %zu\n", size);
            data = (uint8_t*)malloc(size+1);
            if (data) {
                data[size] = 0; // nul terminate for convenience
            }
        }
    }
    ~PostBody() {
        if(data) {
            DEBUG_PRINTF("Freeing PostBody of size %zu\n", size);
            free(data);
        }
    }
    void set(uint8_t *buf, size_t offset, size_t len) {
        if(data && (offset + len) <= size) {
            memcpy(data + offset, buf, len);
        }
    }
    
    uint8_t *data = nullptr;
    size_t size = 0;
};

struct PostBufferingRequestHandlerState {
    size_t content_length = 0;
    std::shared_ptr<PostBody> post_body = nullptr;
};

class TwsPostBufferingRequestHandler : public TwsStatefulMiddleware<PostBufferingRequestHandlerState> {
public:
    TwsPostBufferingRequestHandler(void (*handleFullPostBody)(TwsRequest &request, uint8_t *data, size_t len) = nullptr, size_t max_size = 16384) 
        : handleFullPostBody(handleFullPostBody), _max_size(max_size) {}

    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        auto &state = get_state(request);
        if(!state.post_body) {
            if (state.content_length > _max_size) {
                DEBUG_PRINTF("TWS post body too large: %zu > %zu\n", state.content_length, _max_size);
                request.send(413, "text/plain", "Payload Too Large");
                request.finish();
                return -1;
            }
            state.post_body = std::make_shared<PostBody>(state.content_length);
            if (state.content_length > 0 && !state.post_body->data) {
                request.send(500, "text/plain", "Memory Allocation Failed");
                request.finish();
                return -1;
            }
        }
        //DEBUG_PRINTF("Buffering post body: [ %*s ] at index %zu, len %zu\n", (int)len, (char*)data, index, len);
        state.post_body->set(data, index, len);

        if(index+len >= state.content_length) {
            // Finished uploading
            if(handleFullPostBody) {
                handleFullPostBody(request, state.post_body->data, state.content_length);
            }
            if (nextPostBody) return nextPostBody->handlePostBody(request, index, data, len);
            return -1; // Indicate that the upload is complete
        }
        
        if(nextPostBody) {
            return nextPostBody->handlePostBody(request, index, data, len);
        }
        return len;
    }
    void handleHeader(TwsRequest &request, const char *line, int len) override {
        auto &state = get_state(request);

        if(strncasecmp(line, "Content-Length:", 15) == 0) {
            char *endptr;
            int content_length = strtol(line + 15, &endptr, 10);
            if (endptr != line + 15 && content_length > 0) {
                state.content_length = content_length;
            }
        }
        if (nextHeader) nextHeader->handleHeader(request, line, len);
    }
    void handleFree(TwsRequest &request) override {
        auto &state = get_state(request);

        if(state.post_body && state.post_body->data) {
            //DEBUG_PRINTF("Post body was %*s\n", (int)state.content_length, (char*)state.post_body->data);
        }

        state.post_body = nullptr;
        TwsMiddleware::handleFree(request);
    }

    void (*handleFullPostBody)(TwsRequest &request, uint8_t *data, size_t len) = nullptr;
    size_t _max_size;
};

#endif // TWS_BUFFERING_H
