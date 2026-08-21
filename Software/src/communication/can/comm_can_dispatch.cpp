#include "comm_can_dispatch.h"

#include <map>
#include "../../devboard/utils/logging.h"
#include "CanReceiver.h"

// Moved verbatim out of comm_can.cpp: same multimap, same insertion, same
// equal_range fan-out in registration order.
struct CanReceiverRegistration {
  CanReceiver* receiver;
  CAN_Speed speed;
};

static std::multimap<CAN_Interface, CanReceiverRegistration> can_receivers;

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {
  can_receivers.insert({interface, {receiver, speed}});
  DEBUG_PRINTF("CAN receiver registered, total: %d\n", can_receivers.size());
}

bool can_receiver_registered(CAN_Interface interface) {
  return can_receivers.find(interface) != can_receivers.end();
}

CAN_Speed can_receiver_speed(CAN_Interface interface, CAN_Speed fallback) {
  auto it = can_receivers.find(interface);
  return it == can_receivers.end() ? fallback : it->second.speed;
}

void deliver_to_receivers(CAN_frame* rx_frame, CAN_Interface interface) {
  // Send the frame to all the receivers registered for this interface.
  auto receivers = can_receivers.equal_range(interface);

  for (auto it = receivers.first; it != receivers.second; ++it) {
    auto& receiver = it->second;
    receiver.receiver->receive_can_frame(rx_frame);
  }
}

size_t can_receiver_count() {
  return can_receivers.size();
}

void clear_can_receivers() {
  can_receivers.clear();
}
