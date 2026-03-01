#include "TinyWebServer.h"

MultipartUploadHandler::MultipartUploadHandler(TwsRoute *handler, 
    TwsFileUploadHandler *onUpload = nullptr) : TwsStatefulHandler<MultipartUploadHandlerState>(handler), onUpload(onUpload) {
    
    nextPostBody = handler->onPostBody;
    nextHeader = handler->onHeader;

    handler->onPostBody = this;
    handler->onHeader = this;
}

int MultipartUploadHandler::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    auto &state = get_state(request);
    //printf("\n\n\n\n\nHandling post body: index: %zu, len: %zu [%.*s]\n", index, len, (int)len, data);

    // if(state.file == nullptr) {
    //     // Open file for writing
    //     state.file = fopen("/tmp/upload.bin", "wb");
    //     state.file2 = fopen("/tmp/raw.bin", "wb");
    // }

    // fwrite("AAAAAA", 1, 6, state.file2);
    // fwrite(data, 1, len, state.file2);
    // fwrite("XXXXXX", 1, 6, state.file2);

    const char *key = "file"; // Default key for single file uploads
    const char *filename = "upload.bin"; // Default filename

    for(size_t i = 0; i < len; i++) {
        //printf("%ld %02x %ld %d %d\n", index+i, data[i], (index+i) - state.file_start, state.part_header_end_index, state.boundary_index);

        if(state.part_header_end_index>-1) {
            //printf("Checking data[%zu] = %02x, part_header_end_index: %d\n", i, data[i], part_header_end_index);
            if(state.part_header_end_index==0 && data[i] == '\r') state.part_header_end_index++;
            else if(state.part_header_end_index==1 && data[i] == '\n') state.part_header_end_index++;
            else if(state.part_header_end_index==2 && data[i] == '\r') state.part_header_end_index++;
            else if(state.part_header_end_index==3 && data[i] == '\n') {
                // We have reached the end of the part header
                state.part_header_end_index = -1;
                state.file_start = index + i + 1; // Start of the file data after the header
                //printf("Part header ended at index %zu, file starts at %d\n", index + i, state.file_start);
            } else {
                // Reset if we don't match the expected end of header
                state.part_header_end_index = 0;
            }
        }
        //part_header_end_index==-1 && 
        if(state.boundary_length > 0) {
            //printf("Comparing data[%zu] = %02x with boundary[%d] = %02x\n", i, data[i], boundary_index, boundary[boundary_index]);
            if(data[i] == state.boundary[state.boundary_index]) {
                state.boundary_index++;
                if(state.boundary_index >= state.boundary_length) {
                    // There was an active before the boundary started
                    if(state.file_start > -1) {
                        if(onUpload) {
                            if(state.file_start <= (int)index) {
                                // send the whole chunk
                                //printf("final chunk, file_start is %d, index is %zu, len is %zu, i is %ld, boundary_index is %d\n", state.file_start, index, len, i, state.boundary_index);
                                //fwrite("AAAAAA", 1, 6, file); // Debug marker
                                // unsigned -> signed
                                if(i > (size_t)(state.boundary_index+1)) {
                                    //printf("1 writing [%d:%d] %.*s\n", index, index + i - state.boundary_index + 1, i - state.boundary_index + 1, data);
                                    //printf("1 %d\n", i - state.boundary_index + 1);
                                    //fwrite(data, 1, i - state.boundary_index + 1, state.file);
                                    onUpload->handleUpload(request, key, filename, index - state.file_start, data, i - state.boundary_index + 1, true);
                                } else {
                                    // send a zero-byte final chunk
                                    onUpload->handleUpload(request, key, filename, index - state.file_start - (state.boundary_index - i) + 1, (uint8_t*)"", 0, true);
                                }
                                //fwrite("XXXXXX", 1, 6, file); // Debug marker
                            } else {
                                // the file started mid-chunk
                                //printf("i is %ld\n", i);
                                //printf("file_start - index is %ld\n", state.file_start - index);
                                //printf("boundary_index is %d\n", state.boundary_index);

                                int chunk_len = i - (state.file_start - index) - state.boundary_index + 1;

                                //printf("File starts mid-chunk, from %ld to %ld (len %d): %.*s\n", state.file_start - index, i, chunk_len, chunk_len, data + state.file_start - index);
                                    //[%.*s]\n",

                                //fwrite("AAAAAA", 1, 6, file); // Debug marker
                                //printf("2 writing [%d:%d] %.*s\n", state.file_start, state.file_start + chunk_len, chunk_len, data + state.file_start - index);
                                //fwrite(data + state.file_start - index, 1, chunk_len, state.file);
                                onUpload->handleUpload(request, key, filename, 0, data + state.file_start - index, chunk_len, true);
                                //fwrite("XXXXXX", 1, 6, file); // Debug marker
                            }
                        }
                        state.file_start = -1;
                    }

                    state.part_header_end_index = 0;
                    state.boundary_index = 0;
                    //printf("Boundary found at index %zu\n", index + i);
                }
            } else if(state.boundary_index>i && state.file_start > -1) {
                // We were already in a boundary at the start of the buffer
                // - so there will have been data missed off the end of the
                // previous buffer. Send it now.

                //printf("3 writing %.*s\n", state.boundary_index - i, state.boundary);
                //printf("3\n");
                //fwrite(state.boundary, 1, state.boundary_index-i, state.file);
                onUpload->handleUpload(request, key, filename, index - state.file_start - (state.boundary_index-i), (uint8_t*)state.boundary, state.boundary_index - i, false);
                state.boundary_index = 0;
            } else {
                state.boundary_index = 0; // Reset if mismatch
            }
        }
    }

    if(state.file_start>-1) {
        if(onUpload) {
            if(state.file_start <= (int)index) {
                // send the whole chunk
                //fwrite("AAAAAA", 1, 6, file); // Debug marker
                //printf("4 writing [%d:%d] %.*s\n", index, index + len - state.boundary_index, len - state.boundary_index, data);
                //printf("4\n");
                //fwrite(data, 1, len - state.boundary_index, state.file);
                //fwrite("XXXXXX", 1, 6, file); // Debug marker
                onUpload->handleUpload(request, key, filename, index - state.file_start, data, len - state.boundary_index, false);
            } else {
                // the file starts mid-chunk
                //fwrite("AAAAAA", 1, 6, file); // Debug marker
                int start_offset = state.file_start - index;
                int len_to_write = len - (state.file_start - index) - state.boundary_index;
                //printf("5 writing [%d:%d] %.*s\n", index+start_offset, index+start_offset+len_to_write, len_to_write, data + start_offset);
                //printf("5\n");
                //fwrite(data + start_offset, 1, len_to_write, state.file);
                //fwrite("XXXXXX", 1, 6, file); // Debug marker
                onUpload->handleUpload(request, key, filename, 0, data + start_offset, len_to_write, false);
            }


            //onUpload->handleUpload(request, key, filename, index - file_start, data, len, false);
        }
    }

    if(nextPostBody) {
        nextPostBody->handlePostBody(request, index, data, len);
    }

    if(index+len >= state.content_length) {
        // Finished uploading
        // fclose(state.file);
        // fclose(state.file2);
        // state.file = nullptr;
        // state.file2 = nullptr;
        //printf("Upload finished, content length was: %d %d\n", content_length, index+len);
        return -1; // Indicate that the upload is complete
    }        
    return len; // we consumed it all
}
void MultipartUploadHandler::handleHeader(TwsRequest &request, const char *line, int len) {
    auto &state = get_state(request);

    if(strncasecmp(line, "Content-Length:", 15) == 0) {
        char *endptr;
        int content_length = strtol(line + 15, &endptr, 10);
        if (endptr != line + 15 && content_length > 0) {
            state.content_length = content_length;
        }
    } else if(strncasecmp(line, "Content-Type:", 13) == 0) {
        // Check for multipart boundary
        char *boundary = strstr((char*)line, "boundary=");
        if (boundary) {
            boundary += 9; // Skip "boundary="
            //int end = len;
            const char *end = line + len - 1;
            if(*end == ';') end--;
            size_t blen = end - boundary;
            if(blen > sizeof(state.boundary) - 8) {
                //printf("Boundary too long: %d, max is %zu\n", blen, sizeof(state.boundary) - 8);
            } else {
                state.boundary[0] = '\r';
                state.boundary[1] = '\n';
                state.boundary[2] = '-';
                state.boundary[3] = '-';
                strncpy(state.boundary + 4, boundary, blen+1);
                state.boundary[blen + 5] = '\0';
                state.boundary_length = blen + 5;
                state.file_start = -1; // Reset file start index
                state.part_header_end_index = -1;
                // start after the first \r\n which will have already been consumed
                state.boundary_index = 2;
                //printf("Multipart boundary set: [%s] %c\n", state.boundary, state.boundary[0]);
            }
        }
    }

    if(nextHeader) {
        nextHeader->handleHeader(request, line, len);
    }
}
