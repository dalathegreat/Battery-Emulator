#include "TinyWebServer.h"
#include "TwsBuffering.h"
#include "webserver_utils.h"

#include <src/battery/BATTERIES.h>
#include <src/inverter/INVERTERS.h>
#include <src/charger/CanCharger.h>
#include <src/communication/equipmentstopbutton/comm_equipmentstopbutton.h>

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
    float scale; 
};

const FloatToUintSetting FLOAT_TO_UINT_SETTINGS[] = {
    {"BATTPVMAX", 0.0f, 1000.0f, 10.0f}, 
    {"BATTPVMIN", 0.0f, 1000.0f, 10.0f},
    {"MAXPERCENTAGE", 0.0f, 100.0f, 10.0f}, 
    {"MINPERCENTAGE", 0.0f, 100.0f, 10.0f},
    {"MAXCHARGEAMP", 0.0f, 100.0f, 10.0f}, 
    {"MAXDISCHARGEAMP", 0.0f, 100.0f, 10.0f},
    {"TARGETCHVOLT", 0.0f, 1000.0f, 10.0f}, 
    {"TARGETDISCHVOLT", 0.0f, 1000.0f, 10.0f},
    {nullptr, 0.0f, 0.0f, 0.0f}
};

struct StringSetting {
    const char* name;
    uint8_t max_length;
    bool secret;  
};

const StringSetting STRING_SETTINGS[] = {
    {"SSID", 32, false},
    {"PASSWORD", 64, true},
    {"APNAME", 64, false},
    {"APPASSWORD", 64, true},
    {"HOSTNAME", 64, false},
    {"MQTTSERVER", 64, false},
    {"MQTTUSER", 64, false},
    {"MQTTPASSWORD", 64, true},
    {"MQTTTOPIC", 64, false},
    {"MQTTOBJIDPREFIX", 64, false},
    {"MQTTDEVICENAME", 64, false},
    {"HADEVICEID", 64, false},
    {nullptr, 0, false},
};

const char* BOOL_SETTINGS[] = {
    "DBLBTR", "CNTCTRL", "CNTCTRLDBL", "PWMCNTCTRL", "PERBMSRESET", "REMBMSRESET",
    "EXTPRECHARGE", "NOINVDISC", "CANFDASCAN", "WIFIAPENABLED", "STATICIP",
    "PERFPROFILE", "CANLOGUSB", "USBENABLED", "WEBENABLED", "CANLOGSD",
    "SDLOGENABLED", "MQTTENABLED", "MQTTTOPICS", "MQTTCELLV", "HADISC",
    "INVICNT", "DEYEBYD", "INTERLOCKREQ", "DIGITALHVIL", "GTWRHD", "SOCESTIMATED",
    "PYLONOFFSET", "PYLONORDER", "NCCONTACTOR", "TRIBTR", "CNTCTRLTRI",
    "ESPNOWENABLED", "USE_SCALED_SOC", "USEVOLTLIMITS", nullptr
};

const char* DEFAULT_TRUE_BOOL_SETTINGS[] = {
    "WIFIAPENABLED", nullptr
};

extern bool settingsUpdated;

void settingsFullPostBody(TwsRequest &request, uint8_t *data, size_t len) {
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
                if(STRING_SETTINGS[i].secret && strlen(val) == 0) continue;
                if(strlen(val) > STRING_SETTINGS[i].max_length) {
                    errors[STRING_SETTINGS[i].name] = "Value too long.";
                } else if(attempt==1) {
                    settings.saveString(STRING_SETTINGS[i].name, val);
                }
            }
        }
        for(int i=0;BOOL_SETTINGS[i]!=nullptr;i++) {
            if(doc[BOOL_SETTINGS[i]].is<const char*>()) {
                const char *val = doc[BOOL_SETTINGS[i]].as<const char*>();
                bool bval = (strcmp(val, "true")==0 || strcmp(val, "1")==0);
                if(attempt==1) {
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
}

TwsRoute settingsRoute = TwsRoute("/api/internal/settings", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
    BatteryEmulatorSettingsStore settings;
    JsonArray bats = doc["batteries"].to<JsonArray>();
    for(int i=0;i<(int)BatteryType::Highest;i++) bats[i] = name_for_battery_type((BatteryType)i);
    JsonArray invs = doc["inverters"].to<JsonArray>();
    for(int i=0;i<(int)InverterProtocolType::Highest;i++) invs[i] = name_for_inverter_type((InverterProtocolType)i);

    JsonObject sets = doc["settings"].to<JsonObject>();
    for(int i=0;UINT_SETTINGS[i].name!=nullptr;i++) sets[UINT_SETTINGS[i].name] = settings.getUInt(UINT_SETTINGS[i].name, 0);
    for(int i=0;FLOAT_TO_UINT_SETTINGS[i].name!=nullptr;i++) sets[FLOAT_TO_UINT_SETTINGS[i].name] = settings.getUInt(FLOAT_TO_UINT_SETTINGS[i].name, 0) / FLOAT_TO_UINT_SETTINGS[i].scale;
    for(int i=0;STRING_SETTINGS[i].name!=nullptr;i++) {
        if(!STRING_SETTINGS[i].secret) sets[STRING_SETTINGS[i].name] = settings.getString(STRING_SETTINGS[i].name).c_str();
    }
    for(int i=0;BOOL_SETTINGS[i]!=nullptr;i++) {
        bool def = false;
        for(int j=0;DEFAULT_TRUE_BOOL_SETTINGS[j]!=nullptr;j++) {
            if(strcmp(BOOL_SETTINGS[i], DEFAULT_TRUE_BOOL_SETTINGS[j])==0) { def = true; break; }
        }
        sets[BOOL_SETTINGS[i]] = settings.getBool(BOOL_SETTINGS[i], def);
    }
    doc["reboot_required"] = settingsUpdated;
})).use(*new TwsPostBufferingRequestHandler(settingsFullPostBody));
