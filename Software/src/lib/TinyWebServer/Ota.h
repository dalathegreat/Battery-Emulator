#ifndef OTA_H
#define OTA_H

#include "TwsHandler.h"

typedef struct {
  int mode;
} OtaStartState;

class OtaStart : public TwsStatefulMiddleware<OtaStartState> {
public:
    OtaStart() = default;

    void handleRequest(TwsRequest &request) override;
    void handleQueryParam(TwsRequest &request, const char *param, int len, bool final) override;
    
    TwsQueryParamHandler *nextQueryParam = nullptr;
};

typedef struct {
    int content_length;
    bool error = false;
} OtaUploadState;

class OtaUpload : public TwsStatefulMiddleware<OtaUploadState> {
public:
    OtaUpload() = default;

    void handleRequest(TwsRequest &request) override;
    void handleHeader(TwsRequest &request, const char *line, int len) override;
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
};

#endif // OTA_H
