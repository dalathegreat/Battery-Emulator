#include "TinyWebServer.h"
#include "webserver_utils.h"
#include <src/datalayer/datalayer.h>

extern TinyWebServer tinyWebServer;
int can_dumper_connection_id = 0;

static const char *hex = "0123456789abcdef";

char* IRAM_ATTR put_hex(char *ptr, uint32_t value, uint8_t digits) {
    for(int i=digits-1;i>=0;i--) {
        *ptr++ = hex[(value >> (i*4)) & 0x0f];
    }
    return ptr;
}

char* IRAM_ATTR put_time(char *ptr, unsigned long time) {
    // Wrap around after 100000 seconds (about 27.7 hours)
    if(time >= 100000000) time = time % 100000000;

    // Seconds part
    // bool started = false;
    // if(time >= 10000000 || started) { *ptr++ = (time / 10000000) + '0'; time %= 10000000; started = true; }
    // if(time >= 1000000 || started) { *ptr++ = (time / 1000000) + '0'; time %= 1000000; started = true; }
    // if(time >= 100000 || started) { *ptr++ = (time / 100000) + '0'; time %= 100000; started = true; }
    // if(time >= 10000 || started) { *ptr++ = (time / 10000) + '0'; time %= 10000; started = true; }
    // *ptr++ = (time / 1000) + '0'; time %= 1000;
    
    // Milliseconds part
    // *ptr++ = '.';
    // *ptr++ = (time / 100) + '0'; time %= 100;
    // *ptr++ = (time / 10) + '0'; time %= 10;
    // *ptr++ = (time) + '0';
    
    char buf[8];
    int i = 0;
    do {
        buf[i++] = (time % 10) + '0';
        time /= 10;
    } while (time > 0);
    while (i > 0) {
        *ptr++ = buf[--i];
        if(i == 3) {
            *ptr++ = '.';
        }
    }
    return ptr;
}

void IRAM_ATTR dump_can_frame2(const CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
    if (!datalayer.system.info.can_logging_active2) {
        return;
    }

    const int required_space = 29 + frame.DLC*3 + 20;
    if(tinyWebServer.free(can_dumper_connection_id) < required_space) {
        tinyWebServer.write(can_dumper_connection_id, "\n", 1);
        return;
    }
    char line[230];
    char *ptr = line;
    unsigned long currentTime = millis();
    *ptr++ = '('; ptr = put_time(ptr, currentTime); *ptr++ = ')';
    *ptr++ = ' ';
    if(msgDir == MSG_RX) { *ptr++ = 'R'; *ptr++ = 'X'; *ptr++ = '0' + ((int)interface * 2); }
    else { *ptr++ = 'T'; *ptr++ = 'X'; *ptr++ = '1' + ((int)interface * 2); }
    *ptr++ = ' ';
    if(frame.ext_ID) ptr = put_hex(ptr, frame.ID, 8);
    else ptr = put_hex(ptr, frame.ID, 3);
    *ptr++ = ' '; *ptr++ = '[';
    if(frame.DLC > 9) { *ptr++ = '0' + (frame.DLC / 10); *ptr++ = '0' + (frame.DLC % 10); }
    else *ptr++ = '0' + (frame.DLC);
    *ptr++ = ']';
    for(int i=0;i<frame.DLC;i++) { *ptr++ = ' '; ptr = put_hex(ptr, frame.data.u8[i], 2); }
    *ptr++ = '\n';
    if(tinyWebServer.write(can_dumper_connection_id, line, ptr - line) < 0) {
        datalayer.system.info.can_logging_active2 = false;
    }
}

TwsRoute logRoute("/api/log", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
    request.set_writer_callback(CharBufWriter((const char*)datalayer.system.info.logged_can_messages, sizeof(datalayer.system.info.logged_can_messages)));
}));

TwsRoute dumpCanRoute("/dump_can", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n");
    if(can_dumper_connection_id) tinyWebServer.finish(can_dumper_connection_id);
    can_dumper_connection_id = request.get_connection_id();
    datalayer.system.info.can_logging_active2 = true;
}));
