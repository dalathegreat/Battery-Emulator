#ifndef HYUNDAI_IONIQ_28_BATTERY_H
#define HYUNDAI_IONIQ_28_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "HYUNDAI-IONIQ-28-BATTERY-HTML.h"

class HyundaiIoniq28Battery : public CanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  HyundaiIoniq28Battery() : renderer(*this) { datalayer_battery = &datalayer.battery; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "Hyundai Ioniq Electric 28kWh";

  // Getter methods for HTML renderer
  uint16_t get_lead_acid_voltage() const;
  uint16_t get_isolation_resistance() const;
  int16_t get_power_relay_temperature() const;
  uint8_t get_battery_management_mode() const;

 private:
  HyundaiIoniq28BatteryHtmlRenderer renderer;

  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 4050;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2880;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2950;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis250 = 0;  // will store last time a 250ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10 = 0;   // will store last time a 10s CAN Message was send

  uint16_t SOC_BMS = 0;
  uint16_t SOC_Display = 0;
  uint16_t batterySOH = 1000;
  uint16_t CellVoltMax_mV = 3700;
  uint16_t CellVoltMin_mV = 3700;
  uint16_t allowedDischargePower = 0;
  uint16_t allowedChargePower = 0;
  uint16_t batteryVoltage = 3700;
  uint16_t inverterVoltage = 0;
  uint16_t isolation_resistance = 1000;
  uint16_t cellvoltages_mv[96];
  uint16_t leadAcidBatteryVoltage = 120;
  int16_t batteryAmps = 0;
  int16_t temperatureMax = 0;
  int16_t temperatureMin = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t counter_200 = 0;
  int8_t heatertemperature_1 = 0;
  int8_t heatertemperature_2 = 0;
  int8_t powerRelayTemperature = 0;
  bool startedUp = false;
  uint8_t incoming_poll_group = 0xFF;
  uint8_t poll_group = 0;

  CAN_frame IONIQ_200 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x200,
                         .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};
  CAN_frame IONIQ_523 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x523,
                         .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}};
  CAN_frame IONIQ_524 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x524,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //553 Needed frame 200ms
  CAN_frame IONIQ_553 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x553,
                         .data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
  //57F Needed frame 100ms
  CAN_frame IONIQ_57F = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x57F,
                         .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
  //Needed frame 100ms
  CAN_frame IONIQ_2A1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A1,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame IONIQ_7E4_POLL = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E4,
                              .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame IONIQ_7E4_ACK = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
