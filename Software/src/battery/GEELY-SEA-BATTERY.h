#ifndef GEELY_SEA_BATTERY_H
#define GEELY_SEA_BATTERY_H

#include "CanBattery.h"
#include "GEELY-SEA-HTML.h"

class GeelySeaBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Volvo/Zeekr/Geely SEA battery";

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { datalayer_extended.GeelySEA.UserRequestDTCreset = true; }

  bool supports_read_DTC() { return true; }
  void read_DTC() { datalayer_extended.GeelySEA.UserRequestDTCreadout = true; }

  bool supports_reset_BECM() { return true; }
  void reset_BECM() { datalayer_extended.GeelySEA.UserRequestBECMecuReset = true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  GeelySeaHtmlRenderer renderer;

  void readDiagData();

  static const int MAX_PACK_VOLTAGE_LFP_120S_DV = 4380;
  static const int MIN_PACK_VOLTAGE_LFP_120S_DV = 3600;
  static const int MAX_CAPACITY_LFP_WH = 51000;
  static const int MAX_PACK_VOLTAGE_NCM_110S_DV = 4650;
  static const int MIN_PACK_VOLTAGE_NCM_110S_DV = 3210;
  static const int MAX_CAPACITY_NCM_110S_WH = 100000;
  static const int MAX_PACK_VOLTAGE_NCM_107S_DV = 4523;
  static const int MIN_PACK_VOLTAGE_NCM_107S_DV = 3122;
  static const int MAX_CAPACITY_NCM_107S_WH = 69000;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4260;  // Charging is halted if one cell goes above this
  static const int MIN_CELL_VOLTAGE_MV = 2900;  // Charging is halted if one cell goes below this

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1s = 0;   // will store last time a 1s CAN Message was send
  unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

  bool startedUp = false;

  CAN_frame SEA_536 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x536,
      .data = {0x00, 0x41, 0x40, 0x21, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame (38 transmitted ID´s)
  //.data = {0x00, 0x40, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame (33 transmitted ID´s)

  CAN_frame SEA_060 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x060,
                       .data = {0x80, 0x00, 0x01, 0x38, 0xEE, 0x47, 0x00, 0x40}};  //Motor B

  CAN_frame SEA_156 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x156,
                       .data = {0x7F, 0x4E, 0x10, 0x00, 0x00, 0x00, 0x2E, 0x10}};  //Motor A

  CAN_frame SEA_BECMsupplyVoltage_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0xEE, 0x02, 0x00, 0x00, 0x00, 0x00}};  //BECM supply voltage request frame

  CAN_frame SEA_HV_Voltage_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x03, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery voltage request frame

  CAN_frame SEA_SOC_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery SOC request frame

  CAN_frame SEA_SOH_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x9A, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery SOH request frame

  CAN_frame SEA_HighestCellTemp_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x45, 0x00, 0x00, 0x00, 0x00}};  //Highest cell temp request frame

  CAN_frame SEA_AverageCellTemp_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x1B, 0x00, 0x00, 0x00, 0x00}};  //Average cell temp request frame

  CAN_frame SEA_LowestCellTemp_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0xA1, 0x00, 0x00, 0x00, 0x00}};  //Lowest cell temp request frame

  CAN_frame SEA_Interlock_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x1A, 0x00, 0x00, 0x00, 0x00}};  //Interlock status request frame

  CAN_frame SEA_HighestCellVolt_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x07, 0x00, 0x00, 0x00, 0x00}};  //Highest cell volt request frame

  CAN_frame SEA_LowestCellVolt_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x08, 0x00, 0x00, 0x00, 0x00}};  //Lowest cell volt request frame

  CAN_frame SEA_BatteryCurrent_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x02, 0x00, 0x00, 0x00, 0x00}};  //Battery current request frame

  CAN_frame SEA_DTC_Req = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x735,
                           .data = {0x02, 0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};  //DTC request frame

  CAN_frame SEA_Flowcontrol = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x735,
                               .data = {0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Flowcontrol

  CAN_frame SEA_DTC_Erase = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x735,
                             .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};  //Global DTC erase

  CAN_frame SEA_BECM_ECUreset = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x02, 0x11, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00}};  //BECM ECU reset command (reboot/powercycle BECM)
};

#endif
