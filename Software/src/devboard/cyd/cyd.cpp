// This sends Battery Emulator data over ESPNow using the CYD packet format
// Maximum message size for ESPNow V1 is 250 bytes

#include "cyd.h"

#ifdef SMALL_FLASH_DEVICE

// CYD transport disabled on small flash devices
void init_cyd() {}
void update_cyd() {}

#else

#include <WiFi.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <algorithm>
#include <vector>

#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../hal/hal.h"
#include "../safety/safety.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#include "../wifi/wifi.h"

namespace {

namespace cyd_protocol {

struct BATTERY_INFO_TYPE {
  uint32_t total_capacity_Wh = 30000;
  uint32_t reported_total_capacity_Wh = 30000;
  uint16_t max_design_voltage_dV = 5000;
  uint16_t min_design_voltage_dV = 2500;
  uint16_t max_cell_voltage_mV = 4300;
  uint16_t min_cell_voltage_mV = 2700;
  uint16_t max_cell_voltage_deviation_mV = 500;
  uint8_t number_of_cells = 0;
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
};  // 24 bytes

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
  uint16_t soh_pptt = 9900;
  uint16_t voltage_dV = 3700;
  uint16_t cell_max_voltage_mV = 3700;
  uint16_t cell_min_voltage_mV = 3700;
  uint16_t real_soc = 0;
  uint16_t reported_soc = 0;
  uint16_t CAN_error_counter = 0;
  int16_t temperature_max_dC = 0;
  int16_t temperature_min_dC = 0;
  int16_t current_dA = 0;
  int16_t reported_current_dA = 0;
  uint8_t CAN_battery_still_alive = CAN_STILL_ALIVE;
  real_bms_status_enum real_bms_status = BMS_DISCONNECTED;
  led_mode_enum led_mode = CLASSIC;
  balancing_status_enum balancing_status = BALANCING_STATUS_UNKNOWN;
};  // 80 bytes

struct BATTERY_BALANCING_STATUS_TYPE {
  bool cell_balancing_status[MAX_AMOUNT_CELLS];
  uint8_t number_of_cells = 0;
};  // 193 bytes

struct BATTERY_CELL_STATUS_TYPE {
  uint8_t cell_voltages_mV[MAX_AMOUNT_CELLS];
  uint8_t number_of_cells = 0;
};  // 193 bytes

struct BATTERY_CELL_STATUS_16BIT_CHUNK_TYPE {
  uint8_t transfer_id;
  uint8_t total_cells;
  uint8_t start_index;
  uint8_t cells_in_chunk;
  uint16_t cell_voltages_mV[];
} __packed;

struct BATTERY_FAULT_TEXT_CHUNK_TYPE {
  uint8_t transfer_id;
  uint8_t total_chunks;
  uint8_t chunk_index;
  uint8_t text_length;
  char text[];
} __packed;

struct BATTERY_FAULT_EVENT_RECORD_TYPE {
  uint8_t event_id;
  uint8_t level;
  uint8_t data;
  uint8_t count;
  uint32_t age_seconds;
} __packed;

struct BATTERY_FAULT_EVENTS_CHUNK_TYPE {
  uint8_t transfer_id;
  uint8_t total_events;
  uint8_t start_index;
  uint8_t records_in_chunk;
  BATTERY_FAULT_EVENT_RECORD_TYPE records[];
} __packed;

struct BATTERY_AUX_INFO_TYPE {
  uint32_t isolation_resistance_kohm;
} __packed;

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
} __packed;

enum espnow_remote_command_id : uint8_t {
  REMOTE_CMD_SET_PAUSE = 1,
  REMOTE_CMD_SET_CONTACTORS = 2,
  REMOTE_CMD_REBOOT = 3
};

enum espnow_remote_command_result : uint8_t {
  REMOTE_CMD_RESULT_NONE = 0,
  REMOTE_CMD_RESULT_OK = 1,
  REMOTE_CMD_RESULT_INVALID = 2
};

struct BATTERY_REMOTE_COMMAND_TYPE {
  uint8_t command_id;
  uint8_t command_sequence;
  uint8_t command_value;
  uint8_t reserved;
} __packed;

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
  BAT_AUX_INFO = 10
};

struct ESPNOW_BATTERY_MESSAGE {
  uint16_t emulator_id;
  uint8_t battery_id;
  uint8_t esp_message_type;
  uint8_t esp_message[];
} __packed;

}  // namespace cyd_protocol

