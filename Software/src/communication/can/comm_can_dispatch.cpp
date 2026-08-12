#include "comm_can_dispatch.h"

#include <Arduino.h>

#include <map>
#include <vector>
#include "../../datalayer/datalayer.h"
#include "../../devboard/safety/safety.h"
#include "../../devboard/sdcard/sdcard.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/webserver/webserver_can_streaming.h"
#include "CanReceiver.h"
#include "comm_can_device.h"

CanDispatchState can_dispatch;

#ifdef UNIT_TEST
void reset_can_dispatch_state() {
  can_dispatch = CanDispatchState{};
}
#endif

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {
  can_dispatch.receivers.insert({interface, {receiver, speed}});
  DEBUG_PRINTF("CAN receiver registered, total: %d\n", can_dispatch.receivers.size());
}

bool can_receiver_registered(CAN_Interface interface) {
  return can_dispatch.receivers.find(interface) != can_dispatch.receivers.end();
}

CAN_Speed can_receiver_speed(CAN_Interface interface, CAN_Speed fallback) {
  auto it = can_dispatch.receivers.find(interface);
  return it == can_dispatch.receivers.end() ? fallback : it->second.speed;
}

void deliver_to_receivers(CAN_frame* rx_frame, CAN_Interface interface) {
  // Send the frame to all the receivers registered for this interface.
  auto receivers = can_dispatch.receivers.equal_range(interface);

  for (auto it = receivers.first; it != receivers.second; ++it) {
    auto& receiver = it->second;
    receiver.receiver->receive_can_frame(rx_frame);
  }
}

size_t can_receiver_count() {
  return can_dispatch.receivers.size();
}

// The physical CAN devices and the health events they share are not hardware:
// registration order, datalayer slots and event bookkeeping only, so they live
// here with the receiver registry rather than beside the controller drivers.

// A device's datalayer health slot IS its registration order, so no class
// hardcodes an index. Order is init_CAN()'s fixed construction sequence (and
// later the board's declared bus list), which keeps a device's slot stable
// across boots - logs and telemetry reference it by number.
bool register_device(CanDevice* device) {
  if (can_dispatch.devices.size() >= MAX_CAN_DEVICES) {
    logging.println("More CAN devices than datalayer health slots - raise MAX_CAN_DEVICES");
    return false;
  }
  device->device_index = static_cast<uint8_t>(can_dispatch.devices.size());
  can_dispatch.devices.push_back(device);
  return true;
}

void update_can_health_events() {
  /* One event pair per controller, so the raise/clear decision is per device
     and no controller can touch another's event. The masking bug this used to
     guard against - a healthy chip clearing what a faulty sibling had just
     raised - is gone by construction rather than by aggregation (dala's ruling
     on #2799; the enum already carried the second FD chip's pair).

     Consuming the flags is still this function's job: the devices set them from
     wherever they run, and clearing them here is what makes a fault that has
     stopped recurring clear its event on the next pass. */
  for (CanDevice* device : can_dispatch.devices) {
    DATALAYER_CAN_DEVICE_TYPE& flags = datalayer.system.info.can_device[device->device_index];
    const bool ignored = can_errors_ignored(device->device_index);

    if (device->buffer_full_event != EVENT_NOF_EVENTS) {
      if (flags.send_fail && !ignored) {
        set_event(device->buffer_full_event, device->device_index);
      } else if (!flags.send_fail) {
        clear_event(device->buffer_full_event);
      }
    }
    if (device->bus_error_event != EVENT_NOF_EVENTS) {
      if (flags.bus_error && !ignored) {
        set_event(device->bus_error_event, device->device_index);
      } else if (!flags.bus_error) {
        clear_event(device->bus_error_event);
      }
    }

    flags.send_fail = false;
    flags.bus_error = false;
  }
}

const std::vector<CanDevice*>& unique_can_devices() {
  return can_dispatch.devices;
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
    can_dispatch.device_for[interface] = device;
  }
}

CanDevice* device_for_interface(CAN_Interface interface) {
  return interface < NO_CAN_INTERFACE ? can_dispatch.device_for[interface] : nullptr;
}

int can_device_index_for(CAN_Interface interface) {
  if (interface >= NO_CAN_INTERFACE || can_dispatch.device_for[interface] == nullptr) {
    return -1;
  }
  return can_dispatch.device_for[interface]->device_index;
}

bool route_frame_to_device(const CAN_frame& tx_frame, CAN_Interface interface) {
  CanDevice* dev = (interface < NO_CAN_INTERFACE) ? can_dispatch.device_for[interface] : nullptr;
  if (dev == nullptr) {
    return false;
  }
  if (!dev->try_send(tx_frame)) {
    datalayer.system.info.can_device[dev->device_index].send_fail = true;
  }
  return true;
}

uint16_t user_selected_CAN_ID_cutoff_filter = 0;  //Messages below this ID will not be logged in webserver

// For formatting CAN frames considerably faster than using snprintf
static const char* hex = "0123456789abcdef";

static char* put_hex(char* ptr, uint32_t value, uint8_t digits) {
  for (int i = digits - 1; i >= 0; i--) {
    *ptr++ = hex[(value >> (i * 4)) & 0x0f];
  }
  return ptr;
}

static char* put_time(char* ptr, unsigned long time) {
  // Wrap around after 100000 seconds (about 27.7 hours)
  if (time >= 100000000)
    time = time % 100000000;

  char buf[8];
  int i = 0;
  do {
    buf[i++] = (time % 10) + '0';
    time /= 10;
  } while (time > 0);
  while (i > 0) {
    *ptr++ = buf[--i];
    if (i == 3) {
      *ptr++ = '.';
    }
  }
  return ptr;
}

