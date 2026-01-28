#ifndef MG_HS_PHEV_BATTERY_H
#define MG_HS_PHEV_BATTERY_H

#include "UdsCanBattery.h"

class MgHsPHEVBattery : public UdsCanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual uint16_t handle_pid(uint16_t pid, uint32_t value);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MG HS PHEV 16.6kWh battery";

 private:
  void update_soc(uint16_t soc_times_ten);

  static const uint16_t TOTAL_CAPACITY_WH = 16600;
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

  static const uint16_t MAX_CHARGE_POWER_W = 3000;
  static const uint16_t CHARGE_TRICKLE_POWER_W = 20;
  static const uint16_t DERATE_CHARGE_ABOVE_SOC = 9000;  // in 0.01% units

  static const uint16_t MAX_DISCHARGE_POWER_W = 4000;
  static const uint16_t DERATE_DISCHARGE_BELOW_SOC = 1500;  // in 0.01% units
  static const uint16_t DISCHARGE_MIN_SOC = 1000;

  uint16_t cellVoltageValidTime = 0;

  uint8_t previousState = 0;
  enum MG_HS_RESET_STATE { IDLE, SENDING_DIAG, SENDING_RESET, WAITING_RESET_COMPLETE };
  MG_HS_RESET_STATE resetProgress = IDLE;
  uint8_t resetTimeout = 0;
  static const uint8_t CELL_VOLTAGE_TIMEOUT = 10;  // in seconds

  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0x36, 0xB0}};

  static constexpr CAN_frame MG_HS_1F1 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x1F1,
                                          .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // Enter UDS extended-diagnostics mode
  static constexpr CAN_frame MG_HS_7E5_DIAG = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7E5,
                                               .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // BMS hard reset
  static constexpr CAN_frame MG_HS_7E5_RESET = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 8,
                                                .ID = 0x7E5,
                                                .data = {0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
