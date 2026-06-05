#include "TinyWebServer.h"

#ifndef LOCAL
#include <src/devboard/utils/logging.h>
#endif

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

extern "C" {
    #ifdef LOCAL
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <errno.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define IRAM_ATTR
    void vTaskDelay(int ms) {
        usleep(ms * 1000);
    }
    #else
    #include "lwip/sockets.h"
    #endif
}

// crude placeholder
TwsRoute nullHandler("/nope");

TinyWebServer::TinyWebServer(uint16_t port, TwsRoute *handlers[]) {
    _port = port;
    _handlers = handlers;

    for (int i = 0; i < slot_count(); i++) {
        slots[i] = new TwsRequest();
        slots[i]->slot_id = i;
    }

    for(int i = 0; _handlers[i] != nullptr; i++) {
        if(_handlers[i]->onAlloc) {
            // Allocate state data for this handler
            unsigned int alloc_size = _handlers[i]->onAlloc->handleAlloc(sizeof(slots[0]->state_data), 0);
            if(alloc_size > sizeof(slots[0]->state_data)) {
                DEBUG_PRINTF("TWS handler %s requires more state data than available (%d > %zu)\n", 
                    _handlers[i]->path, alloc_size, sizeof(slots[0]->state_data));
                _handlers[i] = &nullHandler; // Replace with a null handler
            } else if(alloc_size > max_handler_state_size) {
                max_handler_state_size = alloc_size;
            }
        }
    }
}

TinyWebServer::~TinyWebServer() {
    if (_listen_socket > -1) {
        close(_listen_socket);
        _listen_socket = -1;
    }

    for(int i=0; i < slot_count(); i++) {
        slots[i]->reset();
        delete slots[i];
    }
}

void TinyWebServer::open_listen_port() {
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) return;
    _listen_socket = listen_socket;

    const int enable = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        DEBUG_PRINTF("setsockopt(SO_REUSEADDR) failed\n");

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = 0;//(uint32_t) _addr;
    server.sin_port = htons(_port);
    if (bind(_listen_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        DEBUG_PRINTF("TWS bind error\n");
        return;
    }

    const uint8_t backlog = 5;
    if (listen(_listen_socket , backlog) < 0) {
        close(_listen_socket);
        DEBUG_PRINTF("TWS listen error\n");
        return;
    }
    int flags = fcntl(_listen_socket, F_GETFL);
    fcntl(_listen_socket, F_SETFL, flags | O_NONBLOCK);
}

char* trim_delimiter_and_whitespace(char *str, int *len) {
    // Modify the string in-place to remove the last character and trim leading
    // and trailing whitespace, and return a pointer to the start of the trimmed
    // string (with whitespace skipped). A nul terminator is added after the
    // last non-white-space character.

    // The new length of the string is written back to *len.

    (*len)--;
    while(*len > 0 && (str[(*len)-1]==' ' || str[(*len)-1]=='\r' || str[(*len)-1]=='\n')) {
        (*len)--;
    }
    str[*len] = '\0'; // Null-terminate the trimmed string
    return str;
};

char* trim_delimiter_and_trailing_whitespace(char *str, int *len) {
    // Like trim_delimiter_and_whitespace, but only trims trailing whitespace.

    (*len)--;
    while(*len > 0 && (str[(*len)-1]==' ' || str[(*len)-1]=='\r' || str[(*len)-1]=='\n')) {
        (*len)--;
    }
    while(*len > 0 && str[0]==' ') {
        str++;
        (*len)--;
    }
    str[*len] = '\0'; // Null-terminate the trimmed string
    return str;
};

const char *HTTP_400 = "HTTP/1.1 400 x\r\n"
                       "Connection: close\r\n"
                       "\r\n";

const char *HTTP_404 = "HTTP/1.1 404 x\r\n"
                       "Connection: close\r\n"
                       "\r\n";
        
//static_assert(strlen(HTTP_404) < TwsRequest::SEND_BUFFER_SIZE, "HTTP_404 response is too large for send buffer");

