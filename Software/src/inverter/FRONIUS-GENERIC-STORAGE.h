#ifndef FRONIUS_GENERIC_STORAGE_H
#define FRONIUS_GENERIC_STORAGE_H

#include "../devboard/utils/events.h"
#include "BYD-MODBUS.h"

/* Fronius "Generic Storage" over Modbus RTU, register map version 2.

   The same protocol family as the BYD HVS/HVM driver this class builds on, which speaks version 1
   and has to pass for a BYD Battery-Box. From version 2 on the battery is commissioned in the GEN24
   as "Compatible Battery" and may identify as itself: own manufacturer, model and display name,
   32 bit capacity and power fields, a nameplate carrying the cell temperature range, and the
   inverter's name and firmware version written back to us.

   The inverter also tells the battery which operating mode it wants (opMode), which this driver
   uses to open the contactors while the inverter has no use for the battery. A GEN24 brings a
   battery up through INACTIVE (DC connected, no power) to ACTIVE, and parks it in STANDBY.

   Map: generic_storage_protocol_v2.xml. All 32 and 64 bit values are sent high word first. */
class FroniusGenericStorageInverter : public BydModbusInverter {
 public:
  FroniusGenericStorageInverter() : BydModbusInverter(REG_CONTROL) {}
  const char* name() override { return Name; }
  void update_values() override;
  bool controls_contactor() override { return true; }
  bool allows_contactor_closing() override { return contactor_closing_allowed; }
  static constexpr const char* Name = "Fronius Generic Storage";

  // Operating modes, shared by MonitoringData.state (what the battery is in) and ControlData.opMode
  // (what the inverter asks for)
  static constexpr uint16_t MODE_STANDBY = 0;
  static constexpr uint16_t MODE_INACTIVE = 1;
  static constexpr uint16_t MODE_DARKSTART = 2;
  static constexpr uint16_t MODE_ACTIVE = 3;
  static constexpr uint16_t MODE_FAULT = 4;
  static constexpr uint16_t MODE_UPDATING = 5;
  // What a Fronius battery puts back into opMode once it has taken a request over. Used here as the
  // start value, so "nothing asked yet" can be told from a request for STANDBY (0)
  static constexpr uint16_t MODE_NO_REQUEST = 0x00FF;

  // For the settings page. Empty strings until the inverter has written its details
  const char* inverter_name() const { return inverter_name_text; }
  const char* inverter_firmware() const { return inverter_firmware_text; }
  // The last opMode the inverter wrote, MODE_NO_REQUEST before the first one
  uint16_t requested_mode() const { return requested_op_mode; }
  static const char* mode_name(uint16_t mode);

 protected:
  void handle_static_data() override;

  // First register of each block of the version 2 map
  static constexpr uint16_t REG_IDENTIFICATION = 100;     // DeviceIdentificationData, 122 registers
  static constexpr uint16_t REG_NAMEPLATE = 300;          // NameplateData, 35 registers
  static constexpr uint16_t REG_MONITORING = 400;         // MonitoringData, 30 registers
  static constexpr uint16_t REG_CONTROL = 500;            // ControlData, 10 registers, written by the inverter
  static constexpr uint16_t REG_FAULTS = 1000;            // FaultBuffers, 10 entries of 10 registers
  static constexpr uint16_t REG_UPDATE = 2000;            // UpdateType and UpdateTypeVendorChannel, 19 registers
  static constexpr uint16_t REG_INVERTER_DETAILS = 3000;  // InverterDetails, 42 registers, written by the inverter

  static constexpr uint8_t FAULT_ENTRIES = 10;
  static constexpr uint8_t FAULT_ENTRY_SIZE = 10;

 private:
  void put_u32(uint16_t reg, uint32_t value);
  void put_u64(uint16_t reg, uint64_t value);
  // Two characters per register, high byte first, zero padded
  void put_string(uint16_t reg, uint8_t registers, const char* text);
  // out has to hold registers * 2 + 1 characters. Anything but plain text comes out as '?'
  void get_string(uint16_t reg, uint8_t registers, char* out);

  void update_nameplate();
  void update_monitoring();
  // Follows opMode and decides whether the contactors may be closed
  void update_contactor_request();
  // Picks up the name and firmware version the inverter writes to InverterDetails
  void handle_inverter_details();
  // Turns emulator error events into FaultBuffers entries, which the inverter shows in its event log
  void update_fault_buffers();
  void push_fault(uint16_t type, uint8_t battery, int16_t data, uint64_t utc_s);
  // The mode reported in MonitoringData.state
  uint16_t reported_state();
  // The contactors are closed, as far as the emulator can tell
  bool bus_connected();

  // NameplateData.BatteryCapabilityFlags
  static constexpr uint32_t CAPABILITY_ISOLATION_CHECK = 0x04;  // Inverter includes the battery in its insulation test
  // NameplateData.CalMode
  static constexpr uint32_t CAL_MODE_NONE = 0x01;  // No calibration charge needed
  // MonitoringData.flags
  static constexpr uint32_t FLAG_CHARGING = 0x01;
  static constexpr uint32_t FLAG_DISCHARGING = 0x02;
  static constexpr uint32_t FLAG_FULL = 0x04;
  static constexpr uint32_t FLAG_LINK_OK = 0x80;  // Left clear for 5 polls, the inverter reports a broken cable
  // MonitoringData.TAmbient when there is no such sensor
  static constexpr uint16_t TEMPERATURE_NOT_AVAILABLE = 0x8000;

  // With no request at all for this long the inverter is taken to be off, and its last opMode void
  static constexpr uint32_t INVERTER_SILENCE_MS = 60000;
  // Once the inverter stops asking for the battery, the contactors stay closed until the current has
  // gone, but never longer than this
  static constexpr uint32_t OPEN_WAIT_MS = 10000;
  static constexpr int16_t OPEN_CURRENT_LIMIT_dA = 20;

  uint16_t requested_op_mode = MODE_NO_REQUEST;
  uint16_t reached_state = MODE_STANDBY;
  uint16_t last_inverter_rebooting = 0;
  bool contactor_closing_allowed = false;
  bool contactor_lock = false;  // "Lock contactors closed after first close request" has seen that request
  bool open_pending = false;
  uint32_t open_pending_since_ms = 0;

  char inverter_name_text[65] = {0};
  char inverter_firmware_text[21] = {0};

  // Fault entries need a wall clock time, which only the inverter has
  uint64_t utc_reference_s = 0;
  uint32_t utc_reference_ms = 0;
  uint16_t fault_count = 0;

  struct FaultMapping {
    EVENTS_ENUM_TYPE event;  // The pack 1 variant
    uint8_t variants;        // 3 when the event exists per pack, in 1, 2, 3 order
    uint8_t code;            // Fault type, looked up by the inverter under MSG_PREFIX
  };
  static const FaultMapping FAULT_MAP[];
  static const uint8_t FAULT_MAP_SIZE;
  // One bit per pack variant, set once the fault has been handed to the inverter
  uint8_t fault_reported[24] = {0};

  // Selects the table the inverter takes its event texts from. Has to be one it knows, or the
  // event log shows nothing but the number. "BYD2" is the one of the BYD Battery-Box Premium HVS/HVM
  static constexpr const char* MSG_PREFIX = "BYD2";
};

#endif