constexpr uint8_t kBroadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr size_t kEspNowTotalPayloadLimit = 250;
constexpr size_t kEspNowHeaderSize = 4;
constexpr size_t kCell16BitChunkMetaSize = 4;
constexpr size_t kFaultTextChunkMetaSize = 4;
constexpr size_t kFaultEventsChunkMetaSize = 4;
constexpr size_t kMaxCell16BitValuesPerChunk =
    (kEspNowTotalPayloadLimit - kEspNowHeaderSize - kCell16BitChunkMetaSize) / sizeof(uint16_t);
constexpr size_t kMaxFaultTextBytesPerChunk = kEspNowTotalPayloadLimit - kEspNowHeaderSize - kFaultTextChunkMetaSize;
constexpr size_t kMaxFaultEventRecordsPerChunk =
    (kEspNowTotalPayloadLimit - kEspNowHeaderSize - kFaultEventsChunkMetaSize) /
    sizeof(cyd_protocol::BATTERY_FAULT_EVENT_RECORD_TYPE);

esp_now_peer_info_t g_cyd_peer = {};
bool g_cyd_initialized = false;
unsigned long g_last_update_ms = 0;
int g_num_batteries = 1;
uint16_t g_emulator_id = 0;
uint8_t g_cell_16bit_transfer_id = 0;
uint8_t g_fault_text_transfer_id = 0;
uint8_t g_fault_events_transfer_id = 0;
std::vector<EventData> g_order_events;
portMUX_TYPE g_remote_command_mux = portMUX_INITIALIZER_UNLOCKED;
uint8_t g_remote_ack_sequence = 0;
uint8_t g_remote_ack_command_id = 0;
uint8_t g_remote_ack_result = cyd_protocol::REMOTE_CMD_RESULT_NONE;
bool g_remote_state_force_send = false;
bool g_remote_reboot_requested = false;
uint32_t g_remote_reboot_due_ms = 0;

struct PendingRemoteCommand {
  bool pending = false;
  uint8_t command_id = 0;
  uint8_t command_sequence = 0;
  uint8_t command_value = 0;
};

PendingRemoteCommand g_remote_pending_command;

void queue_remote_ack(uint8_t command_sequence, uint8_t command_id, uint8_t result) {
  g_remote_ack_sequence = command_sequence;
  g_remote_ack_command_id = command_id;
  g_remote_ack_result = result;
  g_remote_state_force_send = true;
}

void process_pending_remote_command() {
  PendingRemoteCommand command;

  portENTER_CRITICAL(&g_remote_command_mux);
  command = g_remote_pending_command;
  g_remote_pending_command.pending = false;
  portEXIT_CRITICAL(&g_remote_command_mux);

  if (!command.pending) {
    return;
  }

  switch (command.command_id) {
    case cyd_protocol::REMOTE_CMD_SET_PAUSE:
      setBatteryPause(command.command_value != 0U, false, datalayer.system.info.equipment_stop_active);
      queue_remote_ack(command.command_sequence, command.command_id, cyd_protocol::REMOTE_CMD_RESULT_OK);
      break;

    case cyd_protocol::REMOTE_CMD_SET_CONTACTORS:
      if (command.command_value == 0U) {
        setBatteryPause(true, false, true);
      } else {
        setBatteryPause(false, false, false);
      }
      queue_remote_ack(command.command_sequence, command.command_id, cyd_protocol::REMOTE_CMD_RESULT_OK);
      break;

    case cyd_protocol::REMOTE_CMD_REBOOT:
      queue_remote_ack(command.command_sequence, command.command_id, cyd_protocol::REMOTE_CMD_RESULT_OK);
      g_remote_reboot_requested = true;
      g_remote_reboot_due_ms = millis() + 200U;
      break;

    default:
      queue_remote_ack(command.command_sequence, command.command_id, cyd_protocol::REMOTE_CMD_RESULT_INVALID);
      break;
  }
}

