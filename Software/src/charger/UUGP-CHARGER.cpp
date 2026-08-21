#include "UUGP-CHARGER.h"

#include <time.h>

#include "../datalayer/datalayer.h"
#include <cstring>

volatile uint16_t uugp_power_limit_W = 10000;
volatile uint16_t uugp_discharge_cutoff_soc = 80;
volatile bool uugp_allow_discharge_to_home_grid = false;
volatile uint8_t uugp_start_mode = 1;

UUGPCharger::UUGPCharger()
    : Charger(ChargerType::UUGP) {
  register_transmitter(this);
  register_receiver(this);
}

const char* UUGPCharger::name() {
  return Name;
}

float UUGPCharger::outputPowerDC() {
  return datalayer.charger.uugp_active_power_W
}

float UUGPCharger::HVDC_output_voltage() {
  return datalayer.charger.uugp_charging_voltage_V;
}

float UUGPCharger::HVDC_output_current() {
  return datalayer.charger.uugp_charging_current_A;
}

bool UUGPCharger::ensure_serial() {
  if (serial_initialized) {
    return true;
  }
  serial_initialized =
    rs485_begin(Name, serial, 9600, SERIAL_8N1);
  return serial_initialized;
}

uint16_t UUGPCharger::next_transaction() {
  ++transaction_id;

  if (transaction_id == 0) {
    transaction_id = 1;
  }

  return transaction_id;
}

void UUGPCharger::send_frame(uint8_t function,
                             const uint8_t* payload,
                             size_t payload_length) {
  /*
   * UUGP's supplied examples use:
   *
   * Transaction ID : 2
   * Protocol ID    : 2 (always 0)
   * Length         : 2
   * Unit ID        : 1
   * Function       : 1
   * Data           : N
   *
   * No CRC is present in the supplied examples.
   */

  const uint16_t transaction = next_transaction();
  expected_transaction_id = transaction;

  const uint16_t length =
      static_cast<uint16_t>(2 + payload_length);

  uint8_t header[8];

  header[0] = transaction >> 8;
  header[1] = transaction & 0xff;

  header[2] = 0;
  header[3] = 0;

  header[4] = length >> 8;
  header[5] = length & 0xff;

  header[6] = UNIT_ID;
  header[7] = function;

  serial.write(header, sizeof(header));

  if (payload_length > 0) {
    serial.write(payload, payload_length);
  }

  serial.flush();
}

void UUGPCharger::read_registers(uint8_t function,
                                 uint16_t address,
                                 uint16_t count) {
  uint8_t payload[4];

  payload[0] = address >> 8;
  payload[1] = address & 0xff;
  payload[2] = count >> 8;
  payload[3] = count & 0xff;

  expected_register = address;
  expected_count = count;

  send_frame(function, payload, sizeof(payload));
}

void UUGPCharger::write_single(uint16_t address,
                               uint16_t value) {
  uint8_t payload[4];

  payload[0] = address >> 8;
  payload[1] = address & 0xff;
  payload[2] = value >> 8;
  payload[3] = value & 0xff;

  send_frame(FC_WRITE_SINGLE, payload, sizeof(payload));
}

void UUGPCharger::write_multiple(uint16_t address,
                                 const uint16_t* values,
                                 uint16_t count) {
  if (count == 0 || count > 32) {
    return;
  }

  uint8_t payload[69];

  payload[0] = address >> 8;
  payload[1] = address & 0xff;
  payload[2] = count >> 8;
  payload[3] = count & 0xff;
  payload[4] = count * 2;

  for (uint16_t i = 0; i < count; ++i) {
    payload[5 + i * 2] = values[i] >> 8;
    payload[6 + i * 2] = values[i] & 0xff;
  }

  send_frame(FC_WRITE_MULTIPLE,
             payload,
             5 + count * 2);
}

uint16_t UUGPCharger::get_max_pack_voltage_dV() const {
  return datalayer.battery.info.max_design_voltage_dV;
}

uint16_t UUGPCharger::bms_power_limit_W() const {
  uint32_t power;

  /*
   * UUGP control mode:
   *
   * 0 = Stop
   * 1 = Charging
   * 2 = Discharging
   *
   * 1 -> BMS maximum charge power
   * 0/2 -> BMS maximum discharge power
   */
  if (datalayer.charger.uugp_control_mode == 1) {
    power = datalayer.battery.status.max_charge_power_W;
  } else {
    power = datalayer.battery.status.max_discharge_power_W;
  }

  if (power > 22000) {
    power = 22000;
  }

  return static_cast<uint16_t>(power);
}

