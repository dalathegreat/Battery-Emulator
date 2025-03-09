#include "../include.h"
#ifdef BMW_I3_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-I3-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

#define ALIVE_MAX_VALUE 14  // BMW CAN messages contain alive counter, goes from 0...14

enum BatterySize { BATTERY_60AH, BATTERY_94AH, BATTERY_120AH };
static BatterySize detectedBattery = BATTERY_60AH;
static BatterySize detectedBattery2 = BATTERY_60AH;  // For double battery setups

enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

static CmdState cmdState = SOC;

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

/* CAN messages from PT-CAN2 not needed to operate the battery
0AA 105 13D 0BB 0AD 0A5 150 100 1A1 10E 153 197 429 1AA 12F 59A 2E3 2BE 211 2b3 3FD 2E8 2B7 108 29D 29C 29B 2C0 330
3E9 32F 19E 326 55E 515 509 50A 51A 2F5 3A4 432 3C9 
*/

CAN_frame BMW_10B = {.FD = false,
                     .ext_ID = false,
                     .DLC = 3,
                     .ID = 0x10B,
                     .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command
CAN_frame BMW_12F = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x12F,
                     .data = {0xE6, 0x24, 0x86, 0x1A, 0xF1, 0x31, 0x30, 0x00}};  //0x12F Wakeup VCU
CAN_frame BMW_13E = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x13E,
                     .data = {0xFF, 0x31, 0xFA, 0xFA, 0xFA, 0xFA, 0x0C, 0x00}};
CAN_frame BMW_192 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x192,
                     .data = {0xFF, 0xFF, 0xA3, 0x8F, 0x93, 0xFF, 0xFF, 0xFF}};
CAN_frame BMW_19B = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x19B,
                     .data = {0x20, 0x40, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF}};
CAN_frame BMW_1D0 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x1D0,
                     .data = {0x4D, 0xF0, 0xAE, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame BMW_2CA = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x2CA, .data = {0x57, 0x57}};
CAN_frame BMW_2E2 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x2E2,
                     .data = {0x4F, 0xDB, 0x7F, 0xB9, 0x07, 0x51, 0xff, 0x00}};
CAN_frame BMW_30B = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x30B,
                     .data = {0xe1, 0xf0, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xff}};
CAN_frame BMW_328 = {.FD = false, .ext_ID = false, .DLC = 6, .ID = 0x328, .data = {0xB0, 0xE4, 0x87, 0x0E, 0x30, 0x22}};
CAN_frame BMW_37B = {.FD = false, .ext_ID = false, .DLC = 6, .ID = 0x37B, .data = {0x40, 0x00, 0x00, 0xFF, 0xFF, 0x00}};
CAN_frame BMW_380 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 7,
                     .ID = 0x380,
                     .data = {0x56, 0x5A, 0x37, 0x39, 0x34, 0x34, 0x34}};
CAN_frame BMW_3A0 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3A0,
                     .data = {0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC}};
CAN_frame BMW_3A7 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 7,
                     .ID = 0x3A7,
                     .data = {0x05, 0xF5, 0x0A, 0x00, 0x4F, 0x11, 0xF0}};
CAN_frame BMW_3C5 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3C5,
                     .data = {0x30, 0x05, 0x47, 0x70, 0x2c, 0xce, 0xc3, 0x34}};
CAN_frame BMW_3CA = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3CA,
                     .data = {0x87, 0x80, 0x30, 0x0C, 0x0C, 0x81, 0xFF, 0xFF}};
CAN_frame BMW_3D0 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x3D0, .data = {0xFD, 0xFF}};
CAN_frame BMW_3E4 = {.FD = false, .ext_ID = false, .DLC = 6, .ID = 0x3E4, .data = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame BMW_3E5 = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x3E5, .data = {0xFC, 0xFF, 0xFF}};
CAN_frame BMW_3E8 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x3E8, .data = {0xF0, 0xFF}};  //1000ms OBD reset
CAN_frame BMW_3EC = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3EC,
                     .data = {0xF5, 0x10, 0x00, 0x00, 0x80, 0x25, 0x0F, 0xFC}};
CAN_frame BMW_3F9 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3F9,
                     .data = {0xA7, 0x2A, 0x00, 0xE2, 0xA6, 0x30, 0xC3, 0xFF}};
CAN_frame BMW_3FB = {.FD = false, .ext_ID = false, .DLC = 6, .ID = 0x3FB, .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00}};
CAN_frame BMW_3FC = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x3FC, .data = {0xC0, 0xF9, 0x0F}};
CAN_frame BMW_418 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x418,
                     .data = {0xFF, 0x7C, 0xFF, 0x00, 0xC0, 0x3F, 0xFF, 0xFF}};
CAN_frame BMW_41D = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x41D, .data = {0xFF, 0xF7, 0x7F, 0xFF}};
CAN_frame BMW_433 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 4,
                     .ID = 0x433,
                     .data = {0xFF, 0x00, 0x0F, 0xFF}};  // HV specification
CAN_frame BMW_512 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x512,
                     .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};  // 0x512 Network management
CAN_frame BMW_592_0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x592,
                       .data = {0x86, 0x10, 0x07, 0x21, 0x6e, 0x35, 0x5e, 0x86}};
CAN_frame BMW_592_1 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x592,
                       .data = {0x86, 0x21, 0xb4, 0xdd, 0x00, 0x00, 0x00, 0x00}};
CAN_frame BMW_5F8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x5F8,
                     .data = {0x64, 0x01, 0x00, 0x0B, 0x92, 0x03, 0x00, 0x05}};