void OnDataRecv(const esp_now_recv_info_t* recv_info, const uint8_t* incoming_data, int len) {
  (void)recv_info;

  if (incoming_data == nullptr ||
      len < static_cast<int>(kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_REMOTE_COMMAND_TYPE))) {
    return;
  }

  const auto* message = reinterpret_cast<const cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(incoming_data);
  if (message->battery_id != 1 || message->esp_message_type != cyd_protocol::BAT_REMOTE_COMMAND) {
    return;
  }

  const auto* remote_command = reinterpret_cast<const cyd_protocol::BATTERY_REMOTE_COMMAND_TYPE*>(message->esp_message);

  portENTER_CRITICAL(&g_remote_command_mux);
  g_remote_pending_command.pending = true;
  g_remote_pending_command.command_id = remote_command->command_id;
  g_remote_pending_command.command_sequence = remote_command->command_sequence;
  g_remote_pending_command.command_value = remote_command->command_value;
  portEXIT_CRITICAL(&g_remote_command_mux);
}

uint32_t get_isolation_resistance_kohm() {
  // Prefer values already reported in kilo-ohms
  if (datalayer_extended.meb.isolation_resistance > 0U) {
    return datalayer_extended.meb.isolation_resistance;
  }
  if (datalayer_extended.KiaHyundai64.isolation_resistance_kOhm > 0U) {
    return datalayer_extended.KiaHyundai64.isolation_resistance_kOhm;
  }
  if (datalayer_extended.nissanleaf.Insulation > 0U) {
    return datalayer_extended.nissanleaf.Insulation;
  }
  if (datalayer_extended.boltampera.battery_isolation_kohm > 0U) {
    return datalayer_extended.boltampera.battery_isolation_kohm;
  }
  if (datalayer_extended.stellantisECMP.pid_insulation_res > 0U &&
      datalayer_extended.stellantisECMP.pid_insulation_res != 255U) {
    return datalayer_extended.stellantisECMP.pid_insulation_res;
  }
  if (datalayer_extended.tesla.BMS_isolationResistance > 0U) {
    return static_cast<uint32_t>(datalayer_extended.tesla.BMS_isolationResistance) * 10U;
  }

  return 0U;
}

const DATALAYER_BATTERY_INFO_TYPE* get_battery_info(int battery_index) {
  if (battery_index == 0) {
    return &datalayer.battery.info;
  }
  if (battery_index == 1) {
    return &datalayer.battery2.info;
  }
  return &datalayer.battery3.info;
}

const DATALAYER_BATTERY_STATUS_TYPE* get_battery_status(int battery_index) {
  if (battery_index == 0) {
    return &datalayer.battery.status;
  }
  if (battery_index == 1) {
    return &datalayer.battery2.status;
  }
  return &datalayer.battery3.status;
}

void send_packet(const uint8_t* payload, size_t length, const char* error_text) {
  const esp_err_t result = esp_now_send(kBroadcastAddress, payload, length);
  if (result != ESP_OK) {
    logging.println(error_text);
  }
}

void send_battery_info(const DATALAYER_BATTERY_INFO_TYPE& info, uint8_t battery_index) {
  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  auto* out = reinterpret_cast<cyd_protocol::BATTERY_INFO_TYPE*>(message->esp_message);

  memcpy(out, &info, sizeof(cyd_protocol::BATTERY_INFO_TYPE));
  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_INFO;

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_INFO_TYPE),
              "Error sending the CYD Battery Info data");
}

void send_battery_status(const DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t battery_index) {
  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  auto* out = reinterpret_cast<cyd_protocol::BATTERY_STATUS_TYPE*>(message->esp_message);

  memcpy(out, &status, sizeof(cyd_protocol::BATTERY_STATUS_TYPE));
  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_STATUS;

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_STATUS_TYPE),
              "Error sending the CYD Battery Status data");
}

void send_battery_balancing(const DATALAYER_BATTERY_INFO_TYPE& info, const DATALAYER_BATTERY_STATUS_TYPE& status,
                            uint8_t battery_index) {
  cyd_protocol::BATTERY_BALANCING_STATUS_TYPE balancing{};
  balancing.number_of_cells = info.number_of_cells;
  for (uint8_t i = 0; i < balancing.number_of_cells; i++) {
    balancing.cell_balancing_status[i] = status.cell_balancing_status[i];
  }

  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  memcpy(message->esp_message, &balancing, sizeof(balancing));
  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_BALANCE;

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_BALANCING_STATUS_TYPE),
              "Error sending the CYD Battery Balancing data");
}

