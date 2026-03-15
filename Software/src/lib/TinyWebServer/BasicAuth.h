#ifndef BASIC_AUTH_H
#define BASIC_AUTH_H

#include "TwsHandler.h"

typedef struct {
    bool authed;
} BasicAuthState;

class BasicAuth : public TwsStatefulMiddleware<BasicAuthState> {
public:
    BasicAuth() = default;

    void handleHeader(TwsRequest &request, const char *line, int len) override;
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleRequest(TwsRequest &request) override;
private:
    bool denyIfUnauthed(TwsRequest &request);
};

#endif // BASIC_AUTH_H
