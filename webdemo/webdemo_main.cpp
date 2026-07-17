
#include <FreeRTOS.h>
#include <src/battery/BATTERIES.h>
#include <src/charger/CHARGERS.h>
#include <src/devboard/hal/hal.h>
#include <src/devboard/utils/events.h>
#include <src/inverter/INVERTERS.h>
#include <src/lib/TinyWebServer/TinyWebServer.h>
#include <task.h>
#include <csignal>

extern TinyWebServer tinyWebServer;
extern void dump_can_frame2(const CAN_frame& frame, CAN_Interface interface, frameDirection msgDir);
extern bool settingsUpdated;

void rebootHandler(int signum) {
  // Simulated reboot
  set_millis64(UINT64_MAX);
  if (battery3)
    delete battery3;
  if (battery2)
    delete battery2;
  if (battery)
    delete battery;
  battery = nullptr;
  battery2 = nullptr;
  battery3 = nullptr;
  if (esp32hal)
    delete esp32hal;
  init_hal();
  init_events();
  set_event(EVENT_RESET_POWERON, 0);
  init_stored_settings();
  setup_battery();
  setup_inverter();
  settingsUpdated = false;
}

void scheduleReboot(int signum) {
  // Schedule a reboot in 1 second to allow the current request to complete
  //signal(SIGALRM, rebootHandler);
  //alarm(1);
  rebootHandler(0);
}

void webServerTask(void* pvParameters) {
  tiny_web_server_loop(&tinyWebServer);
}

void canFakerTask(void* pvParameters) {
  while (true) {
    // Simulate CAN messages being received every 100ms
    CAN_frame frame;
    frame.ID = 0x123;
    frame.DLC = 8;
    frame.FD = false;
    frame.ext_ID = false;
    dump_can_frame2(frame, CAN_NATIVE, MSG_TX);
    for (int i = 0; i < 8; i++) {
      frame.data.u8[i] = i;
    }

    for (int i = 0; i < 5; i++) {
      frame.data.u8[0] = i;  // Vary the first byte to simulate changing data
      dump_can_frame2(frame, CAN_NATIVE, MSG_RX);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void core_loop(void*) {
  while (true) {
    update_pause_state();

    if (battery)
      battery->update_values();
    if (battery2)
      battery2->update_values();
    if (battery3)
      battery3->update_values();
    if (inverter)
      inverter->update_values();

    unsigned long currentMillis = millis();
    if (battery)
      dynamic_cast<CanBattery*>(battery)->transmit(currentMillis);
    if (battery2)
      dynamic_cast<CanBattery*>(battery2)->transmit(currentMillis);
    if (battery3)
      dynamic_cast<CanBattery*>(battery3)->transmit(currentMillis);
    if (inverter)
      dynamic_cast<CanInverterProtocol*>(inverter)->transmit(currentMillis);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main() {
  signal(SIGUSR1, scheduleReboot);

  rebootHandler(0);

  xTaskCreate(webServerTask, "Webserver", 4096, NULL, 1, NULL);
  //xTaskCreate(canFakerTask, "CANFaker", 4096, NULL, 1, NULL);
  xTaskCreate(core_loop, "CoreLoop", 4096, NULL, 1, NULL);

  vTaskStartScheduler();

  return 0;
}
