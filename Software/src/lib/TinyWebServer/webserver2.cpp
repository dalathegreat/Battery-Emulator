#include "TinyWebServer.h"

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
// extern const char index_html_header[];
// extern const char index_html_footer[];
//extern String processor(const String& var);
//extern String cellmonitor_processor(const String& var);
//extern String get_firmware_info_processor(const String& var);
extern void debug_logger_processor(String &content);

int can_dumper_connection_id = 0;

// Dummy var still used elsewhere
bool ota_active = false;

TwsRequestWriterCallbackFunction StringWriter(std::shared_ptr<String> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        const int remaining = response->length() - alreadyWritten;
        if(remaining <= 0) {
            //DEBUG_PRINTF("TWS finished %d %d\n", alreadyWritten, response->length());
            req.finish(); // No more data to write, finish the request
            return;
        }
        req.write_direct(response->c_str() + alreadyWritten, remaining);
    };
}

class StringLike {
public:
    // Basic constructor that will copy the string unless the compiler elides
    StringLike(String str) : _str(str), _length(_str.length()) {}
    // Allow the original string to be copied/moved directly without the extra copy during construction
    StringLike(String &str) : _str(str), _length(_str.length()) {}
    StringLike(const char *chr) : _chr(chr), _length(strlen(chr)) {}
    StringLike(const char *chr, int length) : _chr(chr) , _length(length) {}

    const char* c_str() const {
        return _chr ? _chr : _str.c_str();
    }
    int length() const {
        return _length;
    }
private:
    String _str;
    const char *_chr = nullptr;
    int _length = 0;
};

TwsRequestWriterCallbackFunction StringListWriter(std::shared_ptr<std::vector<StringLike>> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        if(alreadyWritten==0) {
            //DEBUG_PRINTF("first String is at %p\n", &(response->at(0)));
        }
        for(auto &str : *response) {
            int length = str.length();
            if(alreadyWritten >= length) {
                alreadyWritten -= length;
                //DEBUG_PRINTF("Skipping already written part, alreadyWritten is now %d\n", alreadyWritten);
                continue; // Skip already written parts
            }

            //DEBUG_PRINTF("Going to write: [%s] from offset %d\n", str.c_str(), alreadyWritten);

            const int remaining = length - alreadyWritten;
            if(req.write_direct(str.c_str() + alreadyWritten, remaining) < remaining) {
                // Done for now
                //DEBUG_PRINTF("done for now\n");
                return;
            }
            alreadyWritten = 0; // Reset alreadyWritten for the next string
        }  

        req.finish();
    };
}

TwsRequestWriterCallbackFunction CharBufWriter(const char* buf, int len) {
    return [buf, len](TwsRequest &req, int alreadyWritten) {
        const int remaining = len - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); // No more data to write, finish the request
            return;
        }
        req.write_direct(buf + alreadyWritten, remaining);
    };
}


TwsRoute eOtaStartHandler("/ota/start");
OtaStart eOtaStart(&eOtaStartHandler);


TwsRoute eOtaUploadHandler("/ota/upload", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        DEBUG_PRINTF("Finished request on /ota/upload\n");
        request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nOK");
        request.finish();
    })
);
OtaUpload eOtaUpload(&eOtaUploadHandler);

extern const char* version_number;
const char common_javascript[] = COMMON_JAVASCRIPT;

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
DigestAuthSessionManager sessions;
/*
Md5DigestAuth authHandler(&indexHandler, [](const char *username, char *output) -> int {
    if(strcmp(username, "auth")==0) {
        memcpy(output, "0169de8171b73f43b80a560059a38aa9", 32); // auth:auth
        return 32;
    }
    if(strcmp(username, "foo")==0) {
        memcpy(output, "e0a109b991367f513dfa73bbae05fb07", 32); // foo:bar
        return 32;
    }
    if(strcmp(username, "ping")==0) {
        memcpy(output, "9e659e97072e03958a07f326163bfc52", 32); // ping:pong
        return 32;
    }
    return 0;
}, &sessions);
*/

// forward declaration
extern TinyWebServer tinyWebServer;

struct UintSetting {
    const char* name;
    uint32_t min;
    uint32_t max;
};

const UintSetting UINT_SETTINGS[] = {
    // Name, min value, max value
    {"INVTYPE", 0, (uint32_t)InverterProtocolType::Highest-1},
    {"INVCOMM", 0, (uint32_t)comm_interface::Highest-1},
    {"BATTTYPE", 0, (uint32_t)BatteryType::Highest-1},
    {"BATTCHEM", 0, (uint32_t)battery_chemistry_enum::Highest-1},
    {"BATTCOMM", 0, (uint32_t)comm_interface::Highest-1},
    {"BATTCVMAX", 0, 5000},
    {"BATTCVMIN", 0, 5000},
    {"CHGTYPE", 0, (uint32_t)ChargerType::Highest-1},
    {"CHGCOMM", 0, (uint32_t)comm_interface::Highest-1},
    {"EQSTOP", 0, (uint32_t)STOP_BUTTON_BEHAVIOR::Highest-1},
    {"BATT2COMM", 0, (uint32_t)comm_interface::Highest-1},
    {"BATT3COMM", 0, (uint32_t)comm_interface::Highest-1},
    {"SHUNTTYPE", 0, (uint32_t)ShuntType::Highest-1},
    {"SHUNTCOMM", 0, (uint32_t)comm_interface::Highest-1},
    {"MAXPRETIME", 0, 120000},
    {"WIFICHANNEL", 0, 14},
    {"DCHGPOWER", 0, 100000},
    {"CHGPOWER", 0, 100000},
    {"LOCALIP1", 0, 255},
    {"LOCALIP2", 0, 255},
    {"LOCALIP3", 0, 255},
    {"LOCALIP4", 0, 255},
    {"GATEWAY1", 0, 255},
    {"GATEWAY2", 0, 255},
    {"GATEWAY3", 0, 255},
    {"GATEWAY4", 0, 255},
    {"SUBNET1", 0, 255},
    {"SUBNET2", 0, 255},
    {"SUBNET3", 0, 255},
    {"SUBNET4", 0, 255},
    {"MQTTPORT", 0, 65535},
    {"MQTTTIMEOUT", 0, 30000},
    {"SOFAR_ID", 0, 255},
    {"INVCELLS", 0, 65535},
    {"INVMODULES", 0, 65535},
    {"INVCELLSPER", 0, 65535},
    {"INVVLEVEL", 0, 65535},
    {"INVCAPACITY", 0, 65535},
    {"INVBTYPE", 0, 255},
    {"CANFREQ", 0, 40},
    {"CANFDFREQ", 0, 40},
    {"PRECHGMS", 0, 120000},
    {"PWMFREQ", 0, 65535},
    {"PWMHOLD", 0, 1023},
    {"GTWCOUNTRY", 0, 65535},
    {"GTWMAPREG", 0, 9},
    {"GTWCHASSIS", 0, 9},
    {"GTWPACK", 0, 9},
    {"LEDMODE", 0, 10},
    {"BATTERY_WH_MAX", 1, 400000},
    {"GPIOOPT1", 0, 255},
    {"GPIOOPT2", 0, 255},
    {"GPIOOPT3", 0, 255},
    {nullptr, 0, 0}
};

