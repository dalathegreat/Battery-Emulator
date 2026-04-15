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

#include "TwsHandler.h"

#include <functional>
#include <cstring>
#include <map>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <type_traits>

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
  TWS_HTTP_OPTIONS = 0x4,
} TwsMethod;

// Forward declare handler interfaces for TwsRoute
class TwsRequestHandler;
class TwsPostBodyHandler;
class TwsHeaderHandler;
class TwsPartialHeaderHandler;
class TwsQueryParamHandler;
class TwsAllocableHandler;
class TwsMiddleware;

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
    int read(char *buf, uint16_t len);
    int read_flush(uint16_t len);

    void send(int code, const char *content_type = "", const char *content = "");
    void finish();
    inline void set_writer_callback(TwsRequestWriterCallbackFunction callback) {
        writer_callback = callback;
        // record the current total written bytes to use as an offset
        writer_callback_written_offset = total_written;
    }
    inline bool active() {
        return socket > -1;
    }
    inline bool is_post() { return method == TWS_HTTP_POST; }
    inline bool is_get() { return method == TWS_HTTP_GET; }
    inline bool is_options() { return method == TWS_HTTP_OPTIONS; }
    inline const char* method_str() {
        if (is_post()) return "POST";
        if (is_options()) return "OPTIONS";
        return "GET";
    }
    inline const char* get_path_wildcard() {
        return path_wildcard;
    }
    inline uint16_t get_slot_id() {
        return slot_id;
    }
    inline uint32_t get_connection_id() {
        return connection_id;
    }
    inline uint8_t* get_state_data() {
        return state_data;
    }
    inline bool reply_started() {
        return total_written > 0;
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
    TwsRoute *handler = nullptr;

    static const int PATH_WILDCARD_SIZE = 64;
    char path_wildcard[PATH_WILDCARD_SIZE] = {0};

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

// TwsRoute represents a route handler for a specific path. These are created
// before startup and passed to the TinyWebServer constructor in an array.
//
// The path can be an exact match (e.g. "/status") or a wildcard ("*") to match
// every request. A wildcard handler should usually be the last handler in the
// array.
//
// Each handler includes a set of optional callbacks for different stages of the
// request, as pointers to Tws*Handler subclasses. Various constructors are
// provided to ease setting these.
class TwsRoute {
public:
    TwsRoute(const char *path) : path(path), path_len(strlen(path)) {}
    TwsRoute(const char *path, TwsRequestHandler *onRequest) :
        path(path), path_len(strlen(path)), onRequest(onRequest) {
    }
    TwsRoute(const char *path, TwsPostBodyHandler *onPostBody) :
        path(path), path_len(strlen(path)), onPostBody(onPostBody) {
    }
    TwsRoute(const char *path, TwsMiddleware *middleware) : TwsRoute(path) {
        this->use(*middleware);
    }

    template<typename M>
    TwsRoute& use(M& middleware) {
        static_assert(std::is_base_of<TwsMiddleware, M>::value, 
            "ERROR: Only classes derived from TwsMiddleware can be passed to .use()!");

        // Point the middleware's 'next' to whatever the route is currently pointing at.
        middleware.nextHeader = this->onHeader;
        this->onHeader = &middleware;

        middleware.nextPartialHeader = this->onPartialHeader;
        this->onPartialHeader = &middleware;

        middleware.nextPostBody = this->onPostBody;
        this->onPostBody = &middleware;

        middleware.nextRequest = this->onRequest;
        this->onRequest = &middleware;

        middleware.nextAlloc = this->onAlloc;
        this->onAlloc = &middleware;

        // Return the route so we can chain calls
        return *this;
    }

    const char *path;
    const uint8_t path_len;
    TwsRequestHandler *onRequest = nullptr;    
    TwsPostBodyHandler *onPostBody = nullptr;
    TwsHeaderHandler *onHeader = nullptr;
    TwsPartialHeaderHandler *onPartialHeader = nullptr;
    TwsQueryParamHandler *onQueryParam = nullptr;
    TwsAllocableHandler *onAlloc = nullptr;
};

class TinyWebServer {
public:
    TinyWebServer(uint16_t port, TwsRoute *handlers[] = nullptr);
    ~TinyWebServer();

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
    // How long before an idle connection is closed.
    static const int IDLE_TIMEOUT_MS = 10000;

protected:
    uint16_t _port;
    int _listen_socket = -1;
    TwsRoute **_handlers = nullptr;
    unsigned int max_handler_state_size = 0;

    void accept_new_connections();
    uint32_t get_waiting_requests(long max_delay_us);
    void handle_request(TwsRequest &request);

    TwsRequest slots[MAX_REQUESTS];
    int last_connection_id = 0;
};

void tiny_web_server_loop(void * pData);
void idle_loop(void * pData);



template<class T>
class TwsWrappedHandler : public TwsRoute {
public:
    TwsWrappedHandler(const char *path, TwsRequestHandler *onRequest = nullptr) :
        TwsRoute(path, onRequest), wrapper(*this) {
    }

    T wrapper;
};

template<class T, class T2>
class TwsWrappedHandler2 : public TwsRoute {
public:
    TwsWrappedHandler2(const char *path, TwsRequestHandler *onRequest = nullptr) :
        TwsRoute(path, onRequest), wrapper(*this), wrapper2(*this) {
    }

    T wrapper;
    T2 wrapper2;
};

template <typename T>
int TwsStatefulMiddleware<T>::handleAlloc(int max, int start) {
    int alloc_size = sizeof(T);
    offset = start;
    start += alloc_size;
    
    // Use TwsMiddleware's nextAlloc pointer to continue the chain
    if (nextAlloc) {
        start = nextAlloc->handleAlloc(max, start);
    }
    return start;
}

template <typename T>
T& TwsStatefulMiddleware<T>::get_state(TwsRequest &request) {
    return *(T*)(request.get_state_data() + offset);
}

#endif // TINY_WEB_SERVER_H
