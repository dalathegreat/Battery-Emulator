#ifndef MULTIPART_UPLOAD_HANDLER_H
#define MULTIPART_UPLOAD_HANDLER_H

#include "TwsHandler.h"

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

class MultipartUploadHandler : public TwsStatefulMiddleware<MultipartUploadHandlerState> {
public:
    MultipartUploadHandler(TwsFileUploadHandler *onUpload = nullptr);
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleHeader(TwsRequest &request, const char *line, int len) override;

    TwsFileUploadHandler *onUpload = nullptr;
};

#endif // MULTIPART_UPLOAD_HANDLER_H
