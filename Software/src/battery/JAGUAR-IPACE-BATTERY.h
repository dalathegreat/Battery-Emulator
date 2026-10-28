#ifndef JAGUAR_IPACE_BATTERY_H
#define JAGUAR_IPACE_BATTERY_H

#include "UdsCanBattery.h"

class JaguarIpaceBattery : public UdsCanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  JaguarIpaceBattery() : UdsCanBattery() { dtc = &datalayer_battery->dtc; }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Jaguar I-PACE";
  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "jaguar_ipace_dtc.json"; }

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  static const int MAX_PACK_VOLTAGE_DV = 4546;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3370;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  // UDS PIDs
  static const uint16_t POLL_BATTERY_VIN = 0xF190;  //Untested

  uint32_t battery_vin = 0;
};

#endif