CAN_frame BMW_6F1_CELL = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0xDD, 0xBF}};
CAN_frame BMW_6F1_SOH = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0x63, 0x35}};
CAN_frame BMW_6F1_SOC = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0xDD, 0xBC}};
CAN_frame BMW_6F1_CELL_VOLTAGE_AVG = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F1,
                                      .data = {0x07, 0x03, 0x22, 0xDF, 0xA0}};
CAN_frame BMW_6F1_CONTINUE = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x6F1, .data = {0x07, 0x30, 0x00, 0x02}};
CAN_frame BMW_6F4_CELL_VOLTAGE_CELLNO = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 7,
                                         .ID = 0x6F4,
                                         .data = {0x07, 0x05, 0x31, 0x01, 0xAD, 0x6E, 0x01}};
CAN_frame BMW_6F4_CELL_CONTINUE = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 6,
                                   .ID = 0x6F4,
                                   .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6E}};

//The above CAN messages need to be sent towards the battery to keep it alive

static uint8_t startup_counter_contactor = 0;
static uint8_t alive_counter_20ms = 0;
static uint8_t alive_counter_100ms = 0;
static uint8_t alive_counter_200ms = 0;
static uint8_t alive_counter_500ms = 0;
static uint8_t alive_counter_1000ms = 0;
static uint8_t alive_counter_5000ms = 0;
static uint8_t BMW_1D0_counter = 0;
static uint8_t BMW_13E_counter = 0;
static uint8_t BMW_380_counter = 0;
static uint32_t BMW_328_counter = 0;

static bool battery_awake = false;
static bool battery2_awake = false;
static bool battery_info_available = false;
static bool battery2_info_available = false;
static bool skipCRCCheck = false;
static bool CRCCheckPassedPreviously = false;
static bool skipCRCCheck_battery2 = false;
static bool CRCCheckPassedPreviously_battery2 = false;

static uint16_t cellvoltage_temp_mV = 0;
static uint32_t battery_serial_number = 0;
static uint32_t battery_available_power_shortterm_charge = 0;
static uint32_t battery_available_power_shortterm_discharge = 0;
static uint32_t battery_available_power_longterm_charge = 0;
static uint32_t battery_available_power_longterm_discharge = 0;
static uint32_t battery_BEV_available_power_shortterm_charge = 0;
static uint32_t battery_BEV_available_power_shortterm_discharge = 0;
static uint32_t battery_BEV_available_power_longterm_charge = 0;
static uint32_t battery_BEV_available_power_longterm_discharge = 0;
static uint16_t battery_energy_content_maximum_Wh = 0;
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
static uint16_t battery_soc = 0;
static uint16_t battery_soc_hvmax = 0;
static uint16_t battery_soc_hvmin = 0;
static uint16_t battery_capacity_cah = 0;
static int16_t battery_temperature_HV = 0;
static int16_t battery_temperature_heat_exchanger = 0;
static int16_t battery_temperature_max = 0;
static int16_t battery_temperature_min = 0;
static int16_t battery_max_charge_amperage = 0;
static int16_t battery_max_discharge_amperage = 0;
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
static uint8_t battery_status_diagnosis_powertrain_maximum_multiplexer = 0;
static uint8_t battery_status_diagnosis_powertrain_immediate_multiplexer = 0;
static uint8_t battery_ID2 = 0;
static uint8_t battery_soh = 99;

