// pauLTU3
// CYD ESP-NOW receiver and UI mapping logic.
// Testing-only build.
// This implementation was developed for bench testing and has not been validated on a real system.
// Remote-control features must be treated as experimental until fully verified on hardware.

#include "espnow_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <lvgl.h>

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "screen_network.h"
#include "ui.h"

namespace {

// Firmware version shown on the main screens.
constexpr const char *kScreenVersionText = "V1.0(Testing)";

// Packet type identifiers sent by the Battery Emulator.
// These values must stay aligned with the sender side.
enum espnow_message_enum : uint8_t {
    BAT_INFO = 1,
    BAT_STATUS = 2,
    BAT_BALANCE = 3,
    BAT_CELL_STATUS = 4,
    BAT_CELL_STATUS_16BIT = 5,
    BAT_FAULT_TEXT = 6,
    BAT_REMOTE_COMMAND = 7,
    BAT_REMOTE_STATE = 8,
    BAT_FAULT_EVENTS = 9,
    BAT_AUX_INFO = 10,
    BAT_REMOTE_NET_INFO = 11
};

// Remote command identifiers sent from the secret screen back to the emulator.
enum espnow_remote_command_id : uint8_t {
    REMOTE_CMD_SET_PAUSE = 1,
    REMOTE_CMD_SET_CONTACTORS = 2,
    REMOTE_CMD_REBOOT = 3
};

// Result codes returned by the emulator inside remote state ACK fields.
enum espnow_remote_command_result : uint8_t {
    REMOTE_CMD_RESULT_NONE = 0,
    REMOTE_CMD_RESULT_OK = 1,
    REMOTE_CMD_RESULT_INVALID = 2
};

#define GENERATE_STRING(STRING) #STRING,
#define RECEIVER_EVENTS_ENUM_TYPE(XX)       \
    XX(EVENT_CANMCP2518FD_INIT_FAILURE)     \
    XX(EVENT_CANMCP2515_INIT_FAILURE)       \
    XX(EVENT_CANFD_BUFFER_FULL)             \
    XX(EVENT_CAN_BUFFER_FULL)               \
    XX(EVENT_CAN_CORRUPTED_WARNING)         \
    XX(EVENT_CAN_BATTERY_MISSING)           \
    XX(EVENT_CAN_BATTERY2_MISSING)          \
    XX(EVENT_CAN_BATTERY3_MISSING)          \
    XX(EVENT_CAN_CHARGER_MISSING)           \
    XX(EVENT_CAN_INVERTER_MISSING)          \
    XX(EVENT_CAN_NATIVE_TX_FAILURE)         \
    XX(EVENT_CHARGE_LIMIT_EXCEEDED)         \
    XX(EVENT_CONTACTOR_WELDED)              \
    XX(EVENT_CONTACTOR_OPEN)                \
    XX(EVENT_DISCHARGE_LIMIT_EXCEEDED)      \
    XX(EVENT_WATER_INGRESS)                 \
    XX(EVENT_12V_LOW)                       \
    XX(EVENT_SOC_PLAUSIBILITY_ERROR)        \
    XX(EVENT_SOC_UNAVAILABLE)               \
    XX(EVENT_STALE_VALUE)                   \
    XX(EVENT_KWH_PLAUSIBILITY_ERROR)        \
    XX(EVENT_BALANCING_START)               \
    XX(EVENT_BALANCING_END)                 \
    XX(EVENT_BATTERY_EMPTY)                 \
    XX(EVENT_BATTERY_FULL)                  \
    XX(EVENT_BATTERY_FUSE)                  \
    XX(EVENT_BATTERY_FROZEN)                \
    XX(EVENT_BATTERY_CAUTION)               \
    XX(EVENT_BATTERY_CHG_STOP_REQ)          \
    XX(EVENT_BATTERY_DISCHG_STOP_REQ)       \
    XX(EVENT_BATTERY_CHG_DISCHG_STOP_REQ)   \
    XX(EVENT_BATTERY_OVERHEAT)              \
    XX(EVENT_BATTERY_OVERVOLTAGE)           \
    XX(EVENT_BATTERY_UNDERVOLTAGE)          \
    XX(EVENT_BATTERY_VALUE_UNAVAILABLE)     \
    XX(EVENT_BATTERY_ISOLATION)             \
    XX(EVENT_BATTERY_REQUESTS_HEAT)         \
    XX(EVENT_BATTERY_WARMED_UP)             \
    XX(EVENT_BATTERY_SOC_RECALIBRATION)     \
    XX(EVENT_BYD_AUTO_SOC_CALIBRATION)      \
    XX(EVENT_BATTERY_SOC_RESET_SUCCESS)     \
    XX(EVENT_BATTERY_SOC_RESET_FAIL)        \
    XX(EVENT_VOLTAGE_DIFFERENCE)            \
    XX(EVENT_SOH_DIFFERENCE)                \
    XX(EVENT_SOH_LOW)                       \
    XX(EVENT_HVIL_FAILURE)                  \
    XX(EVENT_LOW_HEAP_MEMORY)               \
    XX(EVENT_PRECHARGE_FAILURE)             \
    XX(EVENT_INTERNAL_OPEN_FAULT)           \
    XX(EVENT_INVERTER_OPEN_CONTACTOR)       \
    XX(EVENT_INTERFACE_MISSING)             \
    XX(EVENT_MODBUS_INVERTER_MISSING)       \
    XX(EVENT_NO_ENABLE_DETECTED)            \
    XX(EVENT_ERROR_OPEN_CONTACTOR)          \
    XX(EVENT_CELL_CRITICAL_UNDER_VOLTAGE)   \
    XX(EVENT_CELL_CRITICAL_OVER_VOLTAGE)    \
    XX(EVENT_CELL_UNDER_VOLTAGE)            \
    XX(EVENT_CELL_OVER_VOLTAGE)             \
    XX(EVENT_CELL_DEVIATION_HIGH)           \
    XX(EVENT_UNKNOWN_EVENT_SET)             \
    XX(EVENT_OTA_UPDATE)                    \
    XX(EVENT_OTA_UPDATE_TIMEOUT)            \
    XX(EVENT_DUMMY_INFO)                    \
    XX(EVENT_DUMMY_DEBUG)                   \
    XX(EVENT_DUMMY_WARNING)                 \
    XX(EVENT_DUMMY_ERROR)                   \
    XX(EVENT_PERSISTENT_SAVE_INFO)          \
    XX(EVENT_SERIAL_RX_WARNING)             \
    XX(EVENT_SERIAL_RX_FAILURE)             \
    XX(EVENT_SERIAL_TX_FAILURE)             \
    XX(EVENT_SERIAL_TRANSMITTER_FAILURE)    \
    XX(EVENT_SMA_PAIRING)                   \
    XX(EVENT_TASK_OVERRUN)                  \
    XX(EVENT_THERMAL_RUNAWAY)               \
    XX(EVENT_RECOVERY_START)                \
    XX(EVENT_RECOVERY_END)                  \
    XX(EVENT_RESET_UNKNOWN)                 \
    XX(EVENT_RESET_POWERON)                 \
    XX(EVENT_RESET_EXT)                     \
    XX(EVENT_RESET_SW)                      \
    XX(EVENT_RESET_PANIC)                   \
    XX(EVENT_RESET_INT_WDT)                 \
    XX(EVENT_RESET_TASK_WDT)                \
    XX(EVENT_RESET_WDT)                     \
    XX(EVENT_RESET_DEEPSLEEP)               \
    XX(EVENT_RESET_BROWNOUT)                \
    XX(EVENT_RESET_SDIO)                    \
    XX(EVENT_RESET_USB)                     \
    XX(EVENT_RESET_JTAG)                    \
    XX(EVENT_RESET_EFUSE)                   \
    XX(EVENT_RESET_PWR_GLITCH)              \
    XX(EVENT_RESET_CPU_LOCKUP)              \
    XX(EVENT_RJXZS_LOG)                     \
    XX(EVENT_PAUSE_BEGIN)                   \
    XX(EVENT_PAUSE_END)                     \
    XX(EVENT_PID_FAILED)                    \
    XX(EVENT_WIFI_CONNECT)                  \
    XX(EVENT_WIFI_DISCONNECT)               \
    XX(EVENT_MQTT_CONNECT)                  \
    XX(EVENT_MQTT_DISCONNECT)               \
    XX(EVENT_EQUIPMENT_STOP)                \
    XX(EVENT_AUTOMATIC_PRECHARGE_FAILURE)   \
    XX(EVENT_SD_INIT_FAILED)                \
    XX(EVENT_PERIODIC_BMS_RESET)            \
    XX(EVENT_PERIODIC_BMS_RESET_FAILURE)    \
    XX(EVENT_BMS_RESET_REQ_SUCCESS)         \
    XX(EVENT_BMS_RESET_REQ_FAIL)            \
    XX(EVENT_BATTERY_TEMP_DEVIATION_HIGH)   \
    XX(EVENT_GPIO_NOT_DEFINED)              \
    XX(EVENT_GPIO_CONFLICT)                 \
    XX(EVENT_NOF_EVENTS)

// Full Battery Emulator event list used to translate event_id into readable text.
// The order here must match the sender project exactly.
static const char *kEventNames[] = {RECEIVER_EVENTS_ENUM_TYPE(GENERATE_STRING)};

// Human-readable fault level names used by the fault text screen.
static const char *kEventLevels[] = {"INFO", "DEBUG", "WARNING", "ERROR", "UPDATE"};

// Battery-level status values carried in BAT_STATUS.
enum real_bms_status_enum {
    BMS_DISCONNECTED = 0,
    BMS_STANDBY = 1,
    BMS_ACTIVE = 2,
    BMS_FAULT = 3
};

// System-wide status values carried in BAT_REMOTE_STATE.
enum system_status_enum {
    SYS_STANDBY = 0,
    SYS_INACTIVE = 1,
    SYS_DARKSTART = 2,
    SYS_ACTIVE = 3,
    SYS_FAULT = 4,
    SYS_UPDATING = 5
};

// Battery info packet payload.
struct BATTERY_INFO_TYPE {
    uint32_t total_capacity_Wh = 30000;
    uint32_t reported_total_capacity_Wh = 30000;
    uint16_t max_design_voltage_dV = 5000;
    uint16_t min_design_voltage_dV = 2500;
    uint16_t max_cell_voltage_mV = 4300;
    uint16_t min_cell_voltage_mV = 2700;
    uint16_t max_cell_voltage_deviation_mV = 500;
    uint8_t number_of_cells = 0;
    int chemistry = 1;
};

// Main live battery state packet payload.
struct BATTERY_STATUS_TYPE {
    uint32_t remaining_capacity_Wh = 0;
    uint32_t reported_remaining_capacity_Wh = 0;
    uint32_t max_discharge_power_W = 0;
    uint32_t max_charge_power_W = 0;
    uint32_t override_discharge_power_W = 0;
    uint32_t override_charge_power_W = 0;
    int32_t active_power_W = 0;
    int32_t total_charged_battery_Wh = 0;
    int32_t total_discharged_battery_Wh = 0;
    uint16_t max_discharge_current_dA = 0;
    uint16_t max_charge_current_dA = 0;
    uint16_t soh_pptt = 0;
    uint16_t voltage_dV = 0;
    uint16_t cell_max_voltage_mV = 0;
    uint16_t cell_min_voltage_mV = 0;
    uint16_t real_soc = 0;
    uint16_t reported_soc = 0;
    uint16_t CAN_error_counter = 0;
    int16_t temperature_max_dC = 0;
    int16_t temperature_min_dC = 0;
    int16_t current_dA = 0;
    int16_t reported_current_dA = 0;
    uint8_t CAN_battery_still_alive = 0;
    int real_bms_status = BMS_DISCONNECTED;
    int led_mode = 0;
    int balancing_status = 0;
};

// Compact 8-bit cell-voltage payload.
// Each value is sent in 20 mV steps.
struct BATTERY_CELL_STATUS_TYPE {
    uint8_t cell_voltages_mV[192] = {0};
    uint8_t number_of_cells = 0;
};

// Chunked full-resolution 16-bit cell-voltage payload.
struct BATTERY_CELL_STATUS_16BIT_CHUNK_TYPE {
    uint8_t transfer_id;
    uint8_t total_cells;
    uint8_t start_index;
    uint8_t cells_in_chunk;
    uint16_t cell_voltages_mV[]; 
};

// Chunked fault-text transport payload.
struct BATTERY_FAULT_TEXT_CHUNK_TYPE {
    uint8_t transfer_id;
    uint8_t total_chunks;
    uint8_t chunk_index;
    uint8_t text_length;
    char text[];
};

// One structured fault/event record.
struct BATTERY_FAULT_EVENT_RECORD_TYPE {
    uint8_t event_id;
    uint8_t level;
    uint8_t data;
    uint8_t count;
    uint32_t age_seconds;
} __attribute__((packed));

// Chunked transport for structured fault records.
struct BATTERY_FAULT_EVENTS_CHUNK_TYPE {
    uint8_t transfer_id;
    uint8_t total_events;
    uint8_t start_index;
    uint8_t records_in_chunk;
    BATTERY_FAULT_EVENT_RECORD_TYPE records[];
} __attribute__((packed));

// Command packet sent from CYD to Battery Emulator.
struct BATTERY_REMOTE_COMMAND_TYPE {
    uint8_t command_id;
    uint8_t command_sequence;
    uint8_t command_value;
    uint8_t reserved;
} __attribute__((packed));

// Extra battery data that does not fit nicely into the main status packet.
struct BATTERY_AUX_INFO_TYPE {
    uint32_t isolation_resistance_kohm = 0;
} __attribute__((packed));

// Optional network info packet from the emulator side.
struct BATTERY_REMOTE_NET_INFO_TYPE {
    uint8_t sta_connected = 0;
    int16_t rssi_dbm = 0;
    char ip_address[16] = {0};
    char ssid[32] = {0};
} __attribute__((packed));

// Remote state packet used for secret-screen status and command ACK feedback.
struct BATTERY_REMOTE_STATE_TYPE {
    uint8_t ack_sequence;
    uint8_t ack_command_id;
    uint8_t ack_result;
    uint8_t pause_status;
    uint8_t pause_request_on;
    uint8_t equipment_stop_active;
    uint8_t system_status;
    uint8_t contactors_engaged;
    uint8_t inverter_allows_contactor_closing;
    uint8_t battery1_contactors_engaged;
    uint8_t battery2_contactors_engaged;
} __attribute__((packed));

// Common 4-byte ESP-NOW header present on every packet.
struct ESPNOW_HEADER {
    uint16_t emulator_id;
    uint8_t battery_id;
    uint8_t esp_message_type;
} __attribute__((packed));

// Full shared runtime state per battery slot.
// This is the data written by the ESP-NOW callback and consumed by the UI thread.
struct SharedBatteryState {
    bool has_info = false;
    bool has_status = false;
    bool has_cells = false;
    bool has_cells_16bit = false;
    bool has_fault_text = false;
    bool has_fault_events = false;
    bool has_aux_info = false;
    bool has_remote_net_info = false;
    bool has_remote_state = false;
    bool dirty = false;
    bool rx_event_pending = false;
    uint32_t last_packet_ms = 0;
    uint16_t emulator_id = 0;
    uint8_t battery_id = 0;
    uint8_t last_message_type = 0;
    int last_message_len = 0;
    uint32_t rx_packet_count = 0;
    uint32_t ignored_packet_count = 0;
    BATTERY_INFO_TYPE info;
    BATTERY_STATUS_TYPE status;
    BATTERY_AUX_INFO_TYPE aux_info;
    BATTERY_REMOTE_NET_INFO_TYPE remote_net_info;
    BATTERY_CELL_STATUS_TYPE cells;
    uint16_t cells_16bit_mV[192] = {0};
    uint32_t last_cells_16bit_ms = 0;
    uint8_t cell16_transfer_id = 0;
    uint8_t cell16_expected_chunks = 0;
    uint8_t cell16_received_chunks_mask = 0;
    uint8_t cell16_total_cells = 0;
    char fault_text[1536] = {0};
    uint32_t last_fault_text_ms = 0;
    uint8_t fault_transfer_id = 0;
    uint8_t fault_expected_chunks = 0;
    uint32_t fault_received_chunks_mask = 0;
    BATTERY_FAULT_EVENT_RECORD_TYPE fault_events[128] = {};
    uint8_t fault_event_count = 0;
    uint32_t last_fault_events_ms = 0;
    uint8_t fault_events_transfer_id = 0;
    BATTERY_REMOTE_STATE_TYPE remote_state{};
    uint32_t last_remote_state_ms = 0;
    uint8_t last_sent_command_sequence = 0;
    uint8_t pending_command_sequence = 0;
    uint8_t pending_command_id = 0;
    uint8_t pending_command_value = 0;
    uint32_t pending_command_sent_ms = 0;
    bool command_pending = false;
};

// Snapshot copied out of the shared state before updating LVGL.
// This avoids keeping the critical section locked during UI work.
struct UiBatteryState {
    bool has_info = false;
    bool has_status = false;
    bool has_cells = false;
    bool has_cells_16bit = false;
    bool has_fault_text = false;
    bool has_fault_events = false;
    bool has_aux_info = false;
    bool has_remote_net_info = false;
    bool has_remote_state = false;
    uint32_t last_packet_ms = 0;
    uint16_t emulator_id = 0;
    uint8_t battery_id = 0;
    uint8_t last_message_type = 0;
    int last_message_len = 0;
    uint32_t rx_packet_count = 0;
    uint32_t ignored_packet_count = 0;
    bool rx_event_pending = false;
    BATTERY_INFO_TYPE info;
    BATTERY_STATUS_TYPE status;
    BATTERY_AUX_INFO_TYPE aux_info;
    BATTERY_REMOTE_NET_INFO_TYPE remote_net_info;
    BATTERY_CELL_STATUS_TYPE cells;
    uint16_t cells_16bit_mV[192] = {0};
    uint32_t last_cells_16bit_ms = 0;
    uint8_t cell16_total_cells = 0;
    char fault_text[1536] = {0};
    uint32_t last_fault_text_ms = 0;
    BATTERY_FAULT_EVENT_RECORD_TYPE fault_events[128] = {};
    uint8_t fault_event_count = 0;
    uint32_t last_fault_events_ms = 0;
    BATTERY_REMOTE_STATE_TYPE remote_state{};
    uint32_t last_remote_state_ms = 0;
    uint8_t last_sent_command_sequence = 0;
    uint8_t pending_command_sequence = 0;
    uint8_t pending_command_id = 0;
    uint8_t pending_command_value = 0;
    uint32_t pending_command_sent_ms = 0;
    bool command_pending = false;
};

// Smaller view used when only remote-state data is needed.
struct UiRemoteStateView {
    bool has_remote_state = false;
    uint32_t last_remote_state_ms = 0;
    BATTERY_REMOTE_STATE_TYPE remote_state{};
    uint8_t pending_command_id = 0;
    uint32_t pending_command_sent_ms = 0;
    bool command_pending = false;
};

// Receiver channel expected by this screen.
constexpr uint8_t ESPNOW_WIFI_CHANNEL = 1;

// Broadcast peer used both for receive and command send path.
uint8_t g_broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Packet freshness and UI timing constants.
constexpr uint32_t PACKET_TIMEOUT_MS = 3000;
constexpr uint32_t HIGH_RES_CELL_TIMEOUT_MS = 5000;
constexpr uint32_t FAULT_TEXT_TIMEOUT_MS = 5000;
constexpr uint32_t FAULT_EVENTS_TIMEOUT_MS = 5000;
constexpr uint32_t REMOTE_STATE_TIMEOUT_MS = 5000;
constexpr uint32_t COMMAND_ACK_TIMEOUT_MS = 2000;
constexpr uint32_t UI_STALE_HOLD_MS = 3000;
constexpr uint16_t MAX_UI_CHART_POINTS = 192;
constexpr uint8_t BATTERY_SLOT_COUNT = 2;
constexpr size_t ESPNOW_HEADER_SIZE = sizeof(ESPNOW_HEADER);
constexpr uint32_t RX_LOG_INTERVAL_MS = 1000;
constexpr uint8_t RED_LED_PIN = 4;
constexpr uint8_t GREEN_LED_PIN = 16;
constexpr uint8_t BLUE_LED_PIN = 17;

// Critical section used to protect shared state between callback and main loop.
portMUX_TYPE g_state_mux = portMUX_INITIALIZER_UNLOCKED;

// Two battery slots:
// slot 0 -> battery_id 1
// slot 1 -> battery_id 2
SharedBatteryState g_shared_states[BATTERY_SLOT_COUNT];

// One chart series per battery cells screen.
lv_chart_series_t *g_chart_series[BATTERY_SLOT_COUNT] = {nullptr, nullptr};

// External arrays used by LVGL charts.
lv_coord_t g_cell_chart_values[BATTERY_SLOT_COUNT][MAX_UI_CHART_POINTS] = {};

// Timing/state helpers for log throttling and UI behavior.
uint32_t g_last_rx_log_ms = 0;
uint32_t g_last_ui_log_ms = 0;
bool g_waiting_log_printed = false;
int g_last_led_status = -999;
bool g_screen4_controls_installed = false;
lv_obj_t *g_screen4_bound_root = nullptr;
bool g_ui_showing_waiting[BATTERY_SLOT_COUNT] = {false, false};
bool g_screen4_was_active = false;
bool g_last_sta_connected = false;
bool g_has_last_sta_connected = false;
int g_last_screen_tint = -999;
lv_obj_t *g_confirm_overlay = nullptr;
bool g_confirm_visible = false;
uint8_t g_confirm_command_id = 0;
uint8_t g_confirm_command_value = 0;

// Text used by the secret-screen confirmation overlay.
struct ConfirmContent {
    const char *title;
    const char *body;
};

// Convert Battery Emulator battery_id into local slot index.
int battery_slot_from_id(uint8_t battery_id) {
    if (battery_id >= 1U && battery_id <= BATTERY_SLOT_COUNT) {
        return static_cast<int>(battery_id - 1U);
    }
    return -1;
}

// Slot 0 is treated as the primary battery for remote-control state.
SharedBatteryState &primary_shared_state() {
    return g_shared_states[0];
}

// Translate battery status code to user-visible text.
const char *status_to_text(int status) {
    switch (status) {
        case BMS_STANDBY:
            return "STANDBY";
        case BMS_ACTIVE:
            return "ACTIVE";
        case BMS_FAULT:
            return "FAULT";
        case BMS_DISCONNECTED:
        default:
            return "DISCONNECTED";
    }
}

// Translate inverter allow-close flag to text.
const char *inverter_status_to_text(bool allows_contactor_closing) {
    return allows_contactor_closing ? "READY" : "BLOCKED";
}

// Translate simple 0/1 contactor state to text.
const char *binary_contactor_to_text(uint8_t engaged) {
    return engaged != 0U ? "CLOSED" : "OPEN";
}

// Screen-wide background tint colors:
// 0 -> normal
// 1 -> paused/warning
// 2 -> fault
lv_color_t screen_tint_color(int tint_state) {
    switch (tint_state) {
        case 2:
            return lv_color_hex(0x7A1F1F);
        case 1:
            return lv_color_hex(0x4A4200);
        default:
            return lv_color_hex(0x000000);
    }
}

// Translate pause status enum to text for the secret screen.
const char *pause_status_to_text(uint8_t pause_status) {
    switch (pause_status) {
        case 1:
            return "PAUSING";
        case 2:
            return "PAUSED";
        case 3:
            return "RESUMING";
        case 0:
        default:
            return "RUNNING";
    }
}

// Translate emulator contactor state enum to text.
const char *contactors_to_text(uint8_t contactors_engaged) {
    switch (contactors_engaged) {
        case 1:
            return "CLOSED";
        case 2:
            return "FAULT OPEN";
        case 3:
            return "PRECHARGE";
        case 0:
        default:
            return "OPEN";
    }
}

bool remote_state_is_fresh(const UiBatteryState &state) {
    return state.has_remote_state && ((millis() - state.last_remote_state_ms) <= REMOTE_STATE_TIMEOUT_MS);
}

// Same freshness helper for the smaller remote-state view.
bool remote_state_is_fresh(const UiRemoteStateView &state) {
    return state.has_remote_state && ((millis() - state.last_remote_state_ms) <= REMOTE_STATE_TIMEOUT_MS);
}

// Check whether a specific command is still considered in-flight.
bool is_command_pending(const UiBatteryState &state, uint8_t command_id) {
    if (!state.command_pending || state.pending_command_id != command_id) {
        return false;
    }
    return (millis() - state.pending_command_sent_ms) <= COMMAND_ACK_TIMEOUT_MS;
}

// Same pending-command helper for the smaller remote-state view.
bool is_command_pending(const UiRemoteStateView &state, uint8_t command_id) {
    if (!state.command_pending || state.pending_command_id != command_id) {
        return false;
    }
    return (millis() - state.pending_command_sent_ms) <= COMMAND_ACK_TIMEOUT_MS;
}

// True when any command is still waiting for ACK.
bool any_command_pending(const UiRemoteStateView &state) {
    if (!state.command_pending) {
        return false;
    }
    return (millis() - state.pending_command_sent_ms) <= COMMAND_ACK_TIMEOUT_MS;
}

// Build the confirmation-dialog strings for the current action.
ConfirmContent confirm_content_for_command(uint8_t command_id, uint8_t command_value) {
    // Generate the confirmation text based on the action we are about to send.
    switch (command_id) {
        case REMOTE_CMD_SET_PAUSE:
            return command_value != 0U
                       ? ConfirmContent{"Pause Battery", "Do you really want to pause the battery?"}
                       : ConfirmContent{"Resume Battery", "Do you really want to resume the battery?"};
        case REMOTE_CMD_SET_CONTACTORS:
            return command_value != 0U
                       ? ConfirmContent{"Close Contactors", "Do you really want to close the contactors?"}
                       : ConfirmContent{"Open Contactors", "Do you really want to open the contactors?"};
        case REMOTE_CMD_REBOOT:
            return ConfirmContent{"Reboot Emulator", "Do you really want to reboot the emulator?"};
        default:
            return ConfirmContent{"Confirm Action", "Do you really want to do this action?"};
    }
}

// Translate numeric fault level to text.
const char *fault_level_to_text(uint8_t level) {
    return (level < (sizeof(kEventLevels) / sizeof(kEventLevels[0]))) ? kEventLevels[level] : "UNKNOWN";
}

// Translate numeric event id to text.
const char *fault_event_id_to_text(uint8_t event_id) {
    return (event_id < (sizeof(kEventNames) / sizeof(kEventNames[0]))) ? kEventNames[event_id] : "EVENT_UNKNOWN";
}

// Screen activity helpers used to limit redraw work to the visible screen.
bool is_screen4_active() {
    return (ui_SecretScreen != nullptr) && (lv_scr_act() == ui_SecretScreen);
}

bool is_screen2_active() {
    return (ui_Bat1CellsScreen != nullptr) && (lv_scr_act() == ui_Bat1CellsScreen);
}

bool is_screen3_active() {
    return (ui_FaultsScreen != nullptr) && (lv_scr_act() == ui_FaultsScreen);
}

bool is_screen5_active() {
    return (ui_Bat2CellsScreen != nullptr) && (lv_scr_act() == ui_Bat2CellsScreen);
}

bool is_dual_main_active() {
    return (ui_DoubleBatScreen != nullptr) && (lv_scr_act() == ui_DoubleBatScreen);
}

bool is_single_main_active() {
    return (ui_SingleBatScreen != nullptr) && (lv_scr_act() == ui_SingleBatScreen);
}

bool is_dual_battery_mode() {
    // Battery 2 is considered active only while fresh status packets still arrive.
    SharedBatteryState snapshot{};
    portENTER_CRITICAL(&g_state_mux);
    snapshot = g_shared_states[1];
    portEXIT_CRITICAL(&g_state_mux);
    return snapshot.has_status && ((millis() - snapshot.last_packet_ms) <= UI_STALE_HOLD_MS);
}

// Small wrapper around SquareLine's runtime screen change helper.
void load_screen(lv_obj_t **screen, void (*screen_init)(void)) {
    if (screen == nullptr) {
        return;
    }
    _ui_screen_change(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, screen_init);
}

// Choose the correct main screen based on current single/double battery mode.
void load_runtime_main_screen() {
    if (is_dual_battery_mode()) {
        load_screen(&ui_DoubleBatScreen, &ui_DoubleBatScreen_screen_init);
    } else {
        load_screen(&ui_SingleBatScreen, &ui_SingleBatScreen_screen_init);
    }
}

// Main-screen next button always goes to battery 1 cell view.
void screen_nav_main_next_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_screen(&ui_Bat1CellsScreen, &ui_Bat1CellsScreen_screen_init);
}

