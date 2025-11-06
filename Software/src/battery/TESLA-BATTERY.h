#ifndef TESLA_BATTERY_H
#define TESLA_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"
#include "TESLA-HTML.h"

// 0x7FF gateway config, "Gen3" vehicles only, not applicable to Gen2 "classic" Model S and Model X
// These are user configurable from the Webserver UI
extern bool user_selected_tesla_digital_HVIL;
extern uint16_t user_selected_tesla_GTW_country;
extern bool user_selected_tesla_GTW_rightHandDrive;
extern uint16_t user_selected_tesla_GTW_mapRegion;
extern uint16_t user_selected_tesla_GTW_chassisType;
extern uint16_t user_selected_tesla_GTW_packEnergy;

class TeslaBattery : public CanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  TeslaBattery() { allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing; }

  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_clear_isolation() { return true; }
  void clear_isolation() { datalayer.battery.settings.user_requests_tesla_isolation_clear = true; }

  bool supports_reset_BMS() { return true; }
  void reset_BMS() { datalayer.battery.settings.user_requests_tesla_bms_reset = true; }

  bool supports_reset_SOC() { return true; }
  void reset_SOC() { datalayer.battery.settings.user_requests_tesla_soc_reset = true; }

  bool supports_charged_energy() { return true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  TeslaHtmlRenderer renderer;

 protected:
  /* Do not change anything below this line! */
  static const int RAMPDOWN_SOC = 900;  // 90.0 SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int RAMPDOWNPOWERALLOWED = 10000;      // What power we ramp down from towards top balancing
  static const int FLOAT_MAX_POWER_W = 200;           // W, what power to allow for top balancing battery
  static const int FLOAT_START_MV = 20;               // mV, how many mV under overvoltage to start float charging
  static const int MAX_PACK_VOLTAGE_SX_NCMA = 4600;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_SX_NCMA = 3100;   // V+1, if pack voltage goes over this, charge stops
  static const int MAX_PACK_VOLTAGE_3Y_NCMA = 4030;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_NCMA = 3100;   // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_PACK_VOLTAGE_3Y_LFP = 3880;    // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_LFP = 2968;    // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_CELL_DEVIATION_NCA_NCM = 500;  //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_DEVIATION_LFP = 400;      //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_VOLTAGE_NCA_NCM =
      4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_NCA_NCM =
      2950;                                      //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_VOLTAGE_LFP = 3650;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_LFP = 2800;  //Battery is put into emergency stop if one cell goes below this value

  bool operate_contactors = false;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  void printFaultCodesIfActive();

  unsigned long previousMillis10 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis500 = 0;   // will store last time a 500ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent

  //UDS session tracker
  //static bool uds_SessionInProgress = false; // Future use
  //0x221 VCFRONT_LVPowerState
  uint8_t alternateMux = 0;
  uint8_t frameCounter_TESLA_221 = 15;  // Start at 15 for Mux 0
  uint8_t vehicleState = 1;             // "OFF": 0, "DRIVE": 1, "ACCESSORY": 2, "GOING_DOWN": 3
  static const uint8_t CAR_OFF = 0;
  static const uint8_t CAR_DRIVE = 1;
  static const uint8_t ACCESSORY = 2;
  static const uint8_t GOING_DOWN = 3;
  uint8_t powerDownSeconds = 9;  // Car power down (i.e. contactor open) tracking timer, 3 seconds per sendingState
  //0x2E1 VCFRONT_status, 6 mux tracker
  uint8_t muxNumber_TESLA_2E1 = 0;
  //0x334 UI
  bool TESLA_334_INITIAL_SENT = false;
  //0x3A1 VCFRONT_vehicleStatus, 15 frame counter (temporary)
  uint8_t frameCounter_TESLA_3A1 = 0;
  //0x504 TWC_status
  bool TESLA_504_INITIAL_SENT = false;
  //0x7FF GTW_carConfig, 5 mux tracker
  uint8_t muxNumber_TESLA_7FF = 0;
  //Max percentage charge tracker
  uint16_t previous_max_percentage = datalayer.battery.settings.max_percentage;

  //0x082 UI_tripPlanning: "cycle_time" 1000ms
  static constexpr CAN_frame TESLA_082 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x082,
                                          .data = {0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80}};

  //0x102 VCLEFT_doorStatus: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_102 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x102,
                                          .data = {0x22, 0x33, 0x00, 0x00, 0xC0, 0x38, 0x21, 0x08}};

  //0x103 VCRIGHT_doorStatus: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_103 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x103,
                                          .data = {0x22, 0x33, 0x00, 0x00, 0x30, 0xF2, 0x20, 0x02}};

  //0x118 DI_systemStatus: "cycle_time" 50ms, DI_systemStatusChecksum/DI_systemStatusCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_118 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x118,
                         .data = {0xAB, 0x60, 0x2A, 0x00, 0x00, 0x08, 0x00, 0x00}};

  //0x2A8 CMPD_state: "cycle_time" 100ms, different depending on firmware, semi-manual increment for now
  CAN_frame TESLA_2A8 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A8,
                         .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x2C}};

  //0x213 UI_cruiseControl: "cycle_time" 500ms, UI_speedLimitTick/UI_cruiseControlCounter - different depending on firmware, semi-manual increment for now
  CAN_frame TESLA_213 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x213, .data = {0x00, 0x15}};

  //0x221 These frames will/should eventually be migrated to 2 base frames (1 per mux), and then just the relevant bits changed

  //0x221 VCFRONT_LVPowerState "Drive"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_DRIVE (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_DRIVE_Mux0 = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x221,
                                    .data = {0x60, 0x55, 0x55, 0x15, 0x54, 0x51, 0xF1, 0xD8}};

  //0x221 VCFRONT_LVPowerState "Drive"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_DRIVE (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_DRIVE_Mux1 = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x221,
                                    .data = {0x61, 0x05, 0x55, 0x05, 0x00, 0x00, 0x00, 0xE3}};

  //0x221 VCFRONT_LVPowerState "Accessory"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_ACCESSORY (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_ACCESSORY_Mux0 = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 8,
                                        .ID = 0x221,
                                        .data = {0x40, 0x55, 0x55, 0x05, 0x54, 0x51, 0xF5, 0xAC}};

  //0x221 VCFRONT_LVPowerState "Accessory"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_ACCESSORY (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_ACCESSORY_Mux1 = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 8,
                                        .ID = 0x221,
                                        .data = {0x41, 0x05, 0x55, 0x55, 0x01, 0x00, 0x04, 0x18}};

  //0x221 VCFRONT_LVPowerState "Going Down"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts GOING_DOWN (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_GOING_DOWN_Mux0 = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 8,
                                         .ID = 0x221,
                                         .data = {0x00, 0x89, 0x55, 0x06, 0xA4, 0x51, 0xF1, 0xED}};

  //0x221 VCFRONT_LVPowerState "Going Down"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts GOING_DOWN (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_GOING_DOWN_Mux1 = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 8,
                                         .ID = 0x221,
                                         .data = {0x01, 0x09, 0x55, 0x59, 0x00, 0x00, 0x00, 0xDB}};

  //0x221 VCFRONT_LVPowerState "Off"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts OFF (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_OFF_Mux0 = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x221,
                                  .data = {0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0xF1, 0x65}};

  //0x221 VCFRONT_LVPowerState "Off"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts OFF (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_OFF_Mux1 = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x221,
                                  .data = {0x01, 0x01, 0x01, 0x50, 0x00, 0x00, 0x00, 0x76}};

  //0x229 SCCM_rightStalk: "cycle_time" 100ms, SCCM_rightStalkChecksum/SCCM_rightStalkCounter generated via dedicated generateTESLA_229 function for now
  //CRC seemingly related to AUTOSAR ID array... "autosarDataIds": [124,182,240,47,105,163,221,28,86,144,202,9,67,125,183,241] found in Model 3 firmware
  CAN_frame TESLA_229 = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x229, .data = {0x46, 0x00, 0x00}};

  //0x241 VCFRONT_coolant: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_241 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 7,
                                          .ID = 0x241,
                                          .data = {0x35, 0x34, 0x0C, 0x0F, 0x8F, 0x55, 0x00}};

  //0x2D1 VCFRONT_okToUseHighPower: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_2D1 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x2D1, .data = {0xFF, 0x01}};

  //0x2E1, 6 muxes
  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_VEHICLE_AND_RAILS = {.FD = false,
                                                            .ext_ID = false,
                                                            .DLC = 8,
                                                            .ID = 0x2E1,
                                                            .data = {0x29, 0x0A, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0x00}};

  //{0x29, 0x0A, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0x00} INIT
  //{0x29, 0x0A, 0x0D, 0xFF, 0x0F, 0x00, 0x00, 0x00} DRIVE
  //{0x29, 0x0A, 0x09, 0xFF, 0x0F, 0x00, 0x00, 0x00} HV_UP_STANDBY
  //{0x29, 0x0A, 0x0A, 0xFF, 0x0F, 0x00, 0x00, 0x00} ACCESSORY
  //{0x29, 0x0A, 0x06, 0xFF, 0x0F, 0x00, 0x00, 0x00} SLEEP_STANDBY

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_HOMELINK = {.FD = false,
                                                   .ext_ID = false,
                                                   .DLC = 8,
                                                   .ID = 0x2E1,
                                                   .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_REFRIGERANT_SYSTEM = {.FD = false,
                                                             .ext_ID = false,
                                                             .DLC = 8,
                                                             .ID = 0x2E1,
                                                             .data = {0x03, 0x6D, 0x99, 0x02, 0x1B, 0x57, 0x00, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_LV_BATTERY_DEBUG = {.FD = false,
                                                           .ext_ID = false,
                                                           .DLC = 8,
                                                           .ID = 0x2E1,
                                                           .data = {0xFC, 0x1B, 0xD1, 0x99, 0x9A, 0xD8, 0x09, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_MUX_5 = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 8,
                                                .ID = 0x2E1,
                                                .data = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_BODY_CONTROLS = {.FD = false,
                                                        .ext_ID = false,
                                                        .DLC = 8,
                                                        .ID = 0x2E1,
                                                        .data = {0x08, 0x21, 0x04, 0x6E, 0xA0, 0x88, 0x06, 0x04}};

  //0x2E8 EPBR_status: "cycle_time" 100ms
  CAN_frame TESLA_2E8 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2E8,
                         .data = {0x02, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 0x6C}};

  //0x284 UI_vehicleModes: "cycle_time" 500ms
  static constexpr CAN_frame TESLA_284 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x284,
                                          .data = {0x10, 0x00, 0x00, 0x00, 0x00}};

  //0x293 UI_chassisControl: "cycle_time" 500ms, UI_chassisControlChecksum/UI_chassisControlCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_293 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x293,
                         .data = {0x01, 0x0C, 0x55, 0x91, 0x55, 0x15, 0x01, 0xF3}};

  //0x3A1 VCFRONT_vehicleStatus: "cycle_time" 50ms, VCFRONT_vehicleStatusChecksum/VCFRONT_vehicleStatusCounter eventually need to be generated via generateMuxFrameCounterChecksum
  //Looks like 2 muxes, counter at bit 52 width 4 and checksum at bit 56 width 8? Need later software Model3_ETH.compact.json signal file or DBC.
  //Migrated to an array until figured out
  static constexpr CAN_frame TESLA_3A1[16] = {
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0xD0, 0x01}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0xE2, 0xCB}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0xF0, 0x21}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0x02, 0xEB}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0x10, 0x41}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0x22, 0x0B}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0x30, 0x61}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0x42, 0x2B}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0x50, 0x81}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0x62, 0x4B}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0x70, 0xA1}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0x82, 0x6B}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0x90, 0xC1}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0xA2, 0x8B}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0xB0, 0xE1}},
      {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x3A1, .data = {0x08, 0x62, 0x0B, 0x18, 0x00, 0x28, 0xC2, 0xAB}}};

  //0x313 UI_powertrainControl: "cycle_time" 500ms, UI_powertrainControlChecksum/UI_powertrainControlCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_313 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x313,
                         .data = {0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x1B}};

  //0x321 VCFRONT_sensors: "cycle_time" 1000ms
  CAN_frame TESLA_321 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x321,
      .data = {0xEC, 0x71, 0xA7, 0x6E, 0x02, 0x6C, 0x00, 0x04}};  // Last 2 bytes are counter and checksum

  //0x333 UI_chargeRequest: "cycle_time" 500ms, UI_chargeTerminationPct value = 900 [bit 16, width 10, scale 0.1, min 25, max 100]
  CAN_frame TESLA_333 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x333, .data = {0x84, 0x30, 0x84, 0x07, 0x02}};

  //0x334 UI request: "cycle_time" 500ms, initial frame car sends
  static constexpr CAN_frame TESLA_334_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x334,
                                                  .data = {0x3F, 0x3F, 0xC8, 0x00, 0xE2, 0x3F, 0x80, 0x1E}};

  //0x334 UI request: "cycle_time" 500ms, generated via generateFrameCounterChecksum
  CAN_frame TESLA_334 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x334,
                         .data = {0x3F, 0x3F, 0x00, 0x0F, 0xE2, 0x3F, 0x90, 0x75}};

  //0x3B3 UI_vehicleControl2: "cycle_time" 500ms
  static constexpr CAN_frame TESLA_3B3 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x3B3,
                                          .data = {0x90, 0x80, 0x05, 0x08, 0x00, 0x00, 0x00, 0x01}};

  //0x39D IBST_status: "cycle_time" 50ms, IBST_statusChecksum/IBST_statusCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_39D = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x39D, .data = {0xE1, 0x59, 0xC1, 0x27, 0x00}};

  //0x3C2 VCLEFT_switchStatus (Mux0, initial frame car sends): "cycle_time" 50ms, sent once
  static constexpr CAN_frame TESLA_3C2_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x3C2,
                                                  .data = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x5A, 0x05}};

  //0x3C2 VCLEFT_switchStatus (Mux0): "cycle_time" 50ms each mux/SwitchStatusIndex
  static constexpr CAN_frame TESLA_3C2_Mux0 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x3C2,
                                               .data = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x5A, 0x45}};

  //0x3C2 VCLEFT_switchStatus (Mux1): "cycle_time" 50ms each mux/SwitchStatusIndex
  static constexpr CAN_frame TESLA_3C2_Mux1 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x3C2,
                                               .data = {0x29, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x504 Initially sent
  static constexpr CAN_frame TESLA_504_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x504,
                                                  .data = {0x00, 0x1B, 0x06, 0x03, 0x00, 0x01, 0x00, 0x01}};

  //0x55A Unknown but always sent: "cycle_time" 500ms
  static constexpr CAN_frame TESLA_55A = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x55A,
                                          .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer (UK/RHD)
  CAN_frame TESLA_7FF_Mux1 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7FF,
                              .data = {0x01, 0x49, 0x42, 0x47, 0x00, 0x03, 0x15, 0x01}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux2 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x02, 0x66, 0x32, 0x24, 0x04, 0x49, 0x95, 0x82}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer (EU/Long Range)
  CAN_frame TESLA_7FF_Mux3 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7FF,
                              .data = {0x03, 0x01, 0x08, 0x48, 0x01, 0x00, 0x00, 0x12}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux4 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x04, 0x73, 0x03, 0x67, 0x5C, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux5 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x05, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer - later firmware has muxes 6 & 7, needed?
  static constexpr CAN_frame TESLA_7FF_Mux6 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x06, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0xD0}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer - later firmware has muxes 6 & 7, needed?
  static constexpr CAN_frame TESLA_7FF_Mux7 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x07, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}};

  //0x722 BMS_bmbKeepAlive: "cycle_time" 100ms, should only be sent when testing packs or diagnosing problems
  static constexpr CAN_frame TESLA_722 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x722,
                                          .data = {0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}};

  //0x25D CP_status: "cycle_time" 100ms, stops some cpMia errors, but not necessary for standalone pack operation so not used/necessary. Note CP_type for different regions, the below has "IEC_CCS"
  static constexpr CAN_frame TESLA_25D = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x25D,
                                          .data = {0x37, 0x41, 0x01, 0x16, 0x08, 0x00, 0x00, 0x00}};

  //0x602 BMS UDS diagnostic request: on demand
  CAN_frame TESLA_602 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x602,
                         .data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  // Define initial UDS request

  //0x610 BMS Query UDS request: on demand
  static constexpr CAN_frame TESLA_610 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x610,
      .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};  // Define initial UDS request
  uint8_t index_1CF = 0;
  uint8_t index_118 = 0;
  uint8_t contactor_counter = 0;
  uint8_t stateMachineClearIsolationFault = 0xFF;
  uint8_t stateMachineBMSReset = 0xFF;
  uint8_t stateMachineSOCReset = 0xFF;
  uint8_t stateMachineBMSQuery = 0xFF;
  uint16_t battery_cell_max_v = 3300;
  uint16_t battery_cell_min_v = 3300;
  bool cellvoltagesRead = false;
  //0x3d2: 978 BMS_kwhCounter
  uint32_t battery_total_discharge = 0;
  uint32_t battery_total_charge = 0;
  //0x352: 850 BMS_energyStatus
  bool BMS352_mux = false;                            // variable to store when 0x352 mux is present
  uint16_t battery_energy_buffer = 0;                 // kWh
  uint16_t battery_energy_buffer_m1 = 0;              // kWh
  uint16_t battery_energy_to_charge_complete = 0;     // kWh
  uint16_t battery_energy_to_charge_complete_m1 = 0;  // kWh
  uint16_t battery_expected_energy_remaining = 0;     // kWh
  uint16_t battery_expected_energy_remaining_m1 = 0;  // kWh
  bool battery_full_charge_complete = false;          // Changed to bool
  bool battery_fully_charged = false;                 // Changed to bool
  uint16_t battery_ideal_energy_remaining = 0;        // kWh
  uint16_t battery_ideal_energy_remaining_m0 = 0;     // kWh
  uint16_t battery_nominal_energy_remaining = 0;      // kWh
  uint16_t battery_nominal_energy_remaining_m0 = 0;   // kWh
  uint16_t battery_nominal_full_pack_energy = 0;      // Kwh
  uint16_t battery_nominal_full_pack_energy_m0 = 0;   // Kwh
  //0x132 306 HVBattAmpVolt
  uint16_t battery_volts = 0;                  // V
  int16_t battery_amps = 0;                    // A
  int16_t battery_raw_amps = 0;                // A
  uint16_t battery_charge_time_remaining = 0;  // Minutes
  //0x252 594 BMS_powerAvailable
  uint16_t BMS_maxRegenPower = 0;           //rename from battery_regenerative_limit
  uint16_t BMS_maxDischargePower = 0;       // rename from battery_discharge_limit
  uint16_t BMS_maxStationaryHeatPower = 0;  //rename from battery_max_heat_park
  uint16_t BMS_hvacPowerBudget = 0;         //rename from battery_hvac_max_power
  uint8_t BMS_notEnoughPowerForHeatPump = 0;
  uint8_t BMS_powerLimitState = 0;
  uint8_t BMS_inverterTQF = 0;
  //0x2d2: 722 BMSVAlimits
  uint16_t battery_max_discharge_current = 0;
  uint16_t battery_max_charge_current = 0;
  uint16_t BMS_max_voltage = 0;
  uint16_t BMS_min_voltage = 0;
  //0x2b4: 692 PCS_dcdcRailStatus
  uint16_t battery_dcdcHvBusVolt = 0;        // Change name from battery_high_voltage to battery_dcdcHvBusVolt
  uint16_t battery_dcdcLvBusVolt = 0;        // Change name from battery_low_voltage to battery_dcdcLvBusVolt
  uint16_t battery_dcdcLvOutputCurrent = 0;  // Change name from battery_output_current to battery_dcdcLvOutputCurrent
  //0x292: 658 BMS_socStatus
  uint16_t battery_beginning_of_life = 0;  // kWh
  uint16_t battery_soc_min = 0;
  uint16_t battery_soc_max = 0;
  uint16_t battery_soc_ui = 0;  //Change name from battery_soc_vi to reflect DBC battery_soc_ui
  uint16_t battery_soc_ave = 0;
  uint8_t battery_battTempPct = 0;
  //0x392: BMS_packConfig
  uint32_t battery_packMass = 0;
  uint32_t battery_platformMaxBusVoltage = 0;
  uint32_t battery_packConfigMultiplexer = 0;
  uint32_t battery_moduleType = 0;
  uint32_t battery_reservedConfig = 0;
  //0x332: 818 BattBrickMinMax:BMS_bmbMinMax
  int16_t battery_max_temp = 0;  // C*
  int16_t battery_min_temp = 0;  // C*
  uint16_t battery_BrickVoltageMax = 0;
  uint16_t battery_BrickVoltageMin = 0;
  uint8_t battery_BrickTempMaxNum = 0;
  uint8_t battery_BrickTempMinNum = 0;
  uint8_t battery_BrickModelTMax = 0;
  uint8_t battery_BrickModelTMin = 0;
  uint8_t battery_BrickVoltageMaxNum = 0;  //rename from battery_max_vno
  uint8_t battery_BrickVoltageMinNum = 0;  //rename from battery_min_vno
  //0x20A: 522 HVP_contactorState
  uint8_t battery_contactor = 0;  //State of contactor
  uint8_t battery_hvil_status = 0;
  uint8_t battery_packContNegativeState = 0;
  uint8_t battery_packContPositiveState = 0;
  uint8_t battery_packContactorSetState = 0;
  bool battery_packCtrsClosingBlocked = false;    // Change to bool
  bool battery_pyroTestInProgress = false;        // Change to bool
  bool battery_packCtrsOpenNowRequested = false;  // Change to bool
  bool battery_packCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_packCtrsRequestStatus = 0;
  bool battery_packCtrsResetRequestRequired = false;  // Change to bool
  bool battery_dcLinkAllowedToEnergize = false;       // Change to bool
  bool battery_fcContNegativeAuxOpen = false;         // Change to bool
  uint8_t battery_fcContNegativeState = 0;
  bool battery_fcContPositiveAuxOpen = false;  // Change to bool
  uint8_t battery_fcContPositiveState = 0;
  uint8_t battery_fcContactorSetState = 0;
  bool battery_fcCtrsClosingAllowed = false;    // Change to bool
  bool battery_fcCtrsOpenNowRequested = false;  // Change to bool
  bool battery_fcCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_fcCtrsRequestStatus = 0;
  bool battery_fcCtrsResetRequestRequired = false;  // Change to bool
  bool battery_fcLinkAllowedToEnergize = false;     // Change to bool
                                                    //0x72A: BMS_serialNumber
  uint8_t battery_serialNumber[14] = {0};           // Stores raw HEX values for ASCII chars
  bool parsed_battery_serialNumber = false;
  char* battery_manufactureDate;         // YYYY-MM-DD\0
                                         //Via UDS
  uint8_t battery_partNumber[12] = {0};  //stores raw HEX values for ASCII chars
  bool parsed_battery_partNumber = false;
  //Via UDS
  //static uint8_t BMS_partNumber[12] = {0};  //stores raw HEX values for ASCII chars
  //static bool parsed_BMS_partNumber = false;
  //0x300: BMS_info
  uint16_t BMS_info_buildConfigId = 0;
  uint16_t BMS_info_hardwareId = 0;
  uint16_t BMS_info_componentId = 0;
  uint8_t BMS_info_pcbaId = 0;
  uint8_t BMS_info_assemblyId = 0;
  uint16_t BMS_info_usageId = 0;
  uint16_t BMS_info_subUsageId = 0;
  uint8_t BMS_info_platformType = 0;
  uint32_t BMS_info_appCrc = 0;
  uint64_t BMS_info_bootGitHash = 0;
  uint8_t BMS_info_bootUdsProtoVersion = 0;
  uint32_t BMS_info_bootCrc = 0;
  //0x212: 530 BMS_status
  bool BMS_hvacPowerRequest = false;          //Change to bool
  bool BMS_notEnoughPowerForDrive = false;    //Change to bool
  bool BMS_notEnoughPowerForSupport = false;  //Change to bool
  bool BMS_preconditionAllowed = false;       //Change to bool
  bool BMS_updateAllowed = false;             //Change to bool
  bool BMS_activeHeatingWorthwhile = false;   //Change to bool
  bool BMS_cpMiaOnHvs = false;                //Change to bool
  uint8_t BMS_contactorState = 0;
  uint8_t BMS_state = 0;
  uint8_t BMS_hvState = 0;
  uint16_t BMS_isolationResistance = 0;
  bool BMS_chargeRequest = false;    //Change to bool
  bool BMS_keepWarmRequest = false;  //Change to bool
  uint8_t BMS_uiChargeStatus = 0;
  bool BMS_diLimpRequest = false;   //Change to bool
  bool BMS_okToShipByAir = false;   //Change to bool
  bool BMS_okToShipByLand = false;  //Change to bool
  uint32_t BMS_chgPowerAvailable = 0;
  uint8_t BMS_chargeRetryCount = 0;
  bool BMS_pcsPwmEnabled = false;        //Change to bool
  bool BMS_ecuLogUploadRequest = false;  //Change to bool
  uint8_t BMS_minPackTemperature = 0;
  // 0x224:548 PCS_dcdcStatus
  uint8_t PCS_dcdcPrechargeStatus = 0;
  uint8_t PCS_dcdc12VSupportStatus = 0;
  uint8_t PCS_dcdcHvBusDischargeStatus = 0;
  uint16_t PCS_dcdcMainState = 0;
  uint8_t PCS_dcdcSubState = 0;
  bool PCS_dcdcFaulted = false;          //Change to bool
  bool PCS_dcdcOutputIsLimited = false;  //Change to bool
  uint32_t PCS_dcdcMaxOutputCurrentAllowed = 0;
  uint8_t PCS_dcdcPrechargeRtyCnt = 0;
  uint8_t PCS_dcdc12VSupportRtyCnt = 0;
  uint8_t PCS_dcdcDischargeRtyCnt = 0;
  uint8_t PCS_dcdcPwmEnableLine = 0;
  uint8_t PCS_dcdcSupportingFixedLvTarget = 0;
  uint8_t PCS_ecuLogUploadRequest = 0;
  uint8_t PCS_dcdcPrechargeRestartCnt = 0;
  uint8_t PCS_dcdcInitialPrechargeSubState = 0;
  //0x312: 786 BMS_thermalStatus
  uint16_t BMS_powerDissipation = 0;
  uint16_t BMS_flowRequest = 0;
  uint16_t BMS_inletActiveCoolTargetT = 0;
  uint16_t BMS_inletPassiveTargetT = 0;
  uint16_t BMS_inletActiveHeatTargetT = 0;
  uint16_t BMS_packTMin = 0;
  uint16_t BMS_packTMax = 0;
  bool BMS_pcsNoFlowRequest = false;
  bool BMS_noFlowRequest = false;
  //0x3C4: PCS_info
  uint8_t PCS_partNumber[13] = {0};  //stores raw HEX values for ASCII chars
  bool parsed_PCS_partNumber = false;
  uint16_t PCS_info_buildConfigId = 0;
  uint16_t PCS_info_hardwareId = 0;
  uint16_t PCS_info_componentId = 0;
  uint8_t PCS_info_pcbaId = 0;
  uint8_t PCS_info_assemblyId = 0;
  uint16_t PCS_info_usageId = 0;
  uint16_t PCS_info_subUsageId = 0;
  uint8_t PCS_info_platformType = 0;
  uint32_t PCS_info_appCrc = 0;
  uint32_t PCS_info_cpu2AppCrc = 0;
  uint64_t PCS_info_bootGitHash = 0;
  uint8_t PCS_info_bootUdsProtoVersion = 0;
  uint32_t PCS_info_bootCrc = 0;
  //0x2A4; 676 PCS_thermalStatus
  int16_t PCS_chgPhATemp = 0;
  int16_t PCS_chgPhBTemp = 0;
  int16_t PCS_chgPhCTemp = 0;
  int16_t PCS_dcdcTemp = 0;
  int16_t PCS_ambientTemp = 0;
  //0x2C4; 708 PCS_logging
  uint16_t PCS_logMessageSelect = 0;
  uint16_t PCS_dcdcMaxLvOutputCurrent = 0;
  uint16_t PCS_dcdcCurrentLimit = 0;
  uint16_t PCS_dcdcLvOutputCurrentTempLimit = 0;
  uint16_t PCS_dcdcUnifiedCommand = 0;
  uint16_t PCS_dcdcCLAControllerOutput = 0;
  int16_t PCS_dcdcTankVoltage = 0;
  uint16_t PCS_dcdcTankVoltageTarget = 0;
  uint16_t PCS_dcdcClaCurrentFreq = 0;
  int16_t PCS_dcdcTCommMeasured = 0;
  uint16_t PCS_dcdcShortTimeUs = 0;
  uint16_t PCS_dcdcHalfPeriodUs = 0;
  uint16_t PCS_dcdcIntervalMaxFrequency = 0;
  uint16_t PCS_dcdcIntervalMaxHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvOutputCurr = 0;
  uint16_t PCS_dcdcIntervalMinFrequency = 0;
  uint16_t PCS_dcdcIntervalMinHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvOutputCurr = 0;
  uint32_t PCS_dcdc12vSupportLifetimekWh = 0;
  //0x7AA: //1962 HVP_debugMessage:
  uint8_t HVP_debugMessageMultiplexer = 0;
  bool HVP_gpioPassivePyroDepl = false;       //Change to bool
  bool HVP_gpioPyroIsoEn = false;             //Change to bool
  bool HVP_gpioCpFaultIn = false;             //Change to bool
  bool HVP_gpioPackContPowerEn = false;       //Change to bool
  bool HVP_gpioHvCablesOk = false;            //Change to bool
  bool HVP_gpioHvpSelfEnable = false;         //Change to bool
  bool HVP_gpioLed = false;                   //Change to bool
  bool HVP_gpioCrashSignal = false;           //Change to bool
  bool HVP_gpioShuntDataReady = false;        //Change to bool
  bool HVP_gpioFcContPosAux = false;          //Change to bool
  bool HVP_gpioFcContNegAux = false;          //Change to bool
  bool HVP_gpioBmsEout = false;               //Change to bool
  bool HVP_gpioCpFaultOut = false;            //Change to bool
  bool HVP_gpioPyroPor = false;               //Change to bool
  bool HVP_gpioShuntEn = false;               //Change to bool
  bool HVP_gpioHvpVerEn = false;              //Change to bool
  bool HVP_gpioPackCoontPosFlywheel = false;  //Change to bool
  bool HVP_gpioCpLatchEnable = false;         //Change to bool
  bool HVP_gpioPcsEnable = false;             //Change to bool
  bool HVP_gpioPcsDcdcPwmEnable = false;      //Change to bool
  bool HVP_gpioPcsChargePwmEnable = false;    //Change to bool
  bool HVP_gpioFcContPowerEnable = false;     //Change to bool
  bool HVP_gpioHvilEnable = false;            //Change to bool
  bool HVP_gpioSecDrdy = false;               //Change to bool
  uint16_t HVP_hvp1v5Ref = 0;
  int16_t HVP_shuntCurrentDebug = 0;
  bool HVP_packCurrentMia = false;           //Change to bool
  bool HVP_auxCurrentMia = false;            //Change to bool
  bool HVP_currentSenseMia = false;          //Change to bool
  bool HVP_shuntRefVoltageMismatch = false;  //Change to bool
  bool HVP_shuntThermistorMia = false;       //Change to bool
  bool HVP_shuntHwMia = false;               //Change to bool
  uint16_t HVP_info_buildConfigId = 0;
  uint16_t HVP_info_hardwareId = 0;
  uint16_t HVP_info_componentId = 0;
  uint8_t HVP_info_pcbaId = 0;
  uint8_t HVP_info_assemblyId = 0;
  uint16_t HVP_info_usageId = 0;
  uint16_t HVP_info_subUsageId = 0;
  uint8_t HVP_info_platformType = 0;
  uint32_t HVP_info_appCrc = 0;
  uint64_t HVP_info_bootGitHash = 0;
  uint8_t HVP_info_bootUdsProtoVersion = 0;
  uint32_t HVP_info_bootCrc = 0;
  int16_t HVP_dcLinkVoltage = 0;
  int16_t HVP_packVoltage = 0;
  int16_t HVP_fcLinkVoltage = 0;
  uint16_t HVP_packContVoltage = 0;
  int16_t HVP_packNegativeV = 0;
  int16_t HVP_packPositiveV = 0;
  uint16_t HVP_pyroAnalog = 0;
  int16_t HVP_dcLinkNegativeV = 0;
  int16_t HVP_dcLinkPositiveV = 0;
  int16_t HVP_fcLinkNegativeV = 0;
  uint16_t HVP_fcContCoilCurrent = 0;
  uint16_t HVP_fcContVoltage = 0;
  uint16_t HVP_hvilInVoltage = 0;
  uint16_t HVP_hvilOutVoltage = 0;
  int16_t HVP_fcLinkPositiveV = 0;
  uint16_t HVP_packContCoilCurrent = 0;
  uint16_t HVP_battery12V = 0;
  int16_t HVP_shuntRefVoltageDbg = 0;
  int16_t HVP_shuntAuxCurrentDbg = 0;
  int16_t HVP_shuntBarTempDbg = 0;
  int16_t HVP_shuntAsicTempDbg = 0;
  uint8_t HVP_shuntAuxCurrentStatus = 0;
  uint8_t HVP_shuntBarTempStatus = 0;
  uint8_t HVP_shuntAsicTempStatus = 0;
  //0x3aa: HVP_alertMatrix1 Fault codes   // Change to bool
  bool battery_WatchdogReset = false;           //Warns if the processor has experienced a reset due to watchdog reset.
  bool battery_PowerLossReset = false;          //Warns if the processor has experienced a reset due to power loss.
  bool battery_SwAssertion = false;             //An internal software assertion has failed.
  bool battery_CrashEvent = false;              //Warns if the crash signal is detected by HVP
  bool battery_OverDchgCurrentFault = false;    //Warns if the pack discharge is above max discharge current limit
  bool battery_OverChargeCurrentFault = false;  //Warns if the pack discharge current is above max charge current limit
  bool battery_OverCurrentFault = false;  //Warns if the pack current (discharge or charge) is above max current limit.
  bool battery_OverTemperatureFault = false;  //A pack module temperature is above maximum temperature limit
  bool battery_OverVoltageFault = false;      //A brick voltage is above maximum voltage limit
  bool battery_UnderVoltageFault = false;     //A brick voltage is below minimum voltage limit
  bool battery_PrimaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from primary BMB chain are mia
  bool battery_SecondaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
  bool battery_BmbMismatchFault =
      false;  //Warns if the primary and secondary BMB chain readings don't match with each other
  bool battery_BmsHviMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_CpMiaFault = false;       //Warns if the CP node is mia on HVS CAN
  bool battery_PcsMiaFault = false;      //The PCS node is mia on HVS CAN
  bool battery_BmsFault = false;         //Warns if the BMS ECU has faulted
  bool battery_PcsFault = false;         //Warns if the PCS ECU has faulted
  bool battery_CpFault = false;          //Warns if the CP ECU has faulted
  bool battery_ShuntHwMiaFault = false;  //Warns if the shunt current reading is not available
  bool battery_PyroMiaFault = false;     //Warns if the pyro squib is not connected
  bool battery_hvsMiaFault = false;      //Warns if the pack contactor hw fault
  bool battery_hviMiaFault = false;      //Warns if the FC contactor hw fault
  bool battery_Supply12vFault = false;   //Warns if the low voltage (12V) battery is below minimum voltage threshold
  bool battery_VerSupplyFault = false;   //Warns if the Energy reserve voltage supply is below minimum voltage threshold
  bool battery_HvilFault = false;        //Warn if a High Voltage Inter Lock fault is detected
  bool battery_BmsHvsMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_PackVoltMismatchFault =
      false;                         //Warns if the pack voltage doesn't match approximately with sum of brick voltages
  bool battery_EnsMiaFault = false;  //Warns if the ENS line is not connected to HVC
  bool battery_PackPosCtrArcFault = false;  //Warns if the HVP detectes series arc at pack contactor
  bool battery_packNegCtrArcFault = false;  //Warns if the HVP detectes series arc at FC contactor
  bool battery_ShuntHwAndBmsMiaFault = false;
  bool battery_fcContHwFault = false;
  bool battery_robinOverVoltageFault = false;
  bool battery_packContHwFault = false;
  bool battery_pyroFuseBlown = false;
  bool battery_pyroFuseFailedToBlow = false;
  bool battery_CpilFault = false;
  bool battery_PackContactorFellOpen = false;
  bool battery_FcContactorFellOpen = false;
  bool battery_packCtrCloseBlocked = false;
  bool battery_fcCtrCloseBlocked = false;
  bool battery_packContactorForceOpen = false;
  bool battery_fcContactorForceOpen = false;
  bool battery_dcLinkOverVoltage = false;
  bool battery_shuntOverTemperature = false;
  bool battery_passivePyroDeploy = false;
  bool battery_logUploadRequest = false;
  bool battery_packCtrCloseFailed = false;
  bool battery_fcCtrCloseFailed = false;
  bool battery_shuntThermistorMia = false;
  //0x320: 800 BMS_alertMatrix
  uint8_t BMS_matrixIndex = 0;  // Changed to bool
  bool BMS_a061_robinBrickOverVoltage = false;
  bool BMS_a062_SW_BrickV_Imbalance = false;
  bool BMS_a063_SW_ChargePort_Fault = false;
  bool BMS_a064_SW_SOC_Imbalance = false;
  bool BMS_a127_SW_shunt_SNA = false;
  bool BMS_a128_SW_shunt_MIA = false;
  bool BMS_a069_SW_Low_Power = false;
  bool BMS_a130_IO_CAN_Error = false;
  bool BMS_a071_SW_SM_TransCon_Not_Met = false;
  bool BMS_a132_HW_BMB_OTP_Uncorrctbl = false;
  bool BMS_a134_SW_Delayed_Ctr_Off = false;
  bool BMS_a075_SW_Chg_Disable_Failure = false;
  bool BMS_a076_SW_Dch_While_Charging = false;
  bool BMS_a017_SW_Brick_OV = false;
  bool BMS_a018_SW_Brick_UV = false;
  bool BMS_a019_SW_Module_OT = false;
  bool BMS_a021_SW_Dr_Limits_Regulation = false;
  bool BMS_a022_SW_Over_Current = false;
  bool BMS_a023_SW_Stack_OV = false;
  bool BMS_a024_SW_Islanded_Brick = false;
  bool BMS_a025_SW_PwrBalance_Anomaly = false;
  bool BMS_a026_SW_HFCurrent_Anomaly = false;
  bool BMS_a087_SW_Feim_Test_Blocked = false;
  bool BMS_a088_SW_VcFront_MIA_InDrive = false;
  bool BMS_a089_SW_VcFront_MIA = false;
  bool BMS_a090_SW_Gateway_MIA = false;
  bool BMS_a091_SW_ChargePort_MIA = false;
  bool BMS_a092_SW_ChargePort_Mia_On_Hv = false;
  bool BMS_a034_SW_Passive_Isolation = false;
  bool BMS_a035_SW_Isolation = false;
  bool BMS_a036_SW_HvpHvilFault = false;
  bool BMS_a037_SW_Flood_Port_Open = false;
  bool BMS_a158_SW_HVP_HVI_Comms = false;
  bool BMS_a039_SW_DC_Link_Over_Voltage = false;
  bool BMS_a041_SW_Power_On_Reset = false;
  bool BMS_a042_SW_MPU_Error = false;
  bool BMS_a043_SW_Watch_Dog_Reset = false;
  bool BMS_a044_SW_Assertion = false;
  bool BMS_a045_SW_Exception = false;
  bool BMS_a046_SW_Task_Stack_Usage = false;
  bool BMS_a047_SW_Task_Stack_Overflow = false;
  bool BMS_a048_SW_Log_Upload_Request = false;
  bool BMS_a169_SW_FC_Pack_Weld = false;
  bool BMS_a050_SW_Brick_Voltage_MIA = false;
  bool BMS_a051_SW_HVC_Vref_Bad = false;
  bool BMS_a052_SW_PCS_MIA = false;
  bool BMS_a053_SW_ThermalModel_Sanity = false;
  bool BMS_a054_SW_Ver_Supply_Fault = false;
  bool BMS_a176_SW_GracefulPowerOff = false;
  bool BMS_a059_SW_Pack_Voltage_Sensing = false;
  bool BMS_a060_SW_Leakage_Test_Failure = false;
  bool BMS_a077_SW_Charger_Regulation = false;
  bool BMS_a081_SW_Ctr_Close_Blocked = false;
  bool BMS_a082_SW_Ctr_Force_Open = false;
  bool BMS_a083_SW_Ctr_Close_Failure = false;
  bool BMS_a084_SW_Sleep_Wake_Aborted = false;
  bool BMS_a094_SW_Drive_Inverter_MIA = false;
  bool BMS_a099_SW_BMB_Communication = false;
  bool BMS_a105_SW_One_Module_Tsense = false;
  bool BMS_a106_SW_All_Module_Tsense = false;
  bool BMS_a107_SW_Stack_Voltage_MIA = false;
  bool BMS_a121_SW_NVRAM_Config_Error = false;
  bool BMS_a122_SW_BMS_Therm_Irrational = false;
  bool BMS_a123_SW_Internal_Isolation = false;
  bool BMS_a129_SW_VSH_Failure = false;
  bool BMS_a131_Bleed_FET_Failure = false;
  bool BMS_a136_SW_Module_OT_Warning = false;
  bool BMS_a137_SW_Brick_UV_Warning = false;
  bool BMS_a138_SW_Brick_OV_Warning = false;
  bool BMS_a139_SW_DC_Link_V_Irrational = false;
  bool BMS_a141_SW_BMB_Status_Warning = false;
  bool BMS_a144_Hvp_Config_Mismatch = false;
  bool BMS_a145_SW_SOC_Change = false;
  bool BMS_a146_SW_Brick_Overdischarged = false;
  bool BMS_a149_SW_Missing_Config_Block = false;
  bool BMS_a151_SW_external_isolation = false;
  bool BMS_a156_SW_BMB_Vref_bad = false;
  bool BMS_a157_SW_HVP_HVS_Comms = false;
  bool BMS_a159_SW_HVP_ECU_Error = false;
  bool BMS_a161_SW_DI_Open_Request = false;
  bool BMS_a162_SW_No_Power_For_Support = false;
  bool BMS_a163_SW_Contactor_Mismatch = false;
  bool BMS_a164_SW_Uncontrolled_Regen = false;
  bool BMS_a165_SW_Pack_Partial_Weld = false;
  bool BMS_a166_SW_Pack_Full_Weld = false;
  bool BMS_a167_SW_FC_Partial_Weld = false;
  bool BMS_a168_SW_FC_Full_Weld = false;
  bool BMS_a170_SW_Limp_Mode = false;
  bool BMS_a171_SW_Stack_Voltage_Sense = false;
  bool BMS_a174_SW_Charge_Failure = false;
  bool BMS_a179_SW_Hvp_12V_Fault = false;
  bool BMS_a180_SW_ECU_reset_blocked = false;
};

class TeslaModel3YBattery : public TeslaBattery {
 public:
  TeslaModel3YBattery(battery_chemistry_enum chemistry) { datalayer.battery.info.chemistry = chemistry; }
  static constexpr const char* Name = "Tesla Model 3/Y";
  virtual void setup(void);
};

class TeslaModelSXBattery : public TeslaBattery {
 public:
  TeslaModelSXBattery() {}
  static constexpr const char* Name = "Tesla Model S/X";
  virtual void setup(void);
};

#endif