void send_battery_cell_status(const DATALAYER_BATTERY_INFO_TYPE& info, const DATALAYER_BATTERY_STATUS_TYPE& status,
                              uint8_t battery_index) {
  // Pack cell voltages into 8-bit values
  cyd_protocol::BATTERY_CELL_STATUS_TYPE cells{};
  cells.number_of_cells = info.number_of_cells;
  for (uint8_t i = 0; i < cells.number_of_cells; i++) {
    cells.cell_voltages_mV[i] = status.cell_voltages_mV[i] / 20U;
  }

  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  memcpy(message->esp_message, &cells, sizeof(cells));
  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_CELL_STATUS;

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_CELL_STATUS_TYPE),
              "Error sending the CYD Cell Status data");
}

void send_battery_cell_status_16bit(const DATALAYER_BATTERY_INFO_TYPE& info,
                                    const DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t battery_index) {
  // Send full 16-bit cell voltages in multiple chunks
  const uint8_t total_cells = std::min<uint8_t>(info.number_of_cells, MAX_AMOUNT_CELLS);
  if (total_cells == 0) {
    return;
  }

  for (uint8_t start_index = 0; start_index < total_cells; start_index += kMaxCell16BitValuesPerChunk) {
    const uint8_t cells_in_chunk =
        std::min<uint8_t>(kMaxCell16BitValuesPerChunk, static_cast<uint8_t>(total_cells - start_index));

    uint8_t payload[kEspNowTotalPayloadLimit] = {0};
    auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
    auto* chunk = reinterpret_cast<cyd_protocol::BATTERY_CELL_STATUS_16BIT_CHUNK_TYPE*>(message->esp_message);

    message->emulator_id = g_emulator_id;
    message->battery_id = battery_index + 1;
    message->esp_message_type = cyd_protocol::BAT_CELL_STATUS_16BIT;

    chunk->transfer_id = g_cell_16bit_transfer_id;
    chunk->total_cells = total_cells;
    chunk->start_index = start_index;
    chunk->cells_in_chunk = cells_in_chunk;

    for (uint8_t i = 0; i < cells_in_chunk; i++) {
      chunk->cell_voltages_mV[i] = status.cell_voltages_mV[start_index + i];
    }

    send_packet(payload, kEspNowHeaderSize + kCell16BitChunkMetaSize + cells_in_chunk * sizeof(uint16_t),
                "Error sending the CYD 16-bit Cell Status data");
  }

  g_cell_16bit_transfer_id++;
}

String build_fault_text() {
  g_order_events.clear();

  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if ((event_pointer->state == EVENT_STATE_ACTIVE) || (event_pointer->state == EVENT_STATE_ACTIVE_LATCHED)) {
      g_order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }

  if (g_order_events.empty()) {
    return "No faults";
  }

  std::sort(g_order_events.begin(), g_order_events.end(), compareEventsByTimestampDesc);

  String text;
  text.reserve(768);

  for (const auto& event : g_order_events) {
    const EVENTS_STRUCT_TYPE* event_pointer = event.event_pointer;
    const uint64_t age_ms = millis64() - event_pointer->timestamp;
    const uint32_t age_seconds = static_cast<uint32_t>(age_ms / 1000ULL);
    text += String(get_event_level_string(event.event_handle));
    text += " | ";
    text += String(get_event_enum_string(event.event_handle));
    text += " | age=";
    text += String(age_seconds);
    text += "s | data=";
    text += String(event_pointer->data);
    text += " | count=";
    text += String(event_pointer->occurences);
    text += "\n";
    text += get_event_message_string(event.event_handle);
    text += "\n\n";
  }

  g_order_events.clear();
  return text;
}

void send_fault_text(uint8_t battery_index) {
  // Send fault text in chunks
  const String fault_text = build_fault_text();
  const size_t total_length = fault_text.length();
  const uint8_t total_chunks =
      std::max<size_t>(1, (total_length + kMaxFaultTextBytesPerChunk - 1) / kMaxFaultTextBytesPerChunk);

  for (uint8_t chunk_index = 0; chunk_index < total_chunks; chunk_index++) {
    const size_t chunk_offset = chunk_index * kMaxFaultTextBytesPerChunk;
    const size_t bytes_remaining = (chunk_offset < total_length) ? (total_length - chunk_offset) : 0;
    const uint8_t text_length = std::min<size_t>(bytes_remaining, kMaxFaultTextBytesPerChunk);

    uint8_t payload[kEspNowTotalPayloadLimit] = {0};
    auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
    auto* chunk = reinterpret_cast<cyd_protocol::BATTERY_FAULT_TEXT_CHUNK_TYPE*>(message->esp_message);

    message->emulator_id = g_emulator_id;
    message->battery_id = battery_index + 1;
    message->esp_message_type = cyd_protocol::BAT_FAULT_TEXT;

    chunk->transfer_id = g_fault_text_transfer_id;
    chunk->total_chunks = total_chunks;
    chunk->chunk_index = chunk_index;
    chunk->text_length = text_length;

    if (text_length > 0) {
      memcpy(chunk->text, fault_text.c_str() + chunk_offset, text_length);
    }

    send_packet(payload, kEspNowHeaderSize + kFaultTextChunkMetaSize + text_length,
                "Error sending the CYD Fault Text data");
  }

  g_fault_text_transfer_id++;
}

