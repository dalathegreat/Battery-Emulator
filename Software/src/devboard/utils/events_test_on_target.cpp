#include "events.h"
#include "timer.h"

typedef enum {
  ETOT_INIT,
  ETOT_FIRST_WAIT,
  ETOT_INFO,
  ETOT_INFO_CLEAR,
  ETOT_DEBUG,
  ETOT_DEBUG_CLEAR,
  ETOT_WARNING,
  ETOT_WARNING_CLEAR,
  ETOT_ERROR,
  ETOT_ERROR_CLEAR,
  ETOT_ERROR_LATCHED,
  ETOT_DONE
} ETOT_TYPE;

MyTimer timer(5000);
ETOT_TYPE events_test_state = ETOT_INIT;

void run_sequence_on_target(void) {
#ifdef INCLUDE_EVENTS_TEST
  switch (events_test_state) {
    case ETOT_INIT:
      timer.set_interval(10000);
      events_test_state = ETOT_FIRST_WAIT;
      Serial.println("Events test: initialized");
      Serial.print("datalayer.battery.status.bms_status: ");
      Serial.println(datalayer.battery.status.bms_status);
      break;
    case ETOT_FIRST_WAIT:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        events_test_state = ETOT_INFO;
        set_event(EVENT_DUMMY_INFO, 123);
        set_event(EVENT_DUMMY_INFO, 234);  // 234 should show, occurrence 1
        Serial.println("Events test: info event set, data: 234");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_INFO:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        clear_event(EVENT_DUMMY_INFO);
        events_test_state = ETOT_INFO_CLEAR;
        Serial.println("Events test : info event cleared");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_INFO_CLEAR:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        events_test_state = ETOT_DEBUG;
        set_event(EVENT_DUMMY_DEBUG, 111);
        set_event(EVENT_DUMMY_DEBUG, 222);  // 222 should show, occurrence 1
        Serial.println("Events test : debug event set, data: 222");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_DEBUG:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        clear_event(EVENT_DUMMY_DEBUG);
        events_test_state = ETOT_DEBUG_CLEAR;
        Serial.println("Events test : info event cleared");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_DEBUG_CLEAR:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        events_test_state = ETOT_WARNING;
        set_event(EVENT_DUMMY_WARNING, 234);
        set_event(EVENT_DUMMY_WARNING, 121);  // 121 should show, occurrence 1
        Serial.println("Events test : warning event set, data: 121");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_WARNING:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        clear_event(EVENT_DUMMY_WARNING);
        events_test_state = ETOT_WARNING_CLEAR;
        Serial.println("Events test : warning event cleared");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_WARNING_CLEAR:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        events_test_state = ETOT_ERROR;
        set_event(EVENT_DUMMY_ERROR, 221);
        set_event(EVENT_DUMMY_ERROR, 133);  // 133 should show, occurrence 1
        Serial.println("Events test : error event set, data: 133");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_ERROR:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        clear_event(EVENT_DUMMY_ERROR);
        events_test_state = ETOT_ERROR_CLEAR;
        Serial.println("Events test : error event cleared");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_ERROR_CLEAR:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        events_test_state = ETOT_ERROR_LATCHED;
        set_event_latched(EVENT_DUMMY_ERROR, 221);
        set_event_latched(EVENT_DUMMY_ERROR, 133);  // 133 should show, occurrence 1
        Serial.println("Events test : latched error event set, data: 133");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_ERROR_LATCHED:
      if (timer.elapsed()) {
        timer.set_interval(8000);
        clear_event(EVENT_DUMMY_ERROR);
        events_test_state = ETOT_DONE;
        Serial.println("Events test : latched error event cleared?");
        Serial.print("datalayer.battery.status.bms_status: ");
        Serial.println(datalayer.battery.status.bms_status);
      }
      break;
    case ETOT_DONE:
    default:
      break;
  }
#endif
}
