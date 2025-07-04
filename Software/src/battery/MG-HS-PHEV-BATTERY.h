#ifndef MG_HS_PHEV_BATTERY_H
#define MG_HS_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef MG_HS_PHEV_BATTERY
#define SELECTED_BATTERY_CLASS MgHsPHEVBattery
#endif

class MgHsPHEVBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MG HS PHEV 16.6kWh battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3100;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis70 = 0;   // will store last time a 70ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send

  int BMS_SOC = 0;
  // For calculating charge and discharge power
  float RealVoltage;
  float RealSoC;
  float tempfloat;

  uint8_t messageindex = 0;  //For polling switchcase

  const int MaxChargePower = 3000;  // Maximum allowable charge power, excluding the taper
  const int StartChargeTaper = 90;  // Battery percentage above which the charge power will taper to zero
  const float ChargeTaperExponent =
      1;  // Shape of charge power taper to zero. 1 is linear. >1 reduces quickly and is small at nearly full.
  const int TricklePower = 20;  // Minimimum trickle charge or discharge power (W)

  const int MaxDischargePower = 4000;  // Maximum allowable discharge power, excluding the taper
  const int MinSoC = 20;               // Minimum SoC allowed
  const int StartDischargeTaper = 30;  // Battery percentage below which the discharge power will taper to zero
  const float DischargeTaperExponent =
      1;  // Shape of discharge power taper to zero. 1 is linear. >1 reduces quickly and is small at nearly full.

  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x36, 0xB0}};
  CAN_frame MG_HS_1F1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1F1,
                         .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MG_HS_7E5_B0_42 = {.FD = false,  // Get Battery voltage
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x42, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_43 = {.FD = false,  // Get Battery current
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x43, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_46 = {.FD = false,  // Get Battery SoC
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x46, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_47 = {.FD = false,  // Get BMS error code
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x47, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_48 = {.FD = false,  // Get BMS status
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x48, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_49 = {.FD = false,  // Get System main relay B status
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x49, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_4A = {.FD = false,  // Get System main relay G status
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x4A, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_52 = {.FD = false,  // Get System main relay P status
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x52, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_56 = {.FD = false,  // Get Max cell temperature
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x56, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_57 = {.FD = false,  // Get Min call temperature
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x57, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_58 = {.FD = false,  // Get Max cell voltage
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x58, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_59 = {.FD = false,  // Get Min call voltage
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x59, 0x00, 0x00, 0x00, 0x00}};

  CAN_frame MG_HS_7E5_B0_61 = {.FD = false,  // Get Battery SoH
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x61, 0x00, 0x00, 0x00, 0x00}};
};

#endif
