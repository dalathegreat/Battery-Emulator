#include "TinyWebServer.h"
#include "TwsBuffering.h"
#include "BasicAuth.h"
#include "webserver_utils.h"
#include <src/battery/BATTERIES.h>
#include <src/datalayer/datalayer.h>
#include <src/devboard/hal/hal.h>
#include <src/devboard/safety/safety.h>
#include <src/devboard/utils/millis64.h>
#include <src/inverter/INVERTERS.h>
#include "esp_wifi.h"
#include <algorithm>

extern const char* version_number;
extern bool wifiap_enabled;
extern bool emulator_pause_request_ON;
extern bool contactor_control_enabled;
extern int contactorStatus;
extern BatteryType user_selected_battery_type;
extern const char* HTTP_404;

static const char* get_inverter_status() {
    if(get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state == EVENT_STATE_ACTIVE || get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state == EVENT_STATE_ACTIVE_LATCHED) {
        return "ERROR";
    } else if(!datalayer.system.status.inverter_allows_contactor_closing) {
        return "INACTIVE";
    } else {
        return "OK";
    }
}

TwsRoute apiBatOldRoute = TwsRoute("/api/batold", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully(HTTP_RESPONSE);
    if(battery) {
        auto &renderer = battery->get_status_renderer();
        auto response = std::make_shared<String>(renderer.get_status_html());
        request.set_writer_callback(StringWriter(response));
    }
}));//.use(*new BasicAuth());

TwsRoute statusRoute("/api/status", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
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
        wifi_config_t info;
        if (esp_wifi_get_config(WIFI_IF_AP, &info) == ESP_OK) {
            doc["ap_ssid"] = (info.ap.ssid_len > 0) ? String((char*)info.ap.ssid, info.ap.ssid_len) : String((char*)info.ap.ssid);
        }
        doc["ap_ip"] = WiFi.softAPIP().toString();
    }

    doc["status"] = get_emulator_status_string(get_emulator_status());
    doc["bms_status"] = getBMSStatus(datalayer.battery.status.bms_status);
    doc["pause_status"] = get_emulator_pause_status();
    doc["inverter_status"] = get_inverter_status();
    doc["real_bms_status"] = datalayer.battery.status.real_bms_status;
    doc["pause"] = emulator_pause_request_ON;
    doc["estop"] = datalayer.system.info.equipment_stop_active;

    JsonArray batteries = doc["battery"].to<JsonArray>();
    auto add_battery = [&](auto& bat_data) {
        JsonObject bat = batteries.add<JsonObject>();
        bat["protocol"] = datalayer.system.info.battery_protocol;
        bat["chemistry"] = bat_data.info.chemistry;
        bat["soc_scaling"] = bat_data.settings.soc_scaling_active;
        bat["real_soc"] = static_cast<float>(bat_data.status.real_soc) / 100.0f;
        bat["reported_soc"] = static_cast<float>(bat_data.status.reported_soc) / 100.0f;
        bat["soh"] = static_cast<float>(bat_data.status.soh_pptt) / 100.0f;
        bat["v"] = static_cast<float>(bat_data.status.voltage_dV) / 10.0f;
        bat["i"] = static_cast<float>(bat_data.status.current_dA) / 10.0f;
        bat["p"] = bat_data.status.active_power_W;
        bat["total_capacity"] = bat_data.info.total_capacity_Wh;
        bat["reported_total_capacity"] = bat_data.info.reported_total_capacity_Wh;
        bat["remaining_capacity"] = bat_data.status.remaining_capacity_Wh;
        bat["reported_remaining_capacity"] = bat_data.status.reported_remaining_capacity_Wh;
        bat["temp_max"] = static_cast<float>(bat_data.status.temperature_max_dC) / 10.0f;
        bat["temp_min"] = static_cast<float>(bat_data.status.temperature_min_dC) / 10.0f;
        bat["charge_i_max"] = static_cast<float>(bat_data.status.max_charge_current_dA) / 10.0f;
        bat["discharge_i_max"] = static_cast<float>(bat_data.status.max_discharge_current_dA) / 10.0f;
        bat["charge_p_max"] = bat_data.status.max_charge_power_W;
        bat["discharge_p_max"] = bat_data.status.max_discharge_power_W;
        bat["cell_mv_max"] = bat_data.status.cell_max_voltage_mV;
        bat["cell_mv_min"] = bat_data.status.cell_min_voltage_mV;
        bat["inverter_limits_discharge"] = bat_data.settings.inverter_limits_discharge;
        bat["user_settings_limit_discharge"] = bat_data.settings.user_settings_limit_discharge;
        bat["inverter_limits_charge"] = bat_data.settings.inverter_limits_charge;
        bat["user_settings_limit_charge"] = bat_data.settings.user_settings_limit_charge;
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
}));

