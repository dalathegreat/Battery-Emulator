#ifndef TINY_WEB_SERVER_H
#define TINY_WEB_SERVER_H

#ifndef LOCAL
// Arduino build

#include "Arduino.h"
#include "stdlib_noniso.h"
#include <src/devboard/utils/logging.h>

#include "FS.h"
#include "StreamString.h"
#define HARDWARE "ESP32"

# else 
// Local (Linux) build

unsigned long millis(void);

#define DEBUG_PRINTF(fmt, ...)                                                                  \
  do {                                                                                          \
    printf(fmt, ##__VA_ARGS__);                                                       \
  } while (0)

#endif

#include <functional>
#include <cstring>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

extern "C" {
#include <mbedtls/md5.h>
#include <mbedtls/sha256.h>
}

const int TWS_AWAITING_METHOD = 0;
const int TWS_AWAITING_PATH = 1;
const int TWS_AWAITING_QUERY_STRING = 2;
const int TWS_AWAITING_VERSION = 3;
const int TWS_AWAITING_HEADER = 4;
const int TWS_AWAITING_PARTIAL_HEADER = 5;
const int TWS_SKIPPING_HEADER = 6;
const int TWS_AWAITING_BODY = 7;
const int TWS_WRITING_OUT = 8;

typedef enum {
  TWS_HTTP_GET = 0x1,
  TWS_HTTP_POST = 0x2,
} TwsMethod;

class TwsRequest;
class TwsHandler;

typedef std::function<void(TwsRequest &request, int alreadyWritten)> TwsRequestWriterCallbackFunction;

class TwsRequest {
public:
    uint32_t write(const char *buf);
    uint32_t write(const char *buf, uint16_t len);
    bool write_fully(const char *buf);
    bool write_fully(const char *buf, uint16_t len);
    uint32_t write_direct(const char *buf, uint16_t len);
    int available();
    int free();
    int read_flush(uint16_t len);

    void send(int code, const char *content_type = "", const char *content = "");
    void finish();
    void set_writer_callback(TwsRequestWriterCallbackFunction callback) {
        writer_callback = callback;
        // record the current total written bytes to use as an offset
        writer_callback_written_offset = total_written;
    }
    inline bool active() {
        return socket > -1;
    }
    inline bool is_post() {
        return method == TWS_HTTP_POST;
    }
    uint16_t get_slot_id() {
        return slot_id;
    }
    uint32_t get_connection_id() {
        return connection_id;
    }

    uint8_t* get_state_data() {
        return state_data;
    }

    inline static int sub_mod(int a, int b) {
        while(a >= b) {
            a -= b;
        }
        return a;
    }

protected:
    // The fd of the accepted socket connection
    int socket = -1;
    // The index into the server's requests array
    uint8_t slot_id = -1;
    // A unique identifier for this connection, incremented for each new
    // connection. Useful to identify whether a slot has been reused since.
    uint32_t connection_id = -1;

    // Set the size of the send buffer. This needs to be large enough to
    // handle the largest HTTP response (including headers) that you need to
    // send in one go, without using a callback (usually just error responses).
    static const int SEND_BUFFER_SIZE = 512;

    // Set the size of the receive buffer. This value affects several direct
    // limitations in the protocol handling:
    // - The maximum length of the request path
    // - The maximum length of a header that can be passed on without being
    //   split into partial chunks
    // - The maximum length of a query parameter (as a key=value string) that
    //   can passed on without being split
    static const int RECV_BUFFER_SIZE = 512;

    char send_buffer[SEND_BUFFER_SIZE];
    struct __attribute__((packed)) {
        char recv_buffer[RECV_BUFFER_SIZE];
        // We add a nul after the recv buffer, so we can pass chunks at the end
        // of recv_buffer as nul-terminated strings.
        char _overread_protection = 0;
    };

    volatile uint16_t send_buffer_len = 0;
    uint16_t send_buffer_write_ptr = 0;
    uint16_t send_buffer_read_ptr = 0;
    volatile uint16_t recv_buffer_len = 0;
    uint16_t recv_buffer_write_ptr = 0;
    uint16_t recv_buffer_read_ptr = 0;
    uint16_t recv_buffer_scan_ptr = 0;
    uint16_t recv_buffer_scan_len = 0;

    uint32_t total_written = 0;
    uint32_t body_read = 0;

    uint8_t parse_state = TWS_AWAITING_METHOD;
    uint8_t method = 0;
    bool pending_direct_write = false;
    bool done = false;
    TwsHandler *handler = nullptr;

    TwsRequestWriterCallbackFunction writer_callback = nullptr;
    int writer_callback_written_offset = 0;

    unsigned long last_activity = 0;

    // Size of the state data array, used by handlers to store things during the
    // lifetime of a request. If this is too small, handlers may fail to
    // initialize at startup.
    static const int STATE_DATA_SIZE = 350;//288;//24;//113;

    uint8_t state_data[STATE_DATA_SIZE] = {0};

    int scan(char delim1);
    int scan(char delim1, char delim2);
    int scan(char delim1, char delim2, char delim3);
    int available_contiguous();
    char* get_peek_ptr(char *buf, uint16_t len);
    char* get_read_ptr(char *buf, uint16_t len);
    bool match(const char *str);
    bool match(const char *str, uint16_t len);
    bool recv_buffer_full();

    void reset();
    void abort();

    void perform_io();
    int recv();
    void tick();

    friend class TinyWebServer;
};


class TwsPostBodyHandler {
public:
    virtual bool handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) = 0;
};

