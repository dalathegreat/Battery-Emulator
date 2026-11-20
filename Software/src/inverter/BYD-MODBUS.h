#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H

#include "../devboard/utils/events.h"
#include "../devboard/utils/types.h"
#include "ModbusInverterProtocol.h"

class BydModbusInverter : public ModbusInverterProtocol {
 public:
  /** The Fronius Gen24 stops charge and discharge below -10 degrees C, because the pack it was
   * designed for is LFP. Caps a reading between -9.0 and -20.0 C to -9.0 C, and leaves anything
   * else alone: colder than -20 C is a fault, not weather. Pure, so it can be tested directly */
  static int16_t clamp_cold_temperature(int16_t temperature_dC);

  BydModbusInverter() : ModbusInverterProtocol(21) {}
  const char* name() override { return Name; }
  bool setup() override;
  void update_values();
  static constexpr const char* Name = "BYD HVS/HVM over Modbus";

 protected:
  // Fills the registers that never change, and creates the FaultBuffers. Called once from setup()
  void handle_static_data();

 private:
  void verify_temperature();
  // What this inverter reports, after the cold-weather cap. Kept here rather than written back
  // into the datalayer, which every other consumer reads.
  int16_t reported_temperature_min_dC = 0;
  int16_t reported_temperature_max_dC = 0;
  void verify_inverter_modbus();
  // Consumes the ControlData block the inverter writes: WatchDogTimeout (402), UTC (403-406)
  // and RebootCommand (407)
  void handle_inverter_control_data();
  void handle_update_data_modbusp201_byd();
  void handle_update_data_modbusp301_byd();
  int16_t byd_power_W();
  // Turns emulator error events into FaultBuffers entries, which the inverter shows in its event log
  void update_fault_buffers();
  void push_fault(uint16_t type, uint8_t battery, int16_t data, uint32_t now_ms);
  // Gives the entries written while the clock was not yet valid the time they were raised
  void stamp_faults(uint32_t now_ms);
  // A uint64 over four registers, high word first
  void put_u64(uint16_t reg, uint64_t value);

  // Register 303 status bits sent back to the inverter: bit 7 = normal operation,
  // bit 0 = charging, bit 1 = discharging
  static const uint16_t BYD_MODE_IDLE = 128;
  static const uint16_t BYD_MODE_CHARGING = 129;
  static const uint16_t BYD_MODE_DISCHARGING = 130;

  static const uint8_t HISTORY_LENGTH =
      5;  // Amount of samples(minutes) that needs to match for register to be considered stale
  unsigned long previousMillis60s = 0;  // will store last time a 60s event occured
  uint16_t register_401_history[5] = {0};
  uint8_t history_index = 0;
  uint16_t last_register_407 = 0;  // Last RebootCommand seen, so only changes are acted on
  uint16_t last_register_408 = 0;  // Last DarkstartEnable seen, so only changes are logged
  // Bounds for a WatchDogTimeout accepted from register 402. 0 means the inverter is not using the
  // field, and an implausibly large value would push inverter-missing detection out of usefulness.
  static const uint32_t WATCHDOG_TIMEOUT_MIN_S = 5;
  static const uint32_t WATCHDOG_TIMEOUT_MAX_S = 3600;
  uint16_t bms_char_dis_status = BYD_MODE_IDLE;
  bool all_401_values_equal = false;

  // FaultBuffers, 10 entries of 10 registers from 1000 on, which the inverter reads every 20 s
  static constexpr uint16_t REG_FAULTS = 1000;
  static constexpr uint8_t FAULT_ENTRIES = 10;
  static constexpr uint8_t FAULT_ENTRY_SIZE = 10;
  uint16_t fault_count = 0;
  /* Fault entries are stamped from the ESP32's clock, which the inverter's own clock sets (see
     system_clock.h). A fault raised before that clock is valid goes into the buffers with no
     timestamp, which the inverter takes as an empty entry. Once it is valid, the entry gets the time
     the fault was raised, worked back from the uptime noted here. Fronius' own Reserva battery
     re-stamps its stored faults the same way when the inverter writes UTC. */
  uint32_t fault_entry_ms[FAULT_ENTRIES] = {0};
  bool fault_entry_unstamped[FAULT_ENTRIES] = {false};

  struct FaultMapping {
    EVENTS_ENUM_TYPE event;  // The pack 1 variant
    uint8_t variants;        // 3 when the event exists per pack, in 1, 2, 3 order
    uint8_t code;            // Fault type, looked up by the inverter in its BYD2 table
  };
  static const FaultMapping FAULT_MAP[];
  static const uint8_t FAULT_MAP_SIZE;
  // One bit per pack variant, set once the fault has been handed to the inverter
  uint8_t fault_reported[24] = {0};
};

#endif
