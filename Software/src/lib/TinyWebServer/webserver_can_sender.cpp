#include "CanSender.h"
#include "TinyWebServer.h"

TwsRoute canSenderRoute = TwsRoute("/api/cansend", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                            "\r\n");
    })
).use(*new CanSender());