void UUGPCharger::initialize_system_time() {
  time_t now = time(nullptr);

  struct tm local_time {};

  localtime_r(&now, &local_time);

  /*
   * ESP32/newlib exposes the local UTC offset through tm_gmtoff.
   * UUGP expects a signed 16-bit integer.
   *
   * Negative values therefore become two's-complement Uint16:
   *
   * -1 -> 0xffff
   * -2 -> 0xfffe
   * etc.
   */
  int32_t timezone_hours =
      local_time.tm_gmtoff / 3600;

  if (timezone_hours < -12) {
    timezone_hours = -12;
  }

  if (timezone_hours > 12) {
    timezone_hours = 12;
  }

  switch (initialization_step) {
    case 0:
      write_single(
          REG_TIMEZONE,
          static_cast<uint16_t>(
              static_cast<int16_t>(timezone_hours)));
      break;

    case 1:
      write_single(
          REG_YEAR,
          static_cast<uint16_t>(
              local_time.tm_year % 100));
      break;

    case 2:
      write_single(
          REG_DAY,
          static_cast<uint16_t>(
              local_time.tm_mday));
      break;

    case 3:
      write_single(
          REG_HOUR,
          static_cast<uint16_t>(
              local_time.tm_hour));
      break;

    case 4:
      write_single(
          REG_MINUTE,
          static_cast<uint16_t>(
              local_time.tm_min));
      break;

    case 5:
      write_single(
          REG_SECOND,
          static_cast<uint16_t>(
              local_time.tm_sec));
      break;
  }
}

void UUGPCharger::initialize_current_limiting() {
    switch (initialization_step) {     
        case 6:
          write_single(REG_POWER_LIMIT, 10000);
          break;
        case 7: {
            uint16_t soc = uugp_discharge_cutoff_soc;
            if (soc < 10 || soc > 90) {
                soc = 80;
            }
            write_single(REG_DISCHARGE_CUTOFF_SOC, soc);
            break;
        }
        case 8:
            write_single(REG_CONTROL_MODE, 0);
            break;
            }
}

void UUGPCharger::initialize_pcs_information() {
   const uint16_t max_voltage_dV = get_max_pack_voltage_dV();
   const uint16_t pcs_model = max_voltage_dV < 5700 ? 0 : 1;

   switch (initialization_step) {
     case 9:
       write_single(REG_VBUS_UPPER, max_voltage_dV);
       break;
     case 10:
       write_single(REG_VBUS_LOWER, max_voltage_dV);
       break;
     case 11:
       write_single(REG_PCS_MODEL, pcs_model);
       break;
   }
 }

void UUGPCharger::initialize_start_mode() {
  /*
   * 4033:
   *
   * 0 = Default 485
   * 1 = Card swipe
   * 2 = Plug & Charge
   *
   * Requested default = 1.
   */
  write_single(REG_START_MODE, uugp_start_mode);
if (uugp_start_mode > 2) {
   uugp_start_mode = 1;
 }
}

void UUGPCharger::initialize() {
  if (!ensure_serial()) {
    return;
  }

  switch (initialization_step) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      initialize_system_time();
      break;

    case 6:
    case 7:
    case 8:
      initialize_current_limiting();
      break;

    case 9:
    case 10:
    case 11:
      initialize_pcs_information();
      break; 
     
    case 12:
      initialize_start_mode();
      break;

      default:
      initialization_complete = true;
      return;
  }

  last_setting_ms = millis();

  ++initialization_step;

  if (initialization_step > 12) {
    initialization_complete = true;
  }
}

void UUGPCharger::update_power_limit() {
  uint16_t power_limit;

  if (uugp_allow_discharge_to_home_grid) {
    power_limit = uugp_power_limit_W;

    if (power_limit > 22000) {
      power_limit = 22000;
    }
  } else {
    power_limit = bms_power_limit_W();
  }

  write_single(REG_POWER_LIMIT, power_limit);
}

void UUGPCharger::poll_status() {
  if (millis() - last_status_ms < STATUS_INTERVAL_MS) {
    return;
  }

  last_status_ms = millis();

  switch (status_step) {
    case 0:
      /*
       * EV-side block:
       * 3020..3028
       */
      read_registers(
          FC_READ_INPUT,
          REG_EV_VOLTAGE,
          9);
      status_step = 1;
      break;

    case 1:
      /*
       * DC-side block:
       * 302F..3031
       */
      read_registers(
          FC_READ_INPUT,
          REG_DC_VOLTAGE,
          3);
      status_step = 2;
      break;

    case 2:
      /*
       * Charging information:
       * 3040..3044
       */
      read_registers(
          FC_READ_INPUT,
          REG_CHARGE_MODE,
          5);
      status_step = 3;
      break;

    default:
      /*
       * 4013 is controlled by the PCS/card/plug-and-charge logic,
       * so read its current value instead of continuously overriding it.
       */
      read_registers(
          FC_READ_HOLDING,
          REG_CONTROL_MODE,
          1);
      status_step = 0;
      break;
  }
}

void UUGPCharger::transmit(unsigned long currentMillis) {
  (void)currentMillis;

  if (!ensure_serial()) {
    return;
  }

  if (last_response_ms != 0 &&
     millis() - last_response_ms > 3000) {
   datalayer.charger.uugp_communication_ok = false;
  }

  if (!initialization_complete) {
    if (initialization_step == 0 ||
        millis() - last_setting_ms >= SETTING_INTERVAL_MS) {
      initialize();
    }

    return;
  }

  /*
   * Keep 4011 synchronized with the selected policy.
   *
   * UUGP requires >=15 seconds between setting operations.
   */
  if (millis() - last_setting_ms >= SETTING_INTERVAL_MS) {
    update_power_limit();
    last_setting_ms = millis();
  }

  poll_status();
}