class TwsHeaderHandler {
public:
    virtual void handleHeader(TwsRequest &request, const char *line, int len) = 0;
};

class TwsPartialHeaderHandler {
public:
    virtual int handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) = 0;
};

class TwsQueryParamHandler {
public:
    virtual void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) = 0;
};

class TwsFileUploadHandler {
public:
    virtual void handleUpload(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) = 0;
};

class TwsRequestHandler {
public:
    virtual void handleRequest(TwsRequest &request) = 0;
};

class TwsAllocableHandler {
public:
    virtual int handleAlloc(int max, int start) = 0;
};

// TwsHandler represents a route handler for a specific path. These are created
// before startup and passed to the TinyWebServer constructor in an array.
//
// The path can be an exact match (e.g. "/status") or a wildcard ("*") to match
// every request. A wildcard handler should usually be the last handler in the
// array.
//
// Each handler includes a set of optional callbacks for different stages of the
// request, as pointers to Tws*Handler subclasses.
class TwsHandler {
public:
    TwsHandler(const char *path, TwsRequestHandler *onRequest = nullptr) :
        path(path), onRequest(onRequest) {
    }
    const char *path;
    TwsRequestHandler *onRequest = nullptr;    
    TwsPostBodyHandler *onPostBody = nullptr;
    TwsHeaderHandler *onHeader = nullptr;
    TwsPartialHeaderHandler *onPartialHeader = nullptr;
    TwsQueryParamHandler *onQueryParam = nullptr;
    TwsAllocableHandler *onAlloc = nullptr;
};

template<class T> 
class TwsStatefulHandler : public TwsAllocableHandler {
public:
    TwsStatefulHandler(TwsHandler *handler) {
        nextAlloc = handler->onAlloc;
        handler->onAlloc = this;
    }
    int handleAlloc(int max, int start) override {
        int alloc_size = sizeof(T);

        offset = start;
        start += alloc_size;

        if(nextAlloc) {
            start = nextAlloc->handleAlloc(max, start);
        }
        return start;
    }

    T& get_state(TwsRequest &request) {
        return *(T*)(request.get_state_data() + offset);
    }

    int offset;
    TwsAllocableHandler *nextAlloc = nullptr;
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










// typedef std::function<void(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)>
//   ArUploadHandlerFunction;
// typedef std::function<void(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)> ArBodyHandlerFunction;

class TinyWebServer {
public:
    TinyWebServer(uint16_t port, TwsHandler *handlers[] = nullptr);
    ~TinyWebServer();

    // void on(
    //     const char *uri, 
    //     int method, 
    //     TwsRequestHandlerFunction onRequest
    //     // ArUploadHandlerFunction onUpload, ArBodyHandlerFunction onBody
    // );

    void open_listen_port();
    void tick();
    bool poll();

    int write(uint32_t connection_id, const char *buf, size_t len);
    int free(uint32_t connection_id);
    void finish(uint32_t connection_id);

    // must be <= 32
    static const int MAX_REQUESTS = 3;
    // How long to block polling for activity when there are requests active.
    static const int ACTIVE_POLL_TIME_MS = 10;
    // How long to block polling when there are no requests active.
    static const int IDLE_POLL_TIME_MS = 1000;

protected:
    uint16_t _port;
    int _listen_socket = -1;
    TwsHandler **_handlers = nullptr;
    unsigned int max_handler_state_size = 0;

    void accept_new_connections();
    uint32_t get_waiting_requests(long max_delay_us);
    void handle_request(TwsRequest &request);