void TinyWebServer::handle_request(TwsRequest &request) {
    // a local buffer which is definitely big enough for the largest read and an extra nul
    char buf[sizeof(request.recv_buffer)+1];

    auto call_handle_partial_header = [&](char *ptr, int available, bool final) {
        // Temporarily replace the next character with a nul for safety.
        char old = ptr[available];
        ptr[available] = '\0';
        int consumed = request.handler->onPartialHeader->handlePartialHeader(request, ptr, available, final);
        ptr[available] = old;
        return consumed;
    };

    auto call_request_handler = [&]() {
        if(request.done) {
            // pass
        } else if(request.handler && request.handler->onRequest) {
            // Call the GET handler function
            request.handler->onRequest->handleRequest(request);
            if(request.writer_callback && request.free()>0) request.writer_callback(request, request.total_written - request.writer_callback_written_offset);
            request.parse_state = TWS_WRITING_OUT;
        } else {
            request.write_fully(HTTP_404);
            request.finish();
        }
    };

    while(true) {
        int len = 0;

        if(request.done) break;

        // Step 1: Grab a chunk of data to process, stopping at delimiters where
        // relevant.

        // In each parse state there are only a limited number of valid
        // delimiters, so scan(...) persists the scan position to avoid
        // rescanning data we've already looked at.

        switch(request.parse_state) {
            case TWS_AWAITING_PATH:
                len = request.scan(' ', '?', '\n');
                break;
            case TWS_AWAITING_QUERY_STRING:
                len = request.scan('&', ' ', '\n');
                break;
            case TWS_AWAITING_HEADER:
                len = request.scan('\n');
                if(len==0 && request.recv_buffer_full()) {
                    // We haven't found the end of the header, and the buffer is
                    // full, so we're not going to be able to read a complete
                    // one.
                    if(request.handler && request.handler->onPartialHeader) {
                        // We'll handle it in chunks.
                        request.parse_state = TWS_AWAITING_PARTIAL_HEADER;
                        continue;
                    }
                    // We can't handle it at all, so skip the rest of the header.
                    //DEBUG_PRINTF("TWS client buffer full, skipping header\n");
                    request.read_flush(request.available());
                    request.parse_state = TWS_SKIPPING_HEADER;
                }
                break;
            case TWS_AWAITING_PARTIAL_HEADER:
                len = request.scan('\n');
                if(len==0) {
                    // We still haven't found the end of the header.
                    int available = request.available();
                    if(available==0) break;

                    // Try and handle the chunk we have.
                    int consumed = call_handle_partial_header(request.get_peek_ptr(buf, available), available, false);
                    if(consumed > 0) request.read_flush(consumed);
                    else if(request.recv_buffer_full()) {
                        // We didn't consume anything, and the buffer is full,
                        // so we won't be able to consume any more. Skip the
                        // rest of the header.
                        request.read_flush(available);
                        request.parse_state = TWS_SKIPPING_HEADER;
                    }
                } else {
                    // We found the end of the header, so handle the final chunk.
                    char *ptr = request.get_read_ptr(buf, len);
                    if(len > 1 && ptr[len-2] == '\r') len--;
                    ptr[--len] = '\0';

                    while(len > 0) {
                        int consumed = call_handle_partial_header(ptr, len, true);
                        if(consumed <= 0) break;
                        ptr += consumed;
                        len -= consumed;
                    }
                    request.parse_state = TWS_AWAITING_HEADER;
                }
                break;
            case TWS_SKIPPING_HEADER:
                len = request.scan('\n');
                if(len==0) {
                    // End still not found, flush the entire buffer and keep going
                    request.read_flush(request.available());
                } else {
                    // We found a newline, so we're done skipping this header
                    request.parse_state = TWS_AWAITING_HEADER;
                    // But we're not interested in the data
                    request.read_flush(len);
                    len = 0;
                }
                break;
            case TWS_AWAITING_BODY:
            case TWS_WRITING_OUT:
                // Avoid wrapping reads, to avoid a copy when reading the POST
                // body. The first read will probably be shorter than a whole
                // buffer, but subsequent ones until the last will hopefully be
                // full.
                len = request.available_contiguous();
                break;
            default:
                len = request.scan(' ', '\n');
                break;
        }
        // Nothing new to process, so give up for now.
        if(len==0) {
            if(request.recv_buffer_full()) {
                // We have a full buffer but have failed to parse it, so abandon
                // the request.
                DEBUG_PRINTF("TWS client buffer full, closing connection\n");
                request.abort();
            }
            break;
        }

        // Step 2: Process the chunk of data.

        switch(request.parse_state) {
        case TWS_AWAITING_METHOD:
            if(request.match("GET ")) {
                request.method = TWS_HTTP_GET;
                request.parse_state = TWS_AWAITING_PATH;
            } else if(request.match("POST ")) {
                request.method = TWS_HTTP_POST;
                request.parse_state = TWS_AWAITING_PATH;
            } else if(request.match("OPTIONS ")) {
                request.method = TWS_HTTP_OPTIONS;
                request.parse_state = TWS_AWAITING_PATH;
            } else {
                char *pp = request.get_read_ptr(buf, len);
                DEBUG_PRINTF("TWS client invalid method! [%s]\n", pp);
                request.abort();
            }
            break;
        case TWS_AWAITING_PATH:
            {
                bool found = false;
                for(int j=0;_handlers[j]!=nullptr;j++) {
                    int path_len = _handlers[j]->path_len;
                    
                    // 1. Global wildcard match
                    if(path_len == 1 && _handlers[j]->path[0]=='*') {
                        request.read_flush(len-1); // consume the entire path
                        // TODO: should we store it in path_wildcard instead?
                        goto matched;
                    }

                    // 2. Prefix match (e.g., "/static/*")
                    if(path_len > 1 && _handlers[j]->path[path_len-1] == '*') {
                        int prefix_len = path_len - 1;
                        
                        if ((len - 1) >= prefix_len && request.match(_handlers[j]->path, prefix_len)) {
                            // We matched the prefix! Now extract the wildcard portion.
                            int wildcard_len = (len - 1) - prefix_len;
                            int copy_len = min(wildcard_len, TwsRequest::PATH_WILDCARD_SIZE - 1);
                            
                            request.read(request.path_wildcard, copy_len);
                            request.path_wildcard[copy_len] = '\0'; // Null-terminate it
                            
                            // Flush any excess characters if the path variable
                            // was longer than the wildcard buffer
                            if (wildcard_len > copy_len) {
                                request.read_flush(wildcard_len - copy_len);
                            }
                            
                            goto matched;
                        }
                    }

                    // 3. Exact match
                    if(len == (path_len + 1) && request.match(_handlers[j]->path, path_len)) {
                        goto matched;
                    }

                    continue;
                matched:
                    request.handler = _handlers[j];
                    memset(request.state_data, 0, max_handler_state_size);
                    found = true;
                    break;
                }
                if(!found) {
                    // Never matched so have to manually flush the path
                    request.read_flush(len-1);
                }

                char *delim = request.get_read_ptr(buf, 1);
                if(*delim == '?') {
                    request.parse_state = TWS_AWAITING_QUERY_STRING;
                } else {
                    request.parse_state = TWS_AWAITING_VERSION;
                }
                break;
            }
        case TWS_AWAITING_QUERY_STRING:
            {
                char *buf_ptr = request.get_read_ptr(buf, len);

                // grab the delim now as the trim will overwrite it
                char delim = buf_ptr[len-1];

                if(len>0 && request.handler && request.handler->onQueryParam) {
                    // Call the query param handler function
                    bool final = (delim == '&' || delim == '\n' || delim == ' ');
                    char *trimmed = trim_delimiter_and_whitespace(buf_ptr, &len);
                    request.handler->onQueryParam->handleQueryParam(request, trimmed, len, final);
                // } else {
                //     DEBUG_PRINTF("TWS query param is %s\n", trim_delimiter_and_whitespace(buf_ptr, &len));
                }

                if(delim == '&') {
                    request.parse_state = TWS_AWAITING_QUERY_STRING;
                } else if(delim == '\n') {
                    request.parse_state = TWS_AWAITING_HEADER;
                } else if(delim == ' ') {
                    request.parse_state = TWS_AWAITING_VERSION;
                }
                break;
            }
        case TWS_AWAITING_VERSION:
            if(request.match("HTTP/1.1\r\n") || request.match("HTTP/1.1\n") ||
                request.match("HTTP/1.0\r\n") || request.match("HTTP/1.0\n")) {
                request.parse_state = TWS_AWAITING_HEADER;
            } else {
                // request.read(buf, len);
                // DEBUG_PRINTF("TWS client invalid HTTP version! %s\n", trim_delimiter_and_whitespace(buf, &len));
                request.finish();
            }
            break;
        case TWS_AWAITING_HEADER:
            if(request.match("\r\n") || request.match("\n")) {
                // End of headers, process the request

                if(request.is_post() && request.content_length > 0) {
                    // Prepare to receive the POST body
                    request.parse_state = TWS_AWAITING_BODY;
                } else if(request.is_post() && request.content_length < 0) {
                    // No valid Content-Length header was supplied
                    request.write_fully(HTTP_400);
                    request.finish();
                } else {
                    call_request_handler();
                }
            } else {
                char *buf_ptr = request.get_read_ptr(buf, len);

                if(request.is_post() && len > 15 && strncasecmp(buf_ptr, "Content-Length:", 15)==0) {
                    // Parse the Content-Length header

                    int value_len = len - 15;
                    char *cl_ptr = trim_delimiter_and_whitespace(buf_ptr + 15, &value_len);

                    char *endptr = nullptr;
                    int32_t val = (int32_t)strtoul(cl_ptr, &endptr, 10);

                    if (cl_ptr[0] != '\0' && *endptr == '\0' && val >= 0) {
                        // Only keep the result if it is a valid non-negative integer
                        request.content_length = val;
                    } else {
                        DEBUG_PRINTF("TWS invalid content length: %s\n", cl_ptr);
                    }
                }

                if(request.handler && request.handler->onHeader) {
                    // Don't trim leading whitespace, as we may be mid-chunk
                    char *trimmed = trim_delimiter_and_trailing_whitespace(buf_ptr, &len);
                    request.handler->onHeader->handleHeader(request, trimmed, len);
                }
            }
            break;
        case TWS_AWAITING_BODY:
            {
                if(request.handler && request.handler->onPostBody) {
                    bool post_done = false;
                    bool allow_noncontiguous = false;
                    //bool progress_made = false;

                    // If our read buffer is significantly smaller than the
                    // underlying socket buffer, then we can attempt several
                    // back-to-back recvs and speed up uploads.
                    for(int chunk=0; chunk<((2559/TwsRequest::RECV_BUFFER_SIZE)+1); chunk++) {
                        // We should be able to get a read pointer into the receive
                        // buffer, avoiding a copy, since len is the available
                        // contiguous length.
                        char *read_ptr = request.get_peek_ptr(buf, len);

                        uint32_t remaining = request.content_length - request.body_read;
                        int to_process = min((uint32_t)len, remaining);

                        int consumed = request.handler->onPostBody->handlePostBody(
                            request, request.body_read, (uint8_t*)read_ptr, to_process
                        );

                        if(request.done) {
                            // Handler may have finished the request.
                            return;
                        } else if(request.body_read + consumed >= request.content_length) {
                            post_done = true;
                        } else if(consumed == -1) {
                            // Handler doesn't want any more body but we still have some left.
                            // Must have been a bad request.
                            request.write_fully(HTTP_400);
                            request.finish();
                            return;
                        }

                        if(consumed==0 && len<request.available() && !allow_noncontiguous) {
                            // The handler didn't consume any of our contiguous buffer.
                            // Try again with the whole buffer.
                            allow_noncontiguous = true;
                            len = request.available(); // read the full buffer this time
                            chunk = -1; // ensure another loop
                            continue;
                        }

                        request.body_read += consumed > 0 ? consumed : 0;
                        request.read_flush(consumed > 0 ? consumed : 0);
                        // request.read_flush(post_done ? len : consumed);

                        // request.body_read += post_done ? len : consumed;
                        // request.read_flush(post_done ? len : consumed);

                        if(consumed>0) {
                            // We are consuming data (potentially slowly), so
                            // update last_activity so we don't timeout.
                            request.last_activity = millis();
                        }

                        // Are we done, or is there nothing more to be read?
                        if(post_done || request.recv()==0) break;
                        // We just read some more, go round again!
                        len = request.available_contiguous();
                    }

                    // FIXME - this breaks CanSender currently
                    // if(!progress_made && request.recv_buffer_full()) {
                    //     DEBUG_PRINTF("TWS client body handler stuck, closing connection\n");
                    //     request.abort();
                    //     return;
                    // }

                    if(post_done && !request.done) {
                        DEBUG_PRINTF("TWS finished reading POST body!");
                        if(request.writer_callback) {
                            // The post handler set a writer callback, so skip the normal request handler
                            if(request.free()>0) request.writer_callback(request, request.total_written - request.writer_callback_written_offset);
                            request.parse_state = TWS_WRITING_OUT;
                        } else {
                            call_request_handler();
                        }
                    }
                } else {
                    // Don't accept POST requests that aren't handled
                    request.write_fully(HTTP_404);
                    request.finish();
                }
                return;
            }
        case TWS_WRITING_OUT:
            {
                DEBUG_PRINTF("weird, got data in TWS_WRITING_OUT state, len: %d\n", len);
                //exit(1);
                return;
            }
        }
    }
}

