#ifndef _MASTER_CAN_H_
#define _MASTER_CAN_H_

#include "../../communication/Transmitter.h"
#include "CanReceiver.h"
#include "INTER-UNIT-PROTOCOL.h"

/**
 * MasterCan — Inter-Unit protocol handler for MASTER nodes.
 *
 * Registers on can_config.inter_unit (configured via IUCOMM setting).
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
  void send_contactor_commands();
  void update_slave_aggregation();
  void check_slave_voltage_safety(uint8_t slave_idx);

  unsigned long _last_heartbeat_ms = 0;
  unsigned long _startup_begin_ms = 0;   // Set on first update_values() call
  bool _startup_grace_done = false;      // True after IU_STARTUP_GRACE_S seconds
};

extern MasterCan master_can;
void setup_master_can();

#endif  // _MASTER_CAN_H_
