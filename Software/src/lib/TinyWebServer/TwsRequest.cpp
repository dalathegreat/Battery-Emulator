#include "TinyWebServer.h"

extern "C" {
    #ifdef LOCAL
    #include <sys/socket.h>
    #include <errno.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #else
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include "lwip/sockets.h"
    #endif
}

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void TwsRequest::reset() {
    // Reset a request ready to be reused for a new connection.
    // Don't call this during request handling, use abort() instead.

    // Get the send_buffer_lock here to avoid external writers overwriting fresh
    // connections
    while(send_buffer_lock.test_and_set(std::memory_order_acquire)) {
        vTaskDelay(1);
    }

    if(socket>-1) {
        close(socket);
    }
    socket = -1;
    send_buffer_write_ptr.store(0, std::memory_order_release);
    send_buffer_read_ptr.store(0, std::memory_order_release);
    recv_buffer_len = 0;
    recv_buffer_write_ptr = 0;
    recv_buffer_read_ptr = 0;
    recv_buffer_scan_len = 0;
    recv_buffer_scan_ptr = 0;
    parse_state = TWS_AWAITING_METHOD;
    done = false;
    aborted = false;
    content_length = -1;
    total_written = 0;
    body_read = 0;
    pending_direct_write = false;
    writer_callback = nullptr;
    writer_callback_written_offset = 0;
    if(handler && handler->onAlloc) {
        handler->onAlloc->handleFree(*this);
    }
    path_wildcard[0] = '\0';
    handler = nullptr;
    connection_id = 0;

    send_buffer_lock.clear(std::memory_order_release);
}

void TwsRequest::perform_io() {
    // send any data awaiting sending
    int current_len = send_buffer_len();
    while(current_len > 0) {
        const uint16_t r = send_buffer_read_ptr.load(std::memory_order_acquire);
        const uint16_t contiguous_block_size = sizeof(send_buffer) - r;
        const uint16_t bytes_to_send = min(current_len, contiguous_block_size);
        const int bytes_sent = ::send(socket, &send_buffer[r], bytes_to_send, 0);
        if (bytes_sent > 0) {
            // Advance the read pointer, wrapping around the buffer if necessary.
            send_buffer_read_ptr.store(sub_mod(r + bytes_sent, sizeof(send_buffer)), std::memory_order_release);
            current_len -= bytes_sent;
            last_activity = millis();
        } else if (bytes_sent <= 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            // An error occurred, close the connection
            DEBUG_PRINTF("TWS send error: %d - %s\n", errno, strerror(errno));
            abort();
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
            abort();
            break;
        } else if (bytes_received < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            // An error occurred, close the connection
            DEBUG_PRINTF("TWS recv error on %d: %d - %s\n", slot_id, errno, strerror(errno));
            abort();
            break;
        } else {
            // No more data available to read
            break;
        }
    }
    return total_read;
}

void TwsRequest::tick() {
    if((send_buffer_len()==0 && done && !pending_direct_write) || aborted) {
        // If the send buffer is empty and the request is done, close the connection
        // (or if aborted, finish immediately)
        reset();
        return;
    }

    if(send_buffer_len()==0 && !done && writer_callback) {
        // If the send buffer is empty and a callback is set, call it.
        writer_callback(*this, this->total_written - this->writer_callback_written_offset);
    }

    if ((millis() - last_activity) > TinyWebServer::IDLE_TIMEOUT_MS) {
        // No activity for 10 seconds, close the connection
        DEBUG_PRINTF("TWS client timeout, closing connection %d\n", connection_id);
        reset();
        return;
    }
}

uint32_t TwsRequest::write(const char* buf, uint16_t len) {
    int bytes_sent = 0;
    if(send_buffer_len()==0) {
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

    // Send the rest indirectly
    return bytes_sent + write_indirect(buf, len);
}


// uint32_t IRAM_ATTR TwsRequest::write(const char *buf) {
//     // TODO: worth calling strlen and seeing if we can do a direct write?

//     const uint16_t available_space = (sizeof(send_buffer) - 1) - send_buffer_len();
//     if (available_space == 0) {
//         return 0;
//     }
    
//     int i;
//     for(i = 0; buf[i] != '\0' && i < available_space; ++i) {
//         // Place the byte at the current write position.
//         send_buffer[send_buffer_write_ptr] = buf[i];
//         send_buffer_write_ptr = sub_mod(send_buffer_write_ptr + 1, sizeof(send_buffer));
//     }

//     total_written += i; // Update total written bytes

//     // Return the number of bytes written
//     return i;
// }


uint32_t IRAM_ATTR TwsRequest::write_indirect(const char *buf, uint16_t len) {
    // We can only write up to (buffer size - 1) bytes to distinguish full
    // (write_ptr one behind read_ptr) vs empty (write_ptr == read_ptr).
    const uint16_t available_space = free();
    if (available_space == 0) {
        return 0;
    }
    const uint16_t bytes_to_copy = min(len, available_space);

    uint32_t w = send_buffer_write_ptr.load(std::memory_order_acquire);
    for (uint16_t i = 0; i < bytes_to_copy; ++i) {
        // Place the byte at the current write position.
        send_buffer[w] = buf[i];
        w = sub_mod(w + 1, sizeof(send_buffer));
    }
    send_buffer_write_ptr.store(w, std::memory_order_release);

    total_written += bytes_to_copy; // Update total written bytes
    return bytes_to_copy;    
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
    if(send_buffer_len() > 0) {
        // Can't bypass the send buffer if there is already data in it
        return 0;
    }
    if(socket==-1) return 0;
    const int bytes_written = ::send(socket, buf, len, 0);
    //DEBUG_PRINTF("  ::send write_direct returned %d\n", bytes_written);
    if(bytes_written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
        // An error occurred, close the connection
        DEBUG_PRINTF("TWS write_direct error: %d - %s\n", errno, strerror(errno));
        abort();
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

bool TwsRequest::recv_buffer_full() {
    return recv_buffer_len >= sizeof(recv_buffer); // Return true if the receive buffer is full
}

int TwsRequest::read(char *buf, uint16_t len) {
    if (len == 0 || recv_buffer_len == 0) return 0;
    uint16_t to_read = min(len, (uint16_t)recv_buffer_len);
    char* ptr = get_peek_ptr(buf, to_read);
    // Did get_peek_ptr have to copy the data to a temporary buffer due to
    // wrapping? If so, it will have returned the supplied buf pointer. If not,
    // it will have returned a pointer directly into the receive buffer, which
    // we must now copy from.
    if (ptr != buf) {
        memcpy(buf, ptr, to_read);
    }
    read_flush(to_read);
    return to_read;
}

int TwsRequest::read_flush(uint16_t len) {
    // Discard up to len bytes from the read buffer.

    const uint16_t bytes_to_flush = min(len, recv_buffer_len);
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
    // Scan through the receive buffer for the given delimiter, starting from
    // the current scan pointer, and return the length up to and including the
    // delimiter if found, or 0 if not found.

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
    // Like scan(char), but matching either of two delimiters.

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
    // Like scan(char), but matching any of three delimiters.

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
    // Check if the next bytes in the receive buffer match the given string, and
    // if so, consume them and return true. Otherwise, return false and don't
    // consume anything.
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
    // Abort the request, which will close the connection. Any unsent data will
    // be discarded.
    aborted = true;
}

void TwsRequest::finish() {
    // Finish the request, which will close the connection after all pending
    // data has been sent.
    done = true;
    last_activity = millis(); // Avoid timing out before the connection is closed
}
