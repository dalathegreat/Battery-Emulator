#ifndef FERROAMP_CAN_H
#define FERROAMP_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED
#define SELECTED_INVERTER_CLASS FerroampCanInverter

class FerroampCanInverter : public CanInverterProtocol {
 public:
  void setup();

  void update_values();

  void transmit_can(unsigned long currentMillis);

  void map_can_frame_to_variable(CAN_frame rx_frame);

 private:
  void send_system_data();
  void send_setup_info();

//#define SEND_0  //If defined, the messages will have ID ending with 0 (useful for some inverters)
#define SEND_1                 //If defined, the messages will have ID ending with 1 (useful for some inverters)
#define INVERT_LOW_HIGH_BYTES  //If defined, certain frames will have inverted low/high bytes \
                               //useful for some inverters like Sofar that report the voltages incorrect otherwise
#define SET_30K_OFFSET         //If defined, current values are sent with a 30k offest (useful for ferroamp)

/* Some inverters need to see a specific amount of cells/modules to emulate a specific Pylon battery.
Change the following only if your inverter is generating fault codes about voltage range */
#define TOTAL_CELL_AMOUNT 120  //Adjust this parameter in steps of 120 to add another 14,2kWh of capacity
#define MODULES_IN_SERIES 4
#define CELLS_PER_MODULE 30
#define VOLTAGE_LEVEL 384
#define AH_CAPACITY 37

  //Actual content messages
  CAN_frame PYLON_7310 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7310,
                          .data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
  CAN_frame PYLON_7311 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7311,
                          .data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
  CAN_frame PYLON_7320 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7320,
                          .data = {TOTAL_CELL_AMOUNT, (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES,
                                   CELLS_PER_MODULE, (uint8_t)(VOLTAGE_LEVEL & 0x00FF), (uint8_t)(VOLTAGE_LEVEL >> 8),
                                   AH_CAPACITY, (uint8_t)(AH_CAPACITY >> 8)}};
  CAN_frame PYLON_7321 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7321,
                          .data = {TOTAL_CELL_AMOUNT, (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES,
                                   CELLS_PER_MODULE, (uint8_t)(VOLTAGE_LEVEL & 0x00FF), (uint8_t)(VOLTAGE_LEVEL >> 8),
                                   AH_CAPACITY, (uint8_t)(AH_CAPACITY >> 8)}};
  CAN_frame PYLON_4210 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4210,
                          .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
  CAN_frame PYLON_4220 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4220,
                          .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
  CAN_frame PYLON_4230 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4230,
                          .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
  CAN_frame PYLON_4240 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4240,
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
  CAN_frame PYLON_4250 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4250,
                          .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4260 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4260,
                          .data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
  CAN_frame PYLON_4270 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4270,
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
  CAN_frame PYLON_4280 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4280,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4290 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4290,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4211 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4211,
                          .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
  CAN_frame PYLON_4221 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4221,
                          .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
  CAN_frame PYLON_4231 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4231,
                          .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
  CAN_frame PYLON_4241 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4241,
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
  CAN_frame PYLON_4251 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4251,
                          .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4261 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4261,
                          .data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
  CAN_frame PYLON_4271 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4271,
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
  CAN_frame PYLON_4281 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4281,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4291 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4291,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  uint16_t cell_tweaked_max_voltage_mV = 3300;
  uint16_t cell_tweaked_min_voltage_mV = 3300;
};

#endif
