#include "comm_can_dispatch.h"

#include <Arduino.h>

#include <map>
#include <vector>
#include "../../datalayer/datalayer.h"
#include "../../devboard/safety/safety.h"
#include "../../devboard/sdcard/sdcard.h"
#include "../../devboard/utils/logging.h"
#include "CanReceiver.h"
#include "comm_can_device.h"

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

// The physical CAN devices and the health events they share. Not hardware:
// registration order, datalayer slots and event bookkeeping only, so it lives
// here with the receiver registry rather than beside the controller drivers.
static std::vector<CanDevice*> all_devices;

// Logical interface -> the physical device serving it. Entries may repeat.
static CanDevice* device_for[NO_CAN_INTERFACE] = {};

// A device's datalayer health slot IS its registration order, so no class
// hardcodes an index. Order is init_CAN()'s fixed construction sequence (and
// later the board's declared bus list), which keeps a device's slot stable
// across boots - logs and telemetry reference it by number.
bool register_device(CanDevice* device) {
  if (all_devices.size() >= MAX_CAN_DEVICES) {
    logging.println("More CAN devices than datalayer health slots - raise MAX_CAN_DEVICES");
    return false;
  }
  device->device_index = static_cast<uint8_t>(all_devices.size());
  all_devices.push_back(device);
  return true;
}

void update_can_health_events() {
  static_assert(MAX_CAN_DEVICES <= 8, "the device bitmask in the event payload is a uint8_t");

  // Devices sharing an event collapse into one entry, so the decision to raise
  // or clear is made once from the union of them. Deciding per device would
  // let a healthy controller clear the event a faulty sibling just raised.
  struct EventDevices {
    EVENTS_ENUM_TYPE event;
    uint8_t devices;  // bitmask, bit N = device N
  };
  EventDevices health[2 * MAX_CAN_DEVICES] = {};
  uint8_t count = 0;

  auto accumulate = [&](EVENTS_ENUM_TYPE event, bool active, uint8_t device_index) {
    if (event == EVENT_NOF_EVENTS) {
      return;
    }
    for (uint8_t i = 0; i < count; ++i) {
      if (health[i].event == event) {
        if (active) {
          health[i].devices |= static_cast<uint8_t>(1 << device_index);
        }
        return;
      }
    }
    health[count].event = event;
    health[count].devices = active ? static_cast<uint8_t>(1 << device_index) : 0;
    ++count;
  };

  for (CanDevice* device : all_devices) {
    DATALAYER_CAN_DEVICE_TYPE& flags = datalayer.system.info.can_device[device->device_index];
    accumulate(device->buffer_full_event, flags.send_fail, device->device_index);
    accumulate(device->bus_error_event, flags.bus_error, device->device_index);
    flags.send_fail = false;
    flags.bus_error = false;
  }

  for (uint8_t i = 0; i < count; ++i) {
    if (health[i].devices != 0) {
      set_event(health[i].event, health[i].devices);
    } else {
      clear_event(health[i].event);
    }
  }
}

const std::vector<CanDevice*>& unique_can_devices() {
  return all_devices;
}

CanBusStatusAction evaluate_twai_status(uint32_t status_register) {
  CanBusStatusAction action = {};
  // Bus-off is latching in the controller: it stops participating until it is
  // re-initialised, so recovery is not optional and the error is also reported.
  if ((status_register & kTwaiBusOffStatusBit) != 0) {
    action.reinitialise = true;
    action.flag_bus_error = true;
  }
  // Error-warning state alone means the error counters are high but the
  // controller is still on the bus - report it, but do not reset a working
  // controller out from under the traffic it is carrying.
  if ((status_register & kTwaiErrorStatusBit) != 0) {
    action.flag_bus_error = true;
  }
  return action;
}

void map_interface_to_device(CAN_Interface interface, CanDevice* device) {
  if (interface < NO_CAN_INTERFACE) {
    device_for[interface] = device;
  }
}

CanDevice* device_for_interface(CAN_Interface interface) {
  return interface < NO_CAN_INTERFACE ? device_for[interface] : nullptr;
}

int can_device_index_for(CAN_Interface interface) {
  if (interface >= NO_CAN_INTERFACE || device_for[interface] == nullptr) {
    return -1;
  }
  return device_for[interface]->device_index;
}

