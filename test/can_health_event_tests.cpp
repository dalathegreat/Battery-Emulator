#include <gtest/gtest.h>

#include "../Software/src/communication/can/comm_can_device.h"
#include "../Software/src/communication/can/comm_can_dispatch.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"

#include "Arduino.h"

// Set by the test to model a board's interface -> physical device mapping.
namespace {

// A board whose "native" FD interface is the same chip as the first FD add-on
// (device 2), plus a second FD chip (device 3) - the Stark/BECom shape.
// A device is only bookkeeping to these tests: a slot, a pair of event ids, and
// a transmit that can be told to refuse.
class FakeCanDevice : public CanDevice {
 public:
  // The slot passed here is a placeholder: register_device() overwrites it
  // with the registration order, which is the behaviour under test below.
  FakeCanDevice(uint8_t slot, EVENTS_ENUM_TYPE full, EVENTS_ENUM_TYPE bus) {
    device_index = slot;
    buffer_full_event = full;
    bus_error_event = bus;
  }
  bool init(CAN_Speed) override { return true; }
  bool try_send(const CAN_frame& frame) override {
    sent.push_back(frame.ID);
    return !refuse_sends;
  }
  void poll_receive() override {}
  void stop() override {}
  void restart() override {}

  std::vector<uint32_t> sent;
  bool refuse_sends = false;
};

// reset_all_events() does not clear the ignore windows, so a test that opened
// one would leak it into the next. Closing them takes a zero-length window.
void closeAllIgnoreWindows() {
  for (int i = 0; i < NO_CAN_INTERFACE; ++i) {
    ignore_can_errors_for(static_cast<CAN_Interface>(i), 0);
  }
}

// A board whose "native" FD interface is the same chip as the first FD add-on,
// plus a second FD chip - the Stark/BECom shape. Built through the production
// mapping API so the tests and the firmware share one table.
FakeCanDevice* g_board[4];

void mapSharedFdBoard() {
  static FakeCanDevice native(0, EVENT_CAN_NATIVE_BUFFER_FULL, EVENT_CAN_NATIVE_BUS_ERROR);
  static FakeCanDevice mcp(1, EVENT_CANMCP2515_BUFFER_FULL, EVENT_CANMCP2515_BUS_ERROR);
  static FakeCanDevice fd1(2, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  static FakeCanDevice fd2(3, EVENT_CANFD_2_BUFFER_FULL, EVENT_CANFD_2_BUS_ERROR);
  clear_can_devices();
  register_device(&native);
  register_device(&mcp);
  register_device(&fd1);
  register_device(&fd2);
  map_interface_to_device(CAN_NATIVE, &native);
  map_interface_to_device(CAN_ADDON_MCP2515, &mcp);
  map_interface_to_device(CANFD_NATIVE, &fd1);
  map_interface_to_device(CANFD_ADDON_MCP2518, &fd1);
  map_interface_to_device(CANFD_ADDON_MCP2518_2, &fd2);
  g_board[0] = &native;
  g_board[1] = &mcp;
  g_board[2] = &fd1;
  g_board[3] = &fd2;
}

constexpr uint8_t kFd1 = 1 << 2;
constexpr uint8_t kFd2 = 1 << 3;

}  // namespace

// Both FD chips report through one event, so an ignore window over one of them
// must not silence the other. Before health events were keyed by device, a
// window on the first chip suppressed the second chip's faults as well.
TEST(CanHealthEventTests, IgnoreWindowSuppressesOnlyItsOwnDevice) {
  reset_all_events();
  mapSharedFdBoard();
  set_millis64(10000);
  closeAllIgnoreWindows();

  ignore_can_errors_for(CANFD_ADDON_MCP2518, 1000);  // device 2 only

  // Both chips are reporting a full buffer in the same event.
  set_event(EVENT_CANFD_BUFFER_FULL, kFd1 | kFd2);

  const EVENTS_STRUCT_TYPE* event = get_event_pointer(EVENT_CANFD_BUFFER_FULL);
  EXPECT_EQ(event->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event->data, kFd2) << "the ignored chip should be dropped from the payload, the other kept";
}

// With every device it names inside a window, the event has nothing left to
// report and is dropped entirely - the original point of the ignore window.
TEST(CanHealthEventTests, IgnoreWindowOverAllNamedDevicesDropsTheEvent) {
  reset_all_events();
  mapSharedFdBoard();
  set_millis64(10000);
  closeAllIgnoreWindows();

  ignore_can_errors_for(CANFD_ADDON_MCP2518, 1000);    // device 2
  ignore_can_errors_for(CANFD_ADDON_MCP2518_2, 1000);  // device 3

  set_event(EVENT_CANFD_BUFFER_FULL, kFd1 | kFd2);

  EXPECT_NE(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->state, EVENT_STATE_ACTIVE);
}

// A window opened on either logical name of a shared controller must reach it:
// the suppression is a property of the chip, not of the name used to ask.
TEST(CanHealthEventTests, EitherLogicalNameOpensTheWindowOnTheSharedChip) {
  reset_all_events();
  mapSharedFdBoard();
  set_millis64(10000);
  closeAllIgnoreWindows();

  ignore_can_errors_for(CANFD_NATIVE, 1000);  // the other name for device 2

  set_event(EVENT_CANFD_BUS_ERROR, kFd1);

  EXPECT_NE(get_event_pointer(EVENT_CANFD_BUS_ERROR)->state, EVENT_STATE_ACTIVE);
}

// Once the window has passed the events must come back.
TEST(CanHealthEventTests, EventsReturnAfterTheWindowExpires) {
  reset_all_events();
  mapSharedFdBoard();
  set_millis64(10000);
  closeAllIgnoreWindows();

  ignore_can_errors_for(CANFD_ADDON_MCP2518, 1000);
  set_millis64(11500);  // window closed

  set_event(EVENT_CANFD_BUFFER_FULL, kFd1);

  EXPECT_EQ(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->data, kFd1);
}

// Init-failure payloads are controller error codes, not device bitmasks, so
// they must pass through the health-event masking untouched.
TEST(CanHealthEventTests, InitFailurePayloadIsNotTreatedAsADeviceMask) {
  reset_all_events();
  mapSharedFdBoard();
  set_millis64(10000);
  closeAllIgnoreWindows();

  ignore_can_errors_for(CANFD_ADDON_MCP2518, 1000);

  set_event(EVENT_CANMCP2518FD_INIT_FAILURE, 0x04);  // error code that looks like device 2

  const EVENTS_STRUCT_TYPE* event = get_event_pointer(EVENT_CANMCP2518FD_INIT_FAILURE);
  EXPECT_EQ(event->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event->data, 0x04) << "an init error code must survive the ignore window intact";
}

// --- The aggregation itself ------------------------------------------------
//
// Until now these tests only covered the ignore-window masking in events.cpp;
// update_can_health_events() was stubbed out of the test build, so the function
// that actually decides whether a shared event is raised had no direct test.

namespace {

// A device is only bookkeeping to this function: a slot index and a pair of
// event ids.

class CanHealthAggregationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    clear_can_devices();
    reset_all_events();
    closeAllIgnoreWindows();
    datalayer = DataLayer();
    set_millis64(50000);
  }
  void TearDown() override { clear_can_devices(); }
};

}  // namespace

