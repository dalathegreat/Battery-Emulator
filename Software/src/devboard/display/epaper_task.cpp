#include "epaper_task.h"
#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <time.h>
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../inverter/InverterProtocol.h"
#include "../hal/hal.h"
#include "ui_dashboard.h"

extern DataLayer datalayer;
extern const char* version_number;

// ========================================================
// 🔴 Variables for 3-Color Display (B/W/Red)
// ========================================================
unsigned long last_3c_update_time = 0;
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT>* display3C = nullptr;

// ========================================================
// ⚪ Variables for Black & White Display (Fast Refresh)
// ========================================================
unsigned long last_bw_update_time = 0;
int bw_partial_refresh_count = 0;  // Cycle counter to clear screen ghosting (Anti-Ghosting)
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>* displayBW = nullptr;

// ========================================================
// 🔴 Setup & Update: 3-Color Display (B/W/Red)
// ========================================================
void setupEpaper3C() {
  if (esp32hal->EPD_CS_PIN() == GPIO_NUM_NC)
    return;

  Serial.println("E-Paper 3C: Initializing...");
  pinMode(esp32hal->EPD_CS_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_DC_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_RST_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_BUSY_PIN(), INPUT_PULLUP);

  display3C = new GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT>(
      GxEPD2_420c(esp32hal->EPD_CS_PIN(), esp32hal->EPD_DC_PIN(), esp32hal->EPD_RST_PIN(), esp32hal->EPD_BUSY_PIN()));

  SPI.end();
  SPI.begin(esp32hal->EPD_SCK_PIN(), -1, esp32hal->EPD_MOSI_PIN(), -1);

  display3C->init(115200, true, 10, false);
  display3C->setRotation(0);

  display3C->setFullWindow();

  BatteryEmulatorSettingsStore settings;
  if (settings.getBool("EPAPREFRESHBTN", false)) {
    if (esp32hal->EPD_REFRESH_BTN_PIN() != GPIO_NUM_NC) {
      pinMode(esp32hal->EPD_REFRESH_BTN_PIN(), INPUT_PULLUP);
    }
    Serial.println("E-Paper 3C: Refresh Button on Pin 40 ENABLED.");
  }

  last_3c_update_time = 0;
  Serial.println("E-Paper 3C: Setup HW done.");
}

void updateEpaper3CDisplay() {
  if (display3C == nullptr)
    return;
  unsigned long currentMillis = millis();
  BatteryEmulatorSettingsStore settings;
  bool btn_enabled = settings.getBool("EPAPREFRESHBTN", false);

  // 3-Color display is slow (10-15s), so update only on button press or every 1 hour.
  bool buttonPressed = false;
  if (esp32hal->EPD_REFRESH_BTN_PIN() != GPIO_NUM_NC && btn_enabled) {
    buttonPressed = (digitalRead(esp32hal->EPD_REFRESH_BTN_PIN()) == LOW);
  }
  // bool buttonPressed = (digitalRead(40) == LOW);
  bool isFirstBoot = (last_3c_update_time == 0);
  bool isOneHourPassed = (currentMillis - last_3c_update_time >= 3600000);

  if (!isFirstBoot && !isOneHourPassed && !buttonPressed) {
    esp_task_wdt_reset();
    return;
  }
  if (buttonPressed && !isFirstBoot && (currentMillis - last_3c_update_time < 30000)) {
    esp_task_wdt_reset();
    return;  // Prevent rapid button pressing (Cooldown)
  }

  Serial.println("E-Paper 3C: Drawing UI...");
  display3C->setFullWindow();
  display3C->firstPage();
  do {
    drawSharedDashboard(display3C, false);
  } while (display3C->nextPage());

  last_3c_update_time = millis();
  esp_task_wdt_reset();
}

// ========================================================
// ⚪ Setup & Update: 2-Color Display (B/W) Fast Refresh
// ========================================================
void setupEpaperBW() {
  if (esp32hal->EPD_CS_PIN() == GPIO_NUM_NC)
    return;

  Serial.println("E-Paper BW: Initializing Fast Refresh Mode...");
  pinMode(esp32hal->EPD_CS_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_DC_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_RST_PIN(), OUTPUT);
  pinMode(esp32hal->EPD_BUSY_PIN(), INPUT_PULLUP);

  displayBW = new GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>(
      GxEPD2_420(esp32hal->EPD_CS_PIN(), esp32hal->EPD_DC_PIN(), esp32hal->EPD_RST_PIN(), esp32hal->EPD_BUSY_PIN()));

  SPI.end();
  SPI.begin(esp32hal->EPD_SCK_PIN(), -1, esp32hal->EPD_MOSI_PIN(), -1);

  displayBW->init(115200, true, 10, false);
  displayBW->setRotation(0);

  // Clear the screen to solid white once on boot
  displayBW->setFullWindow();
  displayBW->fillScreen(GxEPD_WHITE);
  displayBW->display(false);

  last_bw_update_time = 0;
  bw_partial_refresh_count = 0;
  Serial.println("E-Paper BW: Setup HW done.");
}

void updateEpaperBWDisplay() {
  if (displayBW == nullptr)
    return;
  unsigned long currentMillis = millis();

  // ⚡ B/W display is very fast, allow updating every 2 seconds (Real-time)
  if (currentMillis - last_bw_update_time < 2000 && last_bw_update_time != 0) {
    esp_task_wdt_reset();
    return;
  }

  // 🧹 Anti-Ghosting System: Force a full screen redraw (Full Refresh) every 30 cycles
  // To prevent ink ghosting from rapid Partial Refreshes
  if (bw_partial_refresh_count == 0 || bw_partial_refresh_count >= 30) {
    displayBW->setFullWindow();
    displayBW->firstPage();
    do {
      displayBW->fillScreen(GxEPD_WHITE);
      drawSharedDashboard(displayBW, false);
    } while (displayBW->nextPage());

    bw_partial_refresh_count = 1;  // Reset counter
  } else {
    // ⚡ Partial Refresh Mode: Update only changed areas, screen will not flash black
    // Setting full screen size in Partial mode lets the library automatically calculate the Differential Update!
    displayBW->setPartialWindow(0, 0, displayBW->width(), displayBW->height());
    displayBW->firstPage();
    do {
      displayBW->fillScreen(GxEPD_WHITE);
      drawSharedDashboard(displayBW, false);
    } while (displayBW->nextPage());

    bw_partial_refresh_count++;
  }

  last_bw_update_time = millis();
  esp_task_wdt_reset();
}