static uint16_t cellvoltage2_temp_mV = 0;
static uint32_t battery2_serial_number = 0;
static uint32_t battery2_available_power_shortterm_charge = 0;
static uint32_t battery2_available_power_shortterm_discharge = 0;
static uint32_t battery2_available_power_longterm_charge = 0;
static uint32_t battery2_available_power_longterm_discharge = 0;
static uint32_t battery2_BEV_available_power_shortterm_charge = 0;
static uint32_t battery2_BEV_available_power_shortterm_discharge = 0;
static uint32_t battery2_BEV_available_power_longterm_charge = 0;
static uint32_t battery2_BEV_available_power_longterm_discharge = 0;
static uint16_t battery2_energy_content_maximum_Wh = 0;
static uint16_t battery2_display_SOC = 0;
static uint16_t battery2_volts = 0;
static uint16_t battery2_HVBatt_SOC = 0;
static uint16_t battery2_DC_link_voltage = 0;
static uint16_t battery2_max_charge_voltage = 0;
static uint16_t battery2_min_discharge_voltage = 0;
static uint16_t battery2_predicted_energy_charge_condition = 0;
static uint16_t battery2_predicted_energy_charging_target = 0;
static uint16_t battery2_actual_value_power_heating = 0;  //0 - 4094 W
static uint16_t battery2_prediction_voltage_shortterm_charge = 0;
static uint16_t battery2_prediction_voltage_shortterm_discharge = 0;
static uint16_t battery2_prediction_voltage_longterm_charge = 0;
static uint16_t battery2_prediction_voltage_longterm_discharge = 0;
static uint16_t battery2_prediction_duration_charging_minutes = 0;
static uint16_t battery2_target_voltage_in_CV_mode = 0;
static uint16_t battery2_soc = 0;
static uint16_t battery2_soc_hvmax = 0;
static uint16_t battery2_soc_hvmin = 0;
static uint16_t battery2_capacity_cah = 0;
static int16_t battery2_temperature_HV = 0;
static int16_t battery2_temperature_heat_exchanger = 0;
static int16_t battery2_temperature_max = 0;
static int16_t battery2_temperature_min = 0;
static int16_t battery2_max_charge_amperage = 0;
static int16_t battery2_max_discharge_amperage = 0;
static int16_t battery2_current = 0;
static uint8_t battery2_status_error_isolation_external_Bordnetz = 0;
static uint8_t battery2_status_error_isolation_internal_Bordnetz = 0;
static uint8_t battery2_request_cooling = 0;
static uint8_t battery2_status_valve_cooling = 0;
static uint8_t battery2_status_error_locking = 0;
static uint8_t battery2_status_precharge_locked = 0;
static uint8_t battery2_status_disconnecting_switch = 0;
static uint8_t battery2_status_emergency_mode = 0;
static uint8_t battery2_request_service = 0;
static uint8_t battery2_error_emergency_mode = 0;
static uint8_t battery2_status_error_disconnecting_switch = 0;
static uint8_t battery2_status_warning_isolation = 0;
static uint8_t battery2_status_cold_shutoff_valve = 0;
static uint8_t battery2_request_open_contactors = 0;
static uint8_t battery2_request_open_contactors_instantly = 0;
static uint8_t battery2_request_open_contactors_fast = 0;
static uint8_t battery2_charging_condition_delta = 0;
static uint8_t battery2_status_service_disconnection_plug = 0;
static uint8_t battery2_status_measurement_isolation = 0;
static uint8_t battery2_request_abort_charging = 0;
static uint8_t battery2_prediction_time_end_of_charging_minutes = 0;
static uint8_t battery2_request_operating_mode = 0;
static uint8_t battery2_request_charging_condition_minimum = 0;
static uint8_t battery2_request_charging_condition_maximum = 0;
static uint8_t battery2_status_cooling_HV = 0;      //1 works, 2 does not start
static uint8_t battery2_status_diagnostics_HV = 0;  // 0 all OK, 1 HV protection function error, 2 diag not yet expired
static uint8_t battery2_status_diagnosis_powertrain_maximum_multiplexer = 0;
static uint8_t battery2_status_diagnosis_powertrain_immediate_multiplexer = 0;
static uint8_t battery2_ID2 = 0;
static uint8_t battery2_soh = 99;

static uint8_t message_data[50];
static uint8_t next_data = 0;
static uint8_t current_cell_polled = 0;

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

static uint8_t increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