// The defect the aggregation exists to fix: two devices sharing one event pair,
// one faulty and one healthy. Deciding per device let the healthy one clear
// what the faulty one had just raised.
TEST_F(CanHealthAggregationTest, HealthySiblingDoesNotClearAFaultySiblingsEvent) {
  FakeCanDevice fd1(2, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  FakeCanDevice fd2(3, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  ASSERT_TRUE(register_device(&fd1));
  ASSERT_TRUE(register_device(&fd2));

  datalayer.system.info.can_device[fd1.device_index].send_fail = true;
  datalayer.system.info.can_device[fd2.device_index].send_fail = false;

  update_can_health_events();

  const EVENTS_STRUCT_TYPE* entry = get_event_pointer(EVENT_CANFD_BUFFER_FULL);
  EXPECT_EQ(entry->state, EVENT_STATE_ACTIVE) << "the healthy sibling must not clear the faulty one's event";
  EXPECT_EQ(entry->data, 1 << fd1.device_index) << "the payload names the faulty device";
}

// Both faulty: the payload names both, which a single device index could not
// express.
TEST_F(CanHealthAggregationTest, PayloadNamesEveryFaultyDeviceSharingTheEvent) {
  FakeCanDevice fd1(2, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  FakeCanDevice fd2(3, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  register_device(&fd1);
  register_device(&fd2);

  datalayer.system.info.can_device[fd1.device_index].send_fail = true;
  datalayer.system.info.can_device[fd2.device_index].send_fail = true;

  update_can_health_events();

  EXPECT_EQ(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->data, (1 << fd1.device_index) | (1 << fd2.device_index));
}

// The flags are consumed, so a fault reported once does not keep the event
// raised for ever.
TEST_F(CanHealthAggregationTest, FlagsAreConsumedAndTheEventClearsWhenAllAreHealthy) {
  FakeCanDevice fd1(2, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR);
  register_device(&fd1);

  datalayer.system.info.can_device[fd1.device_index].send_fail = true;
  update_can_health_events();
  ASSERT_EQ(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->state, EVENT_STATE_ACTIVE);
  EXPECT_FALSE(datalayer.system.info.can_device[fd1.device_index].send_fail) << "the flag must be consumed";

  update_can_health_events();
  EXPECT_NE(get_event_pointer(EVENT_CANFD_BUFFER_FULL)->state, EVENT_STATE_ACTIVE);
}

// Devices with their own event pair are independent of each other.
TEST_F(CanHealthAggregationTest, DevicesWithSeparateEventsDoNotInterfere) {
  FakeCanDevice native(0, EVENT_CAN_NATIVE_BUFFER_FULL, EVENT_CAN_NATIVE_BUS_ERROR);
  FakeCanDevice mcp(1, EVENT_CANMCP2515_BUFFER_FULL, EVENT_CANMCP2515_BUS_ERROR);
  register_device(&native);
  register_device(&mcp);

  datalayer.system.info.can_device[native.device_index].bus_error = true;

  update_can_health_events();

  EXPECT_EQ(get_event_pointer(EVENT_CAN_NATIVE_BUS_ERROR)->state, EVENT_STATE_ACTIVE);
  EXPECT_NE(get_event_pointer(EVENT_CANMCP2515_BUS_ERROR)->state, EVENT_STATE_ACTIVE);
}

// Registration order is what assigns the datalayer slot, and the limit is a
// hard one - a device beyond it has nowhere to report its health.
TEST_F(CanHealthAggregationTest, RegistrationOrderAssignsSlotsAndTheLimitIsEnforced) {
  FakeCanDevice a(99, EVENT_CAN_NATIVE_BUFFER_FULL, EVENT_CAN_NATIVE_BUS_ERROR);
  FakeCanDevice b(99, EVENT_CANMCP2515_BUFFER_FULL, EVENT_CANMCP2515_BUS_ERROR);
  ASSERT_TRUE(register_device(&a));
  ASSERT_TRUE(register_device(&b));

  EXPECT_EQ(a.device_index, 0) << "the slot comes from registration order, not from the device";
  EXPECT_EQ(b.device_index, 1);

  FakeCanDevice extra[MAX_CAN_DEVICES] = {
      {99, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR},
      {99, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR},
      {99, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR},
      {99, EVENT_CANFD_BUFFER_FULL, EVENT_CANFD_BUS_ERROR},
  };
  bool refused = false;
  for (int i = 0; i < MAX_CAN_DEVICES; ++i) {
    if (!register_device(&extra[i])) {
      refused = true;
    }
  }
  EXPECT_TRUE(refused) << "registering more devices than health slots must be refused, not overflow";
}

// --- Transmit routing ------------------------------------------------------
//
// The logical->physical lookup on the transmit path was the last piece of the
// device table with no coverage: the host build used to shortcut
// transmit_can_frame_to_interface entirely, so a device_for[] wired to the
// wrong device would have passed the whole suite. Routing is bookkeeping - a
// lookup, a try_send and a flag - so it belongs here rather than on a bench.

namespace {

class CanTxRoutingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    reset_all_events();
    datalayer = DataLayer();
    set_millis64(50000);
    mapSharedFdBoard();
    closeAllIgnoreWindows();
    for (FakeCanDevice* d : g_board) {
      d->sent.clear();
      d->refuse_sends = false;
    }
  }
  void TearDown() override { clear_can_devices(); }

  static CAN_frame frame(uint32_t id) {
    CAN_frame f = {};
    f.ID = id;
    f.DLC = 8;
    return f;
  }
};

}  // namespace

TEST_F(CanTxRoutingTest, FrameReachesTheDeviceBackingItsInterface) {
  CAN_frame f = frame(0x2A0);

  EXPECT_TRUE(route_frame_to_device(f, CAN_ADDON_MCP2515));

  EXPECT_TRUE(g_board[0]->sent.empty()) << "the native device must not see it";
  ASSERT_EQ(g_board[1]->sent.size(), 1u);
  EXPECT_EQ(g_board[1]->sent[0], 0x2A0u);
}

// The case this refactor most had to get right: both FD names are one chip, so
// a frame on either must reach that chip, once.
TEST_F(CanTxRoutingTest, BothSharedFdNamesReachTheSameDeviceOnce) {
  route_frame_to_device(frame(0x100), CANFD_NATIVE);
  route_frame_to_device(frame(0x200), CANFD_ADDON_MCP2518);

  ASSERT_EQ(g_board[2]->sent.size(), 2u) << "both logical names route to the one FD chip";
  EXPECT_EQ(g_board[2]->sent[0], 0x100u);
  EXPECT_EQ(g_board[2]->sent[1], 0x200u);
  EXPECT_TRUE(g_board[3]->sent.empty()) << "the second FD chip must not receive them";
}

TEST_F(CanTxRoutingTest, SecondFdChipIsAddressedSeparately) {
  route_frame_to_device(frame(0x300), CANFD_ADDON_MCP2518_2);

  EXPECT_TRUE(g_board[2]->sent.empty());
  ASSERT_EQ(g_board[3]->sent.size(), 1u);
  EXPECT_EQ(g_board[3]->sent[0], 0x300u);
}

// A bus this board does not have is a quiet drop, not a crash and not a flag.
TEST_F(CanTxRoutingTest, UnmappedInterfaceDropsQuietlyAndFlagsNothing) {
  map_interface_to_device(CANFD_ADDON_MCP2518_2, nullptr);

  EXPECT_FALSE(route_frame_to_device(frame(0x400), CANFD_ADDON_MCP2518_2));

  for (int i = 0; i < MAX_CAN_DEVICES; ++i) {
    EXPECT_FALSE(datalayer.system.info.can_device[i].send_fail) << "slot " << i << " was flagged by a dropped frame";
  }
}

// A refused send flags the device that refused it - in its own slot, and not a
// sibling's. This is what feeds the shared buffer-full event.
TEST_F(CanTxRoutingTest, RefusedSendFlagsThatDeviceAndNotItsSibling) {
  g_board[2]->refuse_sends = true;

  EXPECT_TRUE(route_frame_to_device(frame(0x500), CANFD_ADDON_MCP2518));

  EXPECT_TRUE(datalayer.system.info.can_device[g_board[2]->device_index].send_fail);
  EXPECT_FALSE(datalayer.system.info.can_device[g_board[3]->device_index].send_fail)
      << "the sibling FD chip must not be blamed for the other's failure";
}

// End to end with the aggregation: a refused send must actually surface as the
// shared event, naming the device that failed.
TEST_F(CanTxRoutingTest, ARefusedSendSurfacesAsTheSharedEventNamingTheDevice) {
  g_board[2]->refuse_sends = true;
  route_frame_to_device(frame(0x600), CANFD_NATIVE);

  update_can_health_events();

  const EVENTS_STRUCT_TYPE* entry = get_event_pointer(EVENT_CANFD_BUFFER_FULL);
  EXPECT_EQ(entry->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(entry->data, 1 << g_board[2]->device_index);
}

TEST_F(CanTxRoutingTest, ASuccessfulSendFlagsNothing) {
  route_frame_to_device(frame(0x700), CAN_NATIVE);

  for (int i = 0; i < MAX_CAN_DEVICES; ++i) {
    EXPECT_FALSE(datalayer.system.info.can_device[i].send_fail);
  }
}

// --- Bus-off recovery ------------------------------------------------------
//
// What the TWAI status register means, and what to do about it. Reading the
// register needs silicon; this decision does not, and it is the part that
// matters: a controller that goes bus-off stops participating until it is
// re-initialised, so failing to recover is a silent loss of the whole bus.

TEST(CanBusStatusTest, BusOffBothRecoversAndReportsTheError) {
  const CanBusStatusAction action = evaluate_twai_status(kTwaiBusOffStatusBit);

  EXPECT_TRUE(action.reinitialise) << "a bus-off controller never rejoins on its own";
  EXPECT_TRUE(action.flag_bus_error);
}

// Error-warning state means the counters are high but the controller is still
// carrying traffic. Resetting it here would drop a working bus.
TEST(CanBusStatusTest, ErrorStateIsReportedWithoutResettingTheController) {
  const CanBusStatusAction action = evaluate_twai_status(kTwaiErrorStatusBit);

  EXPECT_FALSE(action.reinitialise) << "a working controller must not be reset out from under its traffic";
  EXPECT_TRUE(action.flag_bus_error);
}

TEST(CanBusStatusTest, BothBitsSetRecoversAndReports) {
  const CanBusStatusAction action = evaluate_twai_status(kTwaiBusOffStatusBit | kTwaiErrorStatusBit);

  EXPECT_TRUE(action.reinitialise);
  EXPECT_TRUE(action.flag_bus_error);
}

TEST(CanBusStatusTest, AHealthyControllerNeedsNoAction) {
  const CanBusStatusAction action = evaluate_twai_status(0);

  EXPECT_FALSE(action.reinitialise);
  EXPECT_FALSE(action.flag_bus_error);
}

// The register carries other state - receive/transmit buffer status and so on.
// Only the two bits above may provoke a reset or an error report.
TEST(CanBusStatusTest, UnrelatedStatusBitsAreIgnored) {
  const uint32_t others = ~(kTwaiBusOffStatusBit | kTwaiErrorStatusBit);

  const CanBusStatusAction action = evaluate_twai_status(others);

  EXPECT_FALSE(action.reinitialise) << "an unrelated status bit must not reset the controller";
  EXPECT_FALSE(action.flag_bus_error);
}