struct FloatToUintSetting {
    const char* name;
    float min;
    float max;
    float scale; // multiply the float by this to get the stored uint value
};

const FloatToUintSetting FLOAT_TO_UINT_SETTINGS[] = {
    // Name, min value, max value, scale
    {"BATTPVMAX", 0.0f, 1000.0f, 10.0f}, // stored as decivolts
    {"BATTPVMIN", 0.0f, 1000.0f, 10.0f},
    {"MAXPERCENTAGE", 0.0f, 100.0f, 10.0f}, // stored as tenths of percent
    {"MINPERCENTAGE", 0.0f, 100.0f, 10.0f},
    {"MAXCHARGEAMP", 0.0f, 100.0f, 10.0f}, // stored as deciamps
    {"MAXDISCHARGEAMP", 0.0f, 100.0f, 10.0f},
    {"TARGETCHVOLT", 0.0f, 1000.0f, 10.0f}, // stored as decivolts
    {"TARGETDISCHVOLT", 0.0f, 1000.0f, 10.0f},
    {nullptr, 0.0f, 0.0f, 0.0f}
};

struct StringSetting {
    const char* name;
    uint8_t max_length;
};

const StringSetting STRING_SETTINGS[] = {
    // Name, max length
    {"SSID", 32},
    {"PASSWORD", 64},
    {"APNAME", 64},
    {"APPASSWORD", 64},
    {"HOSTNAME", 64},
    {"MQTTSERVER", 64},
    {"MQTTUSER", 64},
    {"MQTTPASSWORD", 64},
    {"MQTTTOPIC", 64},
    {"MQTTOBJIDPREFIX", 64},
    {"MQTTDEVICENAME", 64},
    {"HADEVICEID", 64},
    {nullptr, 0},
};

const char* BOOL_SETTINGS[] = {
    "DBLBTR",
    "CNTCTRL",
    "CNTCTRLDBL",
    "PWMCNTCTRL",
    "PERBMSRESET",
    "REMBMSRESET",
    "EXTPRECHARGE",
    "NOINVDISC",
    "CANFDASCAN",
    "WIFIAPENABLED",
    "STATICIP",
    "PERFPROFILE",
    "CANLOGUSB",
    "USBENABLED",
    "WEBENABLED",
    "CANLOGSD",
    "SDLOGENABLED",
    "MQTTENABLED",
    "MQTTTOPICS",
    "MQTTCELLV",
    "HADISC",
    "INVICNT",
    "DEYEBYD",
    "INTERLOCKREQ",
    "DIGITALHVIL",
    "GTWRHD",
    "SOCESTIMATED",
    "PYLONOFFSET",
    "PYLONORDER",
    "NCCONTACTOR",
    "TRIBTR",
    "CNTCTRLTRI",
    "ESPNOWENABLED",
    "USE_SCALED_SOC",
    "USEVOLTLIMITS",
    nullptr
};

const char* DEFAULT_TRUE_BOOL_SETTINGS[] = {
    "WIFIAPENABLED",
    nullptr
};

extern bool settingsUpdated;

extern bool contactor_control_enabled;
extern int contactorStatus; // actually an enum?

class TwsJsonGetFunc : public TwsRequestHandler {
public:
    TwsJsonGetFunc(void (*respond)(TwsRequest& request, JsonDocument& doc)) : respond(respond) {}

    void handleRequest(TwsRequest &request) override {
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

    void (*respond)(TwsRequest& request, JsonDocument& doc);
};

// A wrapper to handle raw POST data via the passed-in callback.
// The callback should return the number of bytes processed.
class TwsRawPostFunc : public TwsPostBodyHandler {
public:
    TwsRawPostFunc(int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len)) : handle(handle) {}
    
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override {
        return handle(request, index, data, len);
    }

    int (*handle)(TwsRequest& request, size_t index, uint8_t *data, size_t len);
};