bool route_frame_to_device(const CAN_frame& tx_frame, CAN_Interface interface) {
  CanDevice* dev = (interface < NO_CAN_INTERFACE) ? device_for[interface] : nullptr;
  if (dev == nullptr) {
    return false;
  }
  if (!dev->try_send(tx_frame)) {
    datalayer.system.info.can_device[dev->device_index].send_fail = true;
  }
  return true;
}

void clear_can_devices() {
  all_devices.clear();
  for (int i = 0; i < NO_CAN_INTERFACE; ++i) {
    device_for[i] = nullptr;
  }
}

uint16_t user_selected_CAN_ID_cutoff_filter = 0;  //Messages below this ID will not be logged in webserver

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  if (offset + 128 > sizeof(datalayer.system.info.logged_can_messages)) {
    // Not enough space, reset and start from the beginning
    offset = 0;
  }
  unsigned long currentTime = millis();
  // Add timestamp
  offset += snprintf(message_string + offset, message_string_size - offset, "(%lu.%03lu) ", currentTime / 1000,
                     currentTime % 1000);

  // Add direction. Multiplying the interface by two ensures that SavvyCAN puts TX and RX in a different bus.
  offset += snprintf(message_string + offset, message_string_size - offset, "%s%d ", (msgDir == MSG_RX) ? "RX" : "TX",
                     (int)(interface * 2) + (msgDir == MSG_RX ? 0 : 1));

  // Add ID and DLC
  offset += snprintf(message_string + offset, message_string_size - offset, "%lX [%u] ", frame.ID, frame.DLC);

  // Add data bytes
  for (uint8_t i = 0; i < frame.DLC; i++) {
    if (i < frame.DLC - 1) {
      offset += snprintf(message_string + offset, message_string_size - offset, "%02X ", frame.data.u8[i]);
    } else {
      offset += snprintf(message_string + offset, message_string_size - offset, "%02X", frame.data.u8[i]);
    }
  }
  // Add linebreak
  offset += snprintf(message_string + offset, message_string_size - offset, "\n");

  datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
}

void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir) {

  if (datalayer.system.info.CAN_usb_logging_active) {
    // Build the whole line first, then write it in one go - and only if the TX
    // buffer has room. This path runs in the core task: a blocked/slow USB host
    // must never stall it (EVENT_TASK_OVERRUN). Frames that don't fit are
    // counted and reported as a gap marker once the port drains.
    static char usb_line[288];  // header + up to 64 CAN-FD data bytes at 3 chars each
    static uint32_t usb_frames_dropped = 0;
    unsigned long currentTime = millis();
    size_t size = snprintf(usb_line, sizeof(usb_line), "(%lu.%02lu) %s%d %lX [%u] ", currentTime / 1000,
                           (currentTime % 1000) / 10, (msgDir == MSG_RX) ? "RX" : "TX",
                           (msgDir == MSG_RX) ? (int)(interface * 2) : (int)(interface * 2) + 1, frame.ID, frame.DLC);
    for (uint8_t i = 0; i < frame.DLC; i++) {
      size += snprintf(usb_line + size, sizeof(usb_line) - size, (i < frame.DLC - 1) ? "%02X " : "%02X\r\n",
                       frame.data.u8[i]);
    }
    if (frame.DLC == 0) {
      size += snprintf(usb_line + size, sizeof(usb_line) - size, "\r\n");
    }

    if ((size_t)Serial.availableForWrite() >= size) {
      if (usb_frames_dropped > 0) {
        char marker[48];
        int marker_len =
            snprintf(marker, sizeof(marker), "[%lu CAN frames not printed]\r\n", (unsigned long)usb_frames_dropped);
        if ((size_t)Serial.availableForWrite() >= size + (size_t)marker_len) {
          Serial.write((const uint8_t*)marker, marker_len);
          usb_frames_dropped = 0;
        }
      }
      Serial.write((const uint8_t*)usb_line, size);
    } else {
      usb_frames_dropped++;
    }
  }

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    if (frame.ID > user_selected_CAN_ID_cutoff_filter) {  //Only log the message if CAN ID is higher than user set value
      dump_can_frame(frame, interface, msgDir);
    }
  }
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, interface, frameDirection(MSG_TX));

#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*tx_frame, frameDirection(MSG_TX));
  }
#endif

  // Invalid or unmapped interface: dropped quietly.
  // TODO: Raise event that coders messed up
  route_frame_to_device(*tx_frame, interface);
}
