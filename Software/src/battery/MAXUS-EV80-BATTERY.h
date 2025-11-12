#ifndef MAXUS_EV80_BATTERY_H
#define MAXUS_EV80_BATTERY_H
#include "CanBattery.h"

class MaxusEV80Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Maxus EV80 battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;
  static const int MIN_PACK_VOLTAGE_DV = 3100;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 3650;  //Charging stops if one cell exceeds this value
  static const int MIN_CELL_VOLTAGE_MV = 2800;  //Discharging stops if one cell goes below this value

  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send

  uint32_t BMS_SOC = 0;
  uint16_t incoming_poll = 0;
  uint16_t poll_state = POLL_BMS_STATE;

  CAN_frame MAXUS_POLL = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x748,
                          .data = {0x03, 0x22, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};
  static constexpr CAN_frame MAXUS_ACK = {.FD = false,  //Ack frame
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x748,
                                          .data = {0x30, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};

  static const int POLL_BMS_STATE = 0xe037;       // bms charger state?
  static const int POLL_CELLVOLTAGE = 0xe113;     // cell volts
  static const int POLL_CELLTEMP = 0xe114;        // cell temperature
  static const int POLL_SOH = 0xe053;             // bms SOH??
  static const int POLL_SOC = 0xe095;             // bms SOC
  static const int POLL_SOC_RAW = 0xe096;         // bms SOC raw
  static const int POLL_CURRENT_SENSOR = 0xe131;  // bms packamps? hv amps to pack (not sure)
  static const int POLL_CCS = 0xe088;             // bms CCS state / ccs amps
  static const int POLL_SAC = 0xe089;             // bms SAC amps?
  static const int POLL_UNKNOWN1 = 0xe090;        // bms CCS ChargeOn? (not sure)
  static const int POLL_UNKNOWN2 = 0xe091;        // bms SAC ChargeOn? (not sure)
};

#endif