    TwsRequest slots[MAX_REQUESTS];
    int last_connection_id = 0;
};


// class TwsRequestHandlerEntry {
// public:
//     const char *path;
//     int method;
//     TwsRequestHandlerFunction onRequest;
//     TwsPostBodyHandlerFunction onPostBody = nullptr;
//     TwsQueryParamHandlerFunction onQueryParam = nullptr;
//     TwsHeaderHandlerFunction onHeader = nullptr;

//     void set_post_body_handler(TwsPostBodyHandlerFunction handler) {
//         onPostBody = handler;
//     }

//     void set_header_handler(TwsHeaderHandlerFunction handler) {
//         onHeader = handler;
//     }

//     TwsRequestHandlerEntry(
//         const char *path, 
//         int method, 
//         TwsRequestHandlerFunction onRequest,
//         TwsPostBodyHandlerFunction onPostBody = nullptr,
//         TwsQueryParamHandlerFunction onQueryParam = nullptr,
//         TwsHeaderHandlerFunction onHeader = nullptr
//     ) : path(path), method(method), onRequest(onRequest), 
//         onPostBody(onPostBody), onQueryParam(onQueryParam), onHeader(onHeader) {

//         printf("TwsRequestHandlerEntry created for path: %s\n", path);

//     }
//     ~TwsRequestHandlerEntry() {
//         printf("TwsRequestHandlerEntry for %s destroyed\n", path);
//     }
// };

template<class T> class TwsStatefulRequestHandler {
public:
    typedef struct {
        int connection_id;
        T state_data;
    } _request_slot;
    _request_slot state[TinyWebServer::MAX_REQUESTS];

    T& get_state(TwsRequest &request) {
        auto slot_id = request.get_slot_id();
        auto connection_id = request.get_connection_id();
        if(state[slot_id].connection_id != connection_id) {
            // Reset state for this slot
            state[slot_id].connection_id = connection_id;
            state[slot_id].state_data = T();
        }
        return state[slot_id].state_data;
    }
};

// typedef struct { 
//     int content_length;
//     //char filename[256]; // Filename for multipart upload
//     char boundary[78]; // Boundary for multipart upload
//     uint8_t boundary_length; // Length of the boundary
//     uint8_t boundary_index;
    
//     FILE *file; // File pointer for multipart upload
// } TwsMultipartUploadState;


// class TwsMultipartUploadHandler : public TwsRequestHandlerEntry, public TwsStatefulRequestHandler<TwsMultipartUploadState> {
// public:

//     TwsMultipartUploadHandler(
//         const char *path, 
//         TwsRequestHandlerFunction onRequest,
//         TwsPostBodyHandlerFunction onPostBody
//         //TwsQueryParamHandlerFunction onQueryParam = nullptr,
//         //TwsHeaderHandlerFunction onHeader = nullptr
//     ) : TwsRequestHandlerEntry(path, TWS_HTTP_POST, onRequest, onPostBody) {
//         printf("TwsMultipartUploadHandler created for path: %s\n", path);
//         set_post_body_handler([this](TwsRequest &request, size_t index, uint8_t *data, size_t len) -> bool {
//             auto &state = get_state(request);

//             if(state.file == nullptr) {
//                 // Open file for writing
//                 state.file = fopen("/tmp/upload.bin", "wb");
//             }

//             // for(size_t i = 0; i < len; i++) {
//             //     if(data[i] == state.boundary[state.boundary_index]) {
//             //         state.boundary_index++;
//             //         if(state.boundary_index >= state.boundary_length) {
//             //             // We found the boundary, reset index
//             //             state.boundary_index = 0;
//             //             printf("Boundary found at index %zu\n", index + i);
//             //             // Handle boundary found logic here if needed
//             //         }
//             //     } else {
//             //         // TODO - if outputting, now send the data up to the boundary?
//             //         state.boundary_index = 0; // Reset if mismatch
//             //     }
//             // }

//             //state.content_length += 1;
//             fwrite(data, 1, len, state.file);
//             fflush(state.file);
//             //printf("woooh %zu %zu %d\n", index, index+len, state.content_length);

//             if(index+len >= state.content_length) {
//                 // Finished uploading
//                 fclose(state.file);
//                 state.file = nullptr;
//                 printf("Upload finished, content length: %d\n", state.content_length);
//                 return true; // Indicate that the upload is complete
//             }

//             return false;
//         });
//         set_header_handler([this](TwsRequest &request, const char *line, int len, bool final) {
//             if(!final) {
//                 // We only care about complete headers
//                 return;
//             }

//             // Handle headers if needed
//             printf("Header: %s\n", line);

//             auto &state = get_state(request);
//             printf("State is %p to %p (%zu)\n", &state, (&state + sizeof(state)), sizeof(state));

//             if(strncasecmp(line, "Content-Length:", 15) == 0) {
//                 char *endptr;
//                 int content_length = strtol(line + 15, &endptr, 10);
//                 if (endptr != line + 15 && content_length > 0) {
//                     state.content_length = content_length;
//                     printf("Content-Length: %d\n", content_length);
//                 }
//             }
//             //  else if(strncasecmp(line, "xontent-Type:", 13) == 0) {
//             //     // Check for multipart boundary
//             //     char *boundary = strstr((char*)line, "boundary=");
//             //     if (boundary) {
//             //         boundary += 9; // Skip "boundary="
//             //         //int end = len;
//             //         const char *end = line + len - 1;
//             //         if(*end == ';') end--;
//             //         auto &state = get_state(request);
//             //         state.boundary[0] = '-';
//             //         state.boundary[1] = '-';
//             //         printf("len be %d\n", (int)(end - boundary));
//             //         strncpy(state.boundary + 2, boundary, end - boundary);
//             //         state.boundary[2 + (end - boundary)] = '-';
//             //         state.boundary[3 + (end - boundary)] = '-';
//             //         state.boundary[4 + (end - boundary)] = '\r';
//             //         state.boundary[5 + (end - boundary)] = '\n';
//             //         state.boundary[6 + (end - boundary)] = '\0';
//             //         state.boundary_length = end - boundary + 7;
//             //         printf("Multipart boundary set: [%s]\n", state.boundary);
//             //     }
//             // }
//         });
//     }
//     ~TwsMultipartUploadHandler() {
//         printf("TwsMultipartUploadHandler destroyed\n");
//     }
// };

void tiny_web_server_loop(void * pData);
void idle_loop(void * pData);

typedef struct {
    size_t content_length = 0;
    uint8_t boundary_length = 0;
    uint8_t boundary_index = 0;
    char boundary[78];
    int part_header_end_index = -1;
    int file_start = -1;
    // FILE *file = nullptr;
    // FILE *file2 = nullptr;
} MultipartUploadHandlerState;

class MultipartUploadHandler : public TwsPostBodyHandler, public TwsHeaderHandler, public TwsStatefulHandler<MultipartUploadHandlerState> {
public:
    MultipartUploadHandler(TwsHandler *handler, TwsFileUploadHandler *onUpload);
    bool handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleHeader(TwsRequest &request, const char *line, int len) override;

