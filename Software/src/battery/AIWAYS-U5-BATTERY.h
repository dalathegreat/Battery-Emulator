#ifndef AIWAYS_U5_BATTERY_H
#define AIWAYS_U5_BATTERY_H
#include "../datalayer/datalayer_extended.h"
#include "../devboard/hal/hal.h"
#include "UdsCanBattery.h"

class AiwaysU5Battery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  AiwaysU5Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : UdsCanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    dtc = &datalayer_battery->dtc;
  }

  // Use the default constructor to create the first or single battery.
  AiwaysU5Battery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Aiways U5 Battery";

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "aiways_u5_dtc.json"; }

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 4170;
  static const int MIN_PACK_VOLTAGE_DV = 2680;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2800;

  unsigned long previousMillis10 = 0;  // will store last time a 10ms CAN Message was sent

  // UDS PIDs
  //The BMS (0x7E1) carries a live measurement block around DIDs 0x8800–0x887F — roughly 120 aggregate battery values
  static const uint16_t PID_PACK_VOLTAGE = 0x8871;
  static const uint16_t PID_PACK_CURRENT = 0x887B;
  static const uint16_t PID_CELL_VOLTAGE = 0x882D;

  uint16_t pid_pack_voltage = 0;
  int16_t pid_pack_current = 0;
  uint16_t pid_cellvoltage = 0;
};
#endif
