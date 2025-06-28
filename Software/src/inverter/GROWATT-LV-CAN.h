#ifndef GROWATT_LV_CAN_H
#define GROWATT_LV_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef GROWATT_LV_CAN
#define SELECTED_INVERTER_CLASS GrowattLvInverter
#endif

class GrowattLvInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "Growatt Low Voltage (48V) protocol via CAN";

 private:
  //Actual content messages
  CAN_frame GROWATT_311 = {.FD = false,  //Voltage and charge limits and status
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x311,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_312 = {.FD = false,  //status bits , pack number, total cell number
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x312,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_313 = {.FD = false,  //voltage, current, temp, soc, soh
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x313,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_314 = {.FD = false,  //capacity, delta V, cycle count
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x314,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_319 = {.FD = false,  //max/min cell voltage, num of cell max/min, protect pack ID
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x319,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_320 = {.FD = false,  //manufacturer name, hw ver, sw  ver, date and time
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x320,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_321 = {.FD = false,  //Update status, ID
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x321,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //Cellvoltages
  CAN_frame GROWATT_315 = {.FD = false,  //Cells 1-4
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x315,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_316 = {.FD = false,  //Cells 5-8
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x316,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_317 = {.FD = false,  //Cells 9-12
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x317,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_318 = {.FD = false,  //Cells 13-16
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x318,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  static const int VOLTAGE_OFFSET_DV = 40;  //Offset in deciVolt from max charge voltage and min discharge voltage
  static const int MAX_VOLTAGE_DV = 630;
  static const int MIN_VOLTAGE_DV = 410;

  uint16_t cell_delta_mV = 0;
  uint16_t ampere_hours_remaining = 0;
  uint16_t ampere_hours_full = 0;
};

#endif