void UUGPCharger::process_input_registers(
    uint16_t address,
    const uint16_t* values,
    uint16_t count) {

  if (address == REG_EV_VOLTAGE && count >= 9) {
    datalayer.charger.uugp_ev_voltage_V = values[0];
    datalayer.charger.uugp_ev_current_A = values[1];
    datalayer.charger.uugp_power_factor = values[3];
    datalayer.charger.uugp_max_output_voltage_V = values[6];
    datalayer.charger.uugp_max_output_current_A = values[7];
    datalayer.charger.uugp_rated_power_W = values[8];
    return;
  }

  if (address == REG_DC_VOLTAGE && count >= 3) {
    datalayer.charger.uugp_dc_voltage_V =
        values[0] * 0.1f;

    datalayer.charger.uugp_dc_current_A =
        static_cast<int16_t>(values[1]) * 0.1f;

    datalayer.charger.uugp_dc_power_derating_kW =
        values[2] * 0.01f;

    return;
  }

  if (address == REG_CHARGE_MODE && count >= 5) {
    datalayer.charger.uugp_control_mode = values[0];

    datalayer.charger.uugp_charging_voltage_V =
        values[1];

    datalayer.charger.uugp_charging_current_A =
        values[2];

    datalayer.charger.uugp_active_power_W =
        values[3];

    datalayer.charger.uugp_vehicle_soc =
        values[4];

    return;
  }
}

void UUGPCharger::process_holding_registers(
    uint16_t address,
    const uint16_t* values,
    uint16_t count) {

  if (address == REG_CONTROL_MODE && count >= 1) {
    datalayer.charger.uugp_control_mode =
        values[0];
  }
}

void UUGPCharger::process_response(
    const uint8_t* frame,
    size_t length) {

  if (length < 9) {
    return;
  }

  if (frame[2] != 0 ||
      frame[3] != 0 ||
      frame[6] != UNIT_ID) {
    return;
  }

  const uint8_t function = frame[7];

  if (function & 0x80) {
    return;
  }

  if (function == FC_WRITE_SINGLE ||
      function == FC_WRITE_MULTIPLE) {
    last_response_ms = millis();
    datalayer.charger.uugp_communication_ok = true;
    return;
  }

  if (function != FC_READ_INPUT &&
      function != FC_READ_HOLDING) {
    return;
  }
  const uint16_t response_transaction =
     (static_cast<uint16_t>(frame[0]) << 8) |
     frame[1];

  if (response_transaction != expected_transaction_id) {
    return;
  }

  const uint8_t byte_count = frame[8];

  if ((byte_count & 1) != 0 ||
      byte_count > 100 ||
      9 + byte_count > length) {
    return;
  }

  const uint16_t count =
      byte_count / 2;

  if (count != expected_count) {
    return;
  }

  uint16_t values[50];

  for (uint16_t i = 0; i < count; ++i) {
    values[i] =
        (static_cast<uint16_t>(
             frame[9 + i * 2])
         << 8) |
        frame[10 + i * 2];
  }

  if (function == FC_READ_INPUT) {
    process_input_registers(
        expected_register,
        values,
        count);
  } else {
    process_holding_registers(
        expected_register,
        values,
        count);
  }

  last_response_ms = millis();
  datalayer.charger.uugp_communication_ok = true;
}

void UUGPCharger::receive() {
  if (!serial_initialized) {
    return;
  }

  while (serial.available() > 0 &&
         rx_length < sizeof(rx_buffer)) {
    rx_buffer[rx_length++] =
        static_cast<uint8_t>(serial.read());
  }

  while (rx_length >= 8) {
    /*
     * Resynchronize to protocol identifier 0x0000.
     */
    if (rx_buffer[2] != 0 ||
        rx_buffer[3] != 0 ||
        rx_buffer[6] != UNIT_ID) {

      memmove(rx_buffer,
              rx_buffer + 1,
              --rx_length);

      continue;
    }

    const uint16_t length =
        (static_cast<uint16_t>(rx_buffer[4]) << 8) |
        rx_buffer[5];

    /*
     * length includes Unit ID + Function + data.
     */
    const size_t frame_length =
        6 + length;

    if (length < 2 ||
        frame_length > sizeof(rx_buffer)) {

      memmove(rx_buffer,
              rx_buffer + 1,
              --rx_length);

      continue;
    }

    if (rx_length < frame_length) {
      return;
    }

    process_response(
        rx_buffer,
        frame_length);

    const size_t remaining =
        rx_length - frame_length;

    if (remaining > 0) {
      memmove(
          rx_buffer,
          rx_buffer + frame_length,
          remaining);
    }

    rx_length = remaining;
  }
}