TwsRoute settingsHandler("/api/internal/settings", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
    BatteryEmulatorSettingsStore settings;

    JsonArray bats = doc["batteries"].to<JsonArray>();
    for(int i=0;i<(int)BatteryType::Highest;i++) {
        bats[i] = name_for_battery_type((BatteryType)i);
    }

    JsonArray invs = doc["inverters"].to<JsonArray>();
    for(int i=0;i<(int)InverterProtocolType::Highest;i++) {
        invs[i] = name_for_inverter_type((InverterProtocolType)i);
    }

    JsonObject sets = doc["settings"].to<JsonObject>();
    for(int i=0;UINT_SETTINGS[i].name!=nullptr;i++) {
        // TODO - handle default values more sensibly?
        sets[UINT_SETTINGS[i].name] = settings.getUInt(UINT_SETTINGS[i].name, 0);
    }
    for(int i=0;FLOAT_TO_UINT_SETTINGS[i].name!=nullptr;i++) {
        sets[FLOAT_TO_UINT_SETTINGS[i].name] = settings.getUInt(FLOAT_TO_UINT_SETTINGS[i].name, 0) / FLOAT_TO_UINT_SETTINGS[i].scale;
    }
    for(int i=0;STRING_SETTINGS[i].name!=nullptr;i++) {
        sets[STRING_SETTINGS[i].name] = settings.getString(STRING_SETTINGS[i].name).c_str();
    }
    for(int i=0;BOOL_SETTINGS[i]!=nullptr;i++) {
        sets[BOOL_SETTINGS[i]] = settings.getBool(BOOL_SETTINGS[i], false);

        for(int j=0;DEFAULT_TRUE_BOOL_SETTINGS[j]!=nullptr;j++) {
            if(strcmp(BOOL_SETTINGS[i], DEFAULT_TRUE_BOOL_SETTINGS[j])==0) {
                sets[BOOL_SETTINGS[i]] = settings.getBool(BOOL_SETTINGS[i], true);
            }
        }
    }

    doc["reboot_required"] = settingsUpdated;
}));
TwsPostBufferingRequestHandler settingsPostHandler(&settingsHandler, [](TwsRequest &request, uint8_t *data, size_t len) {
    JsonDocument errors;

    BatteryEmulatorSettingsStore settings;

    JsonDocument doc;
    deserializeJson(doc, data, len);
    for(int attempt=0;attempt<2;attempt++) {
        for(int i=0;UINT_SETTINGS[i].name!=nullptr;i++) {
            if(doc[UINT_SETTINGS[i].name].is<const char*>()) {
                char *end = nullptr;
                unsigned long val = strtoul(doc[UINT_SETTINGS[i].name].as<const char*>(), &end, 10);
                if(end && *end==0) {
                    if(val < UINT_SETTINGS[i].min || val > UINT_SETTINGS[i].max) {
                        errors[UINT_SETTINGS[i].name] = "Value out of range.";
                    } else if(attempt==1) {
                        DEBUG_PRINTF("Setting %s to %lu\n", UINT_SETTINGS[i].name, val);
                        settings.saveUInt(UINT_SETTINGS[i].name, val);
                    }
                } else {
                    errors[UINT_SETTINGS[i].name] = "Invalid value.";
                }
            }
        }
        for(int i=0;FLOAT_TO_UINT_SETTINGS[i].name!=nullptr;i++) {
            if(doc[FLOAT_TO_UINT_SETTINGS[i].name].is<const char*>()) {
                char *end = nullptr;
                float fval = strtof(doc[FLOAT_TO_UINT_SETTINGS[i].name].as<const char*>(), &end);
                if(end && *end==0) {
                    if(fval < FLOAT_TO_UINT_SETTINGS[i].min || fval > FLOAT_TO_UINT_SETTINGS[i].max) {
                        errors[FLOAT_TO_UINT_SETTINGS[i].name] = "Value out of range.";
                    } else if(attempt==1) {
                        uint32_t val = (uint32_t)(fval * FLOAT_TO_UINT_SETTINGS[i].scale);
                        DEBUG_PRINTF("Setting %s to %lu\n", FLOAT_TO_UINT_SETTINGS[i].name, val);
                        settings.saveUInt(FLOAT_TO_UINT_SETTINGS[i].name, val);
                    }
                } else {
                    errors[FLOAT_TO_UINT_SETTINGS[i].name] = "Invalid value.";
                }
            }
        }
        for(int i=0;STRING_SETTINGS[i].name!=nullptr;i++) {
            if(doc[STRING_SETTINGS[i].name].is<const char*>()) {
                const char *val = doc[STRING_SETTINGS[i].name].as<const char*>();
                if(strlen(val) > STRING_SETTINGS[i].max_length) {
                    errors[STRING_SETTINGS[i].name] = "Value too long.";
                } else if(attempt==1) {
                    DEBUG_PRINTF("Setting %s to %s\n", STRING_SETTINGS[i].name, val);
                    settings.saveString(STRING_SETTINGS[i].name, val);
                }
            }
        }
        for(int i=0;BOOL_SETTINGS[i]!=nullptr;i++) {
            if(doc[BOOL_SETTINGS[i]].is<const char*>()) {
                const char *val = doc[BOOL_SETTINGS[i]].as<const char*>();
                bool bval = (strcmp(val, "true")==0 || strcmp(val, "1")==0);
                if(attempt==1) {
                    DEBUG_PRINTF("Setting %s to %d\n", BOOL_SETTINGS[i], bval);
                    settings.saveBool(BOOL_SETTINGS[i], bval);
                }
            }
        }
        if(errors.size()) {
            auto response = std::make_shared<String>();
            serializeJson(errors, *response);
            request.write_fully("HTTP/1.1 400 Bad\r\n"
                            "Connection: close\r\n"
                            "Content-Type: application/json\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "\r\n");
            request.set_writer_callback(StringWriter(response));
            return;
        }
    }

    settingsUpdated |= settings.were_settings_updated();
    DEBUG_PRINTF("Setting updated? %d\n", settings.were_settings_updated());
});

TwsRoute canSenderHandler("/api/cansend", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                            "\r\n");
    })
);
CanSender canSender(&canSenderHandler);