void update_values_battery2() {  //This function maps all the values fetched via CAN2 to the battery2 datalayer
  if (!battery2_awake) {
    return;
  }

  datalayer.battery2.status.real_soc = (battery2_display_SOC * 50);

  datalayer.battery2.status.voltage_dV = battery2_volts;  //Unit V+1 (5000 = 500.0V)

  datalayer.battery2.status.current_dA = battery2_current;

  datalayer.battery2.info.total_capacity_Wh = battery2_energy_content_maximum_Wh;

  datalayer.battery2.status.remaining_capacity_Wh = battery2_predicted_energy_charge_condition;

  datalayer.battery2.status.soh_pptt = battery2_soh * 100;

  datalayer.battery2.status.max_discharge_power_W = battery2_BEV_available_power_longterm_discharge;

  datalayer.battery2.status.max_charge_power_W = battery2_BEV_available_power_longterm_charge;

  datalayer.battery2.status.temperature_min_dC = battery2_temperature_min * 10;  // Add a decimal

  datalayer.battery2.status.temperature_max_dC = battery2_temperature_max * 10;  // Add a decimal

  if (battery2_info_available) {
    // Start checking safeties. First up, cellvoltages!
    if (detectedBattery2 == BATTERY_60AH) {
      datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_60AH;
      datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60AH;
      datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_60AH;
      datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_60AH;
    } else if (detectedBattery2 == BATTERY_94AH) {
      datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_94AH;
      datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_94AH;
      datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_94AH;
      datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_94AH;
    } else {  // BATTERY_120AH
      datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_120AH;
      datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_120AH;
      datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_120AH;
      datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_120AH;
    }
  }

  // Perform other safety checks
  if (battery2_status_error_locking == 2) {  // HVIL seated?
    set_event(EVENT_HVIL_FAILURE, 2);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (battery2_status_error_disconnecting_switch > 0) {  // Check if contactors are sticking / welded
    set_event(EVENT_CONTACTOR_WELDED, 0);
  } else {
    clear_event(EVENT_CONTACTOR_WELDED);
  }
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the battery datalayer
  if (datalayer.system.settings.equipment_stop_active == true) {
    digitalWrite(WUP_PIN1, LOW);  // Turn off WUP_PIN1
#if defined(WUP_PIN2) && defined(DOUBLE_BATTERY)
    digitalWrite(WUP_PIN2, LOW);  // Turn off WUP_PIN2
#endif                            // defined(WUP_PIN2) &&  defined (DOUBLE_BATTERY)
  } else {
    digitalWrite(WUP_PIN1, HIGH);  // Wake up the battery
#if defined(WUP_PIN2) && defined(DOUBLE_BATTERY)
    digitalWrite(WUP_PIN2, HIGH);  // Wake up the battery2
#endif                             // defined(WUP_PIN2) &&  defined (DOUBLE_BATTERY)
  }

  if (!battery_awake) {
    return;
  }

  datalayer.battery.status.real_soc = (battery_display_SOC * 50);

  datalayer.battery.status.voltage_dV = battery_volts;  //Unit V+1 (5000 = 500.0V)

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh = battery_energy_content_maximum_Wh;

  datalayer.battery.status.remaining_capacity_Wh = battery_predicted_energy_charge_condition;

  datalayer.battery.status.soh_pptt = battery_soh * 100;

  datalayer.battery.status.max_discharge_power_W = battery_BEV_available_power_longterm_discharge;

  datalayer.battery.status.max_charge_power_W = battery_BEV_available_power_longterm_charge;

  datalayer.battery.status.temperature_min_dC = battery_temperature_min * 10;  // Add a decimal

  datalayer.battery.status.temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  if (battery_info_available) {
    // Start checking safeties. First up, cellvoltages!
    if (detectedBattery == BATTERY_60AH) {
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_60AH;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60AH;
      datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_60AH;
      datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_60AH;
    } else if (detectedBattery == BATTERY_94AH) {
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_94AH;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_94AH;
      datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_94AH;
      datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_94AH;
    } else {  // BATTERY_120AH
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_120AH;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_120AH;
      datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_120AH;
      datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_120AH;
    }
  }

  // Perform other safety checks
  if (battery_status_error_locking == 2) {  // HVIL seated?
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (battery_status_error_disconnecting_switch > 0) {  // Check if contactors are sticking / welded
    set_event(EVENT_CONTACTOR_WELDED, 0);
  } else {
    clear_event(EVENT_CONTACTOR_WELDED);
  }

  // Update webserver datalayer
  datalayer_extended.bmwi3.SOC_raw = (battery_HVBatt_SOC * 10);
  datalayer_extended.bmwi3.SOC_dash = (battery_display_SOC * 50);
  datalayer_extended.bmwi3.SOC_OBD2 = battery_soc;
  datalayer_extended.bmwi3.ST_iso_ext = battery_status_error_isolation_external_Bordnetz;
  datalayer_extended.bmwi3.ST_iso_int = battery_status_error_isolation_internal_Bordnetz;
  datalayer_extended.bmwi3.ST_valve_cooling = battery_status_valve_cooling;
  datalayer_extended.bmwi3.ST_interlock = battery_status_error_locking;
  datalayer_extended.bmwi3.ST_precharge = battery_status_precharge_locked;
  datalayer_extended.bmwi3.ST_DCSW = battery_status_disconnecting_switch;
  datalayer_extended.bmwi3.ST_EMG = battery_status_emergency_mode;
  datalayer_extended.bmwi3.ST_WELD = battery_status_error_disconnecting_switch;
  datalayer_extended.bmwi3.ST_isolation = battery_status_warning_isolation;
  datalayer_extended.bmwi3.ST_cold_shutoff_valve = battery_status_cold_shutoff_valve;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x112:  //BMS [10ms] Status Of High-Voltage Battery - 2
      battery_awake = true;
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //This message is only sent if 30C (Wakeup pin on battery) is energized with 12V
      battery_current = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) - 8192;  //deciAmps (-819.2 to 819.0A)
      battery_volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);           //500.0 V
      datalayer.battery.status.voltage_dV = battery_volts;  // Update the datalayer as soon as possible with this info
      battery_HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8 | rx_frame.data.u8[4]);
      battery_request_open_contactors = (rx_frame.data.u8[5] & 0xC0) >> 6;
      battery_request_open_contactors_instantly = (rx_frame.data.u8[6] & 0x03);
      battery_request_open_contactors_fast = (rx_frame.data.u8[6] & 0x0C) >> 2;
      battery_charging_condition_delta = (rx_frame.data.u8[6] & 0xF0) >> 4;
      battery_DC_link_voltage = rx_frame.data.u8[7];
      break;
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
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
      battery_temperature_min = (rx_frame.data.u8[6] - 50);
      battery_temperature_max = (rx_frame.data.u8[7] - 50);
      break;
    case 0x239:                                                                                      //BMS [200ms]
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x2BD:  //BMS [100ms] Status diagnosis high voltage - 1
      battery_awake = true;
      if (!skipCRCCheck) {
        if (calculateCRC(rx_frame, rx_frame.DLC, 0x15) != rx_frame.data.u8[0]) {
          // If calculated CRC does not match transmitted CRC, increase CANerror counter
          datalayer.battery.status.CAN_error_counter++;

          // If the CRC check has never passed before, set the flag to skip future checks. Some SMEs have differing CRC checks.
          if (!CRCCheckPassedPreviously) {
            skipCRCCheck = true;
          }
          break;
        } else {
          // If CRC check passes, update the flag
          CRCCheckPassedPreviously = true;
        }
      }

      // Process the data since CRC check is either passed or skipped
      battery_status_diagnostics_HV = (rx_frame.data.u8[2] & 0x0F);
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x2FF:  //BMS [100ms] Status Heating High-Voltage Battery
      battery_awake = true;
      battery_actual_value_power_heating = (rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4);
      break;
    case 0x363:  //BMS [1s] Identification High-Voltage Battery
      battery_serial_number =
          (rx_frame.data.u8[3] << 24 | rx_frame.data.u8[2] << 16 | rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      break;
    case 0x3C2:  //BMS (94AH exclusive) - Status diagnostics OBD 2 powertrain
      battery_status_diagnosis_powertrain_maximum_multiplexer =
          ((rx_frame.data.u8[1] & 0x03) << 4 | rx_frame.data.u8[0] >> 4);
      battery_status_diagnosis_powertrain_immediate_multiplexer = (rx_frame.data.u8[0] & 0xFC) >> 2;
      break;
    case 0x3EB:  //BMS [1s] Status of charging high-voltage storage - 3
      battery_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x41C:  //BMS [1s] Operating Mode Status Of Hybrid - 2
      battery_status_cooling_HV = (rx_frame.data.u8[1] & 0x03);
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery - 2
      battery_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x431:  //BMS [200ms] Data High-Voltage Battery Unit
      battery_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery_energy_content_maximum_Wh = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5]) * 20;
      if (battery_energy_content_maximum_Wh > 33000) {
        detectedBattery = BATTERY_120AH;
      } else if (battery_energy_content_maximum_Wh > 20000) {
        detectedBattery = BATTERY_94AH;
      } else {
        detectedBattery = BATTERY_60AH;
      }
      break;
    case 0x432:  //BMS [200ms] SOC% info
      battery_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery_display_SOC = rx_frame.data.u8[4];
      break;
    case 0x507:  //BMS [640ms] Network Management - 2 - This message is sent on the bus for sleep coordination purposes
      break;
    case 0x587:  //BMS [5s] Services
      battery_ID2 = rx_frame.data.u8[0];
      break;
    case 0x607:  //BMS - responses to message requests on 0x615
      if ((cmdState == CELL_VOLTAGE_CELLNO || cmdState == CELL_VOLTAGE_CELLNO_LAST) && (rx_frame.data.u8[0] == 0xF4)) {
        if (rx_frame.DLC == 6) {
          transmit_can_frame(&BMW_6F4_CELL_CONTINUE, can_config.battery);  // tell battery to send the cellvoltage
        }
        if (rx_frame.DLC == 8) {  // We have the full value, map it
          datalayer.battery.status.cell_voltages_mV[current_cell_polled - 1] =
              (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        }
      }

      if (rx_frame.DLC > 6 && next_data == 0 && rx_frame.data.u8[0] == 0xf1) {
        uint8_t count = 6;
        while (count < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count++];
        }
        transmit_can_frame(&BMW_6F1_CONTINUE, can_config.battery);  // tell battery to send additional messages

      } else if (rx_frame.DLC > 3 && next_data > 0 && rx_frame.data.u8[0] == 0xf1 &&
                 ((rx_frame.data.u8[1] & 0xF0) == 0x20)) {
        uint8_t count = 2;
        while (count < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count++];
        }

        switch (cmdState) {
          case CELL_VOLTAGE_MINMAX:
            if (next_data >= 4) {
              cellvoltage_temp_mV = (message_data[0] << 8 | message_data[1]);
              if (cellvoltage_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer.battery.status.cell_min_voltage_mV = cellvoltage_temp_mV;
              }
              cellvoltage_temp_mV = (message_data[2] << 8 | message_data[3]);
              if (cellvoltage_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer.battery.status.cell_max_voltage_mV = cellvoltage_temp_mV;
              }
            }
            break;
          case SOH:
            if (next_data >= 4) {
              battery_soh = message_data[3];
              battery_info_available = true;
            }
            break;
          case SOC:
            if (next_data >= 6) {
              battery_soc = (message_data[0] << 8 | message_data[1]);
              battery_soc_hvmax = (message_data[2] << 8 | message_data[3]);
              battery_soc_hvmin = (message_data[4] << 8 | message_data[5]);
            }
            break;
        }
      }
      break;
    default:
      break;
  }
}
void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x112:  //BMS [10ms] Status Of High-Voltage Battery - 2
      battery2_awake = true;
      datalayer.battery2.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //This message is only sent if 30C (Wakeup pin on battery) is energized with 12V
      battery2_current = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) - 8192;  //deciAmps (-819.2 to 819.0A)
      battery2_volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);           //500.0 V
      datalayer.battery2.status.voltage_dV =
          battery2_volts;  // Update the datalayer as soon as possible with this info, needed for contactor control
      battery2_HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8 | rx_frame.data.u8[4]);
      battery2_request_open_contactors = (rx_frame.data.u8[5] & 0xC0) >> 6;
      battery2_request_open_contactors_instantly = (rx_frame.data.u8[6] & 0x03);
      battery2_request_open_contactors_fast = (rx_frame.data.u8[6] & 0x0C) >> 2;
      battery2_charging_condition_delta = (rx_frame.data.u8[6] & 0xF0) >> 4;
      battery2_DC_link_voltage = rx_frame.data.u8[7];
      break;
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
      battery2_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery2_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery2_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery2_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery2_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery2_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery2_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery2_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery2_request_service = (rx_frame.data.u8[2] & 0x03);
      battery2_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery2_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery2_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery2_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      battery2_temperature_HV = (rx_frame.data.u8[4] - 50);
      battery2_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      battery2_temperature_min = (rx_frame.data.u8[6] - 50);
      battery2_temperature_max = (rx_frame.data.u8[7] - 50);
      break;
    case 0x239:                                                                                       //BMS [200ms]
      battery2_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery2_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x2BD:  //BMS [100ms] Status diagnosis high voltage - 1
      battery2_awake = true;
      if (!skipCRCCheck_battery2) {
        if (calculateCRC(rx_frame, rx_frame.DLC, 0x15) != rx_frame.data.u8[0]) {
          // If calculated CRC does not match transmitted CRC, increase CANerror counter
          datalayer.battery2.status.CAN_error_counter++;

          // If the CRC check has never passed before, set the flag to skip future checks. Some SMEs have differing CRC checks.
          if (!CRCCheckPassedPreviously_battery2) {
            skipCRCCheck_battery2 = true;
          }
          break;
        } else {
          // If CRC check passes, update the flag
          CRCCheckPassedPreviously_battery2 = true;
        }
      }

      // Process the data since CRC check is either passed or skipped
      battery2_status_diagnostics_HV = (rx_frame.data.u8[2] & 0x0F);
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery2_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery2_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery2_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery2_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x2FF:  //BMS [100ms] Status Heating High-Voltage Battery
      battery2_awake = true;
      battery2_actual_value_power_heating = (rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4);
      break;
    case 0x363:  //BMS [1s] Identification High-Voltage Battery
      battery2_serial_number =
          (rx_frame.data.u8[3] << 24 | rx_frame.data.u8[2] << 16 | rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      break;
    case 0x3C2:  //BMS (94AH exclusive) - Status diagnostics OBD 2 powertrain
      battery2_status_diagnosis_powertrain_maximum_multiplexer =
          ((rx_frame.data.u8[1] & 0x03) << 4 | rx_frame.data.u8[0] >> 4);
      battery2_status_diagnosis_powertrain_immediate_multiplexer = (rx_frame.data.u8[0] & 0xFC) >> 2;
      break;
    case 0x3EB:  //BMS [1s] Status of charging high-voltage storage - 3
      battery2_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery2_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery2_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery2_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery2_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery2_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery2_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery2_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x41C:  //BMS [1s] Operating Mode Status Of Hybrid - 2
      battery2_status_cooling_HV = (rx_frame.data.u8[1] & 0x03);
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery - 2
      battery2_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery2_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery2_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery2_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x431:  //BMS [200ms] Data High-Voltage Battery Unit
      battery2_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery2_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery2_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery2_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery2_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery2_energy_content_maximum_Wh = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5]) * 20;
      if (battery2_energy_content_maximum_Wh > 33000) {
        detectedBattery2 = BATTERY_120AH;
      } else if (battery2_energy_content_maximum_Wh > 20000) {
        detectedBattery2 = BATTERY_94AH;
      } else {
        detectedBattery2 = BATTERY_60AH;
      }
      break;
    case 0x432:  //BMS [200ms] SOC% info
      battery2_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery2_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery2_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery2_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery2_display_SOC = rx_frame.data.u8[4];
      break;
    case 0x507:  //BMS [640ms] Network Management - 2 - This message is sent on the bus for sleep coordination purposes
      break;
    case 0x587:  //BMS [5s] Services
      battery2_ID2 = rx_frame.data.u8[0];
      break;
    case 0x607:  //BMS - responses to message requests on 0x615
      if ((cmdState == CELL_VOLTAGE_CELLNO || cmdState == CELL_VOLTAGE_CELLNO_LAST) && (rx_frame.data.u8[0] == 0xF4)) {
        if (rx_frame.DLC == 6) {
          transmit_can_frame(&BMW_6F4_CELL_CONTINUE,
                             can_config.battery_double);  // tell battery to send the cellvoltage
        }
        if (rx_frame.DLC == 8) {  // We have the full value, map it
          datalayer.battery2.status.cell_voltages_mV[current_cell_polled - 1] =
              (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        }
      }

      if (rx_frame.DLC > 6 && next_data == 0 && rx_frame.data.u8[0] == 0xf1) {
        uint8_t count2 = 6;
        while (count2 < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count2++];
        }
        transmit_can_frame(&BMW_6F1_CONTINUE, can_config.battery_double);

      } else if (rx_frame.DLC > 3 && next_data > 0 && rx_frame.data.u8[0] == 0xf1 &&
                 ((rx_frame.data.u8[1] & 0xF0) == 0x20)) {
        uint8_t count2 = 2;
        while (count2 < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count2++];
        }

        switch (cmdState) {
          case CELL_VOLTAGE_MINMAX:
            if (next_data >= 4) {
              cellvoltage2_temp_mV = (message_data[0] << 8 | message_data[1]);
              if (cellvoltage2_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer.battery2.status.cell_min_voltage_mV = cellvoltage2_temp_mV;
              }
              cellvoltage2_temp_mV = (message_data[2] << 8 | message_data[3]);
              if (cellvoltage_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer.battery2.status.cell_max_voltage_mV = cellvoltage2_temp_mV;
              }
            }
            break;
          case SOH:
            if (next_data >= 4) {
              battery2_soh = message_data[3];
              battery2_info_available = true;
            }
            break;
          case SOC:
            if (next_data >= 6) {
              battery2_soc = (message_data[0] << 8 | message_data[1]);
              battery2_soc_hvmax = (message_data[2] << 8 | message_data[3]);
              battery2_soc_hvmin = (message_data[4] << 8 | message_data[5]);
            }
            break;
        }
      }
      break;
    default:
      break;
  }
}
void transmit_can_battery() {
  unsigned long currentMillis = millis();

  if (battery_awake) {
    //Send 20ms message
    if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
      // Check if sending of CAN messages has been delayed too much.
      if ((currentMillis - previousMillis20 >= INTERVAL_20_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
        set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis20));
      } else {
        clear_event(EVENT_CAN_OVERRUN);
      }
      previousMillis20 = currentMillis;

      if (startup_counter_contactor < 160) {
        startup_counter_contactor++;
      } else {                      //After 160 messages, turn on the request
        BMW_10B.data.u8[1] = 0x10;  // Close contactors
      }

      BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
      BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

      alive_counter_20ms = increment_alive_counter(alive_counter_20ms);

      BMW_13E_counter++;
      BMW_13E.data.u8[4] = BMW_13E_counter;

      if (datalayer.battery.status.bms_status == FAULT) {
      }  //If battery is not in Fault mode, allow contactor to close by sending 10B
      else {
        transmit_can_frame(&BMW_10B, can_config.battery);
      }

#ifdef DOUBLE_BATTERY  //If second battery is allowed to join in, also send 10B
      if (datalayer.system.status.battery2_allows_contactor_closing == true) {
        transmit_can_frame(&BMW_10B, can_config.battery_double);
      }
#endif
    }
    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      BMW_12F.data.u8[1] = ((BMW_12F.data.u8[1] & 0xF0) + alive_counter_100ms);
      BMW_12F.data.u8[0] = calculateCRC(BMW_12F, 8, 0x60);

      alive_counter_100ms = increment_alive_counter(alive_counter_100ms);

      transmit_can_frame(&BMW_12F, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_12F, can_config.battery_double);
