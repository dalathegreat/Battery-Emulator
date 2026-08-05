#ifndef _COMM_CAN_DISPATCH_H_
#define _COMM_CAN_DISPATCH_H_

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

// Empties the registry. Exists for tests: the registry is a global that would
// otherwise carry registrations between cases.
void clear_can_receivers();

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

// Empties the device list. For tests; init_CAN builds it once at boot.
void clear_can_devices();

#endif  // _COMM_CAN_DISPATCH_H_