void TinyWebServer::accept_new_connections() {
    // Find a free client slot
    int slot_index = -1;
    for (int i = 0; i < slot_count(); i++) {
        if (slots[i]->socket == -1) {
            slot_index = i;
            break;
        }
    }
    if (slot_index < 0) {
        // No free client slot, cannot accept new connections
        return;
    }

    int socket = accept(_listen_socket, NULL, NULL);

    if (socket < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            // Not a problem, ignore
            return;
        }
        // Another error occurred
        DEBUG_PRINTF("TWS accept error\n");
        return;
    }

    int flags = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);

    // Enabling TCP_NODELAY is likely counter-productive - we want to be able to
    // use the full lwIP send buffer if we can for maximum throughput (and
    // minimise the overhead of write_direct calls made by the core loop).
    // Latency isn't really a concern?
    // int nodelay = 1; 
    // setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    slots[slot_index]->reset();
    slots[slot_index]->connection_id = ++last_connection_id;
    slots[slot_index]->socket = socket;
    slots[slot_index]->last_activity = millis();
}

__attribute__((noinline)) uint32_t TinyWebServer::get_waiting_requests(long max_delay_us) {
    // fd_sets use a lot of stack, so we do the select here in a separate
    // stack frame.

    fd_set read_sockets, write_sockets;
    FD_ZERO(&read_sockets);
    FD_ZERO(&write_sockets);
    FD_SET(_listen_socket, &read_sockets);
    int max_fds = _listen_socket + 1;
    int pending_write_sockets = 0;

    // select(..) only allows fds up to FD_SETSIZE (32 on ESP32), but the max
    // socket fd on ESP32 should be below that.

    for (int i = 0; i < slot_count(); i++) {
        auto &request = *slots[i];
        if (request.active()) {
            FD_SET(request.socket, &read_sockets);
            max_fds = std::max(max_fds, request.socket + 1);
            if (request.send_buffer_len() > 0 || request.pending_direct_write) {
                FD_SET(request.socket, &write_sockets);
                pending_write_sockets |= (1 << i);
            }
        }
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = max_delay_us;

    // We're using a 32-bit bitvector for request status
    static_assert(MAX_REQUESTS <= sizeof(int)*8);
    int ret = 0;

    int activity = select(max_fds, &read_sockets, &write_sockets, NULL, &timeout);
    if(activity>0) {
        // Check if the socket has closed
        if (FD_ISSET(_listen_socket, &read_sockets)) {
            // New connection available
            accept_new_connections();
        }
        // Check each client socket for activity
        for (int i = 0; i < slot_count(); i++) {
            auto &request = *slots[i];
            if (request.active() && FD_ISSET(request.socket, &read_sockets)) {
                // There is data waiting to be read, so flag a tick.
                ret |= (1 << i);
            } else if(request.active() && (pending_write_sockets & (1 << i)) && FD_ISSET(request.socket, &write_sockets)) {
                // There is room for writing, and we are waiting to write, flag
                // a tick.
                ret |= (1 << i);
                // Any pending direct write has now been written, so reset the flag.
                request.pending_direct_write = false;
            }
        }
    }
    return ret;
}

bool TinyWebServer::poll() {
    long max_delay_us = IDLE_POLL_TIME_MS * 1000;
    bool active_polling = false;

    for(int i = 0; i < slot_count(); i++) {
        if(slots[i]->active()) {
            // If there are active clients, we poll more frequently (due to
            // writer callbacks, etc).
            max_delay_us = ACTIVE_POLL_TIME_MS * 1000;
            active_polling = true;
            break;
        }
    }

    uint32_t waiting_requests = get_waiting_requests(max_delay_us);

    for(int i = 0; i < slot_count(); i++) {
        if(slots[i]->active()) {
            if(waiting_requests & (1 << i)) {
                // Client has activity, process it
                slots[i]->perform_io();
                if(!slots[i]->active()) continue;
            }
           
            // Handle the client regardless of activity, since there may be
            // writer callbacks that now have data ready.
            handle_request(*slots[i]);
            if(!slots[i]->active()) continue;

            // Handle completions and timeouts.
            slots[i]->tick();
        }
    }

    return active_polling;
}

int IRAM_ATTR TinyWebServer::write(uint32_t connection_id, const char *data, size_t len) {
    // Write to the given connection ID (if it is still active).

    // Find the request with the given connection ID
    for (int i = 0; i < slot_count(); i++) {
        if (slots[i]->connection_id == connection_id) {
            // Lock the slot
            while(slots[i]->send_buffer_lock.test_and_set(std::memory_order_acquire)) {
                // Avoid priority inversion with an explicit delay. An
                // alternative would be a proper FreeRTOS mutex instead of this
                // spinlock, but that has much worse uncontended performance.
                vTaskDelay(1);
            }
            // Recheck we still have a valid connection after acquiring the lock
            int ret = (
                (slots[i]->active() && slots[i]->connection_id == connection_id && slots[i]->reply_started())
                ? slots[i]->write_indirect(data, len)
                : -1 // Connection lost while waiting for lock, report failure
            );
            slots[i]->send_buffer_lock.clear(std::memory_order_release);
            return ret;
        }
    }
    return -1; // Connection ID not found
}

int IRAM_ATTR TinyWebServer::free(uint32_t connection_id) {
    // Return the free space in the send buffer for the given connection ID (if
    // it is still active).

    // Find the request with the given connection ID
    for (int i = 0; i < slot_count(); i++) {
        if (slots[i]->active() && slots[i]->connection_id == connection_id) {
            return slots[i]->free();
        }
    }
    return -1; // Connection ID not found
}

void TinyWebServer::finish(uint32_t connection_id) {
    // Find the request with the given connection ID
    for (int i = 0; i < slot_count(); i++) {
        if (slots[i]->active() && slots[i]->connection_id == connection_id) {
            slots[i]->finish();
            return;
        }
    }
}
