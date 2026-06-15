#ifndef _MASTER_CAN_H_
#define _MASTER_CAN_H_

#include "../../battery/Battery.h"
#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "CanReceiver.h"
#include "INTER-UNIT-MASTER-HTML.h"
#include "INTER-UNIT-PROTOCOL.h"

/**
 * InterUnitMasterBattery — virtual Battery that makes safety.cpp treat the
 * master node as a real battery.  update_values() only keeps CAN_battery_still_alive
 * fresh; the actual data is already written into datalayer.battery by
 * MasterCan::update_slave_aggregation() which runs just before this.
 */
class InterUnitMasterBattery : public Battery {
 public:
  static constexpr const char* Name = "Inter-Unit Master";

  void setup() override;
  void update_values() override;

  const char* interface_name() override { return "Inter-Unit CAN"; }

  BatteryHtmlRenderer& get_status_renderer() override { return _renderer; }

 private:
  InterUnitMasterHtmlRenderer _renderer;
};

/**
 * MasterCan — Inter-Unit protocol handler for MASTER nodes.
 *
 * Registers on can_config.battery (configured via BATTCOMM setting).
 *
 * Every second:
 *   1. Sends heartbeat broadcast (0x200)
 *   2. Sends per-slave contactor commands (0x201..0x208)
 *
 * On receiving slave STATUS/POWER/INFO frames:
 *   - Updates datalayer.system.slave_nodes[node_id-1]
 *   - Manages still_alive countdown per slave
 *
 * Every second (update_values):
 *   - Decrements still_alive counters
 *   - Checks spændings-matchning (voltage difference) like check_interconnect_available()
 *   - Aggregates all online slaves into datalayer.battery
 */
class MasterCan : public CanReceiver, public Transmitter {
 public:
  void begin();
  void receive_can_frame(CAN_frame* rx_frame) override;
  void transmit(unsigned long currentMillis) override;

  /** Must be called every 1 second to update aggregation and slave health */
  void update_values();

 private:
  void send_heartbeat();
  void send_contactor_commands(unsigned long currentMillis);
  void update_slave_aggregation();
  void check_slave_voltage_safety(uint8_t slave_idx);
  /** True only when the slave has reported IDENT and its FW + battery type match.
   *  An unverified or mismatched slave must never prejoin or close its contactor. */
  bool node_identity_ok(uint8_t slave_idx) const;

  unsigned long _last_update_values_ms = 0;  // Throttle per-second logic to once/sec (see update_values)
  unsigned long _last_heartbeat_ms = 0;
  unsigned long _next_contactor_tx_ms = 0;
  uint8_t _next_contactor_idx = 0;
  bool _contactor_burst_pending = false;
  unsigned long _startup_begin_ms = 0;  // Set when first slave comes online
  bool _startup_grace_done = false;     // True after IU_STARTUP_GRACE_S seconds
  bool _estop_was_active = false;       // Tracks e-stop transitions to force re-qualification
};

extern MasterCan master_can;

#endif  // _MASTER_CAN_H_
