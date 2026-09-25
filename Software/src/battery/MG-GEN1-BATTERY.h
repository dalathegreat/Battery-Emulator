#pragma once

#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class MgGen1Battery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  MgGen1Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan, bool* allowed_contactor_closing_ptr);
  // Use the default constructor to create the first or single battery.
  MgGen1Battery();

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  virtual void got_battery_type(uint32_t type);
  virtual void reset_BMS() override;
  virtual bool supports_reset_BMS() override;
  virtual void on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) override;

  static constexpr const char* Name = "MG Gen1 (HS/ZS/MG5/MarvelR)";

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "mg_dtc.json"; }

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  void announce_contactor_state(bool state);

  static const uint16_t MAX_CELL_VOLTAGE_NMC_MV =
      4250;  // Trip: battery is put into emergency stop if one cell goes over this value
  static const uint16_t MIN_CELL_VOLTAGE_NMC_MV =
      2650;  // Trip: battery is put into emergency stop if one cell goes below this value
  static const uint16_t MAX_CELL_VOLTAGE_LFP_MV = 3650;
  static const uint16_t MIN_CELL_VOLTAGE_LFP_MV = 2650;

  // UDS PIDs
  static const uint16_t POLL_BATTERY_SOH = 0xB061;
  static const uint16_t POLL_BATTERY_NSTC_SERIAL = 0xB18C;
  static const uint16_t POLL_BATTERY_ECU_HW_NUMBER = 0xF18A;
  static const uint16_t POLL_BATTERY_VIN = 0xF190;
  static const uint16_t POLL_BATTERY_MFR_DATE = 0xF18B;
  static const uint16_t POLL_BATTERY_FINGERPRINT = 0xF183;
  static const uint16_t POLL_BATTERY_VEHICLE_HW_NUMBER = 0xF191;
  static const uint16_t POLL_BATTERY_SYSTEM_HW_NUMBER = 0xF192;
  static const uint16_t POLL_BATTERY_SYSTEM_SW_NUMBER = 0xF194;

  // PIDs read at boot time only (battery identifiers)
  static constexpr uint16_t UDS_BOOT_PID_LIST[] = {POLL_BATTERY_NSTC_SERIAL,
                                                   POLL_BATTERY_ECU_HW_NUMBER,
                                                   0xF120,
                                                   POLL_BATTERY_VEHICLE_HW_NUMBER,
                                                   POLL_BATTERY_FINGERPRINT,
                                                   POLL_BATTERY_MFR_DATE,
                                                   POLL_BATTERY_VIN,
                                                   POLL_BATTERY_SYSTEM_HW_NUMBER,
                                                   POLL_BATTERY_SYSTEM_SW_NUMBER,
                                                   0xF1A2,
                                                   0xF1AA};
  // PIDs read regularly
  static constexpr uint16_t UDS_STEADY_PID_LIST[] = {POLL_BATTERY_SOH};

  static const uint32_t BATTERY_TYPE_MG_HS_PHEV = 0x45443330;    // "ED30"
  static const uint32_t BATTERY_TYPE_MG_ZS_44_NMC = 0x454c3130;  // "EL10"
  static const uint32_t BATTERY_TYPE_MG5_50_LFP = 0x424b3430;    // "BK40"
  static const uint32_t BATTERY_TYPE_MG5_52_NMC = 0x45443930;    // "ED90"
  static const uint32_t BATTERY_TYPE_MG5_61_NMC = 0x45503243;    // "EP2C"
  static const uint32_t BATTERY_TYPE_MG5_69_NMC = 0x454e3243;    // "EN2C"

  uint32_t batteryType = 0;

  // Identifier PID payloads
  uint8_t pid_ecu_hw_number[8] = {0};
  uint8_t pid_f120[16] = {0};
  uint8_t pid_nstc_serial[24] = {0};
  uint8_t pid_fingerprint[10] = {0};
  uint8_t pid_mfr_date[3] = {0};
  uint8_t pid_vin[17] = {0};
  uint8_t pid_vehicle_hw_number[5] = {0};
  uint8_t pid_system_hw_number[10] = {0};
  uint8_t pid_system_sw_number[10] = {0};
  uint8_t pid_f1a2[8] = {0};
  uint8_t pid_f1aa[5] = {0};

  // Contactor control. allowed_contactor_closing is an input from the
  // multi-battery controller; null means we're the primary battery, so we
  // always can close, otherwise we wait for this to be true.
  bool* allowed_contactor_closing = nullptr;
  bool announcedContactorsClosed = false;
  bool contactorCloseReset = false;
  uint8_t eightAcycle = 0;
  uint16_t warmupCounter = 0;

  unsigned long previousMillis10 = 0;
  unsigned long previousMillis20 = 0;

  // Charge/discharge power limits and derating thresholds.
  uint16_t maxChargePowerW = 11000;
  uint16_t maxDischargePowerW = 11000;
  // Latched flags for hysteresis
  bool voltageAtCellMin = false;
  bool voltageAtCellMax = false;

  uint16_t soc = 5000;

  // Diagnostics/status tracking.
  uint32_t tx_count = 0;
  uint32_t rx_count = 0;
  // Positive as long as the cell voltage min/max values are fresh. If they
  // become stale we set max power to zero and open the contactors.
  uint16_t cellVoltageValidTime = 0;
  // Positive if the pack voltage is fresh. Important for the parallel-pack
  // voltage divergence checks.
  uint16_t voltageValidTime = 0;
  int limit_message_counter = 0;
  uint8_t previousState = 0;

  // UDS sequence states for this battery.
  enum MgUdsState : uint16_t {
    // UDS reset sequence
    MG_STATE_RESET_START = 0x01,
    MG_STATE_RESET_DIAG,  // 0x10 0x03 (extended session)
    MG_STATE_RESET_SEND,  // 0x11 0x01 (ECU reset)
  };
  // CAN reception goes to pieces during OTA, so have long enough timeouts to
  // allow for that.
  static const uint8_t CELL_VOLTAGE_TIMEOUT = 25;  // in seconds
  static const uint8_t VOLTAGE_TIMEOUT = 25;       // in seconds

  // Transmit frames.
  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0x36, 0xB0}};
};

inline MgGen1Battery::MgGen1Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan,
                                    bool* allowed_contactor_closing_ptr)
    : UdsCanBattery(targetCan) {
  datalayer_battery = datalayer_ptr;
  allowed_contactor_closing = allowed_contactor_closing_ptr;
}

inline MgGen1Battery::MgGen1Battery() : UdsCanBattery() {
  datalayer_battery = &datalayer.battery;
}
