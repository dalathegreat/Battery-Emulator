#ifndef STELLANTIS_ECMP_BATTERY_H
#define STELLANTIS_ECMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"
#include "ECMP-HTML.h"

#ifdef STELLANTIS_ECMP_BATTERY
#define SELECTED_BATTERY_CLASS EcmpBattery
#endif

class EcmpBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  EcmpHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_DV = 4546;
  static const int MIN_PACK_VOLTAGE_DV = 3210;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2700;

  bool battery_RelayOpenRequest = false;
  uint8_t counter_20ms = 0;
  uint8_t battery_MainConnectorState = 0;
  int16_t battery_current = 0;
  uint16_t battery_voltage = 370;
  uint16_t battery_soc = 0;
  uint16_t cellvoltages[108];
  uint16_t battery_AllowedMaxChargeCurrent = 0;
  uint16_t battery_AllowedMaxDischargeCurrent = 0;
  uint16_t battery_insulationResistanceKOhm = 0;
  int16_t battery_highestTemperature = 0;
  int16_t battery_lowestTemperature = 0;

  unsigned long previousMillis20 = 0;   // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent

  CAN_frame ECMP_382 = {
      .FD = false,  //BSI_Info (VCU) PSA specific
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x382,
      .data = {0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //09 20 on AC charge. 0A 20 on DC charge
  CAN_frame ECMP_0F0 = {.FD = false,                              //VCU (Common)
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0F0,
                        .data = {0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}};
  uint8_t data_0F0_20[16] = {0xFF, 0x0E, 0x1D, 0x2C, 0x3B, 0x4A, 0x59, 0x68,
                             0x77, 0x86, 0x95, 0xA4, 0xB3, 0xC2, 0xD1, 0xE0};
  uint8_t data_0F0_00[16] = {0xF1, 0x00, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,
                             0x79, 0x88, 0x97, 0xA6, 0xB5, 0xC4, 0xD3, 0xE2};
};

#endif