// CAN log formatter: "(12345.678) RX0 123 [8] 01 02 03 ... 0A\n".
size_t format_can_frame(char* buffer, size_t len, const CAN_frame& frame, CAN_Interface interface,
                        frameDirection msgDir) {
  // Worst-case line length: '(' + up-to-8-digit time + optional '.' + ')' + ' '
  // + "RX"/"TX" + channel digit + ' ' + 8-hex ID + ' ' + '[' + 2-digit DLC + ']'
  // + 3 bytes per data byte + '\n'.
  const size_t needed = 1 + 9 + 1 + 1 + 3 + 1 + 8 + 1 + 1 + 2 + 1 + (size_t)frame.DLC * 3 + 1;
  if (needed > len) {
    if (len > 0) {
      buffer[0] = '\0';
    }
    return 0;
  }

  char* ptr = buffer;
  const unsigned long currentTime = millis();
  *ptr++ = '(';
  ptr = put_time(ptr, currentTime);
  *ptr++ = ')';
  *ptr++ = ' ';
  if (msgDir == MSG_RX) {
    *ptr++ = frame.FD ? 'R' : 'r';
    *ptr++ = frame.FD ? 'X' : 'x';
    *ptr++ = '0' + ((int)interface * 2);
  } else {
    *ptr++ = frame.FD ? 'T' : 't';
    *ptr++ = frame.FD ? 'X' : 'x';
    *ptr++ = '1' + ((int)interface * 2);
  }
  *ptr++ = ' ';
  if (frame.ext_ID)
    ptr = put_hex(ptr, frame.ID, 8);
  else
    ptr = put_hex(ptr, frame.ID, 3);
  *ptr++ = ' ';
  *ptr++ = '[';
  if (frame.DLC > 9) {
    *ptr++ = '0' + (frame.DLC / 10);
    *ptr++ = '0' + (frame.DLC % 10);
  } else
    *ptr++ = '0' + (frame.DLC);
  *ptr++ = ']';
  for (int i = 0; i < frame.DLC; i++) {
    *ptr++ = ' ';
    ptr = put_hex(ptr, frame.data.u8[i], 2);
  }
  *ptr++ = '\n';
  *ptr = '\0';
  return (size_t)(ptr - buffer);
}

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
  char* message_string = datalayer.system.info.logged_can_messages;
  size_t offset =
      datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  size_t written = format_can_frame(message_string + offset, message_string_size - offset, frame, interface, msgDir);
  if (written == 0 && offset != 0) {
    // Not enough space left at the tail - wrap around and start from the beginning
    offset = 0;
    written = format_can_frame(message_string, message_string_size, frame, interface, msgDir);
  }
  if (written > 0) {
    datalayer.system.info.logged_can_messages_offset = offset + written;  // Update offset in buffer
  }
}

void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir) {

  if (datalayer.system.info.CAN_usb_logging_active) {
    // Build the whole line first, then write it in one go - and only if the TX
    // buffer has room. This path runs in the core task: a blocked/slow USB host
    // must never stall it (EVENT_TASK_OVERRUN). Frames that don't fit are
    // counted and reported as a gap marker once the port drains.
    unsigned long currentTime = millis();
    size_t size = snprintf(can_dispatch.usb_line, sizeof(can_dispatch.usb_line), "(%lu.%02lu) %s%d %lX [%u] ",
                           currentTime / 1000, (currentTime % 1000) / 10, (msgDir == MSG_RX) ? "RX" : "TX",
                           (msgDir == MSG_RX) ? (int)(interface * 2) : (int)(interface * 2) + 1, frame.ID, frame.DLC);
    for (uint8_t i = 0; i < frame.DLC; i++) {
      size += snprintf(can_dispatch.usb_line + size, sizeof(can_dispatch.usb_line) - size,
                       (i < frame.DLC - 1) ? "%02X " : "%02X\r\n", frame.data.u8[i]);
    }
    if (frame.DLC == 0) {
      size += snprintf(can_dispatch.usb_line + size, sizeof(can_dispatch.usb_line) - size, "\r\n");
    }

    if ((size_t)Serial.availableForWrite() >= size) {
      if (can_dispatch.usb_frames_dropped > 0) {
        char marker[48];
        int marker_len = snprintf(marker, sizeof(marker), "[%lu CAN frames not printed]\r\n",
                                  (unsigned long)can_dispatch.usb_frames_dropped);
        if ((size_t)Serial.availableForWrite() >= size + (size_t)marker_len) {
          Serial.write((const uint8_t*)marker, marker_len);
          can_dispatch.usb_frames_dropped = 0;
        }
      }
      Serial.write((const uint8_t*)can_dispatch.usb_line, size);
    } else {
      can_dispatch.usb_frames_dropped++;
    }
  }

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    if (frame.ID > user_selected_CAN_ID_cutoff_filter) {  //Only log the message if CAN ID is higher than user set value
      dump_can_frame(frame, interface, msgDir);
    }
  }
  if (datalayer.system.info.can_streaming_active) {
    stream_can_frame(frame, interface, msgDir);
  }
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, interface, frameDirection(MSG_TX));

#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*tx_frame, interface, frameDirection(MSG_TX));
  }
#endif

  // Invalid or unmapped interface: dropped quietly.
  // TODO: Raise event that coders messed up
  route_frame_to_device(*tx_frame, interface);
}
