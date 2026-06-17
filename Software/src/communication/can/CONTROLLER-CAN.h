#ifndef _CONTROLLER_CAN_H_
#define _CONTROLLER_CAN_H_

#include "../../battery/Battery.h"
#include "../../communication/Transmitter.h"
#include "../../datalayer/datalayer.h"
#include "CanReceiver.h"
#include "INTER-UNIT-CONTROLLER-HTML.h"
#include "INTER-UNIT-PROTOCOL.h"

/**
 * InterUnitControllerBattery — virtual Battery that makes safety.cpp treat the
 * controller node as a real battery.  update_values() only keeps CAN_battery_still_alive
 * fresh; the actual data is already written into datalayer.battery by
 * ControllerCan::update_node_aggregation() which runs just before this.
 */
class InterUnitControllerBattery : public Battery {
 public:
  static constexpr const char* Name = "Inter-Unit Controller";

  void setup() override;
  void update_values() override;

  const char* interface_name() override { return "Inter-Unit CAN"; }

  BatteryHtmlRenderer& get_status_renderer() override { return _renderer; }

 private:
  InterUnitControllerHtmlRenderer _renderer;
};

/**
 * ControllerCan — Inter-Unit protocol handler for the CONTROLLER node.
 *
 * Registers on can_config.battery (configured via BATTCOMM setting).
 *
 * Every second:
 *   1. Sends heartbeat broadcast (0x300)
 *   2. Sends per-node contactor commands (0x301..0x318)
 *
 * On receiving node STATUS/POWER/INFO frames:
 *   - Updates datalayer.system.battery_nodes[node_id-1]
 *   - Manages still_alive countdown per node
 *
 * Every second (update_values):
 *   - Decrements still_alive counters
 *   - Checks spændings-matchning (voltage difference) like check_interconnect_available()
 *   - Aggregates all online nodes into datalayer.battery
 */
class ControllerCan : public CanReceiver, public Transmitter {
 public:
  void begin();
  void receive_can_frame(CAN_frame* rx_frame) override;
  void transmit(unsigned long currentMillis) override;

  /** Must be called every 1 second to update aggregation and node health */
  void update_values();

  /** True if at least one node is usable for the pack: online, identity verified, no fault
   *  flags and not stale. Balancing nodes count as usable (temporarily idle, not failed).
   *  Used to decide whether to keep the controller's CAN-alive counter fresh — when NO node is
   *  usable the counter expires and safety.cpp raises a system-wide FAULT to the inverter. */
  bool any_node_usable() const;

 private:
  void send_heartbeat();
  void send_contactor_commands(unsigned long currentMillis);
  void update_node_aggregation();
  void check_node_voltage_safety(uint8_t node_idx);
  /** True only when the node has reported IDENT and its FW + battery type match.
   *  An unverified or mismatched node must never prejoin or close its contactor. */
  bool node_identity_ok(uint8_t node_idx) const;

  unsigned long _last_heartbeat_ms = 0;
  unsigned long _next_contactor_tx_ms = 0;
  uint8_t _next_contactor_idx = 0;
  bool _contactor_burst_pending = false;
  unsigned long _startup_begin_ms = 0;  // Set when first node comes online
  bool _startup_grace_done = false;     // True after IU_STARTUP_GRACE_S seconds
  bool _estop_was_active = false;       // Tracks e-stop transitions to force re-qualification
};

extern ControllerCan controller_can;

#endif  // _CONTROLLER_CAN_H_
