#ifndef SANTA_FE_PHEV_BATTERY_H
#define SANTA_FE_PHEV_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"
#include "CanBattery.h"

#ifdef SANTA_FE_PHEV_BATTERY
#define SELECTED_BATTERY_CLASS SantaFePhevBattery
#endif

class SantaFePhevBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  SantaFePhevBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  SantaFePhevBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Santa Fe PHEV";

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2880;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
  uint8_t poll_data_pid = 0;
  uint8_t counter_200 = 0;
  uint8_t checksum_200 = 0;

  uint16_t SOC_Display = 0;
  uint16_t batterySOH = 100;
  uint16_t CellVoltMax_mV = 3700;
  uint16_t CellVoltMin_mV = 3700;
  uint8_t CellVmaxNo = 0;
  uint8_t CellVminNo = 0;
  uint16_t allowedDischargePower = 0;
  uint16_t allowedChargePower = 0;
  uint16_t batteryVoltage = 0;
  int16_t leadAcidBatteryVoltage = 120;
  int8_t temperatureMax = 0;
  int8_t temperatureMin = 0;
  int16_t batteryAmps = 0;
  uint8_t StatusBattery = 0;
  uint16_t cellvoltages_mv[96];

  CAN_frame SANTAFE_200 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x200,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};
  CAN_frame SANTAFE_2A1 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x2A1,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x02}};
  CAN_frame SANTAFE_2F0 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x2F0,
                           .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00}};
  CAN_frame SANTAFE_523 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x523,
                           .data = {0x60, 0x00, 0x60, 0, 0, 0, 0, 0}};
  CAN_frame SANTAFE_7E4_poll = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x7E4,  //Polling frame, 0x22 01 0X
                                .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SANTAFE_7E4_ack = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E4,  //Ack frame, correct PID is returned. Flow control message
                               .data = {0x30, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
