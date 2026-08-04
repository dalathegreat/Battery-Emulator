#ifndef _COMM_CAN_DEVICE_H_
#define _COMM_CAN_DEVICE_H_

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
};

#endif  // _COMM_CAN_DEVICE_H_
