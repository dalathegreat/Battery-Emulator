#include "OMODA-E5-BATTERY.h"
#include <cstring>  //memcpy for PID payloads
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "BATTERIES.h"

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

// Renders characters if printable, otherwise as [xx] hex.
static void print_chars_or_hex(char* buf, const uint8_t* data, uint16_t length) {
  int ptr = 0;
  for (int i = 0; i < length && ptr < 62; i++) {
    if (data[i] >= 32 && data[i] <= 126) {
      buf[ptr++] = (char)data[i];
    } else {
      int written = sprintf(buf + ptr, "[%02x]", data[i]);
      ptr += written;
    }
  }
  buf[ptr] = '\0';
}

String OmodaE5Battery::get_uds_info_html() {
  String content;
  content.reserve(700);
  char buf[128];

  // clang-format off
  content << "<h4>VIN: ";
  print_chars_or_hex(buf, pid_vin, sizeof(pid_vin));
  content << buf << "</h4>"
             "<h4>ECU software number: ";
  print_chars_or_hex(buf, pid_ecu_software_number, sizeof(pid_ecu_software_number));
  content << buf << "</h4>"
             "<h4>ECU hardware number: ";
  print_chars_or_hex(buf, pid_ecu_hardware_number, sizeof(pid_ecu_hardware_number));
  // clang-format on

  return content;
}

void OmodaE5Battery::update_values() {}

void OmodaE5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (DID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {
    case 0x3ea:
    case 0x3f2: {  // Cell voltages (0x3ea: cells 0–79, 0x3f2: cells 80–113)
      uint8_t frame_seq = rx_frame.data.u8[0];

      if (frame_seq >= 1 && frame_seq <= 20) {
        uint16_t id_offset = (rx_frame.ID == 0x3f2) ? 80 : 0;
        uint16_t base_cell_id = id_offset + (frame_seq - 1) * 4;

        // Unpack 4 contiguous 14-bit integers across bytes 1–7
        uint16_t v[4];
        v[0] = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);
        v[1] = ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4);
        v[2] = ((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[6] >> 6);
        v[3] = ((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7];

        // Store parsed voltages, filtering zero-padded unused frame slots
        for (uint8_t i = 0; i < 4; i++) {
          uint16_t cell_id = base_cell_id + i;
          if (cell_id < datalayer_battery->info.number_of_cells) {
            uint16_t voltage = v[i];
            datalayer_battery->status.cell_voltages_mV[cell_id] = voltage;
          }
        }
      }
      break;
    }
    default:
      break;
  }
}

uint16_t OmodaE5Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_POLL_VIN:
      memcpy(pid_vin, data, length > sizeof(pid_vin) ? sizeof(pid_vin) : length);
      break;
    case PID_POLL_ECU_SOFTWARE_NUMBER:
      memcpy(pid_ecu_software_number, data,
             length > sizeof(pid_ecu_software_number) ? sizeof(pid_ecu_software_number) : length);
      break;
    case PID_POLL_ECU_HARDWARE_NUMBER:
      memcpy(pid_ecu_hardware_number, data,
             length > sizeof(pid_ecu_hardware_number) ? sizeof(pid_ecu_hardware_number) : length);
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void OmodaE5Battery::transmit_can(unsigned long currentMillis) {
  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);

  // TODO: Send periodic keepalive/status CAN frames here once they are mapped
}

void OmodaE5Battery::setup(void) {  // Performs one time setup at startup
  // UDS: send requests to 0x79B, accept replies from the BMS on 0x7BB.
  // Same addresses as the other Chery-platform integration (CMFA-EV);
  // TODO: confirm against the real BMS.
  setup_uds(0x7DF, 0, UdsCanBatteryOptions{.fd = true});

  static const uint16_t pid_scan_list[] = {
      PID_POLL_VIN,
      PID_POLL_ECU_SOFTWARE_NUMBER,
      PID_POLL_ECU_HARDWARE_NUMBER,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 114;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
