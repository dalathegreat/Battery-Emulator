#ifndef _COMM_CAN_DISPATCH_H_
#define _COMM_CAN_DISPATCH_H_

#include <map>
#include <vector>

#include "../../devboard/utils/types.h"
#include "comm_can.h"

/* The receiver registry and the fan-out of a received frame to it.
 *
 * Split out of comm_can.cpp so it can be tested: that file also drives the
 * native TWAI controller, the MCP2515 and both MCP2518FDs, so nothing in it is
 * reachable from a host build. What lives here is only bookkeeping - which
 * driver asked for which logical interface, at what speed, and who gets a
 * frame that arrives on one - and depends on no hardware at all.
 *
 * The registration API itself is declared in comm_can.h and unchanged.
 */

// True if any driver registered for this interface. init_CAN() uses this to
// decide which controllers to bring up.
bool can_receiver_registered(CAN_Interface interface);

// The speed requested for an interface, or fallback when nothing registered.
// Where several receivers share an interface the first registration wins,
// which is the behaviour init_CAN() has always had.
CAN_Speed can_receiver_speed(CAN_Interface interface, CAN_Speed fallback);

// Hands a received frame to every receiver registered for the interface.
void deliver_to_receivers(CAN_frame* rx_frame, CAN_Interface interface);

// Number of registrations, for tests and for the registration log line.
size_t can_receiver_count();

// Registers a physical device and assigns its datalayer health slot from
// registration order. False when there are more devices than slots.
bool register_device(class CanDevice* device);

// The physical devices, in registration order.
const std::vector<class CanDevice*>& unique_can_devices();

// Points a logical interface at the physical device that serves it. Several
// interfaces may share one device - on boards where the "native" FD controller
// is the same chip as the first FD add-on, both names map to one device.
void map_interface_to_device(CAN_Interface interface, class CanDevice* device);

// The device backing an interface, or nullptr if none is mapped.
class CanDevice* device_for_interface(CAN_Interface interface);

struct CanReceiverRegistration {
  class CanReceiver* receiver;
  CAN_Speed speed;
};

/* This layer's own mutable state, gathered into one plain struct.
 *
 * It was three file-scope statics plus a function-local one, each reset - or
 * not - on its own. The unit tests run every case in one process, so anything
 * missed carried into the next case and quietly changed its preconditions.
 * print_can_frame's dropped-frame counter was the one nothing reset at all: a
 * case that filled the port left a pending gap marker that surfaced in whatever
 * case printed next, and no case could assert the marker's content without
 * first flushing it by hand.
 *
 * Data only, no methods - the behaviour stays in the free functions below, the
 * same shape the datalayer types use. The struct is the part that matters:
 * reinitialising is one assignment of a fresh instance, so a field added later
 * cannot escape the reset the way a hand-written list of clears lets it.
 *
 * Firmware behaviour is unchanged. It never calls the reset, and the storage
 * duration is the same either way.
 */
struct CanDispatchState {
  // Which driver asked for which logical interface, and at what speed. A
  // multimap: several drivers may share one interface, served in registration
  // order.
  std::multimap<CAN_Interface, CanReceiverRegistration> receivers;

  // The physical devices, in registration order. A device's index here IS its
  // datalayer health slot.
  std::vector<class CanDevice*> devices;

  // Logical interface -> the physical device serving it. Entries may repeat:
  // on boards whose "native" FD controller is the first FD add-on, both names
  // point at one device.
  class CanDevice* device_for[NO_CAN_INTERFACE] = {};

  // print_can_frame's USB console accounting. The line buffer is storage rather
  // than state - it is out of line so a 288-byte frame line never lands on the
  // core task's stack - but it belongs with the counter it is written beside.
  char usb_line[288] = {};
  uint32_t usb_frames_dropped = 0;
};

extern CanDispatchState can_dispatch;

#ifdef UNIT_TEST
// Test-only: puts the layer back to its power-on state in one assignment.
// Firmware has no reason to call it - the state is meant to live as long as the
// process does.
void reset_can_dispatch_state();
#endif

/* What a TWAI controller's status register means.
 *
 * Reading the register needs the controller library and therefore silicon;
 * deciding what its bits imply does not. The bit values are mirrored here so
 * the decision can be tested on a host, and comm_can.cpp static_asserts them
 * against the library's own definitions - if the vendored library ever changes
 * them, that is a build error rather than a silently wrong recovery.
 */
constexpr uint32_t kTwaiErrorStatusBit = 0x40;
constexpr uint32_t kTwaiBusOffStatusBit = 0x80;

struct CanBusStatusAction {
  bool reinitialise;    // the controller is bus-off and has to be brought back up
  bool flag_bus_error;  // report a bus error against this device
};

CanBusStatusAction evaluate_twai_status(uint32_t status_register);

// Hands a frame to the device backing an interface and flags that device if the
// send is refused. Returns false when nothing is mapped, which is a quiet drop.
bool route_frame_to_device(const CAN_frame& tx_frame, CAN_Interface interface);

// Logs one frame to whichever sinks the user has switched on (USB console,
// webserver CAN-log page). Lives here rather than in comm_can.cpp because it
// is pure formatting: comm_can.cpp's dispatch_frame() calls it for RX, the
// transmit path below for TX.
void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir);

#endif  // _COMM_CAN_DISPATCH_H_
