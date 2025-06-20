#ifndef SMA_CAN_TRIPOWER_H
#define SMA_CAN_TRIPOWER_H
#include "../include.h"

#include "CanInverterProtocol.h"
#include "src/devboard/hal/hal.h"

#ifdef SMA_TRIPOWER_CAN
#define SELECTED_INVERTER_CLASS SmaTripowerInverter
#endif

class SmaTripowerInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "SMA Tripower CAN";

  virtual bool controls_contactor() { return true; }
  virtual bool allows_contactor_closing() { return digitalRead(INVERTER_CONTACTOR_ENABLE_PIN) == 1; }

 private:
  const int READY_STATE = 0x03;
  const int STOP_STATE = 0x02;
  const int THIRTY_MINUTES = 1200;

  void transmit_can_init();
  void pushFrame(CAN_frame* frame, std::function<void(void)> callback = []() {});
  void completePairing();

  unsigned long previousMillis250ms = 0;  // will store last time a 250ms CAN Message was send
  unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send
  unsigned long previousMillis2s = 0;     // will store last time a 2s CAN Message was send
  unsigned long previousMillis10s = 0;    // will store last time a 10s CAN Message was send
  unsigned long previousMillis60s = 0;    // will store last time a 60s CAN Message was send

  typedef struct {
    CAN_frame* frame;
    std::function<void(void)> callback;
  } Frame;

  unsigned short listLength = 0;
  Frame framesToSend[20];

  uint32_t inverter_time = 0;
  uint16_t inverter_voltage = 0;
  int16_t inverter_current = 0;
  uint8_t pairing_events = 0;
  bool pairing_completed = false;
  int16_t temperature_average = 0;
  uint16_t ampere_hours_remaining = 0;
  uint16_t timeWithoutInverterAllowsContactorClosing = 0;

  //Actual content messages
  CAN_frame SMA_358 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x358,
                       .data = {0x12, 0x40, 0x0C, 0x80, 0x01, 0x00, 0x01, 0x00}};
  CAN_frame SMA_3D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3D8,
                       .data = {0x04, 0x06, 0x27, 0x10, 0x00, 0x19, 0x00, 0xFA}};
  CAN_frame SMA_458 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x458,
                       .data = {0x00, 0x00, 0x73, 0xAE, 0x00, 0x00, 0x64, 0x64}};
  CAN_frame SMA_4D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4D8,
                       .data = {0x10, 0x62, 0x00, 0x00, 0x00, 0x78, 0x02, 0x08}};
  CAN_frame SMA_518 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x518,
                       .data = {0x00, 0x96, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_558 = {.FD = false,  //Pairing first message
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x558,  // BYD HVS 10.2 kWh (0x66 might be kWh)
                       .data = {0x03, 0x24, 0x00, 0x04, 0x00, 0x66, 0x04, 0x09}};  //Amount of modules? Vendor ID?
  CAN_frame SMA_598 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x598,
                       .data = {0x12, 0xD6, 0x43, 0xA4, 0x00, 0x00, 0x00, 0x00}};  //B0-4 Serial 301100932
  CAN_frame SMA_5D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x5D8,
                       .data = {0x00, 0x42, 0x59, 0x44, 0x00, 0x00, 0x00, 0x00}};  //B Y D
  CAN_frame SMA_618_0 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x618,
                         .data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79}};  //BATTERY
  CAN_frame SMA_618_1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x618,
                         .data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x50, 0x72}};  //-Box Pr
  CAN_frame SMA_618_2 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x618,
                         .data = {0x02, 0x65, 0x6D, 0x69, 0x75, 0x6D, 0x20, 0x48}};  //emium H
  CAN_frame SMA_618_3 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x618,
                         .data = {0x03, 0x56, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00}};  //VS
};

#endif
