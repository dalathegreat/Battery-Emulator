#include "BYD-MODBUS.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"
#include "../lib/eModbus-eModbus/scripts/mbServerFCs.h"

// For modbus register definitions, see https://gitlab.com/pelle8/inverter_resources/-/blob/main/byd_registers_modbus_rtu.md

#define HISTORY_LENGTH 3  // Amount of samples(minutes) that needs to match for register to be considered stale
static unsigned long previousMillis60s = 0;  // will store last time a 60s event occured
static uint32_t user_configured_max_discharge_W = 0;
static uint32_t user_configured_max_charge_W = 0;
static uint32_t max_discharge_W = 0;
static uint32_t max_charge_W = 0;
static uint16_t register_401_history[HISTORY_LENGTH] = {0};
static uint8_t history_index = 0;
static uint8_t bms_char_dis_status = STANDBY;
static bool all_401_values_equal = false;

void BydModbusInverter::update_values() {
  verify_temperature();
  verify_inverter_modbus();
  handle_update_data_modbusp201_byd();
  handle_update_data_modbusp301_byd();
}

void BydModbusInverter::handle_static_data() {
  // Store the data into the array
  static uint16_t si_data[] = {21321, 1};
  static uint16_t byd_data[] = {16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static uint16_t battery_data[] = {16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752,
                                    20594, 25965, 26997, 27936, 18518, 0,     0,     0};
  static uint16_t volt_data[] = {13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12854, 0, 0, 0, 0, 0, 0};
  static uint16_t serial_data[] = {20528, 13104, 21552, 12848, 23089, 14641, 12593, 14384,
                                   12336, 12337, 0,     0,     0,     0,     0,     0};
  static uint16_t static_data[] = {1, 0};
  static uint16_t* data_array_pointers[] = {si_data, byd_data, battery_data, volt_data, serial_data, static_data};
  static uint16_t data_sizes[] = {sizeof(si_data),   sizeof(byd_data),    sizeof(battery_data),
                                  sizeof(volt_data), sizeof(serial_data), sizeof(static_data)};
  static uint16_t i = 100;
  for (uint8_t arr_idx = 0; arr_idx < sizeof(data_array_pointers) / sizeof(uint16_t*); arr_idx++) {
    uint16_t data_size = data_sizes[arr_idx];
    memcpy(&mbPV[i], data_array_pointers[arr_idx], data_size);
    i += data_size / sizeof(uint16_t);
  }
  static uint16_t init_p201[13] = {0, 0, 0, MAX_POWER, MAX_POWER, 0, 0, 53248, 10, 53248, 10, 0, 0};
  memcpy(&mbPV[200], init_p201, sizeof(init_p201));
  static uint16_t init_p301[24] = {0,  0,  128, 0, 0,  0,     0, 0, 0,  2000,  0,   2000,
                                   75, 95, 0,   0, 16, 22741, 0, 0, 13, 52064, 230, 9900};
  memcpy(&mbPV[300], init_p301, sizeof(init_p301));
}

void BydModbusInverter::handle_update_data_modbusp201_byd() {
  mbPV[202] = std::min(datalayer.battery.info.total_capacity_Wh, static_cast<uint32_t>(60000u));  //Cap to 60kWh
  mbPV[205] = (datalayer.battery.info.max_design_voltage_dV);  // Max Voltage, if higher Gen24 forces discharge
  mbPV[206] = (datalayer.battery.info.min_design_voltage_dV);  // Min Voltage, if lower Gen24 disables battery
}

void BydModbusInverter::handle_update_data_modbusp301_byd() {
  if (datalayer.battery.status.current_dA == 0) {
    bms_char_dis_status = STANDBY;
  } else if (datalayer.battery.status.current_dA < 0) {  //Negative value = Discharging
    bms_char_dis_status = DISCHARGING;
  } else {  //Positive value = Charging
    bms_char_dis_status = CHARGING;
  }
  // Convert max discharge Amp value to max Watt
  user_configured_max_discharge_W =
      ((datalayer.battery.settings.max_user_set_discharge_dA * datalayer.battery.info.max_design_voltage_dV) / 100);
  // Use the smaller value, battery reported value OR user configured value
  max_discharge_W = std::min(datalayer.battery.status.max_discharge_power_W, user_configured_max_discharge_W);

  // Convert max charge Amp value to max Watt
  user_configured_max_charge_W =
      ((datalayer.battery.settings.max_user_set_charge_dA * datalayer.battery.info.max_design_voltage_dV) / 100);
  // Use the smaller value, battery reported value OR user configured value
  max_charge_W = std::min(datalayer.battery.status.max_charge_power_W, user_configured_max_charge_W);

  if (datalayer.battery.status.bms_status == ACTIVE) {
    mbPV[308] = datalayer.battery.status.voltage_dV;
  } else {
    mbPV[308] = 0;
  }
  mbPV[300] = datalayer.battery.status.bms_status;
  mbPV[302] = 128 + bms_char_dis_status;
  mbPV[303] = datalayer.battery.status.reported_soc;
  mbPV[304] = std::min(datalayer.battery.info.total_capacity_Wh, static_cast<uint32_t>(60000u));  //Cap to 60kWh
  mbPV[305] =
      std::min(datalayer.battery.status.reported_remaining_capacity_Wh, static_cast<uint32_t>(60000u));  //Cap to 60kWh
  mbPV[306] = std::min(max_discharge_W, static_cast<uint32_t>(30000u));  //Cap to 30000 if exceeding
  mbPV[307] = std::min(max_charge_W, static_cast<uint32_t>(30000u));     //Cap to 30000 if exceeding
  mbPV[310] = datalayer.battery.status.voltage_dV;
  mbPV[312] = datalayer.battery.status.temperature_min_dC;
  mbPV[313] = datalayer.battery.status.temperature_max_dC;
  mbPV[323] = datalayer.battery.status.soh_pptt;
}

void BydModbusInverter::verify_temperature() {
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    return;  // Skip the following section
  }
  // This section checks if the battery temperature is negative, and incase it falls between -9.0 and -20.0C degrees
  // The Fronius Gen24 (and other Fronius inverters also affected), will stop charge/discharge if the battery gets colder than -10°C.
  // This is due to the original battery pack (BYD HVM), is a lithium iron phosphate battery, that cannot be charged in cold weather.
  // When using EV packs with NCM/LMO/NCA chemsitry, this is not a problem, since these chemistries are OK for outdoor cold use.
  if (datalayer.battery.status.temperature_min_dC < 0) {
    if (datalayer.battery.status.temperature_min_dC < -90 &&
        datalayer.battery.status.temperature_min_dC > -200) {  // Between -9.0 and -20.0C degrees
      datalayer.battery.status.temperature_min_dC = -90;       //Cap value to -9.0C
    }
  }
  if (datalayer.battery.status.temperature_max_dC < 0) {  // Signed value on negative side
    if (datalayer.battery.status.temperature_max_dC < -90 &&
        datalayer.battery.status.temperature_max_dC > -200) {  // Between -9.0 and -20.0C degrees
      datalayer.battery.status.temperature_max_dC = -90;       //Cap value to -9.0C
    }
  }
}

void BydModbusInverter::verify_inverter_modbus() {
  // Every 60 seconds, the Gen24 writes to this 401 register, alternating between 00FF and FF00.
  // We sample the register every 60 seconds. Incase the value has not changed for 3 minutes, we raise an event
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    all_401_values_equal = true;
    for (int i = 0; i < HISTORY_LENGTH; ++i) {
      if (register_401_history[i] != mbPV[401]) {
        all_401_values_equal = false;
        break;
      }
    }

    if (all_401_values_equal) {
      set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
    } else {
      clear_event(EVENT_MODBUS_INVERTER_MISSING);
    }

    // Update history
    register_401_history[history_index] = mbPV[401];
    history_index = (history_index + 1) % HISTORY_LENGTH;
  }
}

void BydModbusInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';

  // Init Static data to the RTU Modbus
  handle_static_data();

  // Init Serial2 connected to the RTU Modbus
  RTUutils::prepareHardwareSerial(Serial2);

  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  // Register served function code worker for server
  MBserver.registerWorker(MBTCP_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(MBTCP_ID, WRITE_HOLD_REGISTER, &FC06);
  MBserver.registerWorker(MBTCP_ID, WRITE_MULT_REGISTERS, &FC16);
  MBserver.registerWorker(MBTCP_ID, R_W_MULT_REGISTERS, &FC23);

  // Start ModbusRTU background task
  MBserver.begin(Serial2, MODBUS_CORE);
}