// Battery 1 back returns to whichever main screen is currently valid.
void screen_nav_bat1_back_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_runtime_main_screen();
}

// Battery 1 next goes either to battery 2 cells or directly to faults screen.
void screen_nav_bat1_next_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (is_dual_battery_mode()) {
        load_screen(&ui_Bat2CellsScreen, &ui_Bat2CellsScreen_screen_init);
    } else {
        load_screen(&ui_FaultsScreen, &ui_FaultsScreen_screen_init);
    }
}

// Battery 2 back always returns to battery 1 cells.
void screen_nav_bat2_back_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_screen(&ui_Bat1CellsScreen, &ui_Bat1CellsScreen_screen_init);
}

// Battery 2 next always goes to the shared faults screen.
void screen_nav_bat2_next_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_screen(&ui_FaultsScreen, &ui_FaultsScreen_screen_init);
}

// Faults back depends on whether battery 2 screen is relevant right now.
void screen_nav_faults_back_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (is_dual_battery_mode()) {
        load_screen(&ui_Bat2CellsScreen, &ui_Bat2CellsScreen_screen_init);
    } else {
        load_screen(&ui_Bat1CellsScreen, &ui_Bat1CellsScreen_screen_init);
    }
}

// Faults next opens the secret screen.
void screen_nav_faults_next_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_screen(&ui_SecretScreen, &ui_SecretScreen_screen_init);
}

