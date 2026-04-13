#include "TinyWebServer.h"
#include "TwsBuffering.h"
#include "BasicAuth.h"
#include "CanSender.h"
#include "DigestAuth.h"
#include "Ota.h"
#include "webserver_utils.h"

#include <src/battery/BATTERIES.h>
#include <src/charger/CanCharger.h>
#include <src/communication/equipmentstopbutton/comm_equipmentstopbutton.h>
#include <src/datalayer/datalayer.h>
#include <src/devboard/hal/hal.h>
#include <src/devboard/safety/safety.h>
#include <src/devboard/utils/millis64.h>
#include <src/devboard/webserver/index_html.h>
#include <src/inverter/INVERTERS.h>
#include <src/lib/ayushsharma82-ElegantOTA/src/elop.h>
#include <src/lib/bblanchon-ArduinoJson/ArduinoJson.h>

#include "esp_task_wdt.h"
#include "esp_wifi.h"

extern TwsRoute settingsRoute;
extern TwsRoute statusRoute;
extern TwsRoute cellsRoute;
extern TwsRoute apiBatOldRoute;
extern TwsRoute batextRoute;
extern TwsRoute batcmdRoute;
extern TwsRoute eventsRoute;
extern TwsRoute eventsClearRoute;
extern TwsRoute logRoute;
extern TwsRoute rebootRoute;
extern TwsRoute pauseRoute;
extern TwsRoute estopRoute;
extern TwsRoute dumpCanRoute;
extern TwsRoute statsRoute;

bool ota_active = false;

TwsRoute eOtaStartHandler = TwsRoute("/ota/start").use(*new OtaStart());

TwsRoute eOtaUploadHandler = TwsRoute("/ota/upload", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nOK");
        request.finish();
    })
).use(*new OtaUpload());

TwsRoute canSenderHandler = TwsRoute("/api/cansend", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                            "\r\n");
    })
).use(*new CanSender());

#include "frontend.h"

const char* HTTP_RESPONSE_GZIP = "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Encoding: gzip\r\n"
                        "\r\n";

TwsRoute frontendHandler = TwsRoute("*", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully(HTTP_RESPONSE_GZIP, strlen(HTTP_RESPONSE_GZIP));
    request.set_writer_callback(CharBufWriter((const char*)html_data, sizeof(html_data)));
}));

TwsRoute *default_handlers[] = {
    &statusRoute,
    &cellsRoute,
    &apiBatOldRoute,
    &batextRoute,
    &batcmdRoute,
    &settingsRoute,
    &eventsRoute,
    &eventsClearRoute,
    &logRoute,
    &rebootRoute,
    &pauseRoute,
    &estopRoute,
    &dumpCanRoute,
    &statsRoute,
    &eOtaStartHandler,
    &eOtaUploadHandler,
    &canSenderHandler,
    &frontendHandler,
    nullptr,
};

TinyWebServer tinyWebServer(80, default_handlers);

void tiny_web_server_loop(void * pData) {
    TinyWebServer * server = (TinyWebServer *)pData;
    uint16_t time_since_watchdog_reset_ms = 0;
    server->open_listen_port();
    while (true) {
        if(server->poll()) time_since_watchdog_reset_ms += TinyWebServer::ACTIVE_POLL_TIME_MS;
        else time_since_watchdog_reset_ms += TinyWebServer::IDLE_POLL_TIME_MS;
        if(time_since_watchdog_reset_ms >= 1000) time_since_watchdog_reset_ms = 0;
    }
}