#endif
    }
    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;

      BMW_19B.data.u8[1] = ((BMW_19B.data.u8[1] & 0xF0) + alive_counter_200ms);
      BMW_19B.data.u8[0] = calculateCRC(BMW_19B, 8, 0x6C);

      alive_counter_200ms = increment_alive_counter(alive_counter_200ms);

      transmit_can_frame(&BMW_19B, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_19B, can_config.battery_double);
#endif
    }
    // Send 500ms CAN Message
    if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
      previousMillis500 = currentMillis;

      BMW_30B.data.u8[1] = ((BMW_30B.data.u8[1] & 0xF0) + alive_counter_500ms);
      BMW_30B.data.u8[0] = calculateCRC(BMW_30B, 8, 0xBE);

      alive_counter_500ms = increment_alive_counter(alive_counter_500ms);

      transmit_can_frame(&BMW_30B, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_30B, can_config.battery_double);
#endif
    }
    // Send 640ms CAN Message
    if (currentMillis - previousMillis640 >= INTERVAL_640_MS) {
      previousMillis640 = currentMillis;

      transmit_can_frame(&BMW_512, can_config.battery);  // Keep BMS alive
      transmit_can_frame(&BMW_5F8, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_512, can_config.battery_double);
      transmit_can_frame(&BMW_5F8, can_config.battery_double);
#endif
    }
    // Send 1000ms CAN Message
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      BMW_328_counter++;  // Used to increment seconds
      BMW_328.data.u8[0] = BMW_328_counter;
      BMW_328.data.u8[1] = BMW_328_counter << 8;
      BMW_328.data.u8[2] = BMW_328_counter << 16;
      BMW_328.data.u8[3] = BMW_328_counter << 24;

      BMW_1D0.data.u8[1] = ((BMW_1D0.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_1D0.data.u8[0] = calculateCRC(BMW_1D0, 8, 0xF9);

      BMW_3F9.data.u8[1] = ((BMW_3F9.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3F9.data.u8[0] = calculateCRC(BMW_3F9, 8, 0x38);

      BMW_3EC.data.u8[1] = ((BMW_3EC.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3EC.data.u8[0] = calculateCRC(BMW_3EC, 8, 0x53);

      BMW_3A7.data.u8[1] = ((BMW_3A7.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3A7.data.u8[0] = calculateCRC(BMW_3A7, 8, 0x05);

      alive_counter_1000ms = increment_alive_counter(alive_counter_1000ms);

      transmit_can_frame(&BMW_3E8, can_config.battery);  //Order comes from CAN logs
      transmit_can_frame(&BMW_328, can_config.battery);
      transmit_can_frame(&BMW_3F9, can_config.battery);
      transmit_can_frame(&BMW_2E2, can_config.battery);
      transmit_can_frame(&BMW_41D, can_config.battery);
      transmit_can_frame(&BMW_3D0, can_config.battery);
      transmit_can_frame(&BMW_3CA, can_config.battery);
      transmit_can_frame(&BMW_3A7, can_config.battery);
      transmit_can_frame(&BMW_2CA, can_config.battery);
      transmit_can_frame(&BMW_3FB, can_config.battery);
      transmit_can_frame(&BMW_418, can_config.battery);
      transmit_can_frame(&BMW_1D0, can_config.battery);
      transmit_can_frame(&BMW_3EC, can_config.battery);
      transmit_can_frame(&BMW_192, can_config.battery);
      transmit_can_frame(&BMW_13E, can_config.battery);
      transmit_can_frame(&BMW_433, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_3E8, can_config.battery_double);
      transmit_can_frame(&BMW_328, can_config.battery_double);
      transmit_can_frame(&BMW_3F9, can_config.battery_double);
      transmit_can_frame(&BMW_2E2, can_config.battery_double);
      transmit_can_frame(&BMW_41D, can_config.battery_double);
      transmit_can_frame(&BMW_3D0, can_config.battery_double);
      transmit_can_frame(&BMW_3CA, can_config.battery_double);
      transmit_can_frame(&BMW_3A7, can_config.battery_double);
      transmit_can_frame(&BMW_2CA, can_config.battery_double);
      transmit_can_frame(&BMW_3FB, can_config.battery_double);
      transmit_can_frame(&BMW_418, can_config.battery_double);
      transmit_can_frame(&BMW_1D0, can_config.battery_double);
      transmit_can_frame(&BMW_3EC, can_config.battery_double);
      transmit_can_frame(&BMW_192, can_config.battery_double);
      transmit_can_frame(&BMW_13E, can_config.battery_double);
      transmit_can_frame(&BMW_433, can_config.battery_double);
#endif

      BMW_433.data.u8[1] = 0x01;  // First 433 message byte1 we send is unique, once we sent initial value send this
      BMW_3E8.data.u8[0] = 0xF1;  // First 3E8 message byte0 we send is unique, once we sent initial value send this

      next_data = 0;
      switch (cmdState) {
        case SOC:
          transmit_can_frame(&BMW_6F1_CELL, can_config.battery);
#ifdef DOUBLE_BATTERY
          transmit_can_frame(&BMW_6F1_CELL, can_config.battery_double);
#endif
          cmdState = CELL_VOLTAGE_MINMAX;
          break;
        case CELL_VOLTAGE_MINMAX:
          transmit_can_frame(&BMW_6F1_SOH, can_config.battery);
#ifdef DOUBLE_BATTERY
          transmit_can_frame(&BMW_6F1_SOH, can_config.battery_double);
#endif
          cmdState = SOH;
          break;
        case SOH:
          transmit_can_frame(&BMW_6F1_CELL_VOLTAGE_AVG, can_config.battery);
#ifdef DOUBLE_BATTERY
          transmit_can_frame(&BMW_6F1_CELL_VOLTAGE_AVG, can_config.battery_double);
#endif
          cmdState = CELL_VOLTAGE_CELLNO;
          current_cell_polled = 0;

          break;
        case CELL_VOLTAGE_CELLNO:
          current_cell_polled++;
          if (current_cell_polled > 96) {
            cmdState = CELL_VOLTAGE_CELLNO_LAST;
          } else {
            cmdState = CELL_VOLTAGE_CELLNO;

            BMW_6F4_CELL_VOLTAGE_CELLNO.data.u8[6] = current_cell_polled;
            transmit_can_frame(&BMW_6F4_CELL_VOLTAGE_CELLNO, can_config.battery);
#ifdef DOUBLE_BATTERY
            transmit_can_frame(&BMW_6F4_CELL_VOLTAGE_CELLNO, can_config.battery_double);
#endif
          }
          break;
        case CELL_VOLTAGE_CELLNO_LAST:
          transmit_can_frame(&BMW_6F1_SOC, can_config.battery);
#ifdef DOUBLE_BATTERY
          transmit_can_frame(&BMW_6F1_SOC, can_config.battery_double);
#endif
          cmdState = SOC;
          break;
      }
    }
    // Send 5000ms CAN Message
    if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
      previousMillis5000 = currentMillis;

      BMW_3FC.data.u8[1] = ((BMW_3FC.data.u8[1] & 0xF0) + alive_counter_5000ms);
      BMW_3C5.data.u8[0] = ((BMW_3C5.data.u8[0] & 0xF0) + alive_counter_5000ms);

      transmit_can_frame(&BMW_3FC, can_config.battery);  //Order comes from CAN logs
      transmit_can_frame(&BMW_3C5, can_config.battery);
      transmit_can_frame(&BMW_3A0, can_config.battery);
      transmit_can_frame(&BMW_592_0, can_config.battery);
      transmit_can_frame(&BMW_592_1, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_3FC, can_config.battery_double);
      transmit_can_frame(&BMW_3C5, can_config.battery_double);
      transmit_can_frame(&BMW_3A0, can_config.battery_double);
      transmit_can_frame(&BMW_592_0, can_config.battery_double);
      transmit_can_frame(&BMW_592_1, can_config.battery_double);
#endif

      alive_counter_5000ms = increment_alive_counter(alive_counter_5000ms);

      if (BMW_380_counter < 3) {
        transmit_can_frame(&BMW_380, can_config.battery);  // This message stops after 3 times on startup
#ifdef DOUBLE_BATTERY
        transmit_can_frame(&BMW_380, can_config.battery_double);
#endif
        BMW_380_counter++;
      }
    }
    // Send 10000ms CAN Message
    if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
      previousMillis10000 = currentMillis;

      transmit_can_frame(&BMW_3E5, can_config.battery);  //Order comes from CAN logs
      transmit_can_frame(&BMW_3E4, can_config.battery);
      transmit_can_frame(&BMW_37B, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&BMW_3E5, can_config.battery_double);
      transmit_can_frame(&BMW_3E4, can_config.battery_double);
      transmit_can_frame(&BMW_37B, can_config.battery_double);
#endif

      BMW_3E5.data.u8[0] = 0xFD;  // First 3E5 message byte0 we send is unique, once we sent initial value send this
    }
  } else {
    previousMillis20 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    previousMillis500 = currentMillis;
    previousMillis640 = currentMillis;
    previousMillis1000 = currentMillis;
    previousMillis5000 = currentMillis;
    previousMillis10000 = currentMillis;
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "BMW i3", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //Before we have started up and detected which battery is in use, use 60AH values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_60AH;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60AH;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = NUMBER_OF_CELLS;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  datalayer.battery2.info.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
  datalayer.battery2.status.voltage_dV =
      0;  //Init voltage to 0 to allow contactor check to operate without fear of default values colliding
  datalayer.battery2.info.number_of_cells = NUMBER_OF_CELLS;
#endif
  pinMode(WUP_PIN1, OUTPUT);
  digitalWrite(WUP_PIN1, HIGH);  // Wake up the battery
#if defined(DOUBLE_BATTERY) && defined(WUP_PIN2)
  pinMode(WUP_PIN2, OUTPUT);
  digitalWrite(WUP_PIN2, HIGH);  // Wake up the battery
#endif                           // defined(WUP_PIN2) &&  defined (DOUBLE_BATTERY)
}

#endif