//TwsRequestHandlerEntry default_handlers[] = {
TwsRoute *default_handlers[] = {
    new TwsRoute("/api/status", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
        doc["hardware"] = esp32hal->name();
        doc["firmware"] = version_number;
        doc["temp"] = datalayer.system.info.CPU_temperature;
        doc["uptime"] = millis64();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["max_alloc"] = ESP.getMaxAllocHeap();

        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
        doc["channel"] = WiFi.channel();
        doc["hostname"] = WiFi.getHostname();
        doc["ip"] = WiFi.localIP().toString();
        doc["wifi_status"] = WiFi.status();
        if(wifiap_enabled) {
            // Note: Wifi.softAPSSID is buggy (doesn't handle ssid_len
            // correctly), hence we do it manually.
            wifi_config_t info;
            if (esp_wifi_get_config(WIFI_IF_AP, &info) == ESP_OK) {
                if(info.ap.ssid_len > 0) {
                    doc["ap_ssid"] = String((char*)info.ap.ssid, info.ap.ssid_len);
                } else {
                    doc["ap_ssid"] = info.ap.ssid;
                }
            }
            doc["ap_ip"] = WiFi.softAPIP().toString();
        }

        // what should t these be called?

        // OK/WARNING/ERROR/UPDATING (general BE status, derived from events)
        doc["status"] = get_emulator_status_string(get_emulator_status());
        // ACTIVE/UPDATING/FAULT (almost the same as above, but WARNING comes out as ACTIVE)
        doc["bms_status"] = getBMSStatus(datalayer.battery.status.bms_status);
        // RUNNING/PAUSING/PAUSED/RESUMING/UNKNOWN (progress through the pause state machine)
        doc["pause_status"] = get_emulator_pause_status();
        // Status from the EV BMS itself (only on some battery types)
        doc["real_bms_status"] = datalayer.battery.status.real_bms_status;
        // Whether pause has been requested
        doc["pause"] = emulator_pause_request_ON;
        // Whether the equipment stop has been activated
        doc["estop"] = datalayer.system.info.equipment_stop_active;

        JsonArray batteries = doc["battery"].to<JsonArray>();

        auto add_battery = [&](auto battery) {
            JsonObject bat = batteries.add<JsonObject>();
            bat["protocol"] = datalayer.system.info.battery_protocol;
            bat["chemistry"] = battery.info.chemistry;
            bat["soc_scaling"] = battery.settings.soc_scaling_active;
            bat["real_soc"] = static_cast<float>(battery.status.real_soc) / 100.0f;
            bat["reported_soc"] = static_cast<float>(battery.status.reported_soc) / 100.0f;
            bat["soh"] = static_cast<float>(battery.status.soh_pptt) / 100.0f;
            bat["v"] = static_cast<float>(battery.status.voltage_dV) / 10.0f;
            bat["i"] = static_cast<float>(battery.status.current_dA) / 10.0f;
            bat["p"] = battery.status.active_power_W;
            bat["total_capacity"] = battery.info.total_capacity_Wh;
            bat["reported_total_capacity"] = battery.info.reported_total_capacity_Wh;
            bat["remaining_capacity"] = battery.status.remaining_capacity_Wh;
            bat["reported_remaining_capacity"] = battery.status.reported_remaining_capacity_Wh;
            bat["temp_max"] = static_cast<float>(battery.status.temperature_max_dC) / 10.0f;
            bat["temp_min"] = static_cast<float>(battery.status.temperature_min_dC) / 10.0f;
            bat["charge_i_max"] = static_cast<float>(battery.status.max_charge_current_dA) / 10.0f;
            bat["discharge_i_max"] = static_cast<float>(battery.status.max_discharge_current_dA) / 10.0f;
            bat["charge_p_max"] = battery.status.max_charge_power_W;
            bat["discharge_p_max"] = battery.status.max_discharge_power_W;
            bat["cell_mv_max"] = battery.status.cell_max_voltage_mV;
            bat["cell_mv_min"] = battery.status.cell_min_voltage_mV;

            bat["inverter_limits_discharge"] = battery.settings.inverter_limits_discharge;
            bat["user_settings_limit_discharge"] = battery.settings.user_settings_limit_discharge;
            bat["inverter_limits_charge"] = battery.settings.inverter_limits_charge;
            bat["user_settings_limit_charge"] = battery.settings.user_settings_limit_charge;
            bat["inverter_allows_contactor_closing"] = datalayer.system.status.inverter_allows_contactor_closing;
        };

        if(battery) {
            add_battery(datalayer.battery);
            if(battery2) add_battery(datalayer.battery2);
        }

        if(inverter) {
            JsonObject inv = doc["inverter"].to<JsonObject>();
            inv["name"] = inverter->name();
        }

        if(contactor_control_enabled) {
            JsonObject con = doc["contactor"].to<JsonObject>();
            con["state"] = (int)contactorStatus;
        }

        // A summary of active events
        JsonArray events = doc["events"].to<JsonArray>();
        uint64_t current_timestamp = millis64();
        for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
            auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
            if (event_pointer->occurences > 0) {
                JsonObject ev = events.add<JsonObject>();
                ev["age"] = current_timestamp - event_pointer->timestamp;
                ev["level"] = get_event_level_string((EVENTS_ENUM_TYPE)i);
            }
        }
    })),
    new TwsRoute("/api/cells", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
        JsonArray battery = doc["battery"].to<JsonArray>();
        JsonObject bat = battery.add<JsonObject>();
        bat["temp_min"] = static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0f;
        bat["temp_max"] = static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0f;
        JsonArray data = bat["voltages"].to<JsonArray>();
        for(size_t i=0;i<datalayer.battery.info.number_of_cells && i<MAX_AMOUNT_CELLS;i++) {
            data.add(datalayer.battery.status.cell_voltages_mV[i]);
        }
        if(battery2) {
            JsonObject bat2 = battery.add<JsonObject>();
            bat2["temp_min"] = static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0f;
            bat2["temp_max"] = static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0f;
            JsonArray data2 = bat2["voltages"].to<JsonArray>();
            for(size_t i=0;i<datalayer.battery2.info.number_of_cells && i<MAX_AMOUNT_CELLS;i++) {
                data2.add(datalayer.battery2.status.cell_voltages_mV[i]);
            }
        }
    })),
    new TwsRoute("/api/batold", new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully(HTTP_RESPONSE);
        if(battery) {
            auto &renderer = battery->get_status_renderer();
            auto response = std::make_shared<String>(renderer.get_status_html());
            // response->reserve(5000);
            // response = ;
            request.set_writer_callback(StringWriter(response));
        }
    })),

    new TwsRoute("/api/batext", new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "\r\n");
        // The first four bytes (ESP32 is little endian) are the battery type.
        uint32_t btype = (uint32_t)user_selected_battery_type;
        request.write((const char*)&btype, 4);

        // Then we just send the raw extended datalayer structure for the
        // selected battery type. The frontend can decode this and display it
        // sensibly more easily than we can.
        if(user_selected_battery_type==BatteryType::BoltAmpera) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.boltampera, sizeof(datalayer_extended.boltampera)));
        } else if(user_selected_battery_type==BatteryType::BmwPhev) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.bmwphev, sizeof(datalayer_extended.bmwphev)));
        } else if(user_selected_battery_type==BatteryType::BydAtto3) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.bydAtto3, sizeof(datalayer_extended.bydAtto3)));
        } else if(user_selected_battery_type==BatteryType::CellPowerBms) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.cellpower, sizeof(datalayer_extended.cellpower)));
        } else if(user_selected_battery_type==BatteryType::Chademo) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.chademo, sizeof(datalayer_extended.chademo)));
        } else if(user_selected_battery_type==BatteryType::CmfaEv) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.CMFAEV, sizeof(datalayer_extended.CMFAEV)));
        } else if(user_selected_battery_type==BatteryType::StellantisEcmp) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.stellantisECMP, sizeof(datalayer_extended.stellantisECMP)));
        } else if(user_selected_battery_type==BatteryType::GeelyGeometryC) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.geometryC, sizeof(datalayer_extended.geometryC)));
        } else if(user_selected_battery_type==BatteryType::KiaHyundai64) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.KiaHyundai64, sizeof(datalayer_extended.KiaHyundai64)));
        } else if(user_selected_battery_type==BatteryType::TeslaModel3Y || user_selected_battery_type==BatteryType::TeslaModelSX) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.tesla, sizeof(datalayer_extended.tesla)));
        } else if(user_selected_battery_type==BatteryType::NissanLeaf) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.nissanleaf, sizeof(datalayer_extended.nissanleaf)));
        } else if(user_selected_battery_type==BatteryType::Meb) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.meb, sizeof(datalayer_extended.meb)));
        } else if(user_selected_battery_type==BatteryType::VolvoSpa) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.VolvoPolestar, sizeof(datalayer_extended.VolvoPolestar)));
        } else if(user_selected_battery_type==BatteryType::VolvoSpaHybrid) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.VolvoHybrid, sizeof(datalayer_extended.VolvoHybrid)));
        } else if(user_selected_battery_type==BatteryType::RenaultZoe1) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.zoe, sizeof(datalayer_extended.zoe)));
        } else if(user_selected_battery_type==BatteryType::RenaultZoe2) {
            request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.zoePH2, sizeof(datalayer_extended.zoePH2)));
        }
    })),
    new TwsRoute("/api/batact", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
        JsonObject actions = doc["actions"].to<JsonObject>();
        if(battery) {
            if(battery->supports_reset_SOH()) actions["reset_soh"] = true;
            if(battery->supports_reset_crash()) actions["reset_crash"] = true;
        }
    })),
    &settingsHandler,
    // new TwsRoute("/api/live", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
    //     doc["pause"] = emulator_pause_request_ON;
    //     doc["estop"] = datalayer.system.info.equipment_stop_active;
        
    //     JsonArray batteries = doc["battery"].to<JsonArray>();
    //     auto add_battery = [&](auto battery) {
    //         JsonObject bat = batteries.add<JsonObject>();
    //         // use reported or real?
    //         bat["soc"] = static_cast<float>(battery.status.reported_soc) / 100.0f;
    //         bat["i"] = static_cast<float>(battery.status.current_dA) / 10.0f;
    //     };

    //     if(battery) {
    //         add_battery(datalayer.battery);
    //         if(battery2) add_battery(datalayer.battery2);
    //     }

    //     JsonArray events = doc["events"].to<JsonArray>();

    //     uint64_t current_timestamp = millis64();
    //     for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    //         auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    //         if (event_pointer->occurences > 0) {
    //             JsonObject ev = events.add<JsonObject>();
    //             ev["age"] = current_timestamp - event_pointer->timestamp;
    //             ev["level"] = get_event_level_string((EVENTS_ENUM_TYPE)i);
    //         }
    //     }
    // })),

    /*
    new TwsRoute("/api/tesla", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
        auto &d = datalayer_extended.tesla;
        JsonArray vals = doc["data"].to<JsonArray>();

        vals.add(d.hvil_status);
        vals.add(d.packContNegativeState);
        vals.add(d.packContPositiveState);
        vals.add(d.packContactorSetState);
        vals.add(d.packCtrsClosingBlocked);
        vals.add(d.pyroTestInProgress);
        vals.add(d.battery_packCtrsOpenNowRequested);
        vals.add(d.battery_packCtrsOpenRequested);
        vals.add(d.battery_packCtrsRequestStatus);
        vals.add(d.battery_packCtrsResetRequestRequired);
        vals.add(d.battery_dcLinkAllowedToEnergize);
        vals.add(d.BMS_partNumber);
        vals.add(d.BMS_info_buildConfigId);
        vals.add(d.BMS_info_hardwareId);
        vals.add(d.BMS_info_componentId);
        vals.add(d.BMS_info_pcbaId);
        vals.add(d.BMS_info_assemblyId);
        vals.add(d.BMS_info_usageId);
        vals.add(d.BMS_info_subUsageId);
        vals.add(d.BMS_info_platformType);
        vals.add(d.BMS_info_appCrc);
        vals.add(d.BMS_info_bootGitHash);
        vals.add(d.BMS_info_bootUdsProtoVersion);
        vals.add(d.BMS_info_bootCrc);
        vals.add(d.battery_serialNumber);
        vals.add(d.battery_partNumber);
        vals.add(d.battery_manufactureDate);
        vals.add(d.battery_beginning_of_life);
        vals.add(d.battery_battTempPct);
        vals.add(d.battery_dcdcLvBusVolt);
        vals.add(d.battery_dcdcHvBusVolt);
        vals.add(d.battery_dcdcLvOutputCurrent);
        vals.add(d.BMS352_mux);
        vals.add(d.battery_nominal_full_pack_energy);
        vals.add(d.battery_nominal_full_pack_energy_m0);
        vals.add(d.battery_nominal_energy_remaining);
        vals.add(d.battery_nominal_energy_remaining_m0);
        vals.add(d.battery_ideal_energy_remaining);
        vals.add(d.battery_ideal_energy_remaining_m0);
        vals.add(d.battery_energy_to_charge_complete);
        vals.add(d.battery_energy_to_charge_complete_m1);
        vals.add(d.battery_energy_buffer);
        vals.add(d.battery_energy_buffer_m1);
        vals.add(d.battery_expected_energy_remaining);
        vals.add(d.battery_expected_energy_remaining_m1);
        vals.add(d.battery_full_charge_complete);
        vals.add(d.battery_fully_charged);
        vals.add(d.battery_BrickVoltageMax);
        vals.add(d.battery_BrickVoltageMin);
        vals.add(d.battery_BrickVoltageMaxNum);
        vals.add(d.battery_BrickVoltageMinNum);
        vals.add(d.battery_BrickTempMaxNum);
        vals.add(d.battery_BrickTempMinNum);
        vals.add(d.battery_BrickModelTMax);
        vals.add(d.battery_BrickModelTMin);
        vals.add(d.battery_packConfigMultiplexer);
        vals.add(d.battery_moduleType);
        vals.add(d.battery_reservedConfig);
        vals.add(d.battery_packMass);
        vals.add(d.battery_platformMaxBusVoltage);
        vals.add(d.BMS_min_voltage);
        vals.add(d.BMS_max_voltage);
        vals.add(d.battery_max_charge_current);
        vals.add(d.battery_max_discharge_current);
        vals.add(d.battery_soc_min);
        vals.add(d.battery_soc_max);
        vals.add(d.battery_soc_ave);
        vals.add(d.battery_soc_ui);
        vals.add(d.BMS_hvilFault);
        vals.add(d.BMS_contactorState);
        vals.add(d.BMS_state);
        vals.add(d.BMS_hvState);
        vals.add(d.BMS_isolationResistance);
        vals.add(d.BMS_uiChargeStatus);
        vals.add(d.BMS_diLimpRequest);
        vals.add(d.BMS_chgPowerAvailable);
        vals.add(d.BMS_pcsPwmEnabled);
        vals.add(d.BMS_maxRegenPower);
        vals.add(d.BMS_maxDischargePower);
        vals.add(d.BMS_maxStationaryHeatPower);
        vals.add(d.BMS_hvacPowerBudget);
        vals.add(d.BMS_notEnoughPowerForHeatPump);
        vals.add(d.BMS_powerLimitState);
        vals.add(d.BMS_inverterTQF);
        vals.add(d.BMS_powerDissipation);
        vals.add(d.BMS_flowRequest);
        vals.add(d.BMS_inletActiveCoolTargetT);
        vals.add(d.BMS_inletPassiveTargetT);
        vals.add(d.BMS_inletActiveHeatTargetT);
        vals.add(d.BMS_packTMin);
        vals.add(d.BMS_packTMax);
        vals.add(d.BMS_pcsNoFlowRequest);
        vals.add(d.BMS_noFlowRequest);
        vals.add(d.PCS_dcdcPrechargeStatus);
        vals.add(d.PCS_dcdc12VSupportStatus);
        vals.add(d.PCS_dcdcHvBusDischargeStatus);
        vals.add(d.PCS_dcdcMainState);
        vals.add(d.PCS_dcdcSubState);
        vals.add(d.PCS_dcdcFaulted);
        vals.add(d.PCS_dcdcOutputIsLimited);
        vals.add(d.PCS_dcdcMaxOutputCurrentAllowed);
        vals.add(d.PCS_dcdcPrechargeRtyCnt);
        vals.add(d.PCS_dcdc12VSupportRtyCnt);
        vals.add(d.PCS_dcdcDischargeRtyCnt);
        vals.add(d.PCS_dcdcPwmEnableLine);
        vals.add(d.PCS_dcdcSupportingFixedLvTarget);
        vals.add(d.PCS_dcdcPrechargeRestartCnt);
        vals.add(d.PCS_dcdcInitialPrechargeSubState);
        vals.add(d.PCS_partNumber);
        vals.add(d.PCS_info_buildConfigId);
        vals.add(d.PCS_info_hardwareId);
        vals.add(d.PCS_info_componentId);
        vals.add(d.PCS_info_pcbaId);
        vals.add(d.PCS_info_assemblyId);
        vals.add(d.PCS_info_usageId);
        vals.add(d.PCS_info_subUsageId);
        vals.add(d.PCS_info_platformType);
        vals.add(d.PCS_info_appCrc);
        vals.add(d.PCS_info_cpu2AppCrc);
        vals.add(d.PCS_info_bootGitHash);
        vals.add(d.PCS_info_bootUdsProtoVersion);
        vals.add(d.PCS_info_bootCrc);
        vals.add(d.PCS_dcdcTemp);
        vals.add(d.PCS_ambientTemp);
        vals.add(d.PCS_chgPhATemp);
        vals.add(d.PCS_chgPhBTemp);
        vals.add(d.PCS_chgPhCTemp);
        vals.add(d.PCS_dcdcMaxLvOutputCurrent);
        vals.add(d.PCS_dcdcCurrentLimit);
        vals.add(d.PCS_dcdcLvOutputCurrentTempLimit);
        vals.add(d.PCS_dcdcUnifiedCommand);
        vals.add(d.PCS_dcdcCLAControllerOutput);
        vals.add(d.PCS_dcdcTankVoltage);
        vals.add(d.PCS_dcdcTankVoltageTarget);
        vals.add(d.PCS_dcdcClaCurrentFreq);
        vals.add(d.PCS_dcdcTCommMeasured);
        vals.add(d.PCS_dcdcShortTimeUs);
        vals.add(d.PCS_dcdcHalfPeriodUs);
        vals.add(d.PCS_dcdcIntervalMaxFrequency);
        vals.add(d.PCS_dcdcIntervalMaxHvBusVolt);
        vals.add(d.PCS_dcdcIntervalMaxLvBusVolt);
        vals.add(d.PCS_dcdcIntervalMaxLvOutputCurr);
        vals.add(d.PCS_dcdcIntervalMinFrequency);
        vals.add(d.PCS_dcdcIntervalMinHvBusVolt);
        vals.add(d.PCS_dcdcIntervalMinLvBusVolt);
        vals.add(d.PCS_dcdcIntervalMinLvOutputCurr);
        vals.add(d.PCS_dcdc12vSupportLifetimekWh);
        vals.add(d.HVP_gpioPassivePyroDepl);
        vals.add(d.HVP_gpioPyroIsoEn);
        vals.add(d.HVP_gpioCpFaultIn);
        vals.add(d.HVP_gpioPackContPowerEn);
        vals.add(d.HVP_gpioHvCablesOk);
        vals.add(d.HVP_gpioHvpSelfEnable);
        vals.add(d.HVP_gpioLed);
        vals.add(d.HVP_gpioCrashSignal);
        vals.add(d.HVP_gpioShuntDataReady);
        vals.add(d.HVP_gpioFcContPosAux);
        vals.add(d.HVP_gpioFcContNegAux);
        vals.add(d.HVP_gpioBmsEout);
        vals.add(d.HVP_gpioCpFaultOut);
        vals.add(d.HVP_gpioPyroPor);
        vals.add(d.HVP_gpioShuntEn);
        vals.add(d.HVP_gpioHvpVerEn);
        vals.add(d.HVP_gpioPackCoontPosFlywheel);
        vals.add(d.HVP_gpioCpLatchEnable);
        vals.add(d.HVP_gpioPcsEnable);
        vals.add(d.HVP_gpioPcsDcdcPwmEnable);
        vals.add(d.HVP_gpioPcsChargePwmEnable);
        vals.add(d.HVP_gpioFcContPowerEnable);
        vals.add(d.HVP_gpioHvilEnable);
        vals.add(d.HVP_gpioSecDrdy);
        vals.add(d.HVP_hvp1v5Ref);
        vals.add(d.HVP_shuntCurrentDebug);
        vals.add(d.HVP_packCurrentMia);
        vals.add(d.HVP_auxCurrentMia);
        vals.add(d.HVP_currentSenseMia);
        vals.add(d.HVP_shuntRefVoltageMismatch);
        vals.add(d.HVP_shuntThermistorMia);
        vals.add(d.HVP_partNumber);
        vals.add(d.HVP_info_buildConfigId);
        vals.add(d.HVP_info_hardwareId);
        vals.add(d.HVP_info_componentId);
        vals.add(d.HVP_info_pcbaId);
        vals.add(d.HVP_info_assemblyId);
        vals.add(d.HVP_info_usageId);
        vals.add(d.HVP_info_subUsageId);
        vals.add(d.HVP_info_platformType);
        vals.add(d.HVP_info_appCrc);
        vals.add(d.HVP_info_bootGitHash);
        vals.add(d.HVP_info_bootUdsProtoVersion);
        vals.add(d.HVP_info_bootCrc);
        vals.add(d.HVP_shuntHwMia);
        vals.add(d.HVP_dcLinkVoltage);
        vals.add(d.HVP_packVoltage);
        vals.add(d.HVP_fcLinkVoltage);
        vals.add(d.HVP_packContVoltage);
        vals.add(d.HVP_packNegativeV);
        vals.add(d.HVP_packPositiveV);
        vals.add(d.HVP_pyroAnalog);
        vals.add(d.HVP_dcLinkNegativeV);
        vals.add(d.HVP_dcLinkPositiveV);
        vals.add(d.HVP_fcLinkNegativeV);
        vals.add(d.HVP_fcContCoilCurrent);
        vals.add(d.HVP_fcContVoltage);
        vals.add(d.HVP_hvilInVoltage);
        vals.add(d.HVP_hvilOutVoltage);
        vals.add(d.HVP_fcLinkPositiveV);
        vals.add(d.HVP_packContCoilCurrent);
        vals.add(d.HVP_battery12V);
        vals.add(d.HVP_shuntRefVoltageDbg);
        vals.add(d.HVP_shuntAuxCurrentDbg);
        vals.add(d.HVP_shuntBarTempDbg);
        vals.add(d.HVP_shuntAsicTempDbg);
        vals.add(d.HVP_shuntAuxCurrentStatus);
        vals.add(d.HVP_shuntBarTempStatus);
        vals.add(d.HVP_shuntAsicTempStatus);
    })),
    */
    new TwsRoute("/api/events", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
        JsonArray events = doc["events"].to<JsonArray>();

        std::vector<EventData> order_events;
        for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
            auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
            if (event_pointer->occurences > 0) {
                order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
            }
        }

        std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampDesc);
        uint64_t current_timestamp = millis64();

        for (const auto& event : order_events) {
            EVENTS_ENUM_TYPE event_handle = event.event_handle;
            auto event_pointer = event.event_pointer;

            JsonObject ev = events.add<JsonObject>();
            ev["type"] = get_event_enum_string(event_handle);
            ev["level"] = get_event_level_string(event_handle);
            ev["age"] = current_timestamp - event_pointer->timestamp;
            ev["count"] = event_pointer->occurences;
            ev["data"] = event_pointer->data;
            ev["message"] = get_event_message_string(event_handle);
        }
    })),
    new TwsRoute("/api/log", new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                      "Connection: close\r\n"
                      "Content-Type: text/plain\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n");
        // We don't actually need to send the offset, the frontend can search for the first nul instead
        request.set_writer_callback(CharBufWriter((const char*)datalayer.system.info.logged_can_messages, sizeof(datalayer.system.info.logged_can_messages)));
    })),
    new TwsRoute("/api/reboot", new TwsRequestHandlerFunc([](TwsRequest& request) {
        if(!request.is_post()) {
            request.write_fully(HTTP_405);
            return;
        }

        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\nRebooting server...\n");

        //Equipment STOP without persisting the equipment state before restart
        setBatteryPause(true, true, true, false);
        delay(1000);
        ESP.restart();
    })),
    new TwsRoute("/api/pause", new TwsRawPostFunc([](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> int {
        if(len==1 && data[0]=='1') {
            setBatteryPause(true, false);
        } else if(len==1 && data[0]=='0') {
            setBatteryPause(false, false);
        }
        request.write_fully(HTTP_204);
        request.finish();
        return len;
    })),
    new TwsRoute("/api/estop", new TwsRawPostFunc([](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> int {
        if(len==1 && data[0]=='1') {
            setBatteryPause(true, false, true);  //Pause battery, do not pause CAN, equipment stop on (store to flash)
        } else if(len==1 && data[0]=='0') {
            setBatteryPause(false, false, false);
        }
        request.write_fully(HTTP_204);
        request.finish();
        return len;
    })),
    // new TwsRoute("/log", new TwsRequestHandlerFunc([](TwsRequest& request) {
    //     auto response = std::make_shared<String>();
    //     //auto response = std::make_unique<String>();
    //     response->reserve(17000);
    //     response->concat(HTTP_RESPONSE);
    //     //String response = String();
        
    //     debug_logger_processor(*response);
    //     if(response->length() < 20) {
    //         request.write("HTTP/1.1 500 Internal Server Error\r\n"
    //                       "Connection: close\r\n"
    //                       "Content-Type: text/plain\r\n"
    //                       "\r\nNo response?\n");
    //         request.finish();
    //     } else {
    //         request.set_writer_callback(StringWriter(response));
    //     }
    // })),
    new TwsRoute("/dump_can", new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                      "Connection: close\r\n"
                      "Content-Type: text/plain\r\n"
                      "\r\n"); // CAN log follows:\n\n
        if(can_dumper_connection_id) {
            tinyWebServer.finish(can_dumper_connection_id);
        }
        can_dumper_connection_id = request.get_connection_id();
        datalayer.system.info.can_logging_active2 = true;
    })),
    // new TwsRoute("/update", new TwsRequestHandlerFunc([](TwsRequest& request) {
    //     const char* HEADER = "HTTP/1.1 200 OK\r\n"
    //                   "Connection: close\r\n"
    //                   "Content-Type: text/html\r\n"
    //                   "Content-Encoding: gzip\r\n"
    //                   "\r\n";
    //     request.write_fully(HEADER, strlen(HEADER));
    //     request.set_writer_callback(CharBufWriter((const char*)ELEGANT_HTML, sizeof(ELEGANT_HTML)));
    // })),
    // new TwsRoute("/GetFirmwareInfo", new TwsRequestHandlerFunc([](TwsRequest& request) {
    //     const char* HEADER = "HTTP/1.1 200 OK\r\n"
    //                   "Connection: close\r\n"
    //                   "Content-Type: application/json\r\n"
    //                   "\r\n";
    //     request.write_fully(HEADER, strlen(HEADER));
    //     request.write_fully("{\"hardware\":\"LilyGo T-CAN485\",\"firmware\":\"9.0.0\"}");
    //     // auto response = std::make_shared<String>(get_firmware_info_processor("X"));
    //     // request.set_writer_callback(StringWriter(response));
    // })),
    new TwsRoute("/stats", new TwsRequestHandlerFunc([](TwsRequest& request) {
        request.write_fully("HTTP/1.1 200 OK\r\n"
                            "Connection: close\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n");
        auto response = std::make_shared<String>();
        response->reserve(5000);

        char output[3000] = {0};
        vTaskList(output);
        response->concat(output);
        response->concat("\n");
        vTaskGetRunTimeStats(output);
        response->concat(output);
        // response->concat("\n\nTask Run Time Stats:\n");
        request.set_writer_callback(StringWriter(response));
    })),
    &eOtaStartHandler,
    &eOtaUploadHandler,
    &canSenderHandler,
    &frontendHandler,
    nullptr,
};

