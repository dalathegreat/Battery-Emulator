#include <gtest/gtest.h>

#include <vector>

#include "../Software/src/communication/can/CanReceiver.h"
#include "../Software/src/communication/can/comm_can_dispatch.h"

// Covers the CAN receiver registry and the fan-out of a received frame to it.
//
// This is the contract every battery, inverter, charger and shunt driver relies
// on: register for a logical interface, then receive exactly the frames that
// arrive on it. It is also the contract a rewrite of comm_can.cpp has to
// preserve, so these tests are written against the behaviour as it stands and
// are meant to keep passing across such a rewrite.

namespace {

// Records what it was given, so a test can assert on delivery rather than on
// the registry's internals.
class RecordingReceiver : public CanReceiver {
 public:
  void receive_can_frame(CAN_frame* frame) override { received.push_back(frame->ID); }

  std::vector<uint32_t> received;
};

CAN_frame frame_with_id(uint32_t id) {
  CAN_frame f = {};
  f.ID = id;
  f.DLC = 8;
  return f;
}

class CanDispatchTest : public ::testing::Test {
 protected:
  void SetUp() override { reset_can_dispatch_state(); }
  void TearDown() override { reset_can_dispatch_state(); }
};

}  // namespace

// --- Registration ----------------------------------------------------------

TEST_F(CanDispatchTest, NothingIsRegisteredOnAFreshRegistry) {
  EXPECT_EQ(can_receiver_count(), 0u);
  for (int i = 0; i < NO_CAN_INTERFACE; ++i) {
    EXPECT_FALSE(can_receiver_registered(static_cast<CAN_Interface>(i)));
  }
}

TEST_F(CanDispatchTest, RegisteringMarksOnlyThatInterface) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_ADDON_MCP2515, CAN_Speed::CAN_SPEED_500KBPS);

  EXPECT_TRUE(can_receiver_registered(CAN_ADDON_MCP2515));
  EXPECT_FALSE(can_receiver_registered(CAN_NATIVE));
  EXPECT_FALSE(can_receiver_registered(CANFD_ADDON_MCP2518));
  EXPECT_EQ(can_receiver_count(), 1u);
}

// init_CAN() brings up a controller at the speed its receiver asked for.
TEST_F(CanDispatchTest, RegisteredSpeedIsReportedBack) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_250KBPS);

  EXPECT_EQ(can_receiver_speed(CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS), CAN_Speed::CAN_SPEED_250KBPS);
}

TEST_F(CanDispatchTest, UnregisteredInterfaceReportsTheFallbackSpeed) {
  EXPECT_EQ(can_receiver_speed(CANFD_ADDON_MCP2518_2, CAN_Speed::CAN_SPEED_800KBPS), CAN_Speed::CAN_SPEED_800KBPS);
}

// Several drivers can share one bus - a battery and a shunt, say. The first
// registration decides the speed the controller is brought up at.
TEST_F(CanDispatchTest, FirstRegistrationDecidesTheSpeedWhenSeveralShareABus) {
  RecordingReceiver first;
  RecordingReceiver second;
  register_can_receiver(&first, CAN_NATIVE, CAN_Speed::CAN_SPEED_250KBPS);
  register_can_receiver(&second, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  EXPECT_EQ(can_receiver_speed(CAN_NATIVE, CAN_Speed::CAN_SPEED_100KBPS), CAN_Speed::CAN_SPEED_250KBPS);
  EXPECT_EQ(can_receiver_count(), 2u);
}

// --- Delivery --------------------------------------------------------------

TEST_F(CanDispatchTest, FrameReachesTheReceiverRegisteredForItsInterface) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x123);
  deliver_to_receivers(&frame, CAN_NATIVE);

  ASSERT_EQ(receiver.received.size(), 1u);
  EXPECT_EQ(receiver.received[0], 0x123u);
}

// The isolation that makes several buses usable at once: a frame on one
// interface must not reach a driver listening on another.
TEST_F(CanDispatchTest, FrameDoesNotReachReceiversOnOtherInterfaces) {
  RecordingReceiver on_native;
  RecordingReceiver on_addon;
  register_can_receiver(&on_native, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);
  register_can_receiver(&on_addon, CAN_ADDON_MCP2515, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x456);
  deliver_to_receivers(&frame, CAN_NATIVE);

  EXPECT_EQ(on_native.received.size(), 1u);
  EXPECT_TRUE(on_addon.received.empty()) << "a frame must not cross between interfaces";
}

