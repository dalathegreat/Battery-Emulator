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

  bool supports_reset_crash() { return true; }
  void reset_crash() { datalayer_extended.GeelySEA.UserRequestCrashReset = true; }

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

  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was send
  
  static const uint16_t POLL_BECMsupplyVoltage = 0xEE02;
  static const uint16_t POLL_HV_Voltage = 0x4803;
  static const uint16_t POLL_SOC = 0x4801;
  static const uint16_t POLL_SOH = 0x489A;
  static const uint16_t POLL_HighestCellTemp = 0x4945;
  static const uint16_t POLL_AverageCellTemp = 0x491B;
  static const uint16_t POLL_LowestCellTemp = 0x49A1;
  static const uint16_t POLL_Interlock = 0x491A;
  static const uint16_t POLL_HighestCellVolt = 0x4907;
  static const uint16_t POLL_LowestCellVolt = 0x4908;
  static const uint16_t POLL_BatteryCurrent = 0x4802;
  static const uint16_t POLL_CrashStatus = 0xFE42;

  uint8_t pause_polling_seconds = 0;
  bool DTC_readout_in_progress = false;

  const uint16_t poll_commands[12] = {POLL_BECMsupplyVoltage,
                                      POLL_HV_Voltage,
                                      POLL_SOC,
                                      POLL_SOH,
                                      POLL_HighestCellTemp,
                                      POLL_AverageCellTemp,
                                      POLL_LowestCellTemp,
                                      POLL_Interlock,
                                      POLL_HighestCellVolt,
                                      POLL_LowestCellVolt,
                                      POLL_BatteryCurrent,
                                      POLL_CrashStatus};

  uint8_t poll_index = 0;
  uint16_t currentpoll = POLL_BECMsupplyVoltage;
  uint16_t reply_poll = 0;

  int16_t pack_current_dA = 0;
  uint16_t pack_voltage_dV = 3700;

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
                       .data = {0x80, 0x00, 0x01, 0x38, 0xEE, 0x47, 0x00, 0x40}};  //Motor B on Zeekr battery

  CAN_frame SEA_156 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x156,
                       .data = {0x7F, 0x4E, 0x10, 0x00, 0x00, 0x00, 0x2E, 0x10}};  //Motor A on Zeekr battery
  CAN_frame SEA_171 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x171,
                       .data = {0x00, 0x00, 0x80, 0x00, 0xA0, 0x00, 0x00, 0x6C}};  // Prevents DTC on EX30 battery
  CAN_frame SEA_218 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x218,
                       .data = {0xC0, 0x00, 0x12, 0x3B, 0x1A, 0x30, 0x00, 0x01}};  // Prevents DTC on EX30 battery
  CAN_frame SEA_490 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x490,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x4B, 0xA3}};  // Prevents DTC on EX30 battery
  CAN_frame SEA_103 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x103,
                       .data = {0x20, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x00, 0x00}};  // Prevents DTC on Zeekr battery (Engine Control Module)

  CAN_frame SEA_Polling_Req = {
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
                               .data = {0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Flowcontrol + 5ms delay

  CAN_frame SEA_DTC_Erase = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x735,
                             .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};  //Global DTC erase
  CAN_frame SEA_StartDiag = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x735,
                             .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Start diag session
  CAN_frame SEA_ClearCrash = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x735,
                             .data = {0x04, 0x31, 0x01, 0x30, 0x03, 0x00, 0x00, 0x00}};  //Clear crash status

  CAN_frame SEA_BECM_ECUreset = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x02, 0x11, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00}};  //BECM ECU reset command (reboot/powercycle BECM)
};

#endif