TinyWebServer tinyWebServer(80, default_handlers);

const char *hex = "0123456789abcdef";

char* put_hex(char *ptr, uint32_t value, uint8_t digits) {
    for(int i=digits-1;i>=0;i--) {
        *ptr++ = hex[(value >> (i*4)) & 0x0f];
    }
    // switch(digits) {
    // case 8:
    //     *ptr++ = hex[(value & 0xf0000000) >> 28];
    // case 7:
    //     *ptr++ = hex[(value & 0xf000000) >> 24];
    // case 6:
    //     *ptr++ = hex[(value & 0xf00000) >> 20];
    // case 5:
    //     *ptr++ = hex[(value & 0xf0000) >> 16];
    // case 4:
    //     *ptr++ = hex[(value & 0xf000) >> 12];
    // case 3:
    //     *ptr++ = hex[(value & 0xf00) >> 8];
    // case 2:
    //     *ptr++ = hex[(value & 0xf0) >> 4];
    // case 1:
    //     *ptr++ = hex[(value & 0xf) >> 0];
    // }

    return ptr;
}

char* put_time(char *ptr, unsigned long time) {
    if(time >= 100000000) goto seconds100000;
    if(time >= 10000000) goto seconds10000;
    if(time >= 1000000) goto seconds1000;
    if(time >= 100000) goto seconds100;
    if(time >= 10000) goto seconds10;
    goto seconds;

seconds100000:
    // We'll wrap every 27.8 hours
    time = time % 100000000;
seconds10000:
    *ptr++ = (time / 10000000) + '0';
    time = time % 10000000;
seconds1000:
    *ptr++ = (time / 1000000) + '0';
    time = time % 1000000;
seconds100:
    *ptr++ = (time / 100000) + '0';
    time = time % 100000;
seconds10:
    *ptr++ = (time / 10000) + '0';
    time = time % 10000;
seconds:
    *ptr++ = (time / 1000) + '0';
    time = time % 1000;

    *ptr++ = '.';

    *ptr++ = (time / 100) + '0';
    time = time % 100;
    *ptr++ = (time / 10) + '0';
    time = time % 10;
    *ptr++ = (time) + '0';
    return ptr;
}

