#ifndef GROWATT_HV_CAN_H
#define GROWATT_HV_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef GROWATT_HV_CAN
#define SELECTED_INVERTER_CLASS GrowattHvInverter
#endif

class GrowattHvInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "Growatt High Voltage protocol via CAN";

 private:
  //Total number of Cells (1-512)
  //(Total number of Cells = number of Packs in parallel * number of Modules in series * number of Cells in the module)
  static const int TOTAL_NUMBER_OF_CELLS = 300;
  // Number of Modules in series (1-32)
  static const int NUMBER_OF_MODULES_IN_SERIES = 20;
  // Number of packs in parallel (1-65536)
  static const int NUMBER_OF_PACKS_IN_PARALLEL = 1;
  //Manufacturer abbreviation, part 1
  static const int MANUFACTURER_ASCII_0 = 0x47;  //G
  static const int MANUFACTURER_ASCII_1 = 0x54;  //T

  /* Do not change code below unless you are sure what you are doing */

  //Actual content messages
  CAN_frame GROWATT_3110 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3110,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3120 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3120,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3130 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3130,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3140 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3140,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3150 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3150,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3160 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3160,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3170 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3170,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3180 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3180,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3190 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3190,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3200 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3200,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3210 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3210,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3220 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3220,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3230 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3230,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3240 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3240,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3250 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3250,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3260 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3260,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3270 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3270,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3280 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3280,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3290 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3290,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GROWATT_3F00 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x3F00,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  unsigned long previousMillis1s = 0;  // will store last time a 1s CAN Message was send
  unsigned long previousMillisBatchSend = 0;
  uint32_t unix_time = 0;
  uint16_t ampere_hours_remaining = 0;
  uint16_t ampere_hours_full = 0;
  uint16_t send_times = 0;  // Overflows every 18hours. Cumulative number, plus 1 for each transmission
  uint8_t safety_specification = 0;
  uint8_t charging_command = 0;
  uint8_t discharging_command = 0;
  uint8_t shielding_external_communication_failure = 0;
  uint8_t clearing_battery_fault =
      0;  //When PCS receives the forced charge Mark 1 and Cell under- voltage protection fault, it will send 0XAA
  uint8_t ISO_detection_command = 0;
  uint8_t sleep_wakeup_control = 0;
  uint8_t PCS_working_status = 0;     //00 standby, 01 operating
  uint8_t serial_number_counter = 0;  //0-1-2-0-1-2...
  uint8_t can_message_batch_index = 0;
  const uint8_t delay_between_batches_ms = 10;
  bool inverter_alive = false;
  bool time_to_send_1s_data = false;
};

#endif
