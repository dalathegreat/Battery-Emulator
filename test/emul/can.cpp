#include "emul_can.h"

#include <vector>

#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"
#include "../../Software/src/communication/can/comm_can_device.h"
#include "../../Software/src/communication/can/comm_can_dispatch.h"

// Records every frame the firmware actually put on the wire (UDS requests,
// ISO-TP flow control / consecutive frames, heartbeat frames, ...).
std::vector<CAN_frame> g_emul_transmitted_frames;

void clear_transmitted_frames() {
  g_emul_transmitted_frames.clear();
}

const std::vector<CAN_frame>& get_transmitted_frames() {
  return g_emul_transmitted_frames;
}

// The emulation boundary sits at the controller, not at the transmit function.
// transmit_can_frame_to_interface(), its allowed_to_send_CAN gate, the frame
// logging and the interface->device routing are all real firmware code in the
// test build; the only thing faked is the part that genuinely needs silicon.
//
// Before this, emul/can.cpp replaced the whole transmit function with a
// push_back, so none of that code ran in any test and deleting the routing call
// from the firmware would not have failed a single one.
class RecordingCanDevice : public CanDevice {
 public:
  RecordingCanDevice() {
    name = "emulated";
    initialized = true;
  }

  bool init(CAN_Speed speed) override {
    initialized = true;
    return true;
  }

  bool try_send(const CAN_frame& frame) override {
    if (refuse_sends) {
      return false;
    }
    g_emul_transmitted_frames.push_back(frame);
    return true;
  }

  void poll_receive() override {}
  void stop() override {}
  void restart() override {}
  bool change_speed(CAN_Speed speed) override { return true; }

  // Lets a test make the controller reject a frame the way a full TX buffer
  // would, without having to build its own device.
  bool refuse_sends = false;
};

static RecordingCanDevice emul_device;

void emul_install_can_devices() {
  clear_can_devices();
  emul_device.refuse_sends = false;
  register_device(&emul_device);
  for (int i = 0; i < NO_CAN_INTERFACE; ++i) {
    map_interface_to_device(static_cast<CAN_Interface>(i), &emul_device);
  }
}

void emul_set_can_send_refused(bool refused) {
  emul_device.refuse_sends = refused;
}

uint8_t emul_can_device_index() {
  return emul_device.device_index;
}

// Wired before every test rather than once at startup: the device table is a
// global, and the tests that build their own board leave it holding devices
// that are out of scope by the time the next test transmits. Re-installing per
// test makes that unobservable instead of order-dependent.
//
// OnTestStart runs before the fixture's SetUp, so a fixture that maps its own
// devices still wins.
namespace {
class EmulCanListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo&) override { emul_install_can_devices(); }
};

const bool listener_registered = [] {
  ::testing::UnitTest::GetInstance()->listeners().Append(new EmulCanListener());
  return true;
}();
}  // namespace

// register_can_receiver, the fan-out, the device registry, the health-event
// aggregation, the transmit path and the frame logging all come from the real
// comm_can_dispatch.cpp now.

// The logical->physical map is no longer modelled here: comm_can_dispatch.cpp
// owns the real one and is in the test build, so tests wire a board with
// map_interface_to_device() and there is a single source of truth.

bool change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  return true;
}

void stop_can() {}

void restart_can() {}

char const* getCANInterfaceName(CAN_Interface) {
  return "Foobar";
}

void register_transmitter(Transmitter* transmitter) {}