TEST_F(CanDispatchTest, EveryReceiverOnTheInterfaceGetsTheFrame) {
  RecordingReceiver battery;
  RecordingReceiver shunt;
  RecordingReceiver inverter;
  register_can_receiver(&battery, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);
  register_can_receiver(&shunt, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);
  register_can_receiver(&inverter, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x789);
  deliver_to_receivers(&frame, CAN_NATIVE);

  ASSERT_EQ(battery.received.size(), 1u);
  ASSERT_EQ(shunt.received.size(), 1u);
  ASSERT_EQ(inverter.received.size(), 1u);
  // Contents too, not just counts: delivering the real frame to the first
  // receiver and an empty one to the rest would satisfy the counts alone.
  EXPECT_EQ(battery.received[0], 0x789u);
  EXPECT_EQ(shunt.received[0], 0x789u);
  EXPECT_EQ(inverter.received[0], 0x789u);
}

// Registering the same receiver twice on one interface delivers to it twice.
// Pinned because it is the shape a double-registration bug would take, and
// because nothing in the API prevents it.
TEST_F(CanDispatchTest, RegisteringTheSameReceiverTwiceDeliversTwice) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x99);
  deliver_to_receivers(&frame, CAN_NATIVE);

  EXPECT_EQ(receiver.received.size(), 2u) << "the registry does not de-duplicate";
}

// Exactly once per frame, not once per registration of the same object.
TEST_F(CanDispatchTest, DeliveringTwoFramesGivesTwoCallbacksInOrder) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame first = frame_with_id(0x100);
  CAN_frame second = frame_with_id(0x200);
  deliver_to_receivers(&first, CAN_NATIVE);
  deliver_to_receivers(&second, CAN_NATIVE);

  ASSERT_EQ(receiver.received.size(), 2u);
  EXPECT_EQ(receiver.received[0], 0x100u);
  EXPECT_EQ(receiver.received[1], 0x200u);
}

// A bus nobody registered for is not an error; the frame is simply dropped.
TEST_F(CanDispatchTest, FrameOnAnUnregisteredInterfaceIsDroppedQuietly) {
  RecordingReceiver receiver;
  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x321);
  deliver_to_receivers(&frame, CANFD_ADDON_MCP2518_2);

  EXPECT_TRUE(receiver.received.empty());
}

// The same physical chip serves both FD names on some boards, so the two are
// distinct registry entries and must stay independent here.
TEST_F(CanDispatchTest, TheTwoFdInterfacesAreSeparateRegistryEntries) {
  RecordingReceiver on_fd_native;
  RecordingReceiver on_fd_addon;
  register_can_receiver(&on_fd_native, CANFD_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);
  register_can_receiver(&on_fd_addon, CANFD_ADDON_MCP2518, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x555);
  deliver_to_receivers(&frame, CANFD_ADDON_MCP2518);

  EXPECT_TRUE(on_fd_native.received.empty());
  EXPECT_EQ(on_fd_addon.received.size(), 1u);
}

// The frame is passed by pointer, so a receiver sees the real contents rather
// than a copy made before the data was filled in.
TEST_F(CanDispatchTest, ReceiverSeesTheFrameContents) {
  class ContentChecking : public CanReceiver {
   public:
    void receive_can_frame(CAN_frame* frame) override {
      saw_id = frame->ID;
      saw_first_byte = frame->data.u8[0];
      saw_dlc = frame->DLC;
    }
    uint32_t saw_id = 0;
    uint8_t saw_first_byte = 0;
    uint8_t saw_dlc = 0;
  } receiver;

  register_can_receiver(&receiver, CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS);

  CAN_frame frame = frame_with_id(0x7FF);
  frame.data.u8[0] = 0xAB;
  deliver_to_receivers(&frame, CAN_NATIVE);

  EXPECT_EQ(receiver.saw_id, 0x7FFu);
  EXPECT_EQ(receiver.saw_first_byte, 0xAB);
  EXPECT_EQ(receiver.saw_dlc, 8);
}
