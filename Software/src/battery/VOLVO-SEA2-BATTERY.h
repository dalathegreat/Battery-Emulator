#ifndef VOLVO_SEA2_BATTERY_H
#define VOLVO_SEA2_BATTERY_H

#include "CanBattery.h"
#include "GEELY-SEA-HTML.h"

class VolvoSea2Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Volvo/Zeekr/Geely SEA2 battery";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  
  GeelySeaHtmlRenderer renderer;

  void readDiagData();
  
  static const int MAX_PACK_VOLTAGE_LFP_120S_DV = 4380;
  static const int MIN_PACK_VOLTAGE_LFP_120S_DV = 3600;
  static const int MAX_CAPACITY_LFP_WH = 51000;
  static const int MAX_PACK_VOLTAGE_NCM_107S_DV = 4650;
  static const int MIN_PACK_VOLTAGE_NCM_107S_DV = 3210;
  static const int MAX_CAPACITY_NCM_WH = 69000;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4260;  // Charging is halted if one cell goes above this
  static const int MIN_CELL_VOLTAGE_MV = 2900;  // Charging is halted if one cell goes below this

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1s = 0;   // will store last time a 1s CAN Message was send
  unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

  bool waiting4answer = false;

  CAN_frame SEA2_536 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x536,
      .data = {0x00, 0x41, 0x40, 0x21, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame (38 transmitted ID´s)
      //.data = {0x00, 0x40, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame (33 transmitted ID´s)

  CAN_frame SEA2_372 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x372,
      .data = {0x00, 0xA6, 0x07, 0x14, 0x04, 0x00, 0x80, 0x00}};  //Ambient Temp -->>VERIFY this data content!!!<<--

  CAN_frame SEA2_BECMsupplyVoltage_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0xEE, 0x02, 0x00, 0x00, 0x00, 0x00}};  //BECM supply voltage request frame

  CAN_frame SEA2_HV_Voltage_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x03, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery voltage request frame

  CAN_frame SEA2_SOC_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery SOC request frame

  CAN_frame SEA2_SOH_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x9A, 0x00, 0x00, 0x00, 0x00}};  //High voltage battery SOH request frame

  CAN_frame SEA2_HighestCellTemp_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x45, 0x00, 0x00, 0x00, 0x00}};  //Highest cell temp request frame
      
  CAN_frame SEA2_LowestCellTemp_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x49, 0x1B, 0x00, 0x00, 0x00, 0x00}};  //Lowest cell temp request frame
};

#endif
