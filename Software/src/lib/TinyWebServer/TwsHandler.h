#ifndef TWS_HANDLER_H
#define TWS_HANDLER_H

#include <functional>
#include <stddef.h>
#include <stdint.h>

// Forward declarations to avoid circular include with TinyWebServer.h
class TwsRequest;
class TwsRoute;

class TwsPostBodyHandler {
public:
    // Called when POST body data is received. Returns bytes consumed, or -1 if processing is complete.
    virtual int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) = 0;
};

class TwsHeaderHandler {
public:
    // Called when a complete HTTP header line is received.
    virtual void handleHeader(TwsRequest &request, const char *line, int len) = 0;
};

class TwsPartialHeaderHandler {
public:
    // Called for chunks of a header line. Returns bytes consumed. 'final' is true if this is the end of the line.
    virtual int handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) = 0;
};

class TwsQueryParamHandler {
public:
    // Called for each query string parameter found in the URL. 'final' is true if this is the last parameter.
    virtual void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) = 0;
};

class TwsFileUploadHandler {
public:
    // Called when a file chunk is received during a multipart upload. 'final' is true if this is the last chunk.
    virtual void handleUpload(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) = 0;
};

class TwsRequestHandler {
public:
    // Called when the HTTP request is ready to be handled (headers and optional POST body received).
    virtual void handleRequest(TwsRequest &request) = 0;
};

class TwsAllocableHandler {
public:
    // Called during server initialization to calculate the required state data size for this handler.
    virtual int handleAlloc(int max, int start) = 0;
    // Called when a request is finished to allow cleaning up any allocated state.
    virtual void handleFree(TwsRequest &request) {}
};


class TwsMiddleware : public TwsHeaderHandler, 
                      public TwsPartialHeaderHandler, 
                      public TwsQueryParamHandler,
                      public TwsPostBodyHandler, 
                      public TwsRequestHandler, 
                      public TwsAllocableHandler {
public:
    TwsHeaderHandler *nextHeader = nullptr;
    TwsPartialHeaderHandler *nextPartialHeader = nullptr;
    TwsQueryParamHandler *nextQueryParam = nullptr;
    TwsPostBodyHandler *nextPostBody = nullptr;
    TwsRequestHandler *nextRequest = nullptr;
    TwsAllocableHandler *nextAlloc = nullptr;

    // --- Default Pass-Through Implementations ---
    virtual void handleHeader(TwsRequest &request, const char *line, int len) override {
        if (nextHeader) nextHeader->handleHeader(request, line, len);
    }
    
    virtual int handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) override {
        if (nextPartialHeader) return nextPartialHeader->handlePartialHeader(request, line, len, final);
        return len; // Consume by default to prevent hangs
    }

    virtual void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) override {
        if (nextQueryParam) nextQueryParam->handleQueryParam(request, param, len, final);
    }
    
    virtual int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        if (nextPostBody) return nextPostBody->handlePostBody(request, index, data, len);
        return -1; // Indicate upload is complete if nothing handles it
    }
    
    virtual void handleRequest(TwsRequest &request) override {
        if (nextRequest) nextRequest->handleRequest(request);
    }
    
    virtual int handleAlloc(int max, int start) override {
        if (nextAlloc) return nextAlloc->handleAlloc(max, start);
        return start;
    }
    
    virtual void handleFree(TwsRequest &request) override {
        if (nextAlloc) nextAlloc->handleFree(request);
    }
};

// Handlers that need to maintain state between different phases of the request
// (or between repeat calls to the same callbacks) should subclass this.
//
// A struct T should be defined to hold the state data. This struct will be
// allocated in the request's state data area, and can be accessed using
// get_state(request).

template <typename T>
class TwsStatefulMiddleware : public TwsMiddleware {
public:
    int offset = 0;

    // Use TwsMiddleware's nextAlloc pointer to continue the chain
    virtual int handleAlloc(int max, int start) override;

    // Helper to retrieve the state struct from the request's raw buffer
    T& get_state(TwsRequest &request);
};

class TwsFileUploadHandlerFunc : public TwsFileUploadHandler {
public:
    TwsFileUploadHandlerFunc(std::function<void(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final)> onUpload) :
        onUpload(onUpload) {
    }

    void handleUpload(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) override {
        onUpload(request, key, filename, index, data, len, final);
    }

    std::function<void(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final)> onUpload;
};

class TwsRequestHandlerFunc : public TwsRequestHandler {
public:
    TwsRequestHandlerFunc(std::function<void(TwsRequest &request)> onRequest) : onRequest(onRequest) {}

    void handleRequest(TwsRequest &request) override {
        onRequest(request);
    }

    std::function<void(TwsRequest &request)> onRequest;
};

#endif // TWS_HANDLER_H