    TwsFileUploadHandler *onUpload = nullptr;
    TwsPostBodyHandler *nextPostBody = nullptr;
    TwsHeaderHandler *nextHeader = nullptr;
    TwsAllocableHandler *nextAlloc = nullptr;
};


typedef struct {
    bool authed;
} BasicAuthState;

class BasicAuth : public TwsHeaderHandler, public TwsPostBodyHandler, public TwsRequestHandler, public TwsStatefulHandler<BasicAuthState> {
public:
    BasicAuth(TwsHandler *handler);

    void handleHeader(TwsRequest &request, const char *line, int len) override;

    bool denyIfUnauthed(TwsRequest &request);

    bool handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;

    void handleRequest(TwsRequest &request) override;

    TwsPostBodyHandler *nextPostBody = nullptr;
    TwsHeaderHandler *nextHeader = nullptr;
    TwsRequestHandler *nextRequest = nullptr;
};



template<class T>
class TwsWrappedHandler : public TwsHandler {
public:
    TwsWrappedHandler(const char *path, TwsRequestHandler *onRequest = nullptr) :
        TwsHandler(path, onRequest), wrapper(*this) {
    }

    T wrapper;
};

template<class T, class T2>
class TwsWrappedHandler2 : public TwsHandler {
public:
    TwsWrappedHandler2(const char *path, TwsRequestHandler *onRequest = nullptr) :
        TwsHandler(path, onRequest), wrapper(*this), wrapper2(*this) {
    }

    T wrapper;
    T2 wrapper2;
};

typedef struct {
  int mode;
} EOtaStartState;

class EOtaStart : public TwsRequestHandler, public TwsQueryParamHandler, public TwsStatefulHandler<EOtaStartState> {
public:
    EOtaStart(TwsHandler *handler);

    void handleRequest(TwsRequest &request) override;
    void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) override;
    
    TwsQueryParamHandler *nextQueryParam = nullptr;
};

typedef struct {
    bool error = false;
} EOtaUploadState;

class EOtaUpload : public TwsRequestHandler, public TwsFileUploadHandler, public TwsStatefulHandler<EOtaUploadState> {
public:
    EOtaUpload(TwsHandler *handler);

