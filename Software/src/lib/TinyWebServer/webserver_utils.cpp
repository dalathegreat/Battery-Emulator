#include "webserver_utils.h"

const char* HTTP_RESPONSE = "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "\r\n";
const char* HTTP_204 = "HTTP/1.1 204 n\r\n"
                       "Connection: close\r\n"
                       "\r\n";
const char *HTTP_405 = "HTTP/1.1 405 x\r\n"
                       "Connection: close\r\n"
                       "\r\n";

TwsRequestWriterCallbackFunction StringWriter(std::shared_ptr<String> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        const int remaining = response->length() - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); 
            return;
        }
        req.write_direct(response->c_str() + alreadyWritten, remaining);
    };
}

TwsRequestWriterCallbackFunction StringListWriter(std::shared_ptr<std::vector<StringLike>> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        for(auto &str : *response) {
            int length = str.length();
            if(alreadyWritten >= length) {
                alreadyWritten -= length;
                continue; 
            }

            const int remaining = length - alreadyWritten;
            if(req.write_direct(str.c_str() + alreadyWritten, remaining) < remaining) {
                return;
            }
            alreadyWritten = 0; 
        }  

        req.finish();
    };
}

TwsRequestWriterCallbackFunction CharBufWriter(const char* buf, int len) {
    return [buf, len](TwsRequest &req, int alreadyWritten) {
        const int remaining = len - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); 
            return;
        }
        req.write_direct(buf + alreadyWritten, remaining);
    };
}

void TwsJsonGetFunc::handleRequest(TwsRequest &request) {
    JsonDocument doc;
    respond(request, doc);
    auto response = std::make_shared<String>();
    serializeJson(doc, *response);
    request.write_fully("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "\r\n");
    request.set_writer_callback(StringWriter(response));
}
