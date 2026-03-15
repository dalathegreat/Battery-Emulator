#ifndef CAN_SENDER_H
#define CAN_SENDER_H

#include "TwsHandler.h"

typedef struct {
    uint32_t content_length;
    uint32_t can_interface;
    uint32_t start_millis;
    //bool error = false;
    // Buffer incomplete lines
    //char buf[128];
} CanSenderState;

class CanSender : public TwsStatefulMiddleware<CanSenderState>, public TwsQueryParamHandler {
public:
    CanSender() = default;

    void handleHeader(TwsRequest &request, const char *line, int len) override;
    void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) override;
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;

    TwsQueryParamHandler *nextQueryParam = nullptr;
};

#endif // CAN_SENDER_H
