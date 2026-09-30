#pragma once

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"

// A pack that reports whatever contactor state the test sets, and nothing else.
class StubPack : public Battery {
 public:
  ContactorState state = ContactorState::ASSUMED_CLOSED;

  void setup() override {}
  void update_values() override {}
  const char* interface_name() override { return "stub"; }
  ContactorState contactor_state() override { return state; }
};

// Installs a stub as battery N (1-3), seen on CAN and still talking. The test listener in
// tests.cpp deletes it before the next test.
inline StubPack* install_stub_pack(uint8_t n) {
  StubPack* pack = new StubPack();
  switch (n) {
    case 1:
      battery = pack;
      battery_detected = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 2:
      battery2 = pack;
      battery2_detected = true;
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      battery3 = pack;
      battery3_detected = true;
      datalayer.battery3.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
  }
  return pack;
}