TwsRoute cellsRoute("/api/cells", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
    JsonArray batteryArr = doc["battery"].to<JsonArray>();
    JsonObject bat = batteryArr.add<JsonObject>();
    bat["temp_min"] = static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0f;
    bat["temp_max"] = static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0f;
    JsonArray data = bat["voltages"].to<JsonArray>();
    for(size_t i=0;i<datalayer.battery.info.number_of_cells && i<MAX_AMOUNT_CELLS;i++) {
        data.add(datalayer.battery.status.cell_voltages_mV[i]);
    }
    if(battery2) {
        JsonObject bat2 = batteryArr.add<JsonObject>();
        bat2["temp_min"] = static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0f;
        bat2["temp_max"] = static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0f;
        JsonArray data2 = bat2["voltages"].to<JsonArray>();
        for(size_t i=0;i<datalayer.battery2.info.number_of_cells && i<MAX_AMOUNT_CELLS;i++) {
            data2.add(datalayer.battery2.status.cell_voltages_mV[i]);
        }
    }
}));

TwsRoute batextRoute("/api/batext", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/octet-stream\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
    uint32_t btype = (uint32_t)user_selected_battery_type;
    request.write((const char*)&btype, 4);

    if(user_selected_battery_type==BatteryType::BoltAmpera) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.boltampera, sizeof(datalayer_extended.boltampera)));
    else if(user_selected_battery_type==BatteryType::BmwPhev) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.bmwphev, sizeof(datalayer_extended.bmwphev)));
    else if(user_selected_battery_type==BatteryType::BydAtto3) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.bydAtto3, sizeof(datalayer_extended.bydAtto3)));
    else if(user_selected_battery_type==BatteryType::CellPowerBms) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.cellpower, sizeof(datalayer_extended.cellpower)));
    else if(user_selected_battery_type==BatteryType::Chademo) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.chademo, sizeof(datalayer_extended.chademo)));
    else if(user_selected_battery_type==BatteryType::CmfaEv) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.CMFAEV, sizeof(datalayer_extended.CMFAEV)));
    else if(user_selected_battery_type==BatteryType::StellantisEcmp) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.stellantisECMP, sizeof(datalayer_extended.stellantisECMP)));
    else if(user_selected_battery_type==BatteryType::GeelyGeometryC) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.geometryC, sizeof(datalayer_extended.geometryC)));
    else if(user_selected_battery_type==BatteryType::KiaHyundai64) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.KiaHyundai64, sizeof(datalayer_extended.KiaHyundai64)));
    else if(user_selected_battery_type==BatteryType::TeslaModel3Y || user_selected_battery_type==BatteryType::TeslaModelSX) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.tesla, sizeof(datalayer_extended.tesla)));
    else if(user_selected_battery_type==BatteryType::NissanLeaf) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.nissanleaf, sizeof(datalayer_extended.nissanleaf)));
    else if(user_selected_battery_type==BatteryType::Meb) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.meb, sizeof(datalayer_extended.meb)));
    else if(user_selected_battery_type==BatteryType::VolvoSpa) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.VolvoPolestar, sizeof(datalayer_extended.VolvoPolestar)));
    else if(user_selected_battery_type==BatteryType::VolvoSpaHybrid) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.VolvoHybrid, sizeof(datalayer_extended.VolvoHybrid)));
    else if(user_selected_battery_type==BatteryType::RenaultZoe1) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.zoe, sizeof(datalayer_extended.zoe)));
    else if(user_selected_battery_type==BatteryType::RenaultZoe2) request.set_writer_callback(CharBufWriter((const char*)&datalayer_extended.zoePH2, sizeof(datalayer_extended.zoePH2)));
}));


