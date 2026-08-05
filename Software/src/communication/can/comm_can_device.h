#ifndef BE_COMMUNICATION_CAN_COMM_CAN_DEVICE_H
#define BE_COMMUNICATION_CAN_COMM_CAN_DEVICE_H

#include <vector>

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/types.h"
#include "comm_can.h"

// One instance per physical CAN controller. comm_can.cpp maps the logical
// CAN_Interface enum onto these devices; several logical interfaces may share
// one physical device (e.g. CANFD_NATIVE and CANFD_ADDON_MCP2518 on boards
// whose "native" FD controller is the same MCP2518 chip).
class CanDevice {
 public:
  virtual ~CanDevice() = default;

  // Allocates pins and starts the controller. Returns false on failure, which
  // aborts init_CAN() like the previous per-interface early returns did.
  virtual bool init(CAN_Speed speed) = 0;

  // Returns false if the frame could not be queued (buffer full / not up).
  virtual bool try_send(const CAN_frame& frame) = 0;

  // Drains received frames (bounded per call) and checks bus error state.
  virtual void poll_receive() = 0;

  virtual void stop() = 0;
  virtual void restart() = 0;
  virtual bool change_speed(CAN_Speed speed) { return false; }

  bool initialized = false;
  const char* name = "";

  // The logical interface received frames are logged under. Pinned to the
  // values the pre-table code stamped on RX frames (the shared Stark FD chip
  // logged under CANFD_ADDON_MCP2518, not the lower-numbered CANFD_NATIVE),
  // so existing CAN-log captures and SavvyCAN bus numbers stay comparable.
  CAN_Interface log_interface = NO_CAN_INTERFACE;

  // Health flags live in datalayer.system.info.can_device[device_index] -
  // the datalayer is the observation surface - and the device writes only its
  // own slot.
  uint8_t device_index = 0;
  EVENTS_ENUM_TYPE buffer_full_event = EVENT_NOF_EVENTS;
  EVENTS_ENUM_TYPE bus_error_event = EVENT_NOF_EVENTS;

  // Raises or clears this device's events from its health flags in the
  // datalayer, consuming them. Called periodically by the safety monitor.
  void update_health_events() {
    DATALAYER_CAN_DEVICE_TYPE& health = datalayer.system.info.can_device[device_index];
    if (health.send_fail) {
      set_event(buffer_full_event, 0);
      health.send_fail = false;
    } else {
      clear_event(buffer_full_event);
    }
    if (health.bus_error) {
      set_event(bus_error_event, 0);
      health.bus_error = false;
    } else {
      clear_event(bus_error_event);
    }
  }
};

// Unique physical CAN devices created by init_CAN(), in creation order.
const std::vector<CanDevice*>& unique_can_devices();

#endif  // BE_COMMUNICATION_CAN_COMM_CAN_DEVICE_H
