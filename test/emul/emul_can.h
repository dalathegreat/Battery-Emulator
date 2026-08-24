#ifndef BE_TEST_EMUL_EMUL_CAN_H
#define BE_TEST_EMUL_EMUL_CAN_H

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "../../Software/src/communication/can/comm_can.h"

// Frames the firmware handed to the (emulated) controller, in order.
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

// Clears the device table and wires every logical interface to the recording
// device. A global test environment calls this once before the first test; any
// test that registers devices of its own must call it again in TearDown, or the
// interfaces stay unmapped and later tests silently transmit into nothing.
void emul_install_can_devices();

// Makes the recording device reject frames, as a full controller TX buffer
// would. Reset by emul_install_can_devices().
void emul_set_can_send_refused(bool refused);

// The health slot the recording device was given at registration.
uint8_t emul_can_device_index();

// What the firmware handed to #2769's streaming sink, in order.
struct StreamedFrame {
  CAN_frame frame;
  CAN_Interface interface;
  frameDirection direction;
};
void clear_streamed_frames();
const std::vector<StreamedFrame>& get_streamed_frames();

#endif  // BE_TEST_EMUL_EMUL_CAN_H
