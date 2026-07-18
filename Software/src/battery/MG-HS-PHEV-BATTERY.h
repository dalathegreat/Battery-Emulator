#pragma once

#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class MgGen1HtmlRenderer;

class MgHsPHEVBattery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  MgHsPHEVBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan, bool* allowed_contactor_closing_ptr);
  // Use the default constructor to create the first or single battery.
  MgHsPHEVBattery();

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual uint32_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length, UdsStatus status);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  virtual void got_battery_type(uint32_t type);

  static constexpr const char* Name = "MG Gen1 (HS/ZS/MG5/MarvelR)";

  BatteryHtmlRenderer& get_status_renderer();
  inline const uint8_t* get_pid_f18a() { return (const uint8_t*)pid_f18a; }
  inline const uint8_t* get_pid_f120() { return (const uint8_t*)pid_f120; }
  inline const uint8_t* get_pid_b18c() { return (const uint8_t*)pid_b18c; }
  inline const uint8_t* get_pid_f183() { return (const uint8_t*)pid_f183; }
  inline const uint8_t* get_pid_f18b() { return (const uint8_t*)pid_f18b; }
  inline const uint8_t* get_pid_f190() { return (const uint8_t*)pid_f190; }
  inline const uint8_t* get_pid_f191() { return (const uint8_t*)pid_f191; }
  inline const uint8_t* get_pid_f192() { return (const uint8_t*)pid_f192; }
  inline const uint8_t* get_pid_f194() { return (const uint8_t*)pid_f194; }
  inline const uint8_t* get_pid_f1a2() { return (const uint8_t*)pid_f1a2; }
  inline const uint8_t* get_pid_f1aa() { return (const uint8_t*)pid_f1aa; }
  inline const uint8_t* get_dtcs() { return (const uint8_t*)pid_f1aa; }
  inline int get_uds_address() { return uds_address; }

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  MgGen1HtmlRenderer* renderer;
  uint8_t pid_f18a[8] = {0};
  uint8_t pid_f120[16] = {0};
  uint8_t pid_b18c[24] = {0};
  uint8_t pid_f183[10] = {0};
  uint8_t pid_f18b[3] = {0};
  uint8_t pid_f190[17] = {0};
  uint8_t pid_f191[5] = {0};
  uint8_t pid_f192[10] = {0};
  uint8_t pid_f194[10] = {0};
  uint8_t pid_f1a2[8] = {0};
  uint8_t pid_f1aa[5] = {0};

  void update_soc(uint16_t soc_times_ten);
  void announce_contactor_state(bool state);

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
  static const uint16_t POLL_BATTERY_TYPE = 0xF18A;

  static const uint32_t BATTERY_TYPE_MG_HS_PHEV = 0x53534541;  // "SSEA"
  static const uint32_t BATTERY_TYPE_MG_ZS = 0x5a533131;       // "ZS11"
  //static const uint32_t BATTERY_TYPE_MG5_61_NMC = 0x00010203;  // (EU174A61S/EU199A70S)
  static const uint32_t BATTERY_TYPE_MG5 = 0x00010203;
  // MG5/MarvelR currently unknown!

  uint32_t batteryType = 0;
  //uint32_t pidCount = 0;
  bool contactorCloseReset = false;
  uint8_t eightAcycle = 0;
  uint16_t warmupCounter = 0;
  // An input indicating whether we are allowed to close contactors (from the
  // multi-battery controller). Null means we're the primary battery, so we always can.
  bool* allowed_contactor_closing = nullptr;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis20 = 0;   // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  //unsigned long previousMillis2000 = 0;

  bool announcedContactorsClosed = false;

  uint16_t maxChargePowerW = 11000;
  static const uint16_t CHARGE_TRICKLE_POWER_W = 0;
  static const uint16_t DERATE_CHARGE_ABOVE_SOC = 9500;  // in 0.01% units

  uint16_t maxDischargePowerW = 11000;
  static const uint16_t DERATE_DISCHARGE_BELOW_SOC = 1000;  // in 0.01% units
  static const uint16_t DISCHARGE_MIN_SOC = 500;

  // Positive if the max/min cell voltages were recently updated. If they become stale
  // we set max power to zero.
  uint16_t cellVoltageValidTime = 0;
  // Positive if the pack voltage was recently updated. If stale we open
  // contactors (to avoid an undetected double-battery discrepancy).
  uint16_t voltageValidTime = 0;

  uint16_t highestSeenCellCount = 0;

  uint8_t previousState = 0;
  enum MG_HS_RESET_STATE { IDLE, SENDING_DIAG, SENDING_RESET, WAITING_RESET_COMPLETE };
  MG_HS_RESET_STATE resetProgress = IDLE;
  uint8_t resetTimeout = 0;
  static const uint8_t CELL_VOLTAGE_TIMEOUT = 10;  // in seconds
  static const uint8_t VOLTAGE_TIMEOUT = 10;       // in seconds

  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0x36, 0xB0}};

  // CAN_frame MG_391 = {.FD = false,
  //                     .ext_ID = false,
  //                     .DLC = 8,
  //                     .ID = 0x391,
  //                     .data = {0x10, 0x01, 0xA0, 0x00, 0xD0, 0x00, 0x48, 0x00}};

  // HS is happy with 0e + 7x 00
  // Trying different for MG5...
  static constexpr CAN_frame MG_HS_1F1 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x1F1,
                                          .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // Enter UDS extended-diagnostics mode
  // CAN_frame MG_HS_UDS_DIAG = {.FD = false,
  //                             .ext_ID = false,
  //                             .DLC = 8,
  //                             .ID = 0x7E5,
  //                             .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // // BMS hard reset
  // CAN_frame MG_HS_UDS_RESET = {.FD = false,
  //                              .ext_ID = false,
  //                              .DLC = 8,
  //                              .ID = 0x7E5,
  //                              .data = {0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#include "MG-GEN1-HTML.h"

inline MgHsPHEVBattery::MgHsPHEVBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan,
                                        bool* allowed_contactor_closing_ptr)
    : UdsCanBattery(targetCan) {
  datalayer_battery = datalayer_ptr;
  renderer = new MgGen1HtmlRenderer(*this);
  allowed_contactor_closing = allowed_contactor_closing_ptr;
}

inline MgHsPHEVBattery::MgHsPHEVBattery() : UdsCanBattery() {
  datalayer_battery = &datalayer.battery;
  renderer = new MgGen1HtmlRenderer(*this);
}

inline BatteryHtmlRenderer& MgHsPHEVBattery::get_status_renderer() {
  return *renderer;
}