char* put_time2(char *ptr, unsigned long time) {
    if(time >= 100000000) time %= 100000000;

    // Slow due to all the divides!

    int div = 10000000;
    for(int i=0; i<8; i++) {
        if(time >= div) {
            int digit = time / div;
            if(digit > 0 || i > 4) {
                *ptr++ = digit + '0';
            }
            time = time % div;
        }
        if(i==4) *ptr++ = '.';

        div /= 10;
    }
    return ptr;
}

void dump_can_frame2(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
    // Add 20 extra bytes for future newlines (which indicate overruns)
    const int required_space = 29 + frame.DLC*3 + 20;

    if(tinyWebServer.free(can_dumper_connection_id) < required_space) {
        // Output isn't flushing fast enough, add a newline to indicate an overrun
        tinyWebServer.write(can_dumper_connection_id, "\n", 1);
        return;
    }


    char line[230];
    char *ptr = line;

    unsigned long currentTime = millis();
    //unsigned long start = micros();
    *ptr++ = '(';
    ptr = put_time(ptr, currentTime);

//     if(currentTime >= 100000000) goto seconds100000;
//     if(currentTime >= 10000000) goto seconds10000;
//     if(currentTime >= 1000000) goto seconds1000;
//     if(currentTime >= 100000) goto seconds100;
//     if(currentTime >= 10000) goto seconds10;
//     goto seconds;

// seconds100000:
//     currentTime = currentTime % 100000000;
// seconds10000:
//     *ptr++ = (currentTime / 10000000) + '0';
//     currentTime = currentTime % 10000000;
// seconds1000:
//     *ptr++ = (currentTime / 1000000) + '0';
//     currentTime = currentTime % 1000000;
// seconds100:
//     *ptr++ = (currentTime / 100000) + '0';
//     currentTime = currentTime % 100000;
// seconds10:
//     *ptr++ = (currentTime / 10000) + '0';
//     currentTime = currentTime % 10000;
// seconds:
//     *ptr++ = (currentTime / 1000) + '0';
//     currentTime = currentTime % 1000;

//     *ptr++ = '.';
//     *ptr++ = (currentTime / 100) + '0';
//     currentTime = currentTime % 100;
//     *ptr++ = (currentTime / 10) + '0';
//     currentTime = currentTime % 10;
//     *ptr++ = (currentTime) + '0';

    *ptr++ = ')';
    *ptr++ = ' ';
    if(msgDir == MSG_RX) {
        *ptr++ = 'R';
        *ptr++ = 'X';
        *ptr++ = '0' + ((int)interface * 2);
    } else {
        *ptr++ = 'T';
        *ptr++ = 'X';
        *ptr++ = '1' + ((int)interface * 2);
    }
    *ptr++ = ' ';

    if(frame.ext_ID) {
        ptr = put_hex(ptr, frame.ID, 8);
    } else {
        ptr = put_hex(ptr, frame.ID, 3);
    }
    //     *ptr++ = hex[(frame.ID&0xf0000000) >> 28];
    //     *ptr++ = hex[(frame.ID&0xf000000) >> 24];
    //     *ptr++ = hex[(frame.ID&0xf00000) >> 20];
    //     *ptr++ = hex[(frame.ID&0xf0000) >> 16];
    //     *ptr++ = hex[(frame.ID&0xf000) >> 12];
    // }
    // *ptr++ = hex[(frame.ID&0xf00) >> 8];
    // *ptr++ = hex[(frame.ID&0x0f0) >> 4];
    // *ptr++ = hex[(frame.ID&0x00f) >> 0];
    *ptr++ = ' ';
    *ptr++ = '[';
    if(frame.DLC > 9) {
        *ptr++ = '0' + (frame.DLC / 10);
        *ptr++ = '0' + (frame.DLC % 10);
    } else {
        *ptr++ = '0' + (frame.DLC);
    }
    *ptr++ = ']';
    // *ptr++ = ' ';
    // *ptr++ = hex[(int)((frame.data.u8[0]&0xf0) >> 4)];
    // *ptr++ = hex[(int)((frame.data.u8[0]&0x0f) >> 0)];

    for(int i=0;i<frame.DLC;i++) {
        *ptr++ = ' ';
        ptr = put_hex(ptr, frame.data.u8[i], 2);
        // *ptr++ = hex[(int)((frame.data.u8[i]&0xf0) >> 4)];
        // *ptr++ = hex[(int)((frame.data.u8[i]&0x0f) >> 0)];
    }
    *ptr++ = '\n';

    // unsigned long end = micros();

    int ret = tinyWebServer.write(can_dumper_connection_id, line, ptr - line);
    if(ret < 0) {
        // connection not found, stop logging
        datalayer.system.info.can_logging_active2 = false;
        return;
    }
}


void tiny_web_server_loop(void * pData) {
    TinyWebServer * server = (TinyWebServer *)pData;
    uint16_t time_since_watchdog_reset_ms = 0;

    //esp_task_wdt_add(NULL); // Add the current task to the watchdog

    server->open_listen_port();

    while (true) {
        if(server->poll()) {
            // Was an active poll
            time_since_watchdog_reset_ms += TinyWebServer::ACTIVE_POLL_TIME_MS;
        } else {
            // Was an idle poll
            time_since_watchdog_reset_ms += TinyWebServer::IDLE_POLL_TIME_MS;
        }

        if(time_since_watchdog_reset_ms >= 1000) {
            //esp_task_wdt_reset();
            time_since_watchdog_reset_ms = 0;
        }

        //taskYIELD();
        //taskYIELD();
    }
}
