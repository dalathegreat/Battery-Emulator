#ifndef MG_4_BATTERY_H
#define MG_4_BATTERY_H

#include "CanBattery.h"

class Mg4Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MG4 battery";

 private:
  static const uint16_t MAX_PACK_VOLTAGE_DV = 4370;  //5000 = 500.0V
  static const uint16_t MIN_PACK_VOLTAGE_DV = 3120;
  static const uint16_t MAX_CELL_DEVIATION_MV = 150;
  static const uint16_t MAX_CELL_VOLTAGE_MV =
      4250;  //Battery is put into emergency stop if one cell goes over this value
  static const uint16_t MIN_CELL_VOLTAGE_MV =
      2610;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send

  // For calculating charge and discharge power
  float RealVoltage;
  float RealSoC;
  float tempfloat;

  uint16_t cellVoltageValidTime = 0;
  uint16_t soc1 = 0;
  uint16_t soc2 = 0;
  uint16_t v = 0;

  uint8_t cell_id = 0;
  uint8_t transmitIndex = 0;  //For polling switchcase
  uint8_t previousState = 0;
  static const uint8_t CELL_VOLTAGE_TIMEOUT = 10;  // in seconds

  const uint16_t MaxChargePower = 3000;  // Maximum allowable charge power, excluding the taper
  const uint8_t StartChargeTaper = 90;   // Battery percentage above which the charge power will taper to zero
  const float ChargeTaperExponent =
      1;  // Shape of charge power taper to zero. 1 is linear. >1 reduces quickly and is small at nearly full.
  const uint8_t TricklePower = 20;  // Minimimum trickle charge or discharge power (W)

  const uint16_t MaxDischargePower = 4000;  // Maximum allowable discharge power, excluding the taper
  const uint8_t MinSoC = 20;                // Minimum SoC allowed
  const uint8_t StartDischargeTaper = 30;   // Battery percentage below which the discharge power will taper to zero
  const float DischargeTaperExponent =
      1;  // Shape of discharge power taper to zero. 1 is linear. >1 reduces quickly and is small at nearly full.

  CAN_frame MG4_4F3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4F3,
                       .data = {0xF3, 0x10, 0x48, 0x00, 0xFF, 0xFF, 0x00, 0x11}};
  CAN_frame MG4_047 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x047,
                       .data = {0xE9, 0x03, 0x45, 0x7D, 0x7F, 0xFF, 0xFF, 0xFE}};
};

#endif
