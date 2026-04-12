#include "TinyWebServer.h"
#include "TwsBuffering.h"
#include "webserver_utils.h"

#include <src/battery/BATTERIES.h>
#include <src/charger/CHARGERS.h>
#include <src/inverter/INVERTERS.h>
#include <src/charger/CanCharger.h>
#include <src/communication/equipmentstopbutton/comm_equipmentstopbutton.h>

/* 

Adding settings
---------------

1. Add a new entry to either UINT_SETTINGS, STRING_SETTINGS,
   FLOAT_TO_UINT_SETTINGS, FLOAT_SETTINGS or BOOL_SETTINGS depending on the type
   of the setting.

2. The name of the setting determines the flash storage key as well as the API
   field name and frontend form field name. Names prefixed with `TMP_` are not
   stored in the flash.

3. Setting entries have additional members:
    - min and max values for validation
    - a scale factor for FLOAT_TO_UINT_SETTINGS
    - string length limit for STRING_SETTINGS
    - a secret flag to stop stored passwords being read out
    - an optional update function that is called when the setting is updated via
      the webserver. This can be used to immediately apply changes to the
      datalayer or other components.

4. For `TMP_` settings, add a line to `settingsRoute` to populate the setting
   value in the frontend (since it can't be populated from flash).

5. Add the corresponding frontend form field in `settings.tsx`.

*/

struct UintSetting {
    const char* name;
    uint32_t min;
    uint32_t max;
    void (*update_func)(uint32_t value);
};

