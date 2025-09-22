#include "TinyWebServer.h"

#ifndef LOCAL
#include <src/devboard/utils/logging.h>
#endif

#include <algorithm>
#include <fcntl.h>
#include <memory>

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

extern "C" {
    #ifdef LOCAL
    #include <sys/socket.h>
    #include <errno.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #else
    #include "lwip/sockets.h"
    #endif
}

// crude placeholder
TwsHandler nullHandler("/nope", nullptr);

TinyWebServer::TinyWebServer(uint16_t port, TwsHandler *handlers[]) {
    _port = port;
    _handlers = handlers;
    
    for (int i = 0; i < MAX_REQUESTS; i++) {
        slots[i].slot_id = i;
    }

    for(int i = 0; _handlers[i] != nullptr; i++) {
        if(_handlers[i]->onAlloc) {
            // Allocate state data for this handler
            unsigned int alloc_size = _handlers[i]->onAlloc->handleAlloc(sizeof(slots[0].state_data), 0);
            if(alloc_size > sizeof(slots[0].state_data)) {
                DEBUG_PRINTF("TWS handler %s requires more state data than available (%d > %zu)\n", 
                    _handlers[i]->path, alloc_size, sizeof(slots[0].state_data));
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

    for(int i=0; i < MAX_REQUESTS; i++) {
        slots[i].reset();
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
    // Trim leading and trailing whitespace, returning a pointer into the original string.
    // The last character is always removed.
    (*len)--;
    while(*len > 0 && (str[(*len)-1]==' ' || str[(*len)-1]=='\r' || str[(*len)-1]=='\n')) {
        (*len)--;
    }
    str[*len] = '\0'; // Null-terminate the trimmed string
    return str;
};

char* trim_delimiter_and_trailing_whitespace(char *str, int *len) {
    // Trim trailing whitespace, returning a pointer into the original string.
    // The last character is always removed.
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

const char *HTTP_404 = "HTTP/1.1 404 x\r\n"
                       "Connection: close\r\n"
                       "\r\n";
//static_assert(strlen(HTTP_404) < TwsRequest::SEND_BUFFER_SIZE, "HTTP_404 response is too large for send buffer");

void TinyWebServer::handle_request(TwsRequest &request) {
    

    // a local buffer which is definitely big enough for the largest read and an extra nul
    char buf[sizeof(request.recv_buffer)+1];

    while(true) {
        int len = 0;

        if(request.done) break;

        // Scan up to the next delimiter based on the current parse state
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
                    // We can't fit the whole header into the buffer
                    if(request.handler && request.handler->onPartialHeader) {
                        // The handler accepts partial headers.
                        
                        int available = request.available();

                        // Peek the whole read buffer (will likely copy into buf)
                        char *buf_ptr = request.get_peek_ptr(buf, available);
                        char old_terminator = buf_ptr[available]; // Save first char beyond the end
                        buf_ptr[available] = '\0';

                        int consumed = request.handler->onPartialHeader->handlePartialHeader(request, buf_ptr, available, false);
                        if(consumed<=0) {
                            // Handler didn't consume anything, so it never will. Skip the header.
                            request.read_flush(available);
                            request.parse_state = TWS_SKIPPING_HEADER;
                        } else {
                            request.read_flush(consumed);
                            // We'll handle the rest of the header in bits.
                            request.parse_state = TWS_AWAITING_PARTIAL_HEADER;
                        }
                        buf[available] = old_terminator; // Restore the old terminator
                    } else {
                        // The handler doesn't accept partial headers, so we skip the whole header
                        DEBUG_PRINTF("TWS client buffer full, skipping header\n");
                        request.read_flush(request.available());
                        request.parse_state = TWS_SKIPPING_HEADER;
                    }
                }
                break;
            case TWS_AWAITING_PARTIAL_HEADER:
                len = request.scan('\n');
                if(len==0) {
                    // Still haven't found the end of the header

                    int available = request.available();

                    // Peek the whole read buffer (will copy into buf)
                    char *buf_ptr = request.get_peek_ptr(buf, available);

                    int consumed = request.handler->onPartialHeader->handlePartialHeader(request, buf_ptr, available, false);
                    request.read_flush(consumed);
                } else {
                    char *buf_ptr = request.get_read_ptr(buf, len);
                    // Drop the last letter if there's a \r too
                    if(len>1 && buf_ptr[len-2] == '\r') len--;
                    // Swap the newline for a nul
                    buf_ptr[len-1] = '\0';
                    len -= 1;
                    while(len>0) {
                        int consumed = request.handler->onPartialHeader->handlePartialHeader(request, buf_ptr, len, true);
                        if(consumed<=0) {
                            break;
                        } else {
                            buf_ptr += consumed;
                            len -= consumed;
                        }
                    }
                    len = 0;
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
        if(len==0) {
            if(request.recv_buffer_full()) {
                DEBUG_PRINTF("TWS client buffer full, closing connection\n");
                request.reset();
            }
            break;
        }

        switch(request.parse_state) {
        case TWS_AWAITING_METHOD:
            if(request.match("POST ")) {
                request.method = TWS_HTTP_POST;
                request.parse_state = TWS_AWAITING_PATH;
            } else if(request.match("GET ")) {
                request.method = TWS_HTTP_GET;
                request.parse_state = TWS_AWAITING_PATH;
            } else if(request.match("OPTIONS ")) {
                request.method = TWS_HTTP_OPTIONS;
                request.parse_state = TWS_AWAITING_PATH;
            } else {
                char *pp = request.get_read_ptr(buf, len);
                DEBUG_PRINTF("TWS client invalid method! [%s]\n", pp);
                request.reset();
            }
            break;
        case TWS_AWAITING_PATH:
            {
                bool found = false;
                for(int j=0;_handlers[j]!=nullptr;j++) {
                    int path_len = strlen(_handlers[j]->path);
                    if(path_len == 1 && _handlers[j]->path[0]=='*') {
                        // Wildcard match
                        request.read_flush(len-1); // need to flush since match(...) hasn't consumed the path
                        goto matched;
                    }
                    if(len == (path_len + 1) && request.match(_handlers[j]->path, path_len)) {
                        // Exact match
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
                } else {
                    DEBUG_PRINTF("TWS query param is %s\n", trim_delimiter_and_whitespace(buf_ptr, &len));
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

                if(request.handler && request.handler->onRequest) {
                    if(request.method==TWS_HTTP_POST && request.handler->onPostBody) {
                        // check Content-Length to decide whether to do this?
                        request.parse_state = TWS_AWAITING_BODY;
                    } else {
                        // Call the handler function
                        request.handler->onRequest->handleRequest(request);
                        if(request.writer_callback && request.free()>0) request.writer_callback(request, request.total_written - request.writer_callback_written_offset);
                        request.parse_state = TWS_WRITING_OUT;
                    }
                } else {
                    request.write_fully(HTTP_404);
                    request.finish();
                }
            } else {
                char *buf_ptr = request.get_read_ptr(buf, len);

                if(request.handler && request.handler->onHeader) {
                    // Don't trim leading whitespace, as we may be mid-chunk
                    char *trimmed = trim_delimiter_and_trailing_whitespace(buf_ptr, &len);
                    request.handler->onHeader->handleHeader(request, trimmed, len);
                } else {
                    //DEBUG_PRINTF("TWS client header: [%s]\n", trim(hbuf_ptr, len));
                }
            }
            break;
        case TWS_AWAITING_BODY:
            {
                if(request.handler && request.handler->onPostBody) {
                    bool post_done = false;
                    bool allow_noncontiguous = false;

                    // If our read buffer is significantly smaller than the
                    // underlying socket buffer, then we can attempt several
                    // back-to-back recvs and speed up uploads.
                    for(int chunk=0; chunk<((2559/TwsRequest::RECV_BUFFER_SIZE)+1); chunk++) {
                        // We should be able to get a read pointer into the receive
                        // buffer, avoiding a copy, since len is the available
                        // contiguous length.
                        char *read_ptr = request.get_peek_ptr(buf, len);

                        int consumed = request.handler->onPostBody->handlePostBody(request, request.body_read, (uint8_t*)read_ptr, len);
                        post_done = (consumed == -1);

                        if(consumed==0 && len<request.available() && !allow_noncontiguous) {
                            // The handler didn't consume anything, so it never will.
                            // Try again with the whole buffer.
                            allow_noncontiguous = true;
                            len = request.available(); // read the full buffer this time
                            chunk = -1; // ensure another loop
                            continue;
                        }

                        request.body_read += post_done ? len : consumed;
                        request.read_flush(post_done ? len : consumed);

                        if(consumed>0) {
                            // We are consuming data (potentially slowly), so
                            // update last_activity so we don't timeout.
                            request.last_activity = millis();
                        }

                        // Are we done, or is there nothing more to be read?
                        if(post_done || request.recv()==0) break;
                        // We just read some more!
                        len = request.available_contiguous();
                    }

                    if(post_done && !request.done) {
                        if(request.writer_callback ) {
                            // The post handler set a writer callback, so skip the normal request handler
                            if(request.free()>0) request.writer_callback(request, request.total_written - request.writer_callback_written_offset);
                            request.parse_state = TWS_WRITING_OUT;
                        } else if(request.handler->onRequest) {
                            request.handler->onRequest->handleRequest(request);
                            if(request.writer_callback && request.free()>0) request.writer_callback(request, request.total_written - request.writer_callback_written_offset);
                            request.parse_state = TWS_WRITING_OUT;
                        } else {
                            request.write_fully(HTTP_404);
                            request.finish();
                        }
                    }
                } else {
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
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (slots[i].socket == -1) {
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

    slots[slot_index].reset();
    slots[slot_index].connection_id = ++last_connection_id;
    slots[slot_index].socket = socket;
    slots[slot_index].last_activity = millis();
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
 
    for (int i = 0; i < MAX_REQUESTS; i++) {
        auto &request = slots[i];
        if (request.active()) {
            //DEBUG_PRINTF("FD_SET read %d\n", request.connection_id);
            FD_SET(request.socket, &read_sockets);
            max_fds = std::max(max_fds, request.socket + 1);
            if (request.send_buffer_len > 0 || request.pending_direct_write) {
                //DEBUG_PRINTF("FD_SET write %d\n", request.connection_id);
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
        for (int i = 0; i < MAX_REQUESTS; i++) {
            auto &request = slots[i];
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

    for(int i = 0; i < MAX_REQUESTS; i++) {
        if(slots[i].active()) {
            // If there are active clients, we poll more frequently (due to
            // writer callbacks, etc).
            max_delay_us = ACTIVE_POLL_TIME_MS * 1000;
            active_polling = true;
            break;
        }
    }

    uint32_t waiting_requests = get_waiting_requests(max_delay_us);

    for(int i = 0; i < MAX_REQUESTS; i++) {
        if(slots[i].active()) {
            if(waiting_requests & (1 << i)) {
                // Client has activity, process it
                slots[i].perform_io();
                if(!slots[i].active()) continue;
            }
           
            // Handle the client regardless of activity, since there may be
            // writer callbacks that now have data ready.
            handle_request(slots[i]);
            if(!slots[i].active()) continue;

            // Handle completions and timeouts.
            slots[i].tick();
        }
    }

    return active_polling;
}

int TinyWebServer::write(uint32_t connection_id, const char *data, size_t len) {
    // Find the request with the given connection ID
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (slots[i].active() && slots[i].connection_id == connection_id) {
            return slots[i].write(data, len);
        }
    }
    return -1; // Connection ID not found
}

int TinyWebServer::free(uint32_t connection_id) {
    // Find the request with the given connection ID
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (slots[i].active() && slots[i].connection_id == connection_id) {
            return slots[i].free();
        }
    }
    return -1; // Connection ID not found
}

void TinyWebServer::finish(uint32_t connection_id) {
    // Find the request with the given connection ID
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (slots[i].active() && slots[i].connection_id == connection_id) {
            slots[i].finish();
            return;
        }
    }
}

void TwsRequest::reset() {
    if(socket>-1) {
        close(socket);
    }
    socket = -1;
    send_buffer_len = 0;
    send_buffer_write_ptr = 0;
    send_buffer_read_ptr = 0;
    recv_buffer_len = 0;
    recv_buffer_write_ptr = 0;
    recv_buffer_read_ptr = 0;
    recv_buffer_scan_len = 0;
    recv_buffer_scan_ptr = 0;
    parse_state = TWS_AWAITING_METHOD;
    done = false;
    total_written = 0;
    body_read = 0;
    pending_direct_write = false;
    writer_callback = nullptr;
    writer_callback_written_offset = 0;
    if(handler && handler->onAlloc) {
        handler->onAlloc->handleFree(*this);
    }
    handler = nullptr;
    connection_id = 0;
}

void TwsRequest::perform_io() {
    // send any data awaiting sending
    while(send_buffer_len>0) {
        const uint16_t contiguous_block_size = sizeof(send_buffer) - send_buffer_read_ptr;
        const uint16_t bytes_to_send = min(send_buffer_len, contiguous_block_size);
        const int bytes_sent = ::send(socket, &send_buffer[send_buffer_read_ptr], bytes_to_send, 0);
        //DEBUG_PRINTF("  ::send returned %d\n", bytes_sent);
        if (bytes_sent > 0) {
            // Advance the read pointer, wrapping around the buffer if necessary.
            send_buffer_read_ptr = sub_mod(send_buffer_read_ptr + bytes_sent, sizeof(send_buffer));
            send_buffer_len -= bytes_sent;
            last_activity = millis();
        } else if (bytes_sent <= 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            // An error occurred, close the connection
            DEBUG_PRINTF("TWS send error: %d - %s\n", errno, strerror(errno));
            reset();
            return;
        } else {
            // No more space available to write, break out of the loop
            break;
        }
    }

    // Read any incoming data
    recv();
}

int TwsRequest::recv() {
    int total_read = 0;
    while(recv_buffer_len < sizeof(recv_buffer)) {
        auto contiguous_block_size = sizeof(recv_buffer) - recv_buffer_write_ptr;
        const int bytes_to_read = min(sizeof(recv_buffer) - recv_buffer_len, contiguous_block_size);
        //DEBUG_PRINTF("TWS recv on %d, socket %d\n", client_id, socket);
        int bytes_received = ::recv(socket, &recv_buffer[recv_buffer_write_ptr], bytes_to_read, 0);

        //printf("  ::recv returned %d\n", bytes_received);
        if (bytes_received > 0) {
            // Advance the write pointer, wrapping around the buffer if necessary.
            recv_buffer_write_ptr = sub_mod(recv_buffer_write_ptr + bytes_received, sizeof(recv_buffer));
            recv_buffer_len += bytes_received;
            last_activity = millis();
            total_read += bytes_received;
        } else if (bytes_received == 0) {
            // Connection closed by the client
            //DEBUG_PRINTF("TWS client closed connection\n");
            reset();
            break;
        } else if (bytes_received < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            // An error occurred, close the connection
            DEBUG_PRINTF("TWS recv error on %d: %d - %s\n", slot_id, errno, strerror(errno));
            reset();
            break;
        } else {
            // No more data available to read
            break;
        }
    }
    return total_read;
}

void TwsRequest::tick() {
    if(send_buffer_len==0 && done && !pending_direct_write) {
        // If the send buffer is empty and the request is done, close the connection
        //DEBUG_PRINTF("TWS done, closing %d\n", connection_id);
        reset();
        return;
    }

    if(send_buffer_len==0 && !done && writer_callback) {
        // If the send buffer is empty and a callback is set, call it.
        writer_callback(*this, this->total_written - this->writer_callback_written_offset);
    }

    unsigned long elapsed = millis() - last_activity;
    if(elapsed > TinyWebServer::IDLE_TIMEOUT_MS && elapsed < 0x80000000) {
        // No activity for 10 seconds, close the connection
        // (hacky workaround for stale values in millis)
        DEBUG_PRINTF("TWS client timeout, closing connection %d\n", connection_id);
        reset();
        return;
    }
}

uint32_t TwsRequest::write(const char *buf) {
    if (send_buffer_len >= sizeof(send_buffer)) {
        return 0; // Buffer full
    }
    int i;
    int limit = sizeof(send_buffer) - send_buffer_len;
    for(i = 0; buf[i] != '\0' && i < limit; ++i) {
        // Place the byte at the current write position.
        send_buffer[send_buffer_write_ptr] = buf[i];
        send_buffer_write_ptr = sub_mod(send_buffer_write_ptr + 1, sizeof(send_buffer));
    }

    send_buffer_len += i;
    total_written += i; // Update total written bytes

    // Return the number of bytes written
    return i;
}

uint32_t TwsRequest::write(const char* buf, uint16_t len) {
    int bytes_sent = 0;
    if(send_buffer_len==0) {
        // Try to write directly, skipping the circular buffer.
        bytes_sent = write_direct(buf, len);
        if(bytes_sent>0) {
            len -= bytes_sent;
            buf += bytes_sent;
        }
        if(len==0) {
            // We have sent everything
            return bytes_sent;
        }
    }
    
    const uint16_t available_space = sizeof(send_buffer) - send_buffer_len;
    if (available_space == 0) {
        return 0;
    }
    const uint16_t bytes_to_copy = min(len, available_space);

    for (uint16_t i = 0; i < bytes_to_copy; ++i) {
        // Place the byte at the current write position.
        send_buffer[send_buffer_write_ptr] = buf[i];
        send_buffer_write_ptr = sub_mod(send_buffer_write_ptr + 1, sizeof(send_buffer));
    }

    // Increase the stored data length by the number of bytes copied.
    send_buffer_len += bytes_to_copy;
    total_written += bytes_to_copy; // Update total written bytes
    return bytes_sent + bytes_to_copy;
}

bool TwsRequest::write_fully(const char *buf) {
    size_t wrote = write(buf);
    if(wrote < strlen(buf)) {
        // Not all data was written, return false
        //DEBUG_PRINTF("TWS write_fully failed to write all data: %zu > %d\n", strlen(buf), wrote);
        finish();
        return false;
    }
    return true;
}

bool TwsRequest::write_fully(const char *buf, uint16_t len) {
    int wrote = write(buf, len);
    if(wrote < len) {
        // Not all data was written, return false
        //DEBUG_PRINTF("TWS write_fully failed to write all data: %d > %d\n", len, wrote);
        finish();
        return false;
    }
    return true;
}

uint32_t TwsRequest::write_direct(const char *buf, uint16_t len) {
    // Write data directly to the socket, bypassing the send buffer.
    if(send_buffer_len > 0) {
        // Can't bypass the send buffer if there is already data in it
        return 0;
    }
    if(socket==-1) return 0;
    const int bytes_written = ::send(socket, buf, len, 0);
    //DEBUG_PRINTF("  ::send write_direct returned %d\n", bytes_written);
    if(bytes_written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
        // An error occurred, close the connection
        DEBUG_PRINTF("TWS write_direct error: %d - %s\n", errno, strerror(errno));
        reset();
        return 0;
    } else if(bytes_written < 0) {
        // No data was written, return 0
        return 0;
    }
    total_written += bytes_written; // Update total written bytes
    pending_direct_write = true; // Make sure we listen for completion
    last_activity = millis();
    return bytes_written;
}

// int TwsRequest::read(char *buf, uint16_t len) {
//     if (recv_buffer_len <= 0) {
//         return 0; // No data available
//     }
//     int bytes_to_read = std::min(len, recv_buffer_len);
//     for (int i = 0; i < bytes_to_read; ++i) {
//         buf[i] = recv_buffer[recv_buffer_read_ptr];
//         recv_buffer_read_ptr = sub_mod(recv_buffer_read_ptr + 1, sizeof(recv_buffer));
//     }
//     recv_buffer_len -= bytes_to_read;
//     recv_buffer_scan_ptr = recv_buffer_read_ptr; // Reset scan pointer to the read position
//     recv_buffer_scan_len = 0; // Reset scan length
//     return bytes_to_read;
// }
// int TwsRequest::peek(char *buf, uint16_t len) {
//     int bytes_to_read = std::min(len, recv_buffer_len);
//     //int end_position = sub_mod(recv_buffer_read_ptr + bytes_to_read, sizeof(recv_buffer));
//     if(recv_buffer_read_ptr + bytes_to_read < sizeof(recv_buffer)) {
//         // Read doesn't wrap
//         memcpy(buf, &recv_buffer[recv_buffer_read_ptr], bytes_to_read);
//     } else {
//         // Read wraps, so copy in two parts
//         int first_part_len = sizeof(recv_buffer) - recv_buffer_read_ptr;
//         memcpy(buf, &recv_buffer[recv_buffer_read_ptr], first_part_len);
//         memcpy(&buf[first_part_len], recv_buffer, bytes_to_read - first_part_len);
//     }
//     return bytes_to_read;
// }
int TwsRequest::available() {
    return recv_buffer_len; // Return the number of bytes available to read
}
int TwsRequest::free() {
    return sizeof(send_buffer) - send_buffer_len; // Return the number of free bytes in the send buffer
}
bool TwsRequest::recv_buffer_full() {
    return recv_buffer_len >= sizeof(recv_buffer); // Return true if the receive buffer is full
}
int TwsRequest::read_flush(uint16_t len) {
    const uint16_t bytes_to_flush = min(len, recv_buffer_len);
    //DEBUG_PRINTF("TWS read_flush %d bytes\n", bytes_to_flush);
    recv_buffer_read_ptr = sub_mod(recv_buffer_read_ptr + bytes_to_flush, sizeof(recv_buffer));
    recv_buffer_len -= bytes_to_flush;
    
    if(recv_buffer_len == 0) {
        // Reset the pointers back to the start, to minimise wrapping
        recv_buffer_read_ptr = 0;
        recv_buffer_write_ptr = 0;
    }

    // Reset the scan pointer
    recv_buffer_scan_ptr = recv_buffer_read_ptr;
    recv_buffer_scan_len = 0;

    return bytes_to_flush;
}

int TwsRequest::scan(char delim1) {
    // Keep looping until we catch up with the write pointer
    while(recv_buffer_scan_len<recv_buffer_len) {
        if (recv_buffer[recv_buffer_scan_ptr] == delim1) {
            // Skip over the delimiter
            recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
            int count = recv_buffer_scan_len+1;
            recv_buffer_scan_len = 0;
            // Return the length including the delimiter
            return count;
        }
        recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
        recv_buffer_scan_len++;
    }
    return 0; // No delimiter found, return 0
}
int TwsRequest::scan(char delim1, char delim2) {
    // Keep looping until we catch up with the write pointer
    while(recv_buffer_scan_len<recv_buffer_len) {
        if (recv_buffer[recv_buffer_scan_ptr] == delim1 || recv_buffer[recv_buffer_scan_ptr] == delim2) {
            // Skip over the delimiter
            recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
            int count = recv_buffer_scan_len+1;
            recv_buffer_scan_len = 0;
            // Return the length including the delimiter
            return count;
        }
        recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
        recv_buffer_scan_len++;
    }
    return 0; // No delimiter found, return 0
}
int TwsRequest::scan(char delim1, char delim2, char delim3) {
    // Keep looping until we catch up with the write pointer
    while(recv_buffer_scan_len<recv_buffer_len) {//recv_buffer_scan_ptr != recv_buffer_write_ptr) {
        if (recv_buffer[recv_buffer_scan_ptr] == delim1 || recv_buffer[recv_buffer_scan_ptr] == delim2 || recv_buffer[recv_buffer_scan_ptr] == delim3) {
            // Skip over the delimiter
            recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
            int count = recv_buffer_scan_len+1;
            recv_buffer_scan_len = 0;
            // Return the length including the delimiter
            return count;
        }
        recv_buffer_scan_ptr = sub_mod(recv_buffer_scan_ptr + 1, sizeof(recv_buffer));
        recv_buffer_scan_len++;
    }
    return 0; // No delimiter found, return 0
}
int TwsRequest::available_contiguous() {
    // Get the number of bytes available to read in the receive buffer
    // without wrapping around.
    return min((unsigned int)recv_buffer_len, (unsigned int)(sizeof(recv_buffer) - recv_buffer_read_ptr));
}
char* TwsRequest::get_read_ptr(char *buf, uint16_t len) {
    // Get a pointer to the read position in the receive buffer.
    // If the full read would wrap, copy into the supplied buffer and return
    // that instead.
    char* ret = get_peek_ptr(buf, len);
    read_flush(len); // Update the read pointer
    return ret;

    // if(recv_buffer_read_ptr + len <= sizeof(recv_buffer)) {
    //     // Read doesn't wrap
    //     char* ret = &recv_buffer[recv_buffer_read_ptr];
    //     read_flush(len);
    //     return ret;
    // } else {
    //     if(buf == nullptr) {
    //         return nullptr;
    //     }
    //     read(buf, len);
    //     return buf;
    // }
}
char* TwsRequest::get_peek_ptr(char *buf, uint16_t len) {
    // Get a pointer to the read position in the receive buffer.
    // If the full read would wrap, copy into the supplied buffer and return
    // that instead.
    // Don't update the read pointer, so we can peek at the data.

    if(recv_buffer_read_ptr + len <= sizeof(recv_buffer)) {
        // Read doesn't wrap
        return &recv_buffer[recv_buffer_read_ptr];
    } else {
        if(buf == nullptr) {
            return nullptr;
        }
        memcpy(buf, &recv_buffer[recv_buffer_read_ptr], sizeof(recv_buffer) - recv_buffer_read_ptr);
        memcpy(&buf[sizeof(recv_buffer) - recv_buffer_read_ptr], recv_buffer, len - (sizeof(recv_buffer) - recv_buffer_read_ptr));
        return buf;
    }
}
bool TwsRequest::match(const char *str) {
    int i;
    for(i = 0; str[i] != '\0'; ++i) {
        if (recv_buffer_len <= i || recv_buffer[sub_mod(recv_buffer_read_ptr + i, sizeof(recv_buffer))] != str[i]) {
            return false; // Not a match
        }
    }
    // Consume the matched bytes
    recv_buffer_read_ptr = sub_mod(recv_buffer_read_ptr + i, sizeof(recv_buffer));
    recv_buffer_len -= i;
    return true; // Match found
}
bool TwsRequest::match(const char *str, uint16_t len) {
    if(recv_buffer_len < len) {
        return false;
    }
    // Check if the next 'len' bytes match the given string
    for (int i = 0; i < len; ++i) {
        if (recv_buffer[sub_mod(recv_buffer_read_ptr + i, sizeof(recv_buffer))] != str[i]) {
            return false;
        }
    }
    // Consume the matched bytes
    recv_buffer_read_ptr = sub_mod(recv_buffer_read_ptr + len, sizeof(recv_buffer));
    recv_buffer_len -= len;
    return true;
}

void TwsRequest::send(int code, const char *content_type, const char *content) {
    char buf[] = {'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '0', '0', '0', ' ', 'u', '\r', '\n'};

    code = code % 1000;
    buf[9] = code / 100 + '0';
    code = code % 100;
    buf[10] = code / 10 + '0';
    code = code % 10;
    buf[11] = code + '0';
    write_fully(buf, 16); // Write the HTTP status line

    if(content_type && content_type[0] != '\0') {
        write_fully("Content-Type: ");
        write_fully(content_type);
        write_fully("\r\n");
    }
    // write_fully("Content-Length: ");
    // if(content && content[0] != '\0') {
    //     write_fully(content);
    // } else {
    //     write_fully("0");
    // }
    if(content != nullptr) {
        write_fully("\r\n");
        write_fully(content);
    }
}

void TwsRequest::abort() {
    finish();
}

void TwsRequest::finish() {
    done = true; // Mark the request as done
}

/*
#ifndef LOCAL


// class MyString : public String {
//     // subclass that prints everytime it is copied
// public:
//     MyString() : String() {}
//     MyString(const String &other) : String(other) {
//         DEBUG_PRINTF("MyString copied!\n");
//     }
//     MyString(MyString &&other) noexcept : String(std::move(other)) {
//         DEBUG_PRINTF("MyString moved!\n");
//     }
// };
//typedef String MyString;



void idle_loop(void * pData) {
    while(true) {
        // for(uint32_t i=0;i<100000000;i++) {
        //     asm volatile("nop");
        // }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Adjust the delay as needed

        if(false) {
            // loop through each running task

            auto uxArraySize = uxTaskGetNumberOfTasks();

            // Allocate a TaskStatus_t structure for each task.
            TaskStatus_t *pxTaskStatusArray = (TaskStatus_t*)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
            uint32_t ulTotalRunTime;

            if( pxTaskStatusArray != NULL )
            {
                // Generate the array of TaskStatus_t structures.
                uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

                DEBUG_PRINTF("----------------------------------------------------------------------\n");
                DEBUG_PRINTF("--- Task Monitor --- Total Tasks: %lu ---\n", uxArraySize);
                DEBUG_PRINTF("----------------------------------------------------------------------\n");
                DEBUG_PRINTF("Task Name\t\tStack Base\tStack End\tHigh Water Mark\n");
                DEBUG_PRINTF("----------------------------------------------------------------------\n");


                // Loop through the array of tasks and print the details for each.
                for( int x = 0; x < uxArraySize; x++ )
                {
                    DEBUG_PRINTF( "%-16s\t0x%lX\t0x%lX\t%u\n",
                        pxTaskStatusArray[ x ].pcTaskName,
                        (uint32_t)pxTaskStatusArray[ x ].pxStackBase,
                        0,//(uint32_t)pxTaskStatusArray[ x ].pxEndOfStack,
                        pxTaskStatusArray[ x ].usStackHighWaterMark );
                }
                DEBUG_PRINTF("----------------------------------------------------------------------\n\n");
            }

            // Free the memory allocated for the task status array.
            vPortFree( pxTaskStatusArray );      
        }  

        if(true) {
            char pcWriteBuffer[1500];
            vTaskList(pcWriteBuffer);
            char *ptr = pcWriteBuffer;
            // print a line at a time
            DEBUG_PRINTF("Idle loop:\n");

            char *line_end = strchr(ptr, '\n');
            while(line_end != nullptr) {
                *line_end = '\0'; // Null-terminate the line
                DEBUG_PRINTF("%s\n", ptr);
                ptr = line_end + 1; // Move to the next line
                line_end = strchr(ptr, '\n'); // Find the next line end
            }
        }
    }
}


#endif
*/