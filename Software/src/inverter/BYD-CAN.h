#ifndef BYD_CAN_H
#define BYD_CAN_H

#include "../datalayer/datalayer.h"
#include "CanInverterProtocol.h"

class BydCanInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  void update_values();
  static constexpr const char* Name = "BYD Battery-Box Premium HVS over CAN Bus";

 private:
  void send_initial_data();
  unsigned long previousMillis2s = 0;   // will store last time a 2s CAN Message was send
  unsigned long previousMillis10s = 0;  // will store last time a 10s CAN Message was send
  unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

  static const int FW_MAJOR_VERSION = 0x03;
  static const int FW_MINOR_VERSION = 0x29;
  static const int VOLTAGE_OFFSET_DV = 20;

  CAN_frame BYD_250 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x250,
                       .data = {FW_MAJOR_VERSION, FW_MINOR_VERSION, 0x00, 0x66,
                                (uint8_t)((datalayer.battery.info.reported_total_capacity_Wh / 100) >> 8),
                                (uint8_t)(datalayer.battery.info.reported_total_capacity_Wh / 100), 0x02,
                                0x09}};  //0-1 FW version , Capacity kWh byte4&5 (example 24kWh = 240)
  CAN_frame BYD_290 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x290,
                       .data = {0x06, 0x37, 0x10, 0xD9, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_2D0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x2D0,
                       .data = {0x00, 0x42, 0x59, 0x44, 0x00, 0x00, 0x00, 0x00}};  //BYD
  CAN_frame BYD_3D0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3D0,
                       .data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79}};  //Battery
  //Actual content messages
  CAN_frame BYD_110 = {.FD = false,  //Limits
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x110,
                       .data = {0x01, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_150 = {.FD = false,  //States
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x150,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00}};
  CAN_frame BYD_190 = {.FD = false,  //Alarm
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x190,
                       .data = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_1D0 = {.FD = false,  //Battery Info
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D0,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08}};
  CAN_frame BYD_210 = {.FD = false,  //Cell info
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x210,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  int16_t temperature_average = 0;
  uint16_t inverter_voltage = 0;
  uint16_t inverter_SOC = 0;
  int16_t inverter_current = 0;
  int16_t inverter_temperature = 0;
  uint16_t remaining_capacity_ah = 0;
  uint16_t fully_charged_capacity_ah = 0;
  long inverter_timestamp = 0;
  bool initialDataSent = false;
  bool inverterStartedUp = false;
};

#endif