void send_fault_events(uint8_t battery_index) {
  // Send structured event records
  g_order_events.clear();

  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if ((event_pointer->state == EVENT_STATE_ACTIVE) || (event_pointer->state == EVENT_STATE_ACTIVE_LATCHED)) {
      g_order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }

  std::sort(g_order_events.begin(), g_order_events.end(), compareEventsByTimestampDesc);
  const uint8_t total_events = static_cast<uint8_t>(std::min<size_t>(g_order_events.size(), 255));

  if (total_events == 0) {
    uint8_t payload[kEspNowTotalPayloadLimit] = {0};
    auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
    auto* chunk = reinterpret_cast<cyd_protocol::BATTERY_FAULT_EVENTS_CHUNK_TYPE*>(message->esp_message);

    message->emulator_id = g_emulator_id;
    message->battery_id = battery_index + 1;
    message->esp_message_type = cyd_protocol::BAT_FAULT_EVENTS;
    chunk->transfer_id = g_fault_events_transfer_id;
    chunk->total_events = 0;
    chunk->start_index = 0;
    chunk->records_in_chunk = 0;

    send_packet(payload, kEspNowHeaderSize + kFaultEventsChunkMetaSize, "Error sending the CYD Fault Events data");
    g_fault_events_transfer_id++;
    g_order_events.clear();
    return;
  }

  for (uint8_t start_index = 0; start_index < total_events; start_index += kMaxFaultEventRecordsPerChunk) {
    const uint8_t records_in_chunk =
        std::min<uint8_t>(kMaxFaultEventRecordsPerChunk, static_cast<uint8_t>(total_events - start_index));

    uint8_t payload[kEspNowTotalPayloadLimit] = {0};
    auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
    auto* chunk = reinterpret_cast<cyd_protocol::BATTERY_FAULT_EVENTS_CHUNK_TYPE*>(message->esp_message);

    message->emulator_id = g_emulator_id;
    message->battery_id = battery_index + 1;
    message->esp_message_type = cyd_protocol::BAT_FAULT_EVENTS;
    chunk->transfer_id = g_fault_events_transfer_id;
    chunk->total_events = total_events;
    chunk->start_index = start_index;
    chunk->records_in_chunk = records_in_chunk;

    for (uint8_t i = 0; i < records_in_chunk; i++) {
      const auto& event = g_order_events[start_index + i];
      const EVENTS_STRUCT_TYPE* event_pointer = event.event_pointer;
      const uint64_t age_ms = millis64() - event_pointer->timestamp;

      chunk->records[i].event_id = static_cast<uint8_t>(event.event_handle);
      chunk->records[i].level = static_cast<uint8_t>(event_pointer->level);
      chunk->records[i].data = event_pointer->data;
      chunk->records[i].count = event_pointer->occurences;
      chunk->records[i].age_seconds = static_cast<uint32_t>(age_ms / 1000ULL);
    }

    send_packet(payload,
                kEspNowHeaderSize + kFaultEventsChunkMetaSize +
                    records_in_chunk * sizeof(cyd_protocol::BATTERY_FAULT_EVENT_RECORD_TYPE),
                "Error sending the CYD Fault Events data");
  }

  g_fault_events_transfer_id++;
  g_order_events.clear();
}

void send_aux_info(uint8_t battery_index) {
  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  auto* aux = reinterpret_cast<cyd_protocol::BATTERY_AUX_INFO_TYPE*>(message->esp_message);

  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_AUX_INFO;
  aux->isolation_resistance_kohm = get_isolation_resistance_kohm();

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_AUX_INFO_TYPE),
              "Error sending the CYD Aux Info data");
}

