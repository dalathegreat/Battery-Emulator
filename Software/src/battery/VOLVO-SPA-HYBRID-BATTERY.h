#ifndef VOLVO_SPA_HYBRID_BATTERY_H
#define VOLVO_SPA_HYBRID_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class VolvoSpaHybridBattery : public CanBattery {
 public:
  VolvoSpaHybridBattery() : CanBattery(VolvoSpaHybrid) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Volvo PHEV battery";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 4294; }
  virtual uint16_t min_pack_voltage_dv() { return 2754; }
  virtual uint16_t max_cell_deviation_mv() { return 250; }
  virtual uint16_t max_cell_voltage_mv() { return 4210; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }

 private:
  void readCellVoltages();
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1s = 0;   // will store last time a 1s CAN Message was send
  unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

  float BATT_U = 0;                 //0x3A
  float MAX_U = 0;                  //0x3A
  float MIN_U = 0;                  //0x3A
  float BATT_I = 0;                 //0x3A
  int32_t CHARGE_ENERGY = 0;        //0x1A1
  uint8_t BATT_ERR_INDICATION = 0;  //0x413
  float BATT_T_MAX = 0;             //0x413
  float BATT_T_MIN = 0;             //0x413
  float BATT_T_AVG = 0;             //0x413
  uint16_t SOC_BMS = 0;             //0X37D
  uint16_t SOC_CALC = 0;
  uint16_t CELL_U_MAX = 3700;         //0x37D
  uint16_t CELL_U_MIN = 3700;         //0x37D
  uint8_t CELL_ID_U_MAX = 0;          //0x37D
  uint16_t HvBattPwrLimDchaSoft = 0;  //0x369
  uint16_t HvBattPwrLimDcha1 = 0;     //0x175
  //uint16_t HvBattPwrLimDchaSlowAgi = 0;  //0x177
  //uint16_t HvBattPwrLimChrgSlowAgi = 0;  //0x177
  //uint8_t batteryModuleNumber = 0x10;    // First battery module
  uint8_t battery_request_idx = 0;
  uint8_t rxConsecutiveFrames = 0;
  uint16_t min_max_voltage[2];  //contains cell min[0] and max[1] values in mV
  uint8_t cellcounter = 0;
  uint32_t remaining_capacity = 0;
  uint16_t cell_voltages[102];  //array with all the cellvoltages
  bool startedUp = false;
  uint8_t DTC_reset_counter = 0;

  CAN_frame VOLVO_536 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x536,
                         //.data = {0x00, 0x40, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame
                         .data = {0x00, 0x40, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame

  CAN_frame VOLVO_140_CLOSE = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x140,
                               .data = {0x00, 0x02, 0x00, 0xB7, 0xFF, 0x03, 0xFF, 0x82}};  //Close contactors message

  CAN_frame VOLVO_140_OPEN = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x140,
                              .data = {0x00, 0x02, 0x00, 0x9E, 0xFF, 0x03, 0xFF, 0x82}};  //Open contactor message

  CAN_frame VOLVO_372 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x372,
      .data = {0x00, 0xA6, 0x07, 0x14, 0x04, 0x00, 0x80, 0x00}};  //Ambient Temp -->>VERIFY this data content!!!<<--
  CAN_frame VOLVO_CELL_U_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0x48, 0x06, 0x00, 0x00, 0x00, 0x00}};  //Cell voltage request frame // changed
  CAN_frame VOLVO_FlowControl = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x735,
                                 .data = {0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Flowcontrol
  CAN_frame VOLVO_SOH_Req = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x735,
                             .data = {0x03, 0x22, 0x49, 0x6D, 0x00, 0x00, 0x00, 0x00}};  //Battery SOH request frame
  CAN_frame VOLVO_BECMsupplyVoltage_Req = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x03, 0x22, 0xF4, 0x42, 0x00, 0x00, 0x00, 0x00}};  //BECM supply voltage request frame
  CAN_frame VOLVO_DTC_Erase = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7FF,
                               .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};  //Global DTC erase
  CAN_frame VOLVO_BECM_ECUreset = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x735,
      .data = {0x02, 0x11, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00}};  //BECM ECU reset command (reboot/powercycle BECM)
  CAN_frame VOLVO_DTCreadout = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x7FF,
                                .data = {0x02, 0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Global DTC readout
};

#endif
