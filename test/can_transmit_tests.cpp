#include <gtest/gtest.h>

#include <string>

#include "emul/emul_can.h"

#include "../Software/src/communication/can/comm_can.h"
#include "../Software/src/communication/can/comm_can_device.h"
#include "../Software/src/communication/can/comm_can_dispatch.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"

#include "Arduino.h"
#include "HardwareSerial.h"

/* transmit_can_frame_to_interface() - the whole of it.
 *
 * Everything a driver's send goes through before it reaches a controller: the
 * allowed_to_send_CAN gate, the two logging sinks and the interface -> device
 * routing. Until the emulation boundary moved down to the controller, this
 * function was replaced wholesale in the test build, so none of it ran and
 * deleting any one of those steps from the firmware failed no test.
 *
 * Routing itself is covered against route_frame_to_device() in
 * can_health_event_tests.cpp; what these cases add is that the transmit
 * function reaches it, and under which conditions it does not.
 */

namespace {

class CanTransmitTest : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    allowed_to_send_CAN = true;
    set_millis64(12000);
    clear_transmitted_frames();
    Serial.clear_written();
    Serial.tx_free = 4096;
  }

  void TearDown() override {
    allowed_to_send_CAN = true;
    datalayer.system.info.CAN_usb_logging_active = false;
    datalayer.system.info.can_logging_active = false;
    Serial.clear_written();
    Serial.tx_free = 4096;
  }

  static CAN_frame frame(uint32_t id, uint8_t b0 = 0) {
    CAN_frame f = {};
    f.ID = id;
    f.DLC = 8;
    f.data.u8[0] = b0;
    return f;
  }

  // A named frame: transmit takes a pointer, and &frame(...) would be an rvalue.
  static void send(uint32_t id, CAN_Interface interface, uint8_t b0 = 0) {
    CAN_frame f = frame(id, b0);
    transmit_can_frame_to_interface(&f, interface);
  }
};

}  // namespace

// The gate the safety layer closes on shutdown. It is the only thing standing
// between a fault and the firmware still talking to the inverter, and it sits
// ahead of the logging so a blocked send is not even recorded.
TEST_F(CanTransmitTest, NothingIsSentWhileSendingIsDisallowed) {
  allowed_to_send_CAN = false;

  send(0x123, CAN_NATIVE);

  EXPECT_TRUE(get_transmitted_frames().empty());
}

TEST_F(CanTransmitTest, SendingResumesWhenTheGateReopens) {
  allowed_to_send_CAN = false;
  send(0x123, CAN_NATIVE);
  ASSERT_TRUE(get_transmitted_frames().empty());

  allowed_to_send_CAN = true;
  send(0x123, CAN_NATIVE);

  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(get_transmitted_frames()[0].ID, 0x123u);
}

// The wiring the emulated transmit used to hide: that the function hands the
// frame to the routing table at all.
TEST_F(CanTransmitTest, FrameReachesTheDeviceMappedToTheInterface) {
  send(0x2A0, CAN_ADDON_MCP2515);

  ASSERT_EQ(get_transmitted_frames().size(), 1u);
  EXPECT_EQ(get_transmitted_frames()[0].ID, 0x2A0u);
}

TEST_F(CanTransmitTest, FrameOnAnUnmappedInterfaceIsDroppedQuietly) {
  map_interface_to_device(CANFD_ADDON_MCP2518_2, nullptr);

  send(0x2A0, CANFD_ADDON_MCP2518_2);

  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_FALSE(datalayer.system.info.can_device[emul_can_device_index()].send_fail)
      << "an unmapped interface is nobody's fault; no device may be blamed for it";
}

// A controller that refuses the frame (full TX buffer) is the device's problem,
// and has to land in that device's health slot rather than pass silently.
TEST_F(CanTransmitTest, RefusedSendFlagsTheDeviceThatRefused) {
  emul_set_can_send_refused(true);

  send(0x2A0, CAN_NATIVE);

  EXPECT_TRUE(get_transmitted_frames().empty());
  EXPECT_TRUE(datalayer.system.info.can_device[emul_can_device_index()].send_fail);
}

TEST_F(CanTransmitTest, SuccessfulSendLeavesTheDeviceUnflagged) {
  send(0x2A0, CAN_NATIVE);

  EXPECT_FALSE(datalayer.system.info.can_device[emul_can_device_index()].send_fail);
}

// --- USB console logging -----------------------------------------------------

TEST_F(CanTransmitTest, NothingIsPrintedWhileUsbLoggingIsOff) {
  send(0x123, CAN_NATIVE);

  EXPECT_TRUE(Serial.written.empty());
}

