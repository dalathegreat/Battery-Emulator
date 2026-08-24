#ifndef _COMM_CAN_DISPATCH_H_
#define _COMM_CAN_DISPATCH_H_

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

#endif  // _COMM_CAN_DISPATCH_H_
