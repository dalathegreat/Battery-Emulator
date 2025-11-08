#ifndef MG_HS_PHEV_BATTERY_H
#define MG_HS_PHEV_BATTERY_H

#include "CanBattery.h"

class MgHsPHEVBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MG HS PHEV 16.6kWh battery";

 private:
  void update_soc(uint16_t soc_times_ten);

  static const uint16_t MAX_PACK_VOLTAGE_DV = 3780;  //5000 = 500.0V
  static const uint16_t MIN_PACK_VOLTAGE_DV = 2790;
  static const uint16_t MAX_CELL_DEVIATION_MV = 150;
  static const uint16_t MAX_CELL_VOLTAGE_MV =
      4250;  //Battery is put into emergency stop if one cell goes over this value
  static const uint16_t MIN_CELL_VOLTAGE_MV =
      2610;  //Battery is put into emergency stop if one cell goes below this value

  static const uint16_t POLL_BATTERY_VOLTAGE = 0xB042;
  static const uint16_t POLL_BATTERY_CURRENT = 0xB043;
  static const uint16_t POLL_BATTERY_SOC = 0xB046;
  static const uint16_t POLL_ERROR_CODE = 0xB047;
  static const uint16_t POLL_BMS_STATUS = 0xB048;
  static const uint16_t POLL_MAIN_RELAY_B_STATUS = 0xB049;
  static const uint16_t POLL_MAIN_RELAY_G_STATUS = 0xB04A;
  static const uint16_t POLL_MAIN_RELAY_P_STATUS = 0xB052;
  static const uint16_t POLL_MAX_CELL_TEMPERATURE = 0xB056;
  static const uint16_t POLL_MIN_CELL_TEMPERATURE = 0xB057;
  static const uint16_t POLL_MAX_CELL_VOLTAGE = 0xB058;
  static const uint16_t POLL_MIN_CELL_VOLTAGE = 0xB059;
  static const uint16_t POLL_BATTERY_SOH = 0xB061;

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

  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0x36, 0xB0}};
  CAN_frame MG_HS_1F1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1F1,
                         .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MG_HS_7E5_POLL = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E5,
                              .data = {0x03, 0x22, 0xB0, 0x42, 0x00, 0x00, 0x00, 0x00}};
};

#endif