    void handleRequest(TwsRequest &request) override;
    void handleUpload(TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) override;

    MultipartUploadHandler multipart_handler;
    TwsRequestHandler *nextRequest = nullptr;
};


// void _hash_to_hex(const uint8_t *hash_output, char hash_output_hex[], int hash_length) {
//     for(int i = 0; i < hash_length; i++) {
//         hash_output_hex[i*2] = "0123456789abcdef"[hash_output[i] >> 4];
//         hash_output_hex[i*2 + 1] = "0123456789abcdef"[hash_output[i] & 0x0F];
//     }
// }

class Sha256Hash {
public:
    void init() {
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256
    }

    void update(const char *data, size_t len) {
        mbedtls_sha256_update(&ctx, (const unsigned char *)data, len);
    }

    void toHex(char hash_output_hex[64]) {
        uint8_t hash_output[32];
        mbedtls_sha256_finish(&ctx, hash_output);
        for(int i = 0; i < 32; i++) {
            hash_output_hex[i*2] = "0123456789abcdef"[hash_output[i] >> 4];
            hash_output_hex[i*2 + 1] = "0123456789abcdef"[hash_output[i] & 0x0F];
        }
    }

    int getHashLengthHex() const {
        return 64; // SHA-256 produces a 32-byte hash, which is 64 hex characters
    }

private:
    mbedtls_sha256_context ctx;
};

class Md5Hash {
public:
    void init() {
        mbedtls_md5_init(&ctx);
        mbedtls_md5_starts(&ctx);
    }

    void update(const char *data, size_t len) {
        mbedtls_md5_update(&ctx, (const unsigned char *)data, len);
    }

    void toHex(char hash_output_hex[32]) {
        uint8_t hash_output[16];
        mbedtls_md5_finish(&ctx, hash_output);
        for(int i = 0; i < 16; i++) {
            hash_output_hex[i*2] = "0123456789abcdef"[hash_output[i] >> 4];
            hash_output_hex[i*2 + 1] = "0123456789abcdef"[hash_output[i] & 0x0F];
        }
    }

    int getHashLengthHex() const {
        return 32; // MD5 produces a 16-byte hash, which is 32 hex characters
    }

private:
    mbedtls_md5_context ctx;
};

const int NONCE_LENGTH = 32;
typedef std::array<char, NONCE_LENGTH> Nonce;

template<typename HASH_CONTEXT>
struct DigestAuthState {
    bool authed;
    bool denied;
    uint8_t parse_state;
    uint8_t hash_state;
    HASH_CONTEXT ha2;
    HASH_CONTEXT r;
    char response[64];
    Nonce nonce;
};

//template<uint64_t (*millis64)()>
class DigestAuthSessionManager {
public:
    DigestAuthSessionManager(uint64_t timeout_ms = 3600 * 1000) : timeout_ms(timeout_ms) {}
    bool check(Nonce nonce, uint32_t nc);
private:
    void clear_old_sessions();

    struct Session {
        uint64_t last_seen;
        uint32_t count;
    };
    std::map<Nonce, Session> sessions;
    uint64_t timeout_ms;
};

typedef int (*GetPasswordHashFunc)(const char* username, char* output);



template<typename HASH_CONTEXT, int HASH_TYPE>
class DigestAuth : public TwsHeaderHandler, public TwsPartialHeaderHandler, public TwsPostBodyHandler, public TwsRequestHandler, public TwsStatefulHandler<DigestAuthState<HASH_CONTEXT>> {
public:
    DigestAuth(TwsHandler *handler, GetPasswordHashFunc getPasswordHash, DigestAuthSessionManager *sessionManager = nullptr);
    void handleHeader(TwsRequest &request, const char *line, int len) override;
    int handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) override;
    bool denyIfUnauthed(TwsRequest &request);
    bool handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleRequest(TwsRequest &request) override;

    TwsPostBodyHandler *nextPostBody = nullptr;
    TwsHeaderHandler *nextHeader = nullptr;
    TwsPartialHeaderHandler *nextPartialHeader = nullptr;
    TwsRequestHandler *nextRequest = nullptr;

    GetPasswordHashFunc getPasswordHash;
    DigestAuthSessionManager *sessionManager = nullptr;
    //SessionMap *sessions = nullptr;
};

//static constexpr char *MD5 = "MD5";
using Md5DigestAuth = DigestAuth<Md5Hash, 0>;
using Sha256DigestAuth = DigestAuth<Sha256Hash, 1>;

#endif // TINY_WEB_SERVER_H