const UintSetting UINT_SETTINGS[] = {
    // Name, min value, max value, update_func
    {"INVTYPE", 0, (uint32_t)InverterProtocolType::Highest-1, nullptr},
    {"INVCOMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"BATTTYPE", 0, (uint32_t)BatteryType::Highest-1, nullptr},
    {"BATTCHEM", 0, (uint32_t)battery_chemistry_enum::Highest-1, nullptr},
    {"BATTCOMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"BATTCVMAX", 0, 5000, nullptr},
    {"BATTCVMIN", 0, 5000, nullptr},
    {"CHGTYPE", 0, (uint32_t)ChargerType::Highest-1, nullptr},
    {"CHGCOMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"EQSTOP", 0, (uint32_t)STOP_BUTTON_BEHAVIOR::Highest-1, nullptr},
    {"BATT2COMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"BATT3COMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"SHUNTTYPE", 0, (uint32_t)ShuntType::Highest-1, nullptr},
    {"SHUNTCOMM", 0, (uint32_t)comm_interface::Highest-1, nullptr},
    {"MAXPRETIME", 0, 120000, nullptr},
    {"WIFICHANNEL", 0, 14, nullptr},
    {"DCHGPOWER", 0, 100000, nullptr},
    {"CHGPOWER", 0, 100000, nullptr},
    {"LOCALIP1", 0, 255, nullptr},
    {"LOCALIP2", 0, 255, nullptr},
    {"LOCALIP3", 0, 255, nullptr},
    {"LOCALIP4", 0, 255, nullptr},
    {"GATEWAY1", 0, 255, nullptr},
    {"GATEWAY2", 0, 255, nullptr},
    {"GATEWAY3", 0, 255, nullptr},
    {"GATEWAY4", 0, 255, nullptr},
    {"SUBNET1", 0, 255, nullptr},
    {"SUBNET2", 0, 255, nullptr},
    {"SUBNET3", 0, 255, nullptr},
    {"SUBNET4", 0, 255, nullptr},
    {"MQTTPORT", 0, 65535, nullptr},
    {"MQTTTIMEOUT", 0, 30000, nullptr},
    {"SOFAR_ID", 0, 255, nullptr},
    {"INVCELLS", 0, 65535, nullptr},
    {"INVMODULES", 0, 65535, nullptr},
    {"INVCELLSPER", 0, 65535, nullptr},
    {"INVVLEVEL", 0, 65535, nullptr},
    {"INVCAPACITY", 0, 65535, nullptr},
    {"INVBTYPE", 0, 255, nullptr},
    {"CANFREQ", 0, 40, nullptr},
    {"CANFDFREQ", 0, 40, nullptr},
    {"PRECHGMS", 0, 120000, nullptr},
    {"PWMFREQ", 0, 65535, nullptr},
    {"PWMHOLD", 0, 1023, nullptr},
    {"GTWCOUNTRY", 0, 65535, nullptr},
    {"GTWMAPREG", 0, 9, nullptr},
    {"GTWCHASSIS", 0, 9, nullptr},
    {"GTWPACK", 0, 9, nullptr},
    {"LEDMODE", 0, 10, nullptr},
    {"BATTERY_WH_MAX", 1, 400000, 
        [](uint32_t value) { datalayer.battery.info.total_capacity_Wh = value; }
    },
    {"GPIOOPT1", 0, 255, nullptr},
    {"GPIOOPT2", 0, 255, nullptr},
    {"GPIOOPT3", 0, 255, nullptr},
    {"TMP_CALTARGETSOC", 0, 100,
        [](uint32_t value) { datalayer_extended.bydAtto3.calibrationTargetSOC = value; }
    },
    {"TMP_CALTARGETAH", 0, 1000,
        [](uint32_t value) { datalayer_extended.bydAtto3.calibrationTargetAH = value; }
    },
    {"TMP_FAKEBATTERYV", 0, 1000,
        [](uint32_t value) { if(battery) battery->set_fake_voltage(value); }
    },
    {"TMP_BALFLOATPOWER", 0, UINT32_MAX,
        [](uint32_t value) { datalayer.battery.settings.balancing_float_power_W = value; }
    },
    {"TMP_BALMAXPACKV", 0, UINT32_MAX,
        [](uint32_t value) { datalayer.battery.settings.balancing_max_pack_voltage_dV = value; }
    },
    {"TMP_BALMAXCELLV", 0, UINT32_MAX,
        [](uint32_t value) { datalayer.battery.settings.balancing_max_cell_voltage_mV = value; }
    },
    {"TMP_BALMAXDEVCELLV", 0, UINT32_MAX,
        [](uint32_t value) { datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = value; }
    },
    {nullptr, 0, 0, nullptr}
};

struct FloatToUintSetting {
    const char* name;
    float min;
    float max;
    float scale;
    void (*update_func)(uint32_t value);
};

const FloatToUintSetting FLOAT_TO_UINT_SETTINGS[] = {
    {"BATTPVMAX", 0.0f, 1000.0f, 10.0f, nullptr},
    {"BATTPVMIN", 0.0f, 1000.0f, 10.0f, nullptr},
    {"MAXPERCENTAGE", 0.0f, 100.0f, 100.0f, 
        [](uint32_t value) { datalayer.battery.settings.max_percentage = value; }
    },
    {"MINPERCENTAGE", 0.0f, 100.0f, 100.0f,
        [](uint32_t value) { datalayer.battery.settings.min_percentage = value; }
    },
    {"MAXCHARGEAMP", 0.0f, 100.0f, 10.0f,
        [](uint32_t value) { datalayer.battery.settings.max_user_set_charge_dA = value; }
    },
    {"MAXDISCHARGEAMP", 0.0f, 100.0f, 10.0f,
        [](uint32_t value) { datalayer.battery.settings.max_user_set_discharge_dA = value; }
    },
    {"TARGETCHVOLT", 0.0f, 1000.0f, 10.0f,
        [](uint32_t value) { datalayer.battery.settings.max_user_set_charge_voltage_dV = value; }
    },
    {"TARGETDISCHVOLT", 0.0f, 1000.0f, 10.0f,
        [](uint32_t value) { datalayer.battery.settings.max_user_set_discharge_voltage_dV = value; }
    },
    {"TMP_BALTIME", 0, UINT32_MAX/60000.0f, 60000.0f,
        [](uint32_t value) { datalayer.battery.settings.balancing_max_time_ms = value; }
    },
    {nullptr, 0.0f, 0.0f, 0.0f, nullptr}
};

struct FloatSetting {
    const char* name;
    float min;
    float max;
    void (*update_func)(float value);
};

const FloatSetting FLOAT_SETTINGS[] = {
    {"TMP_CHARGERSETPOINTV", 0.0f, 1000.0f,
        [](float value) { 
            if(value >= CHARGER_MIN_HV && value <= CHARGER_MAX_HV) {
                datalayer.charger.charger_setpoint_HV_VDC = value; 
            }
        }
    },
    {"TMP_CHARGERSETPOINTA", 0.0f, 100.0f,
        [](float value) { 
            if((value <= CHARGER_MAX_A) && (value <= datalayer.battery.settings.max_user_set_charge_dA) &&
                 (value * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER)) {
                datalayer.charger.charger_setpoint_HV_IDC = value; 
            }
        }
    },
    {"TMP_CHARGERENDA", 0.0f, 100.0f,
        [](float value) { datalayer.charger.charger_setpoint_HV_IDC_END = value; }
    },
    {nullptr, 0.0f, 0.0f, nullptr}
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

struct BoolSetting {
    const char* name;
    void (*update_func)(bool value);
};

const BoolSetting BOOL_SETTINGS[] = {
    {"DBLBTR", nullptr},
    {"CNTCTRL", nullptr},
    {"CNTCTRLDBL", nullptr},
    {"PWMCNTCTRL", nullptr},
    {"PERBMSRESET", nullptr},
    {"REMBMSRESET", nullptr},
    {"EXTPRECHARGE", nullptr},
    {"NOINVDISC", nullptr},
    {"CANFDASCAN", nullptr},
    {"WIFIAPENABLED", nullptr},
    {"STATICIP", nullptr},
    {"PERFPROFILE", nullptr},
    {"CANLOGUSB", nullptr},
    {"USBENABLED", nullptr},
    {"WEBENABLED", nullptr},
    {"CANLOGSD", nullptr},
    {"SDLOGENABLED", nullptr},
    {"MQTTENABLED", nullptr},
    {"MQTTTOPICS", nullptr},
    {"MQTTCELLV", nullptr},
    {"HADISC", nullptr},
    {"INVICNT", nullptr},
    {"DEYEBYD", nullptr},
    {"INTERLOCKREQ", nullptr},
    {"DIGITALHVIL", nullptr},
    {"GTWRHD", nullptr},
    {"SOCESTIMATED", nullptr},
    {"PYLONOFFSET", nullptr},
    {"PYLONORDER", nullptr},
    {"NCCONTACTOR", nullptr},
    {"TRIBTR", nullptr},
    {"CNTCTRLTRI", nullptr},
    {"ESPNOWENABLED", nullptr},
    {"USE_SCALED_SOC",
        [](bool value) { datalayer.battery.settings.soc_scaling_active = value; }
    },    
    {"USEVOLTLIMITS", nullptr},
    {"TMP_RECOVERYMODE", [] (bool value) { datalayer.battery.settings.user_requests_forced_charging_recovery_mode = value; } },
    {"TMP_BALANCE", [] (bool value) { datalayer.battery.settings.user_requests_balancing = value; } },
    {"TMP_CHARGERHVENABLED", [] (bool value) { datalayer.charger.charger_HV_enabled = value; } },
    {"TMP_CHARGERAUX12VENABLED", [] (bool value) { datalayer.charger.charger_aux12V_enabled = value; } },
    {nullptr, nullptr}
};

const char* DEFAULT_TRUE_BOOL_SETTINGS[] = {
    "WIFIAPENABLED", nullptr
};

bool isVolatileSetting(const char* name) {
    // Volatile settings start with the TMP_ prefix, and are not saved to flash.
    if(strncmp(name, "TMP_", 4) == 0) return true;
    return false;
}

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
                        if(!isVolatileSetting(UINT_SETTINGS[i].name))
                            settings.saveUInt(UINT_SETTINGS[i].name, val);
                        if(UINT_SETTINGS[i].update_func) UINT_SETTINGS[i].update_func(val);
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
                        if(!isVolatileSetting(FLOAT_TO_UINT_SETTINGS[i].name))
                            settings.saveUInt(FLOAT_TO_UINT_SETTINGS[i].name, val);
                        if(FLOAT_TO_UINT_SETTINGS[i].update_func) FLOAT_TO_UINT_SETTINGS[i].update_func(val);
                    }
                } else {
                    errors[FLOAT_TO_UINT_SETTINGS[i].name] = "Invalid value.";
                }
            }
        }
        for(int i=0;FLOAT_SETTINGS[i].name!=nullptr;i++) {
            if(doc[FLOAT_SETTINGS[i].name].is<const char*>()) {
                char *end = nullptr;
                float val = strtof(doc[FLOAT_SETTINGS[i].name].as<const char*>(), &end);
                if(end && *end==0) {
                    if(val < FLOAT_SETTINGS[i].min || val > FLOAT_SETTINGS[i].max) {
                        errors[FLOAT_SETTINGS[i].name] = "Value out of range.";
                    } else if(attempt==1) {
                        // No float settings are saved to flash currently
                        if(FLOAT_SETTINGS[i].update_func) FLOAT_SETTINGS[i].update_func(val);
                    }
                } else {
                    errors[FLOAT_SETTINGS[i].name] = "Invalid value.";
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
        for(int i=0;BOOL_SETTINGS[i].name!=nullptr;i++) {
            if(doc[BOOL_SETTINGS[i].name].is<const char*>()) {
                const char *val = doc[BOOL_SETTINGS[i].name].as<const char*>();
                bool bval = (strcmp(val, "true")==0 || strcmp(val, "1")==0);
                if(attempt==1) {
                    if(!isVolatileSetting(BOOL_SETTINGS[i].name))
                        settings.saveBool(BOOL_SETTINGS[i].name, bval);
                    if(BOOL_SETTINGS[i].update_func) BOOL_SETTINGS[i].update_func(bval);
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
    for(int i=0;BOOL_SETTINGS[i].name!=nullptr;i++) {
        bool def = false;
        for(int j=0;DEFAULT_TRUE_BOOL_SETTINGS[j]!=nullptr;j++) {
            if(strcmp(BOOL_SETTINGS[i].name, DEFAULT_TRUE_BOOL_SETTINGS[j])==0) { def = true; break; }
        }
        sets[BOOL_SETTINGS[i].name] = settings.getBool(BOOL_SETTINGS[i].name, def);
    }

    // All the volatile settings that aren't stored in flash.

    sets["TMP_CALTARGETSOC"] = datalayer_extended.bydAtto3.calibrationTargetSOC;
    sets["TMP_CALTARGETAH"] = datalayer_extended.bydAtto3.calibrationTargetAH;
    if(battery) sets["TMP_FAKEBATTERYV"] = battery->get_voltage();
    sets["TMP_BALTIME"] = datalayer.battery.settings.balancing_max_time_ms / 60000.0f;
    sets["TMP_BALFLOATPOWER"] = datalayer.battery.settings.balancing_float_power_W;
    sets["TMP_BALMAXPACKV"] = datalayer.battery.settings.balancing_max_pack_voltage_dV;
    sets["TMP_BALMAXCELLV"] = datalayer.battery.settings.balancing_max_cell_voltage_mV;
    sets["TMP_BALMAXDEVCELLV"] = datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV;
    
    sets["TMP_CHARGERSETPOINTV"] = datalayer.charger.charger_setpoint_HV_VDC;
    sets["TMP_CHARGERSETPOINTA"] = datalayer.charger.charger_setpoint_HV_IDC;
    sets["TMP_CHARGERENDA"] = datalayer.charger.charger_setpoint_HV_IDC_END;

    sets["TMP_RECOVERYMODE"] = datalayer.battery.settings.user_requests_forced_charging_recovery_mode;
    sets["TMP_BALANCE"] = datalayer.battery.settings.user_requests_balancing;
    sets["TMP_CHARGERHVENABLED"] = datalayer.charger.charger_HV_enabled;
    sets["TMP_CHARGERAUX12VENABLED"] = datalayer.charger.charger_aux12V_enabled;

    doc["reboot_required"] = settingsUpdated;
})).use(*new TwsPostBufferingRequestHandler(settingsFullPostBody));
