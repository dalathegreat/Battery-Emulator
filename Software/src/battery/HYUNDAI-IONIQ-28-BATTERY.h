#ifndef HYUNDAI_IONIQ_28_BATTERY_H
#define HYUNDAI_IONIQ_28_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../include.h"
#include "CanBattery.h"
#include "HYUNDAI-IONIQ-28-BATTERY-HTML.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS HyundaiIoniq28Battery

class HyundaiIoniq28Battery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  HyundaiIoniq28Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_IONIQ28* extended_ptr,
                        bool* contactor_closing_allowed_ptr, CAN_Interface targetCan)
      : CanBattery(targetCan), renderer(extended_ptr) {
    datalayer_battery = datalayer_ptr;
    contactor_closing_allowed = contactor_closing_allowed_ptr;
    allows_contactor_closing = nullptr;
    datalayer_battery_extended = extended_ptr;
  }

  // Use the default constructor to create the first or single battery.
  HyundaiIoniq28Battery() : renderer(&datalayer_extended.ioniq28) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    contactor_closing_allowed = nullptr;
    datalayer_battery_extended = &datalayer_extended.ioniq28;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Hyundai Ioniq Electric 28kWh";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  HyundaiIoniq28BatteryHtmlRenderer renderer;

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_IONIQ28* datalayer_battery_extended;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  // If not null, this battery listens to this boolean to determine whether contactor closing is allowed
  bool* contactor_closing_allowed;

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
  uint16_t inverterVoltageFrameHigh = 0;
  uint16_t inverterVoltage = 0;
  uint16_t cellvoltages_mv[96];
  int16_t leadAcidBatteryVoltage = 120;
  int16_t batteryAmps = 0;
  int16_t temperatureMax = 0;
  int16_t temperatureMin = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t BMS_ign = 0;
  uint8_t batteryRelay = 0;
  uint8_t counter_200 = 0;
  int8_t heatertemp = 0;
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