TwsRoute batcmdRoute = TwsRoute("/api/batteries/*", new TwsJsonRestFunc([](TwsRequest& request, JsonDocument& doc) {
    std::string_view wildcard(request.get_path_wildcard());

    auto add_battery_commands = [](JsonObject &commands, Battery* battery) {
        if(battery->supports_reset_SOH()) commands["reset_soh"] = true;
        if(battery->supports_reset_crash()) commands["reset_crash"] = true;
        if(battery->supports_clear_isolation()) commands["clear_isolation"] = true;
        if(battery->supports_reset_BMS()) commands["reset_bms"] = true;
        if(battery->supports_reset_SOC()) commands["reset_soc"] = true;
        if(battery->supports_reset_NVROL()) commands["reset_nvrol"] = true;
        if(battery->supports_reset_DTC()) commands["reset_dtc"] = true;
        if(battery->supports_read_DTC()) commands["read_dtc"] = true;
        if(battery->supports_reset_BECM()) commands["reset_becm"] = true;
        if(battery->supports_calibrate_SOC()) commands["calibrate_soc"] = true;
        if(battery->supports_contactor_close()) commands["contactor_close"] = true;
        if(battery->supports_contactor_reset()) commands["contactor_reset"] = true;
        //if(battery->supports_set_fake_voltage()) commands["set_fake_voltage"] = true;
        //if(battery->supports_manual_balancing()) commands["manual_balancing"] = true;
        if(battery->supports_real_BMS_status()) commands["real_bms_status"] = true;
        if(battery->supports_toggle_SOC_method()) commands["toggle_soc_method"] = true;
        if(battery->supports_energy_saving_mode_reset()) commands["energy_saving_mode_reset"] = true;
        if(battery->supports_factory_mode_method()) commands["factory_mode_method"] = true;
        if(battery->supports_chademo_restart()) commands["chademo_restart"] = true;
        if(battery->supports_chademo_stop()) commands["chademo_stop"] = true;
        if(battery->supports_balancing()) commands["balancing"] = true;
        if(battery->is_balancing_active()) commands["balancing_active"] = true;
    };

    if(wildcard.empty()) {
        JsonArray batteries = doc["battery"].to<JsonArray>();
        if(battery) {
            JsonObject bat = batteries.add<JsonObject>();
            bat["id"] = "1";
            JsonObject commands = bat["commands"].to<JsonObject>();
            add_battery_commands(commands, battery);
        }
        if(battery2) {
            JsonObject bat = batteries.add<JsonObject>();
            bat["id"] = "2";
            JsonObject commands = bat["commands"].to<JsonObject>();
            add_battery_commands(commands, battery2);
        }
        if(battery3) {
            JsonObject bat = batteries.add<JsonObject>();
            bat["id"] = "3";
            JsonObject commands = bat["commands"].to<JsonObject>();
            add_battery_commands(commands, battery3);
        }
    }
}, []( TwsRequest& request, uint8_t* data, size_t len) {
    std::string_view wildcard(request.get_path_wildcard());

    int first_slash = wildcard.find('/');
    if (first_slash == -1) {
        request.write_fully(HTTP_404);
        request.finish();
        return true; // The request is now finished
    }

    int second_slash = wildcard.find('/', first_slash + 1);

    std::string_view id_part = wildcard.substr(0, first_slash);
    std::string_view action_part = wildcard.substr(
        first_slash + 1, 
        second_slash == -1 ? std::string_view::npos : (second_slash - first_slash - 1)
    );

    Battery *bat;
    if(battery && id_part=="1") bat = battery;
    else if(battery2 && id_part=="2") bat = battery2;
    else if(battery3 && id_part=="3") bat = battery3;
    else {
        request.write_fully(HTTP_404);
        request.finish();
        return true; // The request is now finished
    }

    if(action_part=="reset_soh" && bat->supports_reset_SOH()) {
        bat->reset_SOH();
    } else if(action_part=="reset_crash" && bat->supports_reset_crash()) {
        bat->reset_crash();
    } else if(action_part=="clear_isolation" && bat->supports_clear_isolation()) {
        bat->clear_isolation();
    } else if(action_part=="reset_bms" && bat->supports_reset_BMS()) {
        bat->reset_BMS();
    } else if(action_part=="reset_soc" && bat->supports_reset_SOC()) {
        bat->reset_SOC();
    } else if(action_part=="reset_nvrol" && bat->supports_reset_NVROL()) {
        bat->reset_NVROL();
    } else if(action_part=="reset_dtc" && bat->supports_reset_DTC()) {
        bat->reset_DTC();
    } else if(action_part=="read_dtc" && bat->supports_read_DTC()) {
        bat->read_DTC();
    } else if(action_part=="reset_becm" && bat->supports_reset_BECM()) {
        bat->reset_BECM();
    } else if(action_part=="calibrate_soc" && bat->supports_calibrate_SOC()) {
        bat->calibrate_SOC();
    } else if(action_part=="contactor_close" && bat->supports_contactor_close()) {
        bat->request_close_contactors();
    } else if(action_part=="contactor_open" && bat->supports_contactor_close()) {
        bat->request_open_contactors();
    } else if(action_part=="contactor_reset" && bat->supports_contactor_reset()) {
        bat->request_open_contactors();
    } else if(action_part=="toggle_soc_method" && bat->supports_toggle_SOC_method()) {
        bat->toggle_SOC_method();
    } else if(action_part=="energy_saving_mode_reset" && bat->supports_energy_saving_mode_reset()) {
        bat->reset_energy_saving_mode();
    } else if(action_part=="factory_mode_method" && bat->supports_factory_mode_method()) {
        bat->set_factory_mode();
    } else if(action_part=="chademo_restart" && bat->supports_chademo_restart()) {
        bat->chademo_restart();
    } else if(action_part=="chademo_stop" && bat->supports_chademo_stop()) {
        bat->chademo_stop();
    } else if(action_part=="start_balancing" && bat->supports_balancing()) {
        bat->initiate_balancing();
    } else if(action_part=="stop_balancing" && bat->supports_balancing()) {
        if(bat->is_balancing_active()) bat->end_balancing();
    } else {
        // The original path is NUL terminated so this is OK
        auto response = std::make_shared<String>(bat->handle_custom_command(action_part.data(), data, len, request.get_connection_id()));
        if(*response == "__SSE__") {
            // Start a SSE stream.
            request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/event-stream\r\n\r\n");
        } else if(*response == "__STREAM__") {
            // Start a generic stream.
            request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/octet-stream\r\n\r\n");
        } else {
            // Send a JSON response.
            request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n");
            request.set_writer_callback(StringWriter(response));
        }
        return true; // The request is now finished
    }

    request.write_fully(HTTP_204);
    request.finish();
    return true; // The request is now finished
}));