void send_remote_state(uint8_t battery_index) {
  // Send current emulator and contactor state
  uint8_t payload[kEspNowTotalPayloadLimit] = {0};
  auto* message = reinterpret_cast<cyd_protocol::ESPNOW_BATTERY_MESSAGE*>(payload);
  auto* remote_state = reinterpret_cast<cyd_protocol::BATTERY_REMOTE_STATE_TYPE*>(message->esp_message);

  message->emulator_id = g_emulator_id;
  message->battery_id = battery_index + 1;
  message->esp_message_type = cyd_protocol::BAT_REMOTE_STATE;

  remote_state->ack_sequence = g_remote_ack_sequence;
  remote_state->ack_command_id = g_remote_ack_command_id;
  remote_state->ack_result = g_remote_ack_result;
  remote_state->pause_status = static_cast<uint8_t>(emulator_pause_status);
  remote_state->pause_request_on = emulator_pause_request_ON ? 1U : 0U;
  remote_state->equipment_stop_active = datalayer.system.info.equipment_stop_active ? 1U : 0U;
  remote_state->system_status = static_cast<uint8_t>(datalayer.system.status.system_status);
  remote_state->contactors_engaged = datalayer.system.status.contactors_engaged;
  remote_state->inverter_allows_contactor_closing = datalayer.system.status.inverter_allows_contactor_closing ? 1U : 0U;
  remote_state->battery1_contactors_engaged = datalayer.system.status.contactors_engaged;
  remote_state->battery2_contactors_engaged = datalayer.system.status.contactors_battery2_engaged ? 1U : 0U;

  send_packet(payload, kEspNowHeaderSize + sizeof(cyd_protocol::BATTERY_REMOTE_STATE_TYPE),
              "Error sending the CYD Remote State data");
}

}  // namespace

void init_cyd() {
  if (!cyd_enabled) {
    return;
  }

  if ((WiFi.getMode() != WIFI_AP_STA) && (WiFi.getMode() != WIFI_STA)) {
    logging.println("WiFi should be initialized before using CYD ESP-NOW");
    return;
  }

  const esp_err_t init_result = esp_now_init();
  if (init_result != ESP_OK) {
    logging.println("CYD ESP-NOW init returned non-OK, continuing with peer setup");
  }

  ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));

  memcpy(g_cyd_peer.peer_addr, kBroadcastAddress, sizeof(kBroadcastAddress));
  g_cyd_peer.channel = 0;
  g_cyd_peer.encrypt = false;

  const esp_err_t peer_result = esp_now_add_peer(&g_cyd_peer);
  if (peer_result != ESP_OK && peer_result != ESP_ERR_ESPNOW_EXIST) {
    logging.println("Failed to add CYD ESP-NOW peer");
    return;
  }

  g_emulator_id = ESP.getEfuseMac() & 0xFFFF;
  g_num_batteries = 1;
  if (battery2) {
    g_num_batteries++;
  }
  if (battery3) {
    g_num_batteries++;
  }

  g_cyd_initialized = true;
}

void update_cyd() {
  if (!cyd_enabled || !g_cyd_initialized) {
    return;
  }

  process_pending_remote_command();

  if (g_remote_state_force_send) {
    send_remote_state(0);
    g_remote_state_force_send = false;
  }

  if (g_remote_reboot_requested && millis() >= g_remote_reboot_due_ms) {
    ESP.restart();
    return;
  }

  const unsigned long current_millis = millis();
  if (current_millis - g_last_update_ms < 1000U) {
    return;
  }

  for (int battery_index = 0; battery_index < g_num_batteries; battery_index++) {
    const DATALAYER_BATTERY_INFO_TYPE* info = get_battery_info(battery_index);
    const DATALAYER_BATTERY_STATUS_TYPE* status = get_battery_status(battery_index);

    send_battery_info(*info, battery_index);
    send_battery_status(*status, battery_index);
    send_aux_info(battery_index);
    send_battery_cell_status(*info, *status, battery_index);
    send_battery_cell_status_16bit(*info, *status, battery_index);
    send_battery_balancing(*info, *status, battery_index);
    send_fault_events(battery_index);
    send_fault_text(battery_index);
    send_remote_state(battery_index);
  }

  g_last_update_ms = current_millis;
}

#endif