TEST_F(CanTransmitTest, TransmittedFrameIsPrintedAsTxOnTheOddBusNumber) {
  datalayer.system.info.CAN_usb_logging_active = true;

  send(0x123, CAN_ADDON_MCP2515, 0xAB);

  // SavvyCAN separates directions by bus: RX gets interface*2, TX that plus one,
  // so MCP2515 (interface 2) transmits on bus 5.
  EXPECT_NE(Serial.written.find("TX5 123 [8] AB"), std::string::npos) << "got: " << Serial.written;
}

// The core task must not stall on a slow USB host, so a line that does not fit
// is dropped and counted instead of blocking.
TEST_F(CanTransmitTest, FrameIsDroppedRatherThanBlockingWhenThePortIsFull) {
  datalayer.system.info.CAN_usb_logging_active = true;
  Serial.tx_free = 4;

  send(0x123, CAN_NATIVE);

  EXPECT_TRUE(Serial.written.empty());
  // Dropping the log line must not drop the frame.
  EXPECT_EQ(get_transmitted_frames().size(), 1u);

  // Once the port drains, the gap is reported rather than passed off as a
  // continuous capture.
  Serial.tx_free = 4096;
  send(0x124, CAN_NATIVE);
  EXPECT_NE(Serial.written.find("[1 CAN frames not printed]"), std::string::npos) << "got: " << Serial.written;
  EXPECT_NE(Serial.written.find("TX1 124"), std::string::npos) << "got: " << Serial.written;
}

TEST_F(CanTransmitTest, DropMarkerIsReportedOnceAndCountsEveryDroppedFrame) {
  datalayer.system.info.CAN_usb_logging_active = true;
  Serial.tx_free = 4;
  send(0x123, CAN_NATIVE);
  send(0x124, CAN_NATIVE);
  send(0x125, CAN_NATIVE);

  Serial.tx_free = 4096;
  send(0x126, CAN_NATIVE);
  ASSERT_NE(Serial.written.find("[3 CAN frames not printed]"), std::string::npos) << "got: " << Serial.written;

  Serial.clear_written();
  send(0x127, CAN_NATIVE);
  EXPECT_EQ(Serial.written.find("not printed"), std::string::npos)
      << "the marker must be paid out once, not repeated: " << Serial.written;
}

// --- Webserver CAN log -------------------------------------------------------

TEST_F(CanTransmitTest, NothingIsLoggedToTheWebserverBufferWhileLoggingIsOff) {
  send(0x123, CAN_NATIVE);

  EXPECT_EQ(datalayer.system.info.logged_can_messages_offset, 0u);
}

TEST_F(CanTransmitTest, LoggedFrameLandsInTheWebserverBuffer) {
  datalayer.system.info.can_logging_active = true;

  send(0x123, CAN_NATIVE);

  EXPECT_GT(datalayer.system.info.logged_can_messages_offset, 0u);
  EXPECT_NE(std::string(datalayer.system.info.logged_can_messages).find("TX1 123 [8]"), std::string::npos)
      << "got: " << datalayer.system.info.logged_can_messages;
}

TEST_F(CanTransmitTest, CutoffFilterKeepsLowIdsOutOfTheWebserverBuffer) {
  datalayer.system.info.can_logging_active = true;
  user_selected_CAN_ID_cutoff_filter = 0x200;

  send(0x100, CAN_NATIVE);
  EXPECT_EQ(datalayer.system.info.logged_can_messages_offset, 0u) << "an ID at or below the cutoff must not be logged";

  send(0x201, CAN_NATIVE);
  EXPECT_GT(datalayer.system.info.logged_can_messages_offset, 0u);

  user_selected_CAN_ID_cutoff_filter = 0;
}

// The dropped-frame count is the one piece of this layer's state that no code
// path clears on its own - it is paid out once and only when a write fits. A
// case that fills the port would otherwise hand its pending gap marker to
// whichever case printed next.
TEST_F(CanTransmitTest, ResettingTheDispatchStateDropsAPendingGapMarker) {
  datalayer.system.info.CAN_usb_logging_active = true;
  Serial.tx_free = 4;
  send(0x123, CAN_NATIVE);
  send(0x124, CAN_NATIVE);
  ASSERT_EQ(can_dispatch.usb_frames_dropped, 2u);

  reset_can_dispatch_state();
  EXPECT_EQ(can_dispatch.usb_frames_dropped, 0u);

  // Nothing owed: the next frame that fits prints on its own.
  emul_install_can_devices();  // the reset emptied the device table too
  Serial.tx_free = 4096;
  Serial.clear_written();
  send(0x125, CAN_NATIVE);
  EXPECT_EQ(Serial.written.find("not printed"), std::string::npos) << "got: " << Serial.written;
  EXPECT_NE(Serial.written.find("TX1 125"), std::string::npos) << "got: " << Serial.written;
}

TEST_F(CanTransmitTest, EveryTestStartsWithNothingOwedOnTheConsole) {
  EXPECT_EQ(can_dispatch.usb_frames_dropped, 0u);
}
