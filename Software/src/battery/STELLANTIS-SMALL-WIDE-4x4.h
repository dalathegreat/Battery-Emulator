#ifndef STELLANTIS_SMALLWIDE_4x4_BATTERY_H
#define STELLANTIS_SMALLWIDE_4x4_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class StellantisSmallWide4x4Battery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  StellantisSmallWide4x4Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  StellantisSmallWide4x4Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
  }

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { UserRequestDTCreset = true; }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis FCA Small Wide 4x4";

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  bool UserRequestDTCreset = false;

  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;  //TODO SET
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis20 = 0;  // will store last time a 20ms CAN Message was send
  uint8_t counter212 = 0xF;            //Counter for CAN message 0x212, goes from 0 to 15 and then resets to 0
  CAN_frame SMALLWIDE_212 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x212,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7E}};
  CAN_frame CLEAR_DTC = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x7E7,
                         .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};

  uint8_t TracBat_EChrgPowLong = 0;  // long/continuous charge power limit [kW]
  uint8_t HVBatDischrgPow30sec = 0;  // 30-second discharge power limit [kW]
};

#endif