// Secret screen back returns to the faults screen.
void screen_nav_secret_back_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    load_screen(&ui_FaultsScreen, &ui_FaultsScreen_screen_init);
}

// Remove generated SquareLine callbacks and replace them with runtime-aware ones.
// This keeps navigation correct even when single/double battery mode changes.
void install_navigation_overrides() {
    static lv_obj_t *last_double_root = nullptr;
    static lv_obj_t *last_single_root = nullptr;
    static lv_obj_t *last_bat1_root = nullptr;
    static lv_obj_t *last_bat2_root = nullptr;
    static lv_obj_t *last_faults_root = nullptr;
    static lv_obj_t *last_secret_root = nullptr;

    if (ui_DoubleBatScreen != nullptr && last_double_root != ui_DoubleBatScreen && ui_NextButS1 != nullptr) {
        while (lv_obj_remove_event_cb(ui_NextButS1, nullptr)) {}
        lv_obj_add_event_cb(ui_NextButS1, screen_nav_main_next_event, LV_EVENT_CLICKED, nullptr);
        last_double_root = ui_DoubleBatScreen;
    }

    if (ui_SingleBatScreen != nullptr && last_single_root != ui_SingleBatScreen && ui_NextButS6 != nullptr) {
        while (lv_obj_remove_event_cb(ui_NextButS6, nullptr)) {}
        lv_obj_add_event_cb(ui_NextButS6, screen_nav_main_next_event, LV_EVENT_CLICKED, nullptr);
        last_single_root = ui_SingleBatScreen;
    }

    if (ui_Bat1CellsScreen != nullptr && last_bat1_root != ui_Bat1CellsScreen) {
        if (ui_BackButS2 != nullptr) {
            while (lv_obj_remove_event_cb(ui_BackButS2, nullptr)) {}
            lv_obj_add_event_cb(ui_BackButS2, screen_nav_bat1_back_event, LV_EVENT_CLICKED, nullptr);
        }
        if (ui_NextButS2 != nullptr) {
            while (lv_obj_remove_event_cb(ui_NextButS2, nullptr)) {}
            lv_obj_add_event_cb(ui_NextButS2, screen_nav_bat1_next_event, LV_EVENT_CLICKED, nullptr);
        }
        last_bat1_root = ui_Bat1CellsScreen;
    }

    if (ui_Bat2CellsScreen != nullptr && last_bat2_root != ui_Bat2CellsScreen) {
        if (ui_BackButS1 != nullptr) {
            while (lv_obj_remove_event_cb(ui_BackButS1, nullptr)) {}
            lv_obj_add_event_cb(ui_BackButS1, screen_nav_bat2_back_event, LV_EVENT_CLICKED, nullptr);
        }
        if (ui_NextButS4 != nullptr) {
            while (lv_obj_remove_event_cb(ui_NextButS4, nullptr)) {}
            lv_obj_add_event_cb(ui_NextButS4, screen_nav_bat2_next_event, LV_EVENT_CLICKED, nullptr);
        }
        last_bat2_root = ui_Bat2CellsScreen;
    }

    if (ui_FaultsScreen != nullptr && last_faults_root != ui_FaultsScreen) {
        if (ui_BackButS3 != nullptr) {
            while (lv_obj_remove_event_cb(ui_BackButS3, nullptr)) {}
            lv_obj_add_event_cb(ui_BackButS3, screen_nav_faults_back_event, LV_EVENT_CLICKED, nullptr);
        }
        if (ui_NextButS3 != nullptr) {
            while (lv_obj_remove_event_cb(ui_NextButS3, nullptr)) {}
            lv_obj_add_event_cb(ui_NextButS3, screen_nav_faults_next_event, LV_EVENT_CLICKED, nullptr);
        }
        last_faults_root = ui_FaultsScreen;
    }

    if (ui_SecretScreen != nullptr && last_secret_root != ui_SecretScreen && ui_BackButS4 != nullptr) {
        while (lv_obj_remove_event_cb(ui_BackButS4, nullptr)) {}
        lv_obj_add_event_cb(ui_BackButS4, screen_nav_secret_back_event, LV_EVENT_CLICKED, nullptr);
        last_secret_root = ui_SecretScreen;
    }
}

// Fix up the active screen if the battery mode changed while the UI is already running.
void enforce_runtime_screen_flow() {
    if (is_dual_main_active() && !is_dual_battery_mode()) {
        load_screen(&ui_SingleBatScreen, &ui_SingleBatScreen_screen_init);
        return;
    }

    if (is_single_main_active() && is_dual_battery_mode()) {
        load_screen(&ui_DoubleBatScreen, &ui_DoubleBatScreen_screen_init);
        return;
    }

    if (is_screen5_active() && !is_dual_battery_mode()) {
        load_screen(&ui_Bat1CellsScreen, &ui_Bat1CellsScreen_screen_init);
    }
}

