#ifndef OMODA_E5_BATTERY_H
#define OMODA_E5_BATTERY_H

#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class OmodaE5Battery : public UdsCanBattery {
 public:
  OmodaE5Battery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "OMODA E5";

  String get_uds_info_html() override;

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 4662;
  static const int MIN_PACK_VOLTAGE_DV = 3150;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 3700;  // Emergency stop if above
  static const int MIN_CELL_VOLTAGE_MV = 2500;  // Emergency stop if below

  // Standard UDS DIDs (ISO 14229-1), supported by most ECUs
  static const int PID_POLL_VIN = 0xF190;
  static const int PID_POLL_ECU_SOFTWARE_NUMBER = 0xF187;
  static const int PID_POLL_ECU_HARDWARE_NUMBER = 0xF18A;

  // Identifier PID payloads
  uint8_t pid_vin[17] = {0};
  uint8_t pid_ecu_software_number[10] = {0};
  uint8_t pid_ecu_hardware_number[10] = {0};
};

#endif
