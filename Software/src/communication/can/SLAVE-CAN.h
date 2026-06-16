#ifndef _SLAVE_CAN_H_
#define _SLAVE_CAN_H_

#include "../../communication/Transmitter.h"
#include "CanReceiver.h"
#include "INTER-UNIT-PROTOCOL.h"

/**
 * SlaveCan — Inter-Unit protocol handler for SLAVE nodes.
 *
 * Registers on can_config.inverter (configured via INVCOMM setting).
 *
 * On heartbeat received: schedules reply frames after (node_id * 5ms)
 *   - STATUS message: voltage, SOC, current, temp_max, flags (contactor + faults)
 *   - POWER message:  max_charge_W, max_discharge_W, remaining_Wh, temp_min
 *   - INFO message:   total_capacity_Wh, voltage limits (every 10th heartbeat)
 *
 * On contactor command received: updates contactor_allowed in datalayer.
 *
 * Safety: if no heartbeat received for CAN_STILL_ALIVE seconds (60s),
 *         slave opens its own contactors (master is offline).
 */
class SlaveCan : public CanReceiver, public Transmitter {
 public:
  void begin();
  void receive_can_frame(CAN_frame* rx_frame) override;
  void transmit(unsigned long currentMillis) override;

  /** True if the master has been heard from within the last CAN_STILL_ALIVE seconds (60s) */
  bool master_online() const { return _master_online; }

 private:
  void send_status_frame();
  void send_power_frame();
  void send_info_frame();
  void send_cell_frame();
  void send_ip_frame();
  void send_ident_frame();
  uint8_t build_fault_flags();

  unsigned long _last_heartbeat_ms = 0;  // millis() when last heartbeat was received
  unsigned long _reply_due_ms = 0;       // millis() when we should send our reply
  bool _reply_pending = false;           // True: we have a reply to send
  uint8_t _heartbeat_count = 0;          // Counts heartbeats to decide when to send INFO
  bool _master_online = false;           // True if master is alive
};

extern SlaveCan slave_can;
void setup_slave_can();

#endif  // _SLAVE_CAN_H_