// Apply button colors only when something actually changed.
void set_button_colors(lv_obj_t *button, lv_color_t bg_color, lv_color_t text_color) {
    if (button == nullptr) {
        return;
    }

    const lv_color_t current_bg = lv_obj_get_style_bg_color(button, LV_PART_MAIN | LV_STATE_DEFAULT);
    const lv_color_t current_text = lv_obj_get_style_text_color(button, LV_PART_MAIN | LV_STATE_DEFAULT);
    const lv_coord_t current_border_width = lv_obj_get_style_border_width(button, LV_PART_MAIN | LV_STATE_DEFAULT);
    const lv_color_t current_border_color = lv_obj_get_style_border_color(button, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (current_bg.full == bg_color.full &&
        current_text.full == text_color.full &&
        current_border_width == 2 &&
        current_border_color.full == lv_color_black().full) {
        return;
    }

    lv_obj_set_style_bg_color(button, bg_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(button, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(button, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// Drive the RGB status LED on the CYD board.
void set_status_led(uint8_t red, uint8_t green, uint8_t blue) {
    analogWrite(RED_LED_PIN, 255 - red);
    analogWrite(GREEN_LED_PIN, 255 - green);
    analogWrite(BLUE_LED_PIN, 255 - blue);
}

// Update LED color for current battery state.
void update_led_for_status(int status) {
    if (status == g_last_led_status) {
        return;
    }

    g_last_led_status = status;

    switch (status) {
        case BMS_STANDBY:
            set_status_led(255, 180, 0);
            break;
        case BMS_ACTIVE:
            set_status_led(0, 255, 0);
            break;
        case BMS_FAULT:
            set_status_led(255, 0, 0);
            break;
        case BMS_DISCONNECTED:
            set_status_led(128, 0, 255);
            break;
        default:
            set_status_led(0, 0, 255);
            break;
    }
}

// Screen-level warning/fault states must have priority over normal battery colors.
// This keeps the RGB LED aligned with the large background tint seen on the UI.
void update_led_for_state(const UiRemoteStateView &remote_state_view, int battery_status) {
    if (remote_state_is_fresh(remote_state_view)) {
        if (remote_state_view.remote_state.system_status == SYS_FAULT) {
            update_led_for_status(BMS_FAULT);
            return;
        }

        if (remote_state_view.remote_state.pause_request_on != 0U ||
            remote_state_view.remote_state.pause_status != 0U) {
            update_led_for_status(BMS_STANDBY);
            return;
        }
    }

    update_led_for_status(battery_status);
}

// Helper overload when the caller already has the full battery snapshot.
void update_led_for_state(const UiBatteryState &state) {
    UiRemoteStateView remote_view{};
    remote_view.has_remote_state = state.has_remote_state;
    remote_view.last_remote_state_ms = state.last_remote_state_ms;
    remote_view.remote_state = state.remote_state;
    remote_view.pending_command_id = state.pending_command_id;
    remote_view.pending_command_sent_ms = state.pending_command_sent_ms;
    remote_view.command_pending = state.command_pending;
    update_led_for_state(remote_view, state.status.real_bms_status);
}

// Apply whole-screen background tint to one LVGL screen object.
void apply_screen_tint_to_obj(lv_obj_t *screen, lv_color_t color) {
    if (screen == nullptr) {
        return;
    }

    const lv_color_t current_bg = lv_obj_get_style_bg_color(screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    const lv_opa_t current_opa = lv_obj_get_style_bg_opa(screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (current_bg.full == color.full && current_opa == LV_OPA_COVER) {
        return;
    }

    lv_obj_set_style_bg_color(screen, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// Decide whether screens should be normal, warning yellow, or fault red.
void update_screen_tint(const UiRemoteStateView &state) {
    int tint_state = 0;
    if (remote_state_is_fresh(state)) {
        if (state.remote_state.system_status == SYS_FAULT) {
            tint_state = 2;
        } else if (state.remote_state.pause_request_on != 0U || state.remote_state.pause_status != 0U) {
            tint_state = 1;
        }
    }

    if (tint_state == g_last_screen_tint) {
        return;
    }
    g_last_screen_tint = tint_state;

    const lv_color_t tint = screen_tint_color(tint_state);
    apply_screen_tint_to_obj(ui_DoubleBatScreen, tint);
    apply_screen_tint_to_obj(ui_SingleBatScreen, tint);
    apply_screen_tint_to_obj(ui_Bat1CellsScreen, tint);
    apply_screen_tint_to_obj(ui_Bat2CellsScreen, tint);
    apply_screen_tint_to_obj(ui_FaultsScreen, tint);
    apply_screen_tint_to_obj(ui_SecretScreen, tint);
}

// Overload that extracts just the remote-state subset from a full battery snapshot.
void update_screen_tint(const UiBatteryState &state) {
    UiRemoteStateView remote_view{};
    remote_view.has_remote_state = state.has_remote_state;
    remote_view.last_remote_state_ms = state.last_remote_state_ms;
    remote_view.remote_state = state.remote_state;
    remote_view.pending_command_id = state.pending_command_id;
    remote_view.pending_command_sent_ms = state.pending_command_sent_ms;
    remote_view.command_pending = state.command_pending;
    update_screen_tint(remote_view);
}

// Safe label setter that avoids redraws when text is unchanged.
void set_label_text(lv_obj_t *obj, const char *text) {
    if (obj != nullptr) {
        const char *safe_text = (text != nullptr) ? text : "";
        const char *current_text = lv_label_get_text(obj);
        if (current_text != nullptr && strcmp(current_text, safe_text) == 0) {
            return;
        }
        lv_label_set_text(obj, safe_text);
    }
}

// Safe textarea setter that avoids redraws when text is unchanged.
void set_textarea_text(lv_obj_t *obj, const char *text) {
    if (obj == nullptr) {
        return;
    }

    const char *safe_text = (text != nullptr) ? text : "";
    const char *current_text = lv_textarea_get_text(obj);
    if (current_text != nullptr && strcmp(current_text, safe_text) == 0) {
        return;
    }

    lv_textarea_set_text(obj, safe_text);
}

// Small printf-style helper for labels.
void set_label_fmt(lv_obj_t *obj, const char *fmt, ...) {
    if (obj == nullptr) {
        return;
    }

    char buffer[32];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    set_label_text(obj, buffer);
}

// Render signed tenths like 12.3 A or -4.5 A.
void set_label_signed_tenths(lv_obj_t *obj, int32_t value, const char *unit) {
    if (obj == nullptr) {
        return;
    }

    const char *sign = value < 0 ? "-" : "";
    const int32_t absolute_value = std::abs(value);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s%ld.%ld %s",
             sign,
             static_cast<long>(absolute_value / 10),
             static_cast<long>(absolute_value % 10),
             unit);
    set_label_text(obj, buffer);
}

// Render watts as kW with two decimal places.
void set_label_signed_hundredths_from_watts(lv_obj_t *obj, int32_t value_watts) {
    if (obj == nullptr) {
        return;
    }

    const char *sign = value_watts < 0 ? "-" : "";
    const int32_t absolute_value = std::abs(value_watts);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s%ld.%02ld kW",
             sign,
             static_cast<long>(absolute_value / 1000),
             static_cast<long>((absolute_value % 1000) / 10));
    set_label_text(obj, buffer);
}

// Show isolation resistance as kOhm or MOhm depending on size.
void set_label_isolation_resistance(lv_obj_t *obj, uint32_t isolation_kohm) {
    if (obj == nullptr) {
        return;
    }

    if (isolation_kohm == 0U) {
        set_label_text(obj, "---");
        return;
    }

    char buffer[32];
    if (isolation_kohm >= 1000U) {
        snprintf(buffer,
                 sizeof(buffer),
                 "%lu.%02lu MOhm",
                 static_cast<unsigned long>(isolation_kohm / 1000U),
                 static_cast<unsigned long>((isolation_kohm % 1000U) / 10U));
    } else {
        snprintf(buffer, sizeof(buffer), "%lu kOhm", static_cast<unsigned long>(isolation_kohm));
    }
    set_label_text(obj, buffer);
}

// Render battery min/max temperature in "min/max°C" format.
void set_label_temp_min_max(lv_obj_t *obj, int16_t min_temp_dC, int16_t max_temp_dC) {
    if (obj == nullptr) {
        return;
    }

    char buffer[32];
    snprintf(buffer,
             sizeof(buffer),
             "%d/%d\xC2\xB0""C",
             static_cast<int>(min_temp_dC / 10),
             static_cast<int>(max_temp_dC / 10));
    set_label_text(obj, buffer);
}

// Update network labels on the secret screen.
// Only CYD network info is shown now.
void update_screen4_network_labels() {
    if (!is_screen4_active()) {
        return;
    }

    if (ui_IPText == nullptr || ui_SSIDText == nullptr || ui_WifiQualText == nullptr) {
        return;
    }

    char buffer[96];

    if (screen_network_is_sta_connected()) {
        snprintf(buffer, sizeof(buffer), "IP: %s", screen_network_sta_ip());
        set_label_text(ui_IPText, buffer);
        snprintf(buffer, sizeof(buffer), "SSID: %s", screen_network_sta_ssid());
        set_label_text(ui_SSIDText, buffer);
        snprintf(buffer, sizeof(buffer), "%d dBm", screen_network_sta_rssi());
        set_label_text(ui_WifiQualText, buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "IP: %s", WiFi.softAPIP().toString().c_str());
        set_label_text(ui_IPText, buffer);
        set_label_text(ui_SSIDText, "SSID: AP only");
        set_label_text(ui_WifiQualText, "--- dBm");
    }

}

// Update pause/contactors/reboot status labels on the secret screen.
void update_screen4_labels(const UiRemoteStateView &state) {
    if (!is_screen4_active()) {
        return;
    }

    if (ui_PauseText == nullptr || ui_OpenContactorsText == nullptr || ui_RebootText == nullptr) {
        return;
    }

    if (!remote_state_is_fresh(state)) {
        set_label_text(ui_PauseText, "Pause (Waiting state)");
        set_label_text(ui_OpenContactorsText, "Contactors (Waiting state)");
        set_label_text(ui_RebootText, "Reboot BE");
        set_button_colors(ui_Pause, lv_palette_main(LV_PALETTE_GREY), lv_color_white());
        set_button_colors(ui_OpenContactors, lv_palette_main(LV_PALETTE_GREY), lv_color_white());
        set_button_colors(ui_Reboot, lv_palette_main(LV_PALETTE_BLUE_GREY), lv_color_white());
        return;
    }

    char pause_text[48];
    if (is_command_pending(state, REMOTE_CMD_SET_PAUSE)) {
        snprintf(pause_text, sizeof(pause_text), "Pause (Sending...)");
        set_button_colors(ui_Pause, lv_palette_main(LV_PALETTE_ORANGE), lv_color_white());
    } else {
        const char *pause_action = (state.remote_state.pause_request_on != 0U || state.remote_state.pause_status != 0U)
                                       ? "Resume"
                                       : "Pause";
        snprintf(pause_text, sizeof(pause_text), "%s (%s)", pause_action, pause_status_to_text(state.remote_state.pause_status));

        if (state.remote_state.pause_request_on != 0U || state.remote_state.pause_status != 0U) {
            set_button_colors(ui_Pause, lv_palette_main(LV_PALETTE_RED), lv_color_white());
        } else {
            set_button_colors(ui_Pause, lv_palette_main(LV_PALETTE_GREEN), lv_color_white());
        }
    }
    set_label_text(ui_PauseText, pause_text);

    char contactor_text[64];
    if (is_command_pending(state, REMOTE_CMD_SET_CONTACTORS)) {
        snprintf(contactor_text, sizeof(contactor_text), "Contactors (Sending...)");
        set_button_colors(ui_OpenContactors, lv_palette_main(LV_PALETTE_ORANGE), lv_color_white());
    } else {
        const char *contactor_action = (state.remote_state.equipment_stop_active != 0U) ? "Close Contactors"
                                                                                         : "Open Contactors";
        snprintf(contactor_text, sizeof(contactor_text), "%s (%s)",
                 contactor_action,
                 contactors_to_text(state.remote_state.contactors_engaged));
        const bool showing_close_action = (state.remote_state.equipment_stop_active != 0U);
        const bool contactors_open = (state.remote_state.contactors_engaged == 0U);
        const bool contactors_closed = (state.remote_state.contactors_engaged == 1U);

        if ((showing_close_action && contactors_closed) || (!showing_close_action && contactors_open)) {
            set_button_colors(ui_OpenContactors, lv_palette_main(LV_PALETTE_GREEN), lv_color_white());
        } else if ((showing_close_action && contactors_open) || (!showing_close_action && contactors_closed)) {
            set_button_colors(ui_OpenContactors, lv_palette_main(LV_PALETTE_RED), lv_color_white());
        } else {
            set_button_colors(ui_OpenContactors, lv_palette_main(LV_PALETTE_YELLOW), lv_color_black());
        }
    }
    set_label_text(ui_OpenContactorsText, contactor_text);

    if (is_command_pending(state, REMOTE_CMD_REBOOT)) {
        set_label_text(ui_RebootText, "Rebooting...");
        set_button_colors(ui_Reboot, lv_palette_main(LV_PALETTE_YELLOW), lv_color_black());
    } else {
        set_label_text(ui_RebootText, "Reboot BE");
        set_button_colors(ui_Reboot, lv_palette_main(LV_PALETTE_BLUE), lv_color_white());
    }
}

// Overload that extracts remote-state subset from a full battery snapshot.
void update_screen4_labels(const UiBatteryState &state) {
    UiRemoteStateView remote_view{};
    remote_view.has_remote_state = state.has_remote_state;
    remote_view.last_remote_state_ms = state.last_remote_state_ms;
    remote_view.remote_state = state.remote_state;
    remote_view.pending_command_id = state.pending_command_id;
    remote_view.pending_command_sent_ms = state.pending_command_sent_ms;
    remote_view.command_pending = state.command_pending;
    update_screen4_labels(remote_view);
}

// Clear local pending-command flag once the matching ACK comes back from the emulator.
void clear_pending_command_if_ack_matches(SharedBatteryState &shared_state,
                                          const BATTERY_REMOTE_STATE_TYPE &remote_state) {
    if (!shared_state.command_pending) {
        return;
    }

    if (remote_state.ack_sequence != shared_state.pending_command_sequence) {
        return;
    }

    if (remote_state.ack_command_id != shared_state.pending_command_id) {
        return;
    }

    shared_state.command_pending = false;
}

void close_confirm_dialog() {
    // Close and fully reset the runtime confirmation dialog state.
    if (g_confirm_overlay != nullptr) {
        lv_obj_del(g_confirm_overlay);
        g_confirm_overlay = nullptr;
    }
    g_confirm_visible = false;
    g_confirm_command_id = 0;
    g_confirm_command_value = 0;
}

// Send one remote-control command packet and mark it as pending locally
// until an ACK or timeout arrives.
bool send_remote_command(uint8_t command_id, uint8_t command_value) {
    ESPNOW_HEADER header{};
    BATTERY_REMOTE_COMMAND_TYPE command{};
    SharedBatteryState &shared_state = primary_shared_state();

    // Update shared pending-command state under lock because the ESP-NOW
    // receive callback may also touch the same ACK fields.
    portENTER_CRITICAL(&g_state_mux);
    // Never allow multiple live commands at the same time. We wait for ACK or timeout.
    if (shared_state.command_pending &&
        (millis() - shared_state.pending_command_sent_ms) <= COMMAND_ACK_TIMEOUT_MS) {
        portEXIT_CRITICAL(&g_state_mux);
        return false;
    }
    // Clear a stale pending flag so the UI can recover if an ACK never arrives.
    if (shared_state.command_pending &&
        (millis() - shared_state.pending_command_sent_ms) > COMMAND_ACK_TIMEOUT_MS) {
        shared_state.command_pending = false;
    }
    shared_state.last_sent_command_sequence++;
    if (shared_state.last_sent_command_sequence == 0U) {
        shared_state.last_sent_command_sequence = 1U;
    }
    const uint8_t sequence = shared_state.last_sent_command_sequence;
    shared_state.pending_command_sequence = sequence;
    shared_state.pending_command_id = command_id;
    shared_state.pending_command_value = command_value;
    shared_state.pending_command_sent_ms = millis();
    shared_state.command_pending = true;
    shared_state.dirty = true;
    portEXIT_CRITICAL(&g_state_mux);

    header.emulator_id = 0;
    header.battery_id = 1;
    header.esp_message_type = BAT_REMOTE_COMMAND;

    command.command_id = command_id;
    command.command_sequence = sequence;
    command.command_value = command_value;
    command.reserved = 0;

    // Wire packet = common header + command payload.
    uint8_t payload[ESPNOW_HEADER_SIZE + sizeof(BATTERY_REMOTE_COMMAND_TYPE)] = {0};
    memcpy(payload, &header, ESPNOW_HEADER_SIZE);
    memcpy(payload + ESPNOW_HEADER_SIZE, &command, sizeof(command));

    // Command is broadcast so the paired emulator can receive it without
    // managing a per-device destination table in the screen code.
    const esp_err_t result = esp_now_send(g_broadcast_address, payload, sizeof(payload));
    if (result != ESP_OK) {
        portENTER_CRITICAL(&g_state_mux);
        shared_state.command_pending = false;
        portEXIT_CRITICAL(&g_state_mux);
        Serial.printf("ESP-NOW command send failed: cmd=%u err=%d\n", static_cast<unsigned>(command_id), static_cast<int>(result));
        return false;
    }

    Serial.printf("ESP-NOW command sent: cmd=%u seq=%u value=%u\n",
                  static_cast<unsigned>(command_id),
                  static_cast<unsigned>(sequence),
                  static_cast<unsigned>(command_value));
    return true;
}

// "Yes" button confirms the action and sends the queued command.
void confirm_yes_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    const uint8_t command_id = g_confirm_command_id;
    const uint8_t command_value = g_confirm_command_value;
    close_confirm_dialog();
    send_remote_command(command_id, command_value);
}

// "No" button only closes the dialog.
void confirm_no_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    close_confirm_dialog();
}

// Create the runtime confirmation overlay used by secret-screen actions.
void open_confirm_dialog(uint8_t command_id, uint8_t command_value) {
    // Only allow one confirmation dialog at a time.
    if (g_confirm_visible) {
        return;
    }

    const ConfirmContent content = confirm_content_for_command(command_id, command_value);
    g_confirm_command_id = command_id;
    g_confirm_command_value = command_value;

    g_confirm_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(g_confirm_overlay);
    lv_obj_set_size(g_confirm_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_confirm_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_confirm_overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(g_confirm_overlay, 0, 0);
    lv_obj_clear_flag(g_confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_confirm_overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = lv_obj_create(g_confirm_overlay);
    lv_obj_set_size(panel, 200, 112);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x5A5A5A), 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, content.title);
    lv_obj_set_width(title, 176);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    lv_obj_t *body = lv_label_create(panel);
    lv_label_set_text(body, content.body);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(body, 176);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_12, 0);

    lv_obj_t *yes_btn = lv_btn_create(panel);
    lv_obj_set_size(yes_btn, 72, 28);
    lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_LEFT, 16, -10);
    lv_obj_set_style_bg_color(yes_btn, lv_color_hex(0x1F8B4C), 0);
    lv_obj_set_style_radius(yes_btn, 8, 0);
    lv_obj_add_event_cb(yes_btn, confirm_yes_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *yes_label = lv_label_create(yes_btn);
    lv_label_set_text(yes_label, "Yes");
    lv_obj_center(yes_label);

    lv_obj_t *no_btn = lv_btn_create(panel);
    lv_obj_set_size(no_btn, 72, 28);
    lv_obj_align(no_btn, LV_ALIGN_BOTTOM_RIGHT, -16, -10);
    lv_obj_set_style_bg_color(no_btn, lv_color_hex(0x8B2E2E), 0);
    lv_obj_set_style_radius(no_btn, 8, 0);
    lv_obj_add_event_cb(no_btn, confirm_no_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *no_label = lv_label_create(no_btn);
    lv_label_set_text(no_label, "No");
    lv_obj_center(no_label);

    g_confirm_visible = true;
}

// Pause/resume button handler for the secret screen.
void pause_button_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    UiRemoteStateView state{};
    SharedBatteryState &shared_state = primary_shared_state();
    portENTER_CRITICAL(&g_state_mux);
    state.has_remote_state = shared_state.has_remote_state;
    state.remote_state = shared_state.remote_state;
    state.last_remote_state_ms = shared_state.last_remote_state_ms;
    state.command_pending = shared_state.command_pending;
    state.pending_command_id = shared_state.pending_command_id;
    state.pending_command_sent_ms = shared_state.pending_command_sent_ms;
    portEXIT_CRITICAL(&g_state_mux);

    // Ignore repeated taps while a command is pending or a confirmation dialog is open.
    if (any_command_pending(state) || g_confirm_visible) {
        return;
    }

    const bool pause_now = !remote_state_is_fresh(state) ||
                           (state.remote_state.pause_request_on == 0U && state.remote_state.pause_status == 0U);
    open_confirm_dialog(REMOTE_CMD_SET_PAUSE, pause_now ? 1U : 0U);
}

// Open/close contactors button handler for the secret screen.
void contactors_button_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    UiRemoteStateView state{};
    SharedBatteryState &shared_state = primary_shared_state();
    portENTER_CRITICAL(&g_state_mux);
    state.has_remote_state = shared_state.has_remote_state;
    state.remote_state = shared_state.remote_state;
    state.last_remote_state_ms = shared_state.last_remote_state_ms;
    state.command_pending = shared_state.command_pending;
    state.pending_command_id = shared_state.pending_command_id;
    state.pending_command_sent_ms = shared_state.pending_command_sent_ms;
    portEXIT_CRITICAL(&g_state_mux);

    // Ignore repeated taps while a command is pending or a confirmation dialog is open.
    if (any_command_pending(state) || g_confirm_visible) {
        return;
    }

    const bool close_contactors = remote_state_is_fresh(state) && (state.remote_state.equipment_stop_active != 0U);
    open_confirm_dialog(REMOTE_CMD_SET_CONTACTORS, close_contactors ? 1U : 0U);
}

// Reboot button handler for the secret screen.
void reboot_button_event(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    UiRemoteStateView state{};
    SharedBatteryState &shared_state = primary_shared_state();
    portENTER_CRITICAL(&g_state_mux);
    state.command_pending = shared_state.command_pending;
    state.pending_command_id = shared_state.pending_command_id;
    state.pending_command_sent_ms = shared_state.pending_command_sent_ms;
    portEXIT_CRITICAL(&g_state_mux);

    // Ignore repeated taps while a command is pending or a confirmation dialog is open.
    if (any_command_pending(state) || g_confirm_visible) {
        return;
    }

    open_confirm_dialog(REMOTE_CMD_REBOOT, 1U);
}

// Replace generated secret-screen button callbacks with project runtime callbacks.
void install_screen4_controls() {
    if (ui_SecretScreen == nullptr || ui_Pause == nullptr || ui_OpenContactors == nullptr || ui_Reboot == nullptr) {
        return;
    }

    if (g_screen4_bound_root != ui_SecretScreen) {
        g_screen4_controls_installed = false;
        g_screen4_bound_root = ui_SecretScreen;
    }

    if (g_screen4_controls_installed) {
        return;
    }

    while (lv_obj_remove_event_cb(ui_Pause, nullptr)) {}
    while (lv_obj_remove_event_cb(ui_PauseContainer, nullptr)) {}
    while (lv_obj_remove_event_cb(ui_OpenContactors, nullptr)) {}
    while (lv_obj_remove_event_cb(ui_OpenContContainer, nullptr)) {}
    while (lv_obj_remove_event_cb(ui_Reboot, nullptr)) {}
    while (lv_obj_remove_event_cb(ui_RebootLilygoContainer, nullptr)) {}

    lv_obj_add_event_cb(ui_Pause, pause_button_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(ui_OpenContactors, contactors_button_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(ui_Reboot, reboot_button_event, LV_EVENT_CLICKED, nullptr);
    update_screen4_network_labels();
    g_screen4_controls_installed = true;
}

// Select the right chart object for a battery slot.
lv_obj_t *chart_for_slot(uint8_t slot) {
    return (slot == 0U) ? ui_Chart2 : ui_Chart1;
}

// Select the right min-cell label for a battery slot.
lv_obj_t *cell_min_label_for_slot(uint8_t slot) {
    return (slot == 0U) ? ui_CellMinValue : ui_CellMinValue1;
}

// Select the right max-cell label for a battery slot.
lv_obj_t *cell_max_label_for_slot(uint8_t slot) {
    return (slot == 0U) ? ui_CellMaxValue : ui_CellMaxValue1;
}

// Select the right deviation label for a battery slot.
lv_obj_t *deviation_label_for_slot(uint8_t slot) {
    return (slot == 0U) ? ui_DevValue : ui_DevValue1;
}

// Bind chart data arrays the first time a chart is used.
void ensure_chart_ready(uint8_t slot) {
    lv_obj_t *chart = chart_for_slot(slot);
    if (chart == nullptr || g_chart_series[slot] != nullptr) {
        return;
    }

    g_chart_series[slot] = lv_chart_get_series_next(chart, nullptr);
    if (g_chart_series[slot] == nullptr) {
        return;
    }

    lv_chart_set_point_count(chart, MAX_UI_CHART_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 2500, 4500);
    lv_chart_set_ext_y_array(chart, g_chart_series[slot], g_cell_chart_values[slot]);
    lv_chart_refresh(chart);
}

// Clear a cells chart when we no longer have fresh data for that battery.
void clear_chart(uint8_t slot) {
    lv_obj_t *chart = chart_for_slot(slot);
    ensure_chart_ready(slot);
    if (chart == nullptr || g_chart_series[slot] == nullptr) {
        return;
    }

    for (uint16_t i = 0; i < MAX_UI_CHART_POINTS; ++i) {
        g_cell_chart_values[slot][i] = 0;
    }
    lv_chart_set_point_count(chart, MAX_UI_CHART_POINTS);
    lv_chart_refresh(chart);
}

// Push current cell voltages into the visible battery chart.
// 16-bit data is preferred when it is fresh; otherwise 8-bit cells are used.
void update_chart_from_cells(uint8_t slot, const UiBatteryState &state) {
    if ((slot == 0U && !is_screen2_active()) || (slot == 1U && !is_screen5_active())) {
        return;
    }

    lv_obj_t *chart = chart_for_slot(slot);
    ensure_chart_ready(slot);
    if (chart == nullptr || g_chart_series[slot] == nullptr) {
        return;
    }

    const bool use_16bit_cells = state.has_cells_16bit && ((millis() - state.last_cells_16bit_ms) <= HIGH_RES_CELL_TIMEOUT_MS);
    if (!use_16bit_cells && !state.has_cells) {
        return;
    }

    uint16_t point_count = use_16bit_cells ? state.cell16_total_cells : state.cells.number_of_cells;
    if (point_count == 0) {
        return;
    }
    if (point_count > MAX_UI_CHART_POINTS) {
        point_count = MAX_UI_CHART_POINTS;
    }

    uint16_t min_mV = 5000;
    uint16_t max_mV = 0;

    for (uint16_t i = 0; i < point_count; ++i) {
        const uint16_t cell_mV = use_16bit_cells ? state.cells_16bit_mV[i]
                                                 : static_cast<uint16_t>(state.cells.cell_voltages_mV[i]) * 20U;
        g_cell_chart_values[slot][i] = static_cast<lv_coord_t>(cell_mV);
        if (cell_mV < min_mV) {
            min_mV = cell_mV;
        }
        if (cell_mV > max_mV) {
            max_mV = cell_mV;
        }
    }

    for (uint16_t i = point_count; i < MAX_UI_CHART_POINTS; ++i) {
        g_cell_chart_values[slot][i] = 0;
    }

    uint16_t range_min = (min_mV > 100) ? static_cast<uint16_t>(min_mV - 100U) : min_mV;
    uint16_t range_max = static_cast<uint16_t>(max_mV + 100U);
    if (range_max <= range_min) {
        range_min = 2500;
        range_max = 4500;
    }

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, range_min, range_max);
    lv_chart_set_point_count(chart, point_count);
    lv_chart_refresh(chart);
}

// Update the shared faults screen from either structured events or fallback text.
void apply_fault_text_ui(const UiBatteryState &state) {
    if (ui_TextArea1 == nullptr || !is_screen3_active()) {
        return;
    }

    if (state.has_fault_events && (millis() - state.last_fault_events_ms) <= FAULT_EVENTS_TIMEOUT_MS) {
        if (state.fault_event_count == 0) {
            set_textarea_text(ui_TextArea1, "No faults");
            return;
        }

        char buffer[2048];
        size_t offset = 0;
        buffer[0] = '\0';

        for (uint8_t i = 0; i < state.fault_event_count; ++i) {
            const auto &event = state.fault_events[i];
            const int written = snprintf(buffer + offset,
                                         sizeof(buffer) - offset,
                                         "%s | %s | age=%lus | data=%u | count=%u\n",
                                         fault_level_to_text(event.level),
                                         fault_event_id_to_text(event.event_id),
                                         static_cast<unsigned long>(event.age_seconds),
                                         static_cast<unsigned>(event.data),
                                         static_cast<unsigned>(event.count));
            if (written <= 0 || static_cast<size_t>(written) >= (sizeof(buffer) - offset)) {
                break;
            }
            offset += static_cast<size_t>(written);
        }

        set_textarea_text(ui_TextArea1, buffer);
        return;
    }

    if (!state.has_fault_text || (millis() - state.last_fault_text_ms) > FAULT_TEXT_TIMEOUT_MS) {
        set_textarea_text(ui_TextArea1, "No faults sent");
        return;
    }

    if (state.fault_text[0] == '\0') {
        set_textarea_text(ui_TextArea1, "No faults");
        return;
    }

    set_textarea_text(ui_TextArea1, state.fault_text);
}

// Show placeholder values while battery packets are missing or stale.
void apply_waiting_ui(uint8_t slot) {
    UiRemoteStateView remote_snapshot{};
    SharedBatteryState &shared_state = primary_shared_state();
    portENTER_CRITICAL(&g_state_mux);
    remote_snapshot.has_remote_state = shared_state.has_remote_state;
    remote_snapshot.remote_state = shared_state.remote_state;
    remote_snapshot.last_remote_state_ms = shared_state.last_remote_state_ms;
    remote_snapshot.command_pending = shared_state.command_pending;
    remote_snapshot.pending_command_id = shared_state.pending_command_id;
    remote_snapshot.pending_command_sent_ms = shared_state.pending_command_sent_ms;
    portEXIT_CRITICAL(&g_state_mux);

    if (g_ui_showing_waiting[slot]) {
        if (slot == 0U) {
            update_screen_tint(remote_snapshot);
            update_screen4_labels(remote_snapshot);
        }
        return;
    }

    g_ui_showing_waiting[slot] = true;

    const bool dual_mode = is_dual_battery_mode();

    if (slot == 0U) {
        if (dual_mode) {
            set_label_text(ui_SOCValue, "---%");
            set_label_text(ui_SOHValue, "---%");
            set_label_text(ui_VoltageValue, "--- V");
            set_label_text(ui_CurrentValue, "--- A");
            set_label_text(ui_PowerValue, "--- kW");
            set_label_text(ui_RezistanceValue, "---");
            set_label_text(ui_TempMinMaxValueBat1, "--/--°C");
            set_label_text(ui_StatusValue, "Waiting");
            set_label_text(ui_Bat1Status, "Waiting");
            if (ui_SOCArc != nullptr && lv_arc_get_value(ui_SOCArc) != 0) {
                lv_arc_set_value(ui_SOCArc, 0);
            }
        } else {
            set_label_text(ui_SOCValue3, "---%");
            set_label_text(ui_SOHValue3, "---%");
            set_label_text(ui_VoltageValue3, "--- V");
            set_label_text(ui_CurrentValue3, "--- A");
            set_label_text(ui_PowerValue3, "--- kW");
            set_label_text(ui_RezistanceValue3, "---");
            set_label_text(ui_TempMinMaxValueSingleBat, "--/--°C");
            set_label_text(ui_StatusValue2, "Waiting");
            set_label_text(ui_SingleBatStatus, "Waiting");
            if (ui_SOCArc3 != nullptr && lv_arc_get_value(ui_SOCArc3) != 0) {
                lv_arc_set_value(ui_SOCArc3, 0);
            }
        }
        if (is_screen2_active()) {
            clear_chart(slot);
        }
        update_led_for_state(remote_snapshot, -1);
        if (ui_TextArea1 != nullptr && is_screen3_active()) {
            set_textarea_text(ui_TextArea1, "No faults sent");
        }
        update_screen_tint(remote_snapshot);
        update_screen4_labels(remote_snapshot);
    } else {
        set_label_text(ui_SOCValue2, "---%");
        set_label_text(ui_SOHValue2, "---%");
        set_label_text(ui_VoltageValue2, "--- V");
        set_label_text(ui_CurrentValue2, "--- A");
        set_label_text(ui_PowerValue2, "--- kW");
        set_label_isolation_resistance(ui_RezistanceValue2, 0);
        set_label_text(ui_TempMinMaxValueBat2, "--/--°C");
        set_label_text(ui_Bat2Status, "Waiting");
        if (ui_SOCArc2 != nullptr && lv_arc_get_value(ui_SOCArc2) != 0) {
            lv_arc_set_value(ui_SOCArc2, 0);
        }
        if (is_screen5_active()) {
            clear_chart(slot);
        }
    }
    if ((slot == 0U && is_screen2_active()) || (slot == 1U && is_screen5_active())) {
        set_label_text(cell_min_label_for_slot(slot), "---mV");
        set_label_text(cell_max_label_for_slot(slot), "---mV");
        set_label_text(deviation_label_for_slot(slot), "---mV");
    }

    if (slot == 0U && !g_waiting_log_printed) {
        Serial.println("ESP-NOW: waiting for BAT_STATUS packets...");
        g_waiting_log_printed = true;
    }
}

// Update the battery status text shown on the correct main screen.
void set_main_battery_status_labels(uint8_t slot, int status, bool dual_mode) {
    const char *status_text = status_to_text(status);
    if (slot == 0U) {
        if (dual_mode) {
            set_label_text(ui_Bat1Status, status_text);
        } else {
            set_label_text(ui_SingleBatStatus, status_text);
        }
        return;
    }

    if (dual_mode) {
        set_label_text(ui_Bat2Status, status_text);
    }
}

// Update inverter status text on the active main-screen layout.
void set_main_inverter_status(const UiBatteryState &state, bool dual_mode) {
    if (state.has_remote_state && ((millis() - state.last_remote_state_ms) <= REMOTE_STATE_TIMEOUT_MS)) {
        const char *status_text =
            inverter_status_to_text(state.remote_state.inverter_allows_contactor_closing != 0U);
        if (dual_mode) {
            set_label_text(ui_StatusValue, status_text);
        } else {
            set_label_text(ui_StatusValue2, status_text);
        }
    } else {
        if (dual_mode) {
            set_label_text(ui_StatusValue, "WAITING");
        } else {
            set_label_text(ui_StatusValue2, "WAITING");
        }
    }
}

// Render one battery slot into the UI using the latest fresh packet snapshot.
void apply_ui_state(uint8_t slot, const UiBatteryState &state) {
    const uint32_t age = millis() - state.last_packet_ms;
    if (!state.has_status || age > UI_STALE_HOLD_MS) {
        apply_waiting_ui(slot);
        return;
    }

    g_ui_showing_waiting[slot] = false;
    const uint16_t deviation_mV = state.status.cell_max_voltage_mV - state.status.cell_min_voltage_mV;

    const bool dual_mode = is_dual_battery_mode();

    if (slot == 0U) {
        if (dual_mode) {
            set_label_fmt(ui_SOCValue, "%u.%02u%%", state.status.reported_soc / 100U, state.status.reported_soc % 100U);
            set_label_fmt(ui_SOHValue, "%u.%02u%%", state.status.soh_pptt / 100U, state.status.soh_pptt % 100U);
            set_label_fmt(ui_VoltageValue, "%u.%u V", state.status.voltage_dV / 10U, state.status.voltage_dV % 10U);
            set_label_signed_tenths(ui_CurrentValue, state.status.reported_current_dA, "A");
            set_label_signed_hundredths_from_watts(ui_PowerValue, state.status.active_power_W);
            set_label_isolation_resistance(ui_RezistanceValue, state.aux_info.isolation_resistance_kohm);
            set_label_temp_min_max(ui_TempMinMaxValueBat1,
                                   state.status.temperature_min_dC,
                                   state.status.temperature_max_dC);
            if (ui_SOCArc != nullptr) {
                const int arc_value = state.status.reported_soc / 100U;
                if (lv_arc_get_value(ui_SOCArc) != arc_value) {
                    lv_arc_set_value(ui_SOCArc, arc_value);
                }
            }
        } else {
            set_label_fmt(ui_SOCValue3, "%u.%02u%%", state.status.reported_soc / 100U, state.status.reported_soc % 100U);
            set_label_fmt(ui_SOHValue3, "%u.%02u%%", state.status.soh_pptt / 100U, state.status.soh_pptt % 100U);
            set_label_fmt(ui_VoltageValue3, "%u.%u V", state.status.voltage_dV / 10U, state.status.voltage_dV % 10U);
            set_label_signed_tenths(ui_CurrentValue3, state.status.reported_current_dA, "A");
            set_label_signed_hundredths_from_watts(ui_PowerValue3, state.status.active_power_W);
            set_label_isolation_resistance(ui_RezistanceValue3, state.aux_info.isolation_resistance_kohm);
            set_label_temp_min_max(ui_TempMinMaxValueSingleBat,
                                   state.status.temperature_min_dC,
                                   state.status.temperature_max_dC);
            if (ui_SOCArc3 != nullptr) {
                const int arc_value = state.status.reported_soc / 100U;
                if (lv_arc_get_value(ui_SOCArc3) != arc_value) {
                    lv_arc_set_value(ui_SOCArc3, arc_value);
                }
            }
        }
        set_main_inverter_status(state, dual_mode);
        set_main_battery_status_labels(slot, state.status.real_bms_status, dual_mode);
        update_led_for_state(state);
        apply_fault_text_ui(state);
        update_screen_tint(state);
        update_screen4_labels(state);
    } else {
        set_label_fmt(ui_SOCValue2, "%u.%02u%%", state.status.reported_soc / 100U, state.status.reported_soc % 100U);
        set_label_fmt(ui_SOHValue2, "%u.%02u%%", state.status.soh_pptt / 100U, state.status.soh_pptt % 100U);
        set_label_fmt(ui_VoltageValue2, "%u.%u V", state.status.voltage_dV / 10U, state.status.voltage_dV % 10U);
        set_label_signed_tenths(ui_CurrentValue2, state.status.reported_current_dA, "A");
        set_label_signed_hundredths_from_watts(ui_PowerValue2, state.status.active_power_W);
        set_label_isolation_resistance(ui_RezistanceValue2, state.aux_info.isolation_resistance_kohm);
        set_label_temp_min_max(ui_TempMinMaxValueBat2,
                               state.status.temperature_min_dC,
                               state.status.temperature_max_dC);
        set_main_battery_status_labels(slot, state.status.real_bms_status, dual_mode);
        if (ui_SOCArc2 != nullptr) {
            const int arc_value = state.status.reported_soc / 100U;
            if (lv_arc_get_value(ui_SOCArc2) != arc_value) {
                lv_arc_set_value(ui_SOCArc2, arc_value);
            }
        }
    }

    if ((slot == 0U && is_screen2_active()) || (slot == 1U && is_screen5_active())) {
        set_label_fmt(cell_min_label_for_slot(slot), "%umV", state.status.cell_min_voltage_mV);
        set_label_fmt(cell_max_label_for_slot(slot), "%umV", state.status.cell_max_voltage_mV);
        set_label_fmt(deviation_label_for_slot(slot), "%umV", deviation_mV);
    }

    update_chart_from_cells(slot, state);

    const uint32_t now = millis();
    if (slot == 0U && now - g_last_ui_log_ms >= RX_LOG_INTERVAL_MS) {
        g_last_ui_log_ms = now;
        Serial.printf("UI updated: SOC=%u.%02u%% SOH=%u.%02u%% V=%u.%u I=%d.%d P=%ldW Dev=%umV Status=%s\n",
                      state.status.reported_soc / 100U,
                      state.status.reported_soc % 100U,
                      state.status.soh_pptt / 100U,
                      state.status.soh_pptt % 100U,
                      state.status.voltage_dV / 10U,
                      state.status.voltage_dV % 10U,
                      std::abs(state.status.reported_current_dA) / 10,
                      std::abs(state.status.reported_current_dA) % 10,
                      static_cast<long>(state.status.active_power_W),
                      deviation_mV,
                      status_to_text(state.status.real_bms_status));
    }

    if (slot == 0U) {
        g_waiting_log_printed = false;
    }
}

// Use normal UI when packets are fresh, otherwise fall back to waiting placeholders.
void apply_cached_or_waiting_ui(uint8_t slot, const UiBatteryState &state) {
    if (state.has_status && (millis() - state.last_packet_ms) <= UI_STALE_HOLD_MS) {
        apply_ui_state(slot, state);
        return;
    }
    apply_waiting_ui(slot);
}

// ESP-NOW receive callback.
// This only parses packets into shared state; LVGL updates happen later in the main loop.
void on_data_recv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
    (void)mac;

    if (incoming_data == nullptr || len < static_cast<int>(ESPNOW_HEADER_SIZE)) {
        return;
    }

    ESPNOW_HEADER header{};
    memcpy(&header, incoming_data, sizeof(header));

    const int slot = battery_slot_from_id(header.battery_id);
    if (slot < 0) {
        return;
    }
    SharedBatteryState &shared_state = g_shared_states[slot];

    // Copy just the packet payload into shared state under ISR-safe lock.
    portENTER_CRITICAL_ISR(&g_state_mux);
    shared_state.rx_packet_count++;
    shared_state.rx_event_pending = true;
    shared_state.last_message_type = header.esp_message_type;
    shared_state.last_message_len = len;
    shared_state.emulator_id = header.emulator_id;
    shared_state.battery_id = header.battery_id;
    shared_state.last_packet_ms = millis();

    // BAT_INFO updates battery design/static info.
    if (header.esp_message_type == BAT_INFO &&
               len > static_cast<int>(ESPNOW_HEADER_SIZE)) {
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        memset(&shared_state.info, 0, sizeof(shared_state.info));
        memcpy(&shared_state.info,
               incoming_data + ESPNOW_HEADER_SIZE,
               (sizeof(shared_state.info) < payload_len) ? sizeof(shared_state.info) : payload_len);
        shared_state.has_info = true;
        shared_state.dirty = true;
    // BAT_STATUS updates live pack measurements.
    } else if (header.esp_message_type == BAT_STATUS &&
               len > static_cast<int>(ESPNOW_HEADER_SIZE)) {
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        memset(&shared_state.status, 0, sizeof(shared_state.status));
        memcpy(&shared_state.status,
               incoming_data + ESPNOW_HEADER_SIZE,
               (sizeof(shared_state.status) < payload_len) ? sizeof(shared_state.status) : payload_len);
        shared_state.has_status = true;
        shared_state.dirty = true;
    // BAT_AUX_INFO carries isolation resistance and similar secondary values.
    } else if (header.esp_message_type == BAT_AUX_INFO &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + sizeof(BATTERY_AUX_INFO_TYPE))) {
        memcpy(&shared_state.aux_info,
               incoming_data + ESPNOW_HEADER_SIZE,
               sizeof(BATTERY_AUX_INFO_TYPE));
        shared_state.has_aux_info = true;
        shared_state.dirty = true;
    // BAT_REMOTE_NET_INFO is parsed even though the current UI does not show it.
    } else if (header.esp_message_type == BAT_REMOTE_NET_INFO &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + sizeof(BATTERY_REMOTE_NET_INFO_TYPE))) {
        memcpy(&shared_state.remote_net_info,
               incoming_data + ESPNOW_HEADER_SIZE,
               sizeof(BATTERY_REMOTE_NET_INFO_TYPE));
        shared_state.has_remote_net_info = true;
        shared_state.dirty = true;
    // BAT_CELL_STATUS holds compact 8-bit cell bars.
    } else if (header.esp_message_type == BAT_CELL_STATUS &&
               len > static_cast<int>(ESPNOW_HEADER_SIZE)) {
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        memset(&shared_state.cells, 0, sizeof(shared_state.cells));
        memcpy(&shared_state.cells,
               incoming_data + ESPNOW_HEADER_SIZE,
               (sizeof(shared_state.cells) < payload_len) ? sizeof(shared_state.cells) : payload_len);
        shared_state.has_cells = true;
        shared_state.dirty = true;
    // BAT_CELL_STATUS_16BIT carries chunked full-resolution cell data.
    } else if (header.esp_message_type == BAT_CELL_STATUS_16BIT &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + 4U)) {
        const auto* chunk =
            reinterpret_cast<const BATTERY_CELL_STATUS_16BIT_CHUNK_TYPE*>(incoming_data + ESPNOW_HEADER_SIZE);
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        const size_t available_cell_bytes = payload_len - 4U;
        const uint8_t max_cells_in_message = static_cast<uint8_t>(available_cell_bytes / sizeof(uint16_t));
        const uint8_t cells_in_chunk = (chunk->cells_in_chunk < max_cells_in_message) ? chunk->cells_in_chunk
                                                                                       : max_cells_in_message;
        const uint8_t total_cells = (chunk->total_cells <= MAX_UI_CHART_POINTS) ? chunk->total_cells
                                                                                 : MAX_UI_CHART_POINTS;

        if (chunk->transfer_id != shared_state.cell16_transfer_id) {
            shared_state.cell16_transfer_id = chunk->transfer_id;
            shared_state.cell16_total_cells = total_cells;
            shared_state.cell16_expected_chunks = static_cast<uint8_t>((total_cells + cells_in_chunk - 1U) /
                                                                         (cells_in_chunk == 0 ? 1U : cells_in_chunk));
            shared_state.cell16_received_chunks_mask = 0;
            memset(shared_state.cells_16bit_mV, 0, sizeof(shared_state.cells_16bit_mV));
        }

        for (uint8_t i = 0; i < cells_in_chunk; ++i) {
            const uint16_t target_index = static_cast<uint16_t>(chunk->start_index) + i;
            if (target_index >= MAX_UI_CHART_POINTS) {
                break;
            }
            shared_state.cells_16bit_mV[target_index] = chunk->cell_voltages_mV[i];
        }

        if (cells_in_chunk > 0) {
            const uint8_t chunk_slot =
                (cells_in_chunk == 0) ? 0U : static_cast<uint8_t>(chunk->start_index / cells_in_chunk);
            if (chunk_slot < 8U) {
                shared_state.cell16_received_chunks_mask |= (1U << chunk_slot);
            }
        }

        shared_state.last_cells_16bit_ms = millis();
        shared_state.has_cells_16bit = true;
        shared_state.dirty = true;
    // BAT_FAULT_TEXT reassembles a larger plain-text faults message.
    } else if (header.esp_message_type == BAT_FAULT_TEXT &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + 4U)) {
        const auto* chunk =
            reinterpret_cast<const BATTERY_FAULT_TEXT_CHUNK_TYPE*>(incoming_data + ESPNOW_HEADER_SIZE);
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        const size_t available_text_bytes = payload_len - 4U;
        const uint8_t text_length = (chunk->text_length < available_text_bytes) ? chunk->text_length
                                                                                 : static_cast<uint8_t>(available_text_bytes);

        if (chunk->transfer_id != shared_state.fault_transfer_id) {
            shared_state.fault_transfer_id = chunk->transfer_id;
            shared_state.fault_expected_chunks = chunk->total_chunks;
            shared_state.fault_received_chunks_mask = 0;
            memset(shared_state.fault_text, 0, sizeof(shared_state.fault_text));
        }

        const size_t text_offset = static_cast<size_t>(chunk->chunk_index) * 240U;
        if (text_offset < sizeof(shared_state.fault_text)) {
            const size_t bytes_to_copy =
                (text_offset + text_length <= sizeof(shared_state.fault_text) - 1U)
                    ? text_length
                    : (sizeof(shared_state.fault_text) - 1U - text_offset);
            memcpy(shared_state.fault_text + text_offset, chunk->text, bytes_to_copy);
            shared_state.fault_text[text_offset + bytes_to_copy] = '\0';
        }

        if (chunk->chunk_index < 32U) {
            shared_state.fault_received_chunks_mask |= (1UL << chunk->chunk_index);
        }
        shared_state.last_fault_text_ms = millis();
        shared_state.has_fault_text = true;
        shared_state.dirty = true;
    // BAT_FAULT_EVENTS reassembles structured fault/event records.
    } else if (header.esp_message_type == BAT_FAULT_EVENTS &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + 4U)) {
        const auto* chunk =
            reinterpret_cast<const BATTERY_FAULT_EVENTS_CHUNK_TYPE*>(incoming_data + ESPNOW_HEADER_SIZE);
        const size_t payload_len = static_cast<size_t>(len) - ESPNOW_HEADER_SIZE;
        const size_t available_record_bytes = payload_len - 4U;
        const uint8_t max_records_in_message =
            static_cast<uint8_t>(available_record_bytes / sizeof(BATTERY_FAULT_EVENT_RECORD_TYPE));
        const uint8_t records_in_chunk =
            (chunk->records_in_chunk < max_records_in_message) ? chunk->records_in_chunk : max_records_in_message;
        const uint8_t capped_total_events = (chunk->total_events <= 128U) ? chunk->total_events : 128U;

        if (chunk->transfer_id != shared_state.fault_events_transfer_id) {
            shared_state.fault_events_transfer_id = chunk->transfer_id;
            shared_state.fault_event_count = capped_total_events;
            memset(shared_state.fault_events, 0, sizeof(shared_state.fault_events));
        }

        for (uint8_t i = 0; i < records_in_chunk; ++i) {
            const uint16_t target_index = static_cast<uint16_t>(chunk->start_index) + i;
            if (target_index >= 128U) {
                break;
            }
            shared_state.fault_events[target_index] = chunk->records[i];
        }

        if (chunk->total_events == 0U) {
            shared_state.fault_event_count = 0;
        }

        shared_state.last_fault_events_ms = millis();
        shared_state.has_fault_events = true;
        shared_state.dirty = true;
    // BAT_REMOTE_STATE drives inverter/contactors/pause secret-screen info and ACKs.
    } else if (header.esp_message_type == BAT_REMOTE_STATE &&
               len >= static_cast<int>(ESPNOW_HEADER_SIZE + sizeof(BATTERY_REMOTE_STATE_TYPE))) {
        memcpy(&shared_state.remote_state,
               incoming_data + ESPNOW_HEADER_SIZE,
               sizeof(BATTERY_REMOTE_STATE_TYPE));
        shared_state.last_remote_state_ms = millis();
        shared_state.has_remote_state = true;
        clear_pending_command_if_ack_matches(shared_state, shared_state.remote_state);
        shared_state.dirty = true;
    } else {
        shared_state.ignored_packet_count++;
    }
    portEXIT_CRITICAL_ISR(&g_state_mux);
}

}  // namespace

