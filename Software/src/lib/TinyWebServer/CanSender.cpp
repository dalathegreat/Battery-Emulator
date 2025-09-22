#include "TinyWebServer.h"

//#include "../../communication/can/comm_can.h"
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "../../lib/pierremolinaro-acan-esp32/ACAN_ESP32.h"
#include "../../lib/pierremolinaro-acan2515/ACAN2515.h"

CanSender::CanSender(TwsHandler *handler) : TwsStatefulHandler<CanSenderState>(handler) {
    handler->onHeader = this;
    handler->onQueryParam = this;
    handler->onPostBody = this;
}

void CanSender::handleHeader(TwsRequest &request, const char *line, int len) {
    auto &state = get_state(request);

    if(strncasecmp(line, "Content-Length:", 15) == 0) {
        char *endptr;
        int content_length = strtol(line + 15, &endptr, 10);
        if (endptr != line + 15 && content_length > 0) {
            state.content_length = content_length;
        }
    }
}

struct __attribute__((packed)) CanFrame {
    uint32_t timestamp;
    uint32_t id;
    uint8_t len;
    uint8_t data[64];
};

extern ACAN2515* can2515;
extern ACAN2517FD* canfd;

bool can_buffer_full(CAN_Interface interface) {
    if(interface == CAN_NATIVE) {
        return ACAN_ESP32::can.driverTransmitBufferCount() >= ACAN_ESP32::can.driverTransmitBufferSize();
    } else if(interface == CAN_ADDON_MCP2515) {
        // We always use buffer 0?
        return can2515 && can2515->transmitBufferCount(0) >= can2515->transmitBufferSize(0);
    } else if(interface == CANFD_ADDON_MCP2518) {
        return canfd && canfd->driverTransmitBufferCount() >= canfd->driverTransmitBufferSize();
    }
    return false;
}

bool send_can_frame(CAN_Interface interface, const CANMessage &frame) {
    // This can be slow as the first call will (for the SPI ones) perform a
    // sequence of blocking SPI transactions. Subsequent calls will be faster as
    // the frames will be buffered instead.
    //
    // (it is not ideal having this called directly from the webserver thread,
    // but the alternative involves queues and locking)

    if(interface == CAN_NATIVE) {
        return ACAN_ESP32::can.tryToSend(frame);
    } else if(interface == CAN_ADDON_MCP2515) {
        return can2515 && can2515->tryToSend(frame);
    } else if(interface == CANFD_ADDON_MCP2518) {
        return canfd && canfd->tryToSend(frame);
    }
    return false;
}

void CanSender::handleQueryParam(TwsRequest &request, const char *param, int len, bool final) {
    auto &state = get_state(request);
    // if param starts with if=
    if(strncmp(param, "if=", 3) == 0) {
        state.can_interface = atoi(param + 3);
    }
}

int CanSender::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    if(len < 9) {
        // Not enough data yet
        return 0;
    }

    auto &state = get_state(request);

    if(index==0) {
        state.start_millis = millis();
    }

    uint8_t *ptr = data;
    size_t remaining = len;
    do {
        CanFrame *frame = (CanFrame*)ptr;
        int frame_length = 9 + frame->len;
        if(remaining < frame_length) {
            // Not enough data yet
            break;
        }
        // if(frame->timestamp == 0xffffffff) {
        //     // special case, frame is just padding
        //     ptr += frame_length;
        //     remaining -= frame_length;
        //     continue;
        // }
        if(frame->len > 64) {
            // Invalid length
            request.write_fully("HTTP/1.1 400 b\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nInvalid CAN frame length.\n");
            request.finish();
            return -1; // finished
        }
        if((millis() - state.start_millis) < frame->timestamp) {
            // Need to wait
            break;
        }
        //DEBUG_PRINTF("CAN interface: %d, buffer full: %d\n", state.can_interface, can_buffer_full((CAN_Interface)state.can_interface));
        if(can_buffer_full((CAN_Interface)state.can_interface)) {
            break;
        }

        DEBUG_PRINTF("CAN frame: timestamp %u id 0x%X len %d data:", frame->timestamp, frame->id, frame->len);
        for(int i=0; i<frame->len; i++) {
            DEBUG_PRINTF(" %02X", frame->data[i]);
        }
        DEBUG_PRINTF("\n");

        CANMessage send_frame = {
            .id = frame->id,
            .ext = frame->id > 0x7FF,
            .len = frame->len,
        };
        memcpy(send_frame.data, frame->data, frame->len);

        if(!send_can_frame((CAN_Interface)state.can_interface, send_frame)) {
            // Failed to send
            request.write_fully("HTTP/1.1 500 e\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\nFailed to send CAN frame.\n");
            request.finish();
            return -1; // finished
        }

        ptr += frame_length;
        remaining -= frame_length;
    } while(remaining >= 9);

    if(index + len >= state.content_length) {
        DEBUG_PRINTF("End of upload (content length %d reached at %d)\n", state.content_length, index + len);

        // Finished uploading
        request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\nOK\n");
        request.finish();
        return -1; // finished
    }

    return len - remaining;
}