#include "BATTERIES.h"
#ifdef BMW_I3_BATTERY
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "BMW-I3-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;     // will store last time a 10ms CAN Message was send
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis30 = 0;     // will store last time a 30ms CAN Message was send
static unsigned long previousMillis50 = 0;     // will store last time a 50ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis2000 = 0;   // will store last time a 2000ms CAN Message was send
static unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send
static const int interval10 = 10;              // interval (ms) at which send CAN Messages
static const int interval20 = 20;              // interval (ms) at which send CAN Messages
static const int interval30 = 30;              // interval (ms) at which send CAN Messages
static const int interval50 = 50;              // interval (ms) at which send CAN Messages
static const int interval100 = 100;            // interval (ms) at which send CAN Messages
static const int interval200 = 200;            // interval (ms) at which send CAN Messages
static const int interval500 = 500;            // interval (ms) at which send CAN Messages
static const int interval640 = 640;            // interval (ms) at which send CAN Messages
static const int interval1000 = 1000;          // interval (ms) at which send CAN Messages
static const int interval2000 = 2000;          // interval (ms) at which send CAN Messages
static const int interval5000 = 5000;          // interval (ms) at which send CAN Messages
static const int interval10000 = 10000;        // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;             // counter for checking if CAN is still alive
static uint16_t CANerror = 0;                  // counter on how many CAN errors encountered
#define MAX_CAN_FAILURES 5000                  // Amount of malformed CAN messages to allow before raising a warning

static const uint16_t WUPonDuration = 477;   // in milliseconds how long WUP should be ON after poweron
static const uint16_t WUPoffDuration = 105;  // in milliseconds how long WUP should be OFF after on pulse
unsigned long lastChangeTime;                // Variables to store timestamps
unsigned long turnOnTime;                    // Variables to store timestamps
enum State { POWERON, STATE_ON, STATE_OFF, STATE_STAY_ON };
static State WUPState = POWERON;

const unsigned char crc8_table[256] =
    {  // CRC8_SAE_J1850_ZER0 formula,0x1D Poly,initial value 0x3F,Final XOR value varies
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0,
        0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
        0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C, 0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23,
        0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
        0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B,
        0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65, 0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
        0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8,
        0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC,
        0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
        0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7, 0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C,
        0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
        0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47,
        0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09, 0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
        0xE3, 0xFE, 0xD9, 0xC4};

/* Not needed messages
0AA 105 13D 0BB 0AD 0A5 150 100 1A1 10E 153 197 429 1AA 12F 59A 2E3 2BE 211 2b3 3FD 2E8 2B7 108 29D 29C 29B 2C0 330
3E9 32F 19E 326 55E 515 509 50A 51A 2F5 3A4 432 3C9 
*/

CAN_frame_t BMW_10B = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x10B,
                       .data = {0xCD, 0x01, 0xFC}};  // Contactor closing command
CAN_frame_t BMW_12F = {
    .FIR = {.B =
                {
                    .DLC = 8,
                    .FF = CAN_frame_std,
                }},
    .MsgID = 0x12F,
    .data = {0xE6, 0x24, 0x86, 0x1A, 0xF1, 0x31, 0x30, 0x00}};  //0x12F Wakeup VCU, not needed for contacor closing
CAN_frame_t BMW_13E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x13E,
                       .data = {0xFF, 0x35, 0xFA, 0xFA, 0xFA, 0xFA, 0x07, 0x00}};
CAN_frame_t BMW_192 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x192,
                       .data = {0xFF, 0xFF, 0xA3, 0x8F, 0x93, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_1D0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1D0,
                       .data = {0x4D, 0xF0, 0xAE, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2CA = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2CA,
                       .data = {0x5E, 0x5E}};
CAN_frame_t BMW_2E2 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2E2,
                       .data = {0xD0, 0xD7, 0x7F, 0xB0, 0x17, 0x51, 0x05, 0x00}};
CAN_frame_t BMW_2EC = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2EC,
                       .data = {0xFC, 0xFF}};
CAN_frame_t BMW_328 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x328,
                       .data = {0xB0, 0xE4, 0x87, 0x0E, 0x30, 0x22}};
CAN_frame_t BMW_380 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x380,
                       .data = {0x56, 0x5A, 0x37, 0x39, 0x34, 0x34, 0x34}};
CAN_frame_t BMW_3A7 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3A7,
                       .data = {0x4D, 0xF0, 0x0A, 0x00, 0x4F, 0x11, 0xF0}};
CAN_frame_t BMW_3CA = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3CA,
                       .data = {0x78, 0x60, 0x30, 0x09, 0x09, 0x83, 0xFF, 0xFF}};
CAN_frame_t BMW_3D0 = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3D0,
                       .data = {0xFD, 0xFF}};
CAN_frame_t BMW_3E5 = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E5,
                       .data = {0xFD, 0xFF, 0xFF}};
CAN_frame_t BMW_3E8 = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E8,
                       .data = {0xF1, 0xFF}};  //1000ms OBD reset
CAN_frame_t BMW_3EC = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3EC,
                       .data = {0xF5, 0x10, 0x00, 0x00, 0x80, 0x25, 0x0F, 0xFC}};
CAN_frame_t BMW_3F9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3F9,  //TODO data wrong
                       .data = {0x4B, 0x00, 0x80, 0xE0, 0x36, 0x31, 0xC3, 0xFF}};
CAN_frame_t BMW_3FB = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3FB,
                       .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00}};
CAN_frame_t BMW_3FC = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3FC,
                       .data = {0xC0, 0xF9, 0x0F}};
CAN_frame_t BMW_418 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x418,
                       .data = {0xFF, 0x3C, 0xFF, 0x00, 0xC0, 0x0F, 0xFF, 0xFF}};
CAN_frame_t BMW_41D = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x41D,
                       .data = {0xFF, 0xF7, 0x7F, 0xFF}};
CAN_frame_t BMW_433 = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x433,
                       .data = {0xFF, 0x00, 0x0F, 0xFF}};  //HV specification - Required - OK mapping
CAN_frame_t BMW_512 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x512,                                             // Required to keep BMS active
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};  // 0x512 Network management

//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_10B_0[15] = {0xCD, 0x19, 0x94, 0x6D, 0xE0, 0x34, 0x78, 0xDB,
                                      0x97, 0x43, 0x0F, 0xF6, 0xBA, 0x6E, 0x81};
static const uint8_t BMW_10B_1[15] = {0x01, 0x02, 0x33, 0x34, 0x05, 0x06, 0x07, 0x08,
                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00};
static const uint8_t BMW_12F_0[15] = {0xC2, 0x9F, 0x78, 0x25, 0xAB, 0xF6, 0x11, 0x4C,
                                      0x10, 0x4D, 0xAA, 0xF7, 0x79, 0x24, 0xC3};
static const uint8_t BMW_1D0_0[15] = {0x4D, 0x10, 0xF7, 0xAA, 0x24, 0x79, 0x9E, 0xC3,
                                      0x9F, 0xC2, 0x25, 0x78, 0xF6, 0xAB, 0x4C};
static const uint8_t BMW_3F9_0[15] = {0x76, 0x2B, 0xCC, 0x91, 0x1F, 0x42, 0xA5, 0xF8,
                                      0xA4, 0xF9, 0x1E, 0x43, 0xCD, 0x90, 0x77};
static const uint8_t BMW_3EC_0[15] = {0xF5, 0xA8, 0x4F, 0x12, 0x9C, 0xC1, 0x26, 0x7B,
                                      0x27, 0x7A, 0x9D, 0xC0, 0x4E, 0x13, 0xF4};

static uint8_t alive_counter_10ms = 0;
static uint8_t alive_counter_20ms = 0;
static uint8_t alive_counter_30ms = 0;
static uint8_t alive_counter_100ms = 0;
static uint8_t alive_counter_200ms = 0;
static uint8_t alive_counter_1000ms = 0;
static uint8_t alive_counter_5000ms = 0;
static uint8_t BMW_1D0_counter = 0;
static uint8_t BMW_13E_counter = 0;
static uint8_t BMW_380_counter = 0;
static uint32_t BMW_328_counter = 0;

static uint32_t battery_serial_number = 0;
static uint32_t battery_available_power_shortterm_charge = 0;
static uint32_t battery_available_power_shortterm_discharge = 0;
static uint32_t battery_available_power_longterm_charge = 0;
static uint32_t battery_available_power_longterm_discharge = 0;
static uint32_t battery_BEV_available_power_shortterm_charge = 0;
static uint32_t battery_BEV_available_power_shortterm_discharge = 0;
static uint32_t battery_BEV_available_power_longterm_charge = 0;
static uint32_t battery_BEV_available_power_longterm_discharge = 0;
static uint16_t battery_energy_content_maximum_kWh = 0;
static uint16_t battery_display_SOC = 0;
static uint16_t battery_volts = 0;
static uint16_t battery_HVBatt_SOC = 0;
static uint16_t battery_DC_link_voltage = 0;
static uint16_t battery_max_charge_voltage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static uint16_t battery_predicted_energy_charge_condition = 0;
static uint16_t battery_predicted_energy_charging_target = 0;
static uint16_t battery_actual_value_power_heating = 0;  //0 - 4094 W
static uint16_t battery_prediction_voltage_shortterm_charge = 0;
static uint16_t battery_prediction_voltage_shortterm_discharge = 0;
static uint16_t battery_prediction_voltage_longterm_charge = 0;
static uint16_t battery_prediction_voltage_longterm_discharge = 0;
static uint16_t battery_prediction_duration_charging_minutes = 0;
static uint16_t battery_target_voltage_in_CV_mode = 0;
static int16_t battery_temperature_HV = 0;
static int16_t battery_temperature_heat_exchanger = 0;
static int16_t battery_temperature_max = 0;
static int16_t battery_temperature_min = 0;
static int16_t battery_max_charge_amperage = 0;
static int16_t battery_max_discharge_amperage = 0;
static int16_t battery_power = 0;
static int16_t battery_current = 0;
static uint8_t battery_status_error_isolation_external_Bordnetz = 0;
static uint8_t battery_status_error_isolation_internal_Bordnetz = 0;
static uint8_t battery_request_cooling = 0;
static uint8_t battery_status_valve_cooling = 0;
static uint8_t battery_status_error_locking = 0;
static uint8_t battery_status_precharge_locked = 0;
static uint8_t battery_status_disconnecting_switch = 0;
static uint8_t battery_status_emergency_mode = 0;
static uint8_t battery_request_service = 0;
static uint8_t battery_error_emergency_mode = 0;
static uint8_t battery_status_error_disconnecting_switch = 0;
static uint8_t battery_status_warning_isolation = 0;
static uint8_t battery_status_cold_shutoff_valve = 0;
static uint8_t battery_request_open_contactors = 0;
static uint8_t battery_request_open_contactors_instantly = 0;
static uint8_t battery_request_open_contactors_fast = 0;
static uint8_t battery_charging_condition_delta = 0;
static uint8_t battery_status_service_disconnection_plug = 0;
static uint8_t battery_status_measurement_isolation = 0;
static uint8_t battery_request_abort_charging = 0;
static uint8_t battery_prediction_time_end_of_charging_minutes = 0;
static uint8_t battery_request_operating_mode = 0;
static uint8_t battery_request_charging_condition_minimum = 0;
static uint8_t battery_request_charging_condition_maximum = 0;
static uint8_t battery_status_cooling_HV = 0;      //1 works, 2 does not start
static uint8_t battery_status_diagnostics_HV = 0;  // 0 all OK, 1 HV protection function error, 2 diag not yet expired

uint8_t calculateCRC(CAN_frame_t rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {
    crc = crc8_table[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //Calculate the SOC% value to send to inverter
  system_real_SOC_pptt = (battery_display_SOC * 100);  //increase Display_SOC range from 0-100 -> 100.00

  system_battery_voltage_dV = battery_volts;  //Unit V+1 (5000 = 500.0V)

  system_battery_current_dA = battery_current;

  system_capacity_Wh = BATTERY_WH_MAX;

  system_remaining_capacity_Wh = (battery_energy_content_maximum_kWh * 1000);  // Convert kWh to Wh

  system_max_charge_power_W = (battery_max_charge_amperage * system_battery_voltage_dV);  // TODO: check scaling

  system_max_discharge_power_W = (battery_max_discharge_amperage * system_battery_voltage_dV);  // TODO: check scaling

  battery_power = (system_battery_current_dA * (system_battery_voltage_dV / 10));

  system_active_power_W = battery_power;

  system_temperature_min_dC = battery_temperature_min * 10;  // Add a decimal

  system_temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }
  // Check if we have encountered any malformed CAN messages
  if (CANerror > MAX_CAN_FAILURES) {
    set_event(EVENT_CAN_RX_WARNING, 0);
  }

#ifdef DEBUG_VIA_USB
  Serial.print("Battery values: ");
  Serial.print("SOC% raw: ");
  Serial.print(battery_display_SOC);
  Serial.print(" Voltage: ");
  Serial.print(battery_volts);
  Serial.print(" Current: ");
  Serial.print(battery_current);
  Serial.print(" kWh remaining: ");
  Serial.print(battery_energy_content_maximum_kWh);
  Serial.print(" Temperature: ");
  Serial.print(battery_temperature_HV);
  Serial.print(" Max charge voltage: ");
  Serial.print(battery_max_charge_voltage / 10);
  Serial.print(" Min discharge voltage: ");
  Serial.print(battery_min_discharge_voltage / 10);
  Serial.print(" Max charge current: ");
  Serial.print(battery_max_charge_amperage);
  Serial.print(" Max discharge current: ");
  Serial.print(battery_max_discharge_amperage);

  Serial.println(" ");
  Serial.print("Values sent to inverter: ");
  Serial.print("SOC%: ");
  Serial.print(system_scaled_SOC_pptt);
  Serial.print(" Battery voltage: ");
  Serial.print(system_battery_voltage_dV);
  Serial.print(" Remaining Wh: ");
  Serial.print(system_remaining_capacity_Wh);
  Serial.print(" Max charge power: ");
  Serial.print(system_max_charge_power_W);
  Serial.print(" Max discharge power: ");
  Serial.print(system_max_discharge_power_W);
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0x112:            //BMS status [10ms]
      CANstillAlive = 12;  //This message is only sent if 30C signal is active
      battery_current = ((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) / 10) - 819;  //Amps
      battery_volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);                 //500.0 V
      battery_HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8 | rx_frame.data.u8[4]);
      battery_request_open_contactors = (rx_frame.data.u8[5] & 0xC0) >> 6;
      battery_request_open_contactors_instantly = (rx_frame.data.u8[6] & 0x03);
      battery_request_open_contactors_fast = (rx_frame.data.u8[6] & 0x0C) >> 2;
      battery_charging_condition_delta = (rx_frame.data.u8[6] & 0xF0) >> 4;
      battery_DC_link_voltage = rx_frame.data.u8[7];
      break;
    case 0x239:  //BMS [200ms]
      /*  if (is_message_corrupt(rx_frame)) {
        CANerror++;
        break;  //Message content malformed, abort reading data from it
      }*/
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);          //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x431:  //Battery capacity [200ms]
      battery_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery_energy_content_maximum_kWh = (((rx_frame.data.u8[6] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
      break;
    case 0x432:  //SOC% charged [200ms]
      battery_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery_display_SOC = (rx_frame.data.u8[4] / 2);
      break;
    case 0x2BD:  //BMS [100ms] Status diagnosis high voltage 1
      battery_status_diagnostics_HV = (rx_frame.data.u8[2] & 0x0F);
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery 2
      battery_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery 1
      battery_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery_request_service = (rx_frame.data.u8[2] & 0x03);
      battery_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      battery_temperature_HV = (rx_frame.data.u8[4] - 50);
      battery_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      battery_temperature_max = (rx_frame.data.u8[6] - 50);
      battery_temperature_min = (rx_frame.data.u8[7] - 50);
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x2FF:  //BMS [100ms] Status Heating High-Voltage Battery
      battery_actual_value_power_heating = (rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4);
      break;
    case 0x3C2:  //BMS (94AH exclusive) - Content unknown
      break;
    case 0x3EB:  //BMS [1s] Status of charging high-voltage storage 3
      battery_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x363:  //BMS [1s] Identification High-Voltage Battery
      battery_serial_number =
          (rx_frame.data.u8[3] << 24 | rx_frame.data.u8[2] << 16 | rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      break;
    case 0x507:  //BMS [640ms] Network Management 2 - This message is sent on the bus for sleep coordination purposes
      break;     // If sent, falling asleep will occur of the bus is delayed by the next 2 seconds
    case 0x587:  //BMS [1s] Services - No use for this message
      break;
    case 0x41C:  //BMS [1s] Status Of Operating Mode Of Hybrid 2
      battery_status_cooling_HV = (rx_frame.data.u8[1] & 0x03);
      break;
    case 0x607:  //BMS - No use for this message
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();

  //Handle WUP signal
  if (WUPState == POWERON) {
    digitalWrite(WUP_PIN, HIGH);
    turnOnTime = currentMillis;
    WUPState = STATE_ON;
  }

  // Check if it's time to change state
  if (WUPState == STATE_ON && currentMillis - turnOnTime >= WUPonDuration) {
    WUPState = STATE_OFF;
    digitalWrite(WUP_PIN, LOW);  // Turn off
    lastChangeTime = currentMillis;
  } else if (WUPState == STATE_OFF && currentMillis - lastChangeTime >= WUPoffDuration) {
    WUPState = STATE_STAY_ON;
    digitalWrite(WUP_PIN, HIGH);  // Turn on and stay on
    lastChangeTime = currentMillis;
  } else if (WUPState == STATE_STAY_ON) {
    // Do nothing, stay in this state forever
  }

  //Send 10ms message
  if (currentMillis - previousMillis10 >= interval10) {
    previousMillis10 = currentMillis;

    alive_counter_10ms++;
    if (alive_counter_10ms > 14) {
      alive_counter_10ms = 0;
    }
  }
  //Send 20ms message
  if (currentMillis - previousMillis20 >= interval20) {
    previousMillis20 = currentMillis;

    //BMW_10B.data.u8[1] = 0x00; // No request active
    BMW_10B.data.u8[1] = 0x10;  // Close contactors
    //BMW_10B.data.u8[1] = 0x30; // Signal invalid

    BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
    BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

    alive_counter_20ms++;
    if (alive_counter_20ms > 14) {
      alive_counter_20ms = 0;
    }

    BMW_13E_counter++;
    BMW_13E.data.u8[4] = BMW_13E_counter;

    ESP32Can.CANWriteFrame(&BMW_10B);
  }
  //Send 30ms message
  if (currentMillis - previousMillis30 >= interval30) {
    previousMillis30 = currentMillis;

    alive_counter_30ms++;
    if (alive_counter_30ms > 14) {
      alive_counter_30ms = 0;
    }
  }
  //Send 50ms message
  if (currentMillis - previousMillis50 >= interval50) {
    previousMillis50 = currentMillis;
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    BMW_12F.data.u8[0] = BMW_12F_0[alive_counter_100ms];
    BMW_12F.data.u8[1] = ((BMW_12F.data.u8[1] & 0xF0) + alive_counter_100ms);

    alive_counter_100ms++;
    if (alive_counter_100ms > 14) {
      alive_counter_100ms = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_12F);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= interval200) {
    previousMillis200 = currentMillis;

    alive_counter_200ms++;
    if (alive_counter_200ms > 14) {
      alive_counter_200ms = 0;
    }
  }
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= interval500) {
    previousMillis500 = currentMillis;
  }
  // Send 640ms CAN Message
  if (currentMillis - previousMillis640 >= interval640) {
    previousMillis640 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_512);  // Keep BMS alive
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;

    BMW_328_counter++;
    BMW_328.data.u8[0] = BMW_328_counter;  //rtc msg. needs to be every 1 sec. first 32 bits are 1 second wrap counter
    BMW_328.data.u8[1] = BMW_328_counter << 8;
    BMW_328.data.u8[2] = BMW_328_counter << 16;
    BMW_328.data.u8[3] = BMW_328_counter << 24;

    BMW_1D0.data.u8[0] = BMW_1D0_0[alive_counter_1000ms];
    BMW_1D0.data.u8[1] = ((BMW_1D0.data.u8[1] & 0xF0) + alive_counter_1000ms);

    BMW_3F9.data.u8[0] = BMW_3F9_0[alive_counter_1000ms];
    BMW_3F9.data.u8[1] = ((BMW_3F9.data.u8[1] & 0xF0) + alive_counter_1000ms);

    BMW_3EC.data.u8[0] = BMW_3EC_0[alive_counter_1000ms];
    BMW_3EC.data.u8[1] = ((BMW_3EC.data.u8[1] & 0xF0) + alive_counter_1000ms);

    if (BMW_328_counter > 1) {
      BMW_433.data.u8[1] = 0x01;
    }

    alive_counter_1000ms++;
    if (alive_counter_1000ms > 14) {
      alive_counter_1000ms = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_3E8);
    ESP32Can.CANWriteFrame(&BMW_328);
    ESP32Can.CANWriteFrame(&BMW_3F9);
    ESP32Can.CANWriteFrame(&BMW_2E2);
    ESP32Can.CANWriteFrame(&BMW_41D);
    ESP32Can.CANWriteFrame(&BMW_3D0);
    ESP32Can.CANWriteFrame(&BMW_3CA);
    ESP32Can.CANWriteFrame(&BMW_3A7);
    ESP32Can.CANWriteFrame(&BMW_2CA);
    ESP32Can.CANWriteFrame(&BMW_3FB);
    ESP32Can.CANWriteFrame(&BMW_418);
    ESP32Can.CANWriteFrame(&BMW_1D0);
    ESP32Can.CANWriteFrame(&BMW_3EC);
    ESP32Can.CANWriteFrame(&BMW_192);
    ESP32Can.CANWriteFrame(&BMW_13E);
    ESP32Can.CANWriteFrame(&BMW_433);
  }
  // Send 2000ms CAN Message
  if (currentMillis - previousMillis2000 >= interval2000) {
    previousMillis2000 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_2EC);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= interval5000) {
    previousMillis5000 = currentMillis;

    alive_counter_5000ms++;
    if (alive_counter_5000ms > 14) {
      alive_counter_5000ms = 0;
    }

    BMW_3FC.data.u8[1] = ((BMW_3FC.data.u8[1] & 0xF0) + alive_counter_5000ms);

    ESP32Can.CANWriteFrame(&BMW_3FC);

    if (BMW_380_counter < 3) {
      ESP32Can.CANWriteFrame(&BMW_380);  // This message stops after 3 times on startup
      BMW_380_counter++;
    }
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= interval10000) {
    previousMillis10000 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_3E5);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  Serial.println("BMW i3 battery selected");

  system_max_design_voltage_dV = 4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  system_min_design_voltage_dV = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