TwsRoute eventsRoute("/api/events", new TwsJsonGetFunc([](TwsRequest& request, JsonDocument& doc) {
    JsonArray events = doc["events"].to<JsonArray>();
    std::vector<EventData> order_events;
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
        auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
        if (event_pointer->occurences > 0) order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
    std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampDesc);
    uint64_t current_timestamp = millis64();
    for (const auto& event : order_events) {
        JsonObject ev = events.add<JsonObject>();
        ev["type"] = get_event_enum_string(event.event_handle);
        ev["level"] = get_event_level_string(event.event_handle);
        ev["age"] = current_timestamp - event.event_pointer->timestamp;
        ev["count"] = event.event_pointer->occurences;
        ev["data"] = event.event_pointer->data;
        ev["message"] = get_event_message_string(event.event_handle);
    }
}));

TwsRoute eventsClearRoute("/api/events/clear", new TwsRequestHandlerFunc([](TwsRequest& request) {
    if(!request.is_post()) { request.write_fully(HTTP_405); return; }
    reset_all_events();
    request.write_fully(HTTP_204);
}));

TwsRoute rebootRoute("/api/reboot", new TwsRequestHandlerFunc([](TwsRequest& request) {
    if(!request.is_post()) { request.write_fully(HTTP_405); return; }
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nRebooting server...\n");
    setBatteryPause(true, true, true, false);
    delay(1000);
    ESP.restart();
}));

TwsRoute pauseRoute("/api/pause", new TwsRawPostFunc([](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> int {
    if(len==1 && data[0]=='1') setBatteryPause(true, false);
    else if(len==1 && data[0]=='0') setBatteryPause(false, false);
    request.write_fully(HTTP_204);
    request.finish();
    return len;
}));

TwsRoute estopRoute("/api/estop", new TwsRawPostFunc([](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> int {
    if(len==1 && data[0]=='1') setBatteryPause(true, false, true);
    else if(len==1 && data[0]=='0') setBatteryPause(false, false, false);
    request.write_fully(HTTP_204);
    request.finish();
    return len;
}));

TwsRoute statsRoute("/api/stats", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n");
    auto response = std::make_shared<String>();
    response->reserve(5000);
    char output[3000] = {0};
    vTaskList(output);
    response->concat(output);
    response->concat("\n");
    vTaskGetRunTimeStats(output);
    response->concat(output);
    request.set_writer_callback(StringWriter(response));
}));