void espnow_receiver_init() {
    // Show firmware version on both possible main-screen layouts.
    set_label_text(ui_Version1, kScreenVersionText);
    set_label_text(ui_Version2, kScreenVersionText);

    // Prepare the RGB status LED.
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    set_status_led(0, 0, 255);

    // Receiver works best without Wi-Fi sleep.
    WiFi.setSleep(false);

    // If the screen is on STA Wi-Fi, use its current channel.
    // Otherwise stay on the fixed ESP-NOW channel.
    const int active_channel = screen_network_is_sta_connected() ? screen_network_sta_channel() : ESPNOW_WIFI_CHANNEL;
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(active_channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // Start ESP-NOW stack.
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    // Register a broadcast peer so command send path can use esp_now_send.
    esp_now_peer_info_t peer_info{};
    memcpy(peer_info.peer_addr, g_broadcast_address, sizeof(g_broadcast_address));
    peer_info.channel = ESPNOW_WIFI_CHANNEL;
    peer_info.encrypt = false;
    if (!esp_now_is_peer_exist(g_broadcast_address)) {
        if (esp_now_add_peer(&peer_info) != ESP_OK) {
            Serial.println("ESP-NOW broadcast peer add failed");
        }
    }

    // Register receive callback after ESP-NOW is ready.
    esp_now_register_recv_cb(on_data_recv);
    Serial.printf("ESP-NOW receiver ready on WiFi channel %d\n", active_channel);
    if (screen_network_is_sta_connected() && active_channel != ESPNOW_WIFI_CHANNEL) {
        Serial.printf("ESP-NOW warning: STA uses channel %d while sender expects %u\n",
                      active_channel,
                      static_cast<unsigned>(ESPNOW_WIFI_CHANNEL));
    }
}

// Public helper used by main.cpp right after ui_init().
void espnow_receiver_show_main_screen() {
    load_runtime_main_screen();
}

void espnow_receiver_update() {
    // Rebind runtime callbacks after any SquareLine screen recreation.
    install_navigation_overrides();
    install_screen4_controls();

    // Fix active screen if single/double battery mode changed.
    enforce_runtime_screen_flow();

    // Secret-screen network labels are updated only when screen 4 is visible.
    const bool screen4_active = is_screen4_active();
    const bool sta_connected = screen_network_is_sta_connected();
    const bool network_state_changed = !g_has_last_sta_connected || (g_last_sta_connected != sta_connected);
    if (screen4_active && (!g_screen4_was_active || network_state_changed)) {
        update_screen4_network_labels();
    }
    g_screen4_was_active = screen4_active;
    g_last_sta_connected = sta_connected;
    g_has_last_sta_connected = true;

    // Process both battery slots independently.
    for (uint8_t slot = 0; slot < BATTERY_SLOT_COUNT; ++slot) {
        UiBatteryState snapshot{};
        bool dirty = false;

        // Copy shared callback-owned state into a local snapshot so LVGL work
        // can happen outside the critical section.
        portENTER_CRITICAL(&g_state_mux);
        const SharedBatteryState &shared_state = g_shared_states[slot];
        snapshot.has_info = shared_state.has_info;
        snapshot.has_status = shared_state.has_status;
        snapshot.has_cells = shared_state.has_cells;
        snapshot.has_cells_16bit = shared_state.has_cells_16bit;
        snapshot.has_fault_text = shared_state.has_fault_text;
        snapshot.has_fault_events = shared_state.has_fault_events;
        snapshot.has_aux_info = shared_state.has_aux_info;
        snapshot.has_remote_net_info = shared_state.has_remote_net_info;
        snapshot.has_remote_state = shared_state.has_remote_state;
        snapshot.last_packet_ms = shared_state.last_packet_ms;
        snapshot.emulator_id = shared_state.emulator_id;
        snapshot.battery_id = shared_state.battery_id;
        snapshot.last_message_type = shared_state.last_message_type;
        snapshot.last_message_len = shared_state.last_message_len;
        snapshot.rx_packet_count = shared_state.rx_packet_count;
        snapshot.ignored_packet_count = shared_state.ignored_packet_count;
        snapshot.rx_event_pending = shared_state.rx_event_pending;
        snapshot.info = shared_state.info;
        snapshot.status = shared_state.status;
        snapshot.aux_info = shared_state.aux_info;
        snapshot.remote_net_info = shared_state.remote_net_info;
        snapshot.cells = shared_state.cells;
        memcpy(snapshot.cells_16bit_mV, shared_state.cells_16bit_mV, sizeof(snapshot.cells_16bit_mV));
        snapshot.last_cells_16bit_ms = shared_state.last_cells_16bit_ms;
        snapshot.cell16_total_cells = shared_state.cell16_total_cells;
        memcpy(snapshot.fault_text, shared_state.fault_text, sizeof(snapshot.fault_text));
        snapshot.last_fault_text_ms = shared_state.last_fault_text_ms;
        memcpy(snapshot.fault_events, shared_state.fault_events, sizeof(snapshot.fault_events));
        snapshot.fault_event_count = shared_state.fault_event_count;
        snapshot.last_fault_events_ms = shared_state.last_fault_events_ms;
        snapshot.remote_state = shared_state.remote_state;
        snapshot.last_remote_state_ms = shared_state.last_remote_state_ms;
        snapshot.last_sent_command_sequence = shared_state.last_sent_command_sequence;
        snapshot.pending_command_sequence = shared_state.pending_command_sequence;
        snapshot.pending_command_id = shared_state.pending_command_id;
        snapshot.pending_command_value = shared_state.pending_command_value;
        snapshot.pending_command_sent_ms = shared_state.pending_command_sent_ms;
        snapshot.command_pending = shared_state.command_pending;
        dirty = shared_state.dirty;
        g_shared_states[slot].dirty = false;
        g_shared_states[slot].rx_event_pending = false;
        portEXIT_CRITICAL(&g_state_mux);

        // Print RX summary only occasionally to avoid serial spam.
        if (snapshot.rx_event_pending) {
            const uint32_t now = millis();
            if (now - g_last_rx_log_ms >= RX_LOG_INTERVAL_MS) {
                g_last_rx_log_ms = now;
                Serial.printf("ESP-NOW RX #%lu: type=%u len=%d emulator=%u battery=%u ignored=%lu hasInfo=%u hasStatus=%u\n",
                              static_cast<unsigned long>(snapshot.rx_packet_count),
                              static_cast<unsigned>(snapshot.last_message_type),
                              snapshot.last_message_len,
                              static_cast<unsigned>(snapshot.emulator_id),
                              static_cast<unsigned>(snapshot.battery_id),
                              static_cast<unsigned long>(snapshot.ignored_packet_count),
                              snapshot.has_info ? 1U : 0U,
                              snapshot.has_status ? 1U : 0U);
            }
        }

        // Redraw when something changed, or when data became stale.
        if (dirty || !snapshot.has_status || (millis() - snapshot.last_packet_ms > PACKET_TIMEOUT_MS)) {
            apply_cached_or_waiting_ui(slot, snapshot);
        }
    }
}
