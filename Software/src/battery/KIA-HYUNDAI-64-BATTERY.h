#ifndef KIA_HYUNDAI_64_BATTERY_H
#define KIA_HYUNDAI_64_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_98S_DV 4110  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_98S_DV 2800
#define MAX_PACK_VOLTAGE_90S_DV 3870
#define MIN_PACK_VOLTAGE_90S_DV 2250
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2950  //Battery is put into emergency stop if one cell goes below this value

void update_number_of_cells();

class KiaHyundai64Battery : public CanBattery {
 public:
  KiaHyundai64Battery(DATALAYER_BATTERY_TYPE* target, CAN_Interface can_interface) : CanBattery(KiaHyundai64) {
    m_target = target;
    m_can_interface = can_interface;
  }
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Kia/Hyundai 64/40kWh battery";
  virtual bool supportsDoubleBattery() { return true; };

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

 private:
  DATALAYER_BATTERY_TYPE* m_target;
  CAN_Interface m_can_interface;
  void update_number_of_cells();

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10 = 0;   // will store last time a 10s CAN Message was send

  uint16_t soc_calculated = 0;
  uint16_t SOC_BMS = 0;
  uint16_t SOC_Display = 0;
  uint16_t batterySOH = 1000;
  uint16_t CellVoltMax_mV = 3700;
  uint16_t CellVoltMin_mV = 3700;
  uint16_t allowedDischargePower = 0;
  uint16_t allowedChargePower = 0;
  uint16_t batteryVoltage = 0;
  uint16_t inverterVoltageFrameHigh = 0;
  uint16_t inverterVoltage = 0;
  uint16_t cellvoltages_mv[98];
  int16_t leadAcidBatteryVoltage = 120;
  int16_t batteryAmps = 0;
  int16_t temperatureMax = 0;
  int16_t temperatureMin = 0;
  int16_t poll_data_pid = 0;
  bool holdPidCounter = false;
  uint8_t CellVmaxNo = 0;
  uint8_t CellVminNo = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t BMS_ign = 0;
  uint8_t batteryRelay = 0;
  uint8_t waterleakageSensor = 164;
  uint8_t counter_200 = 0;
  int8_t temperature_water_inlet = 0;
  int8_t heatertemp = 0;
  int8_t powerRelayTemperature = 0;
  bool startedUp = false;

  CAN_frame KIA_HYUNDAI_200 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x200,
                               .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};
  CAN_frame KIA_HYUNDAI_523 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x523,
                               .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}};
  CAN_frame KIA_HYUNDAI_524 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x524,
                               .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //553 Needed frame 200ms
  CAN_frame KIA64_553 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x553,
                         .data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
  //57F Needed frame 100ms
  CAN_frame KIA64_57F = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x57F,
                         .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
  //Needed frame 100ms
  CAN_frame KIA64_2A1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A1,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA64_7E4_id1 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 01
  CAN_frame KIA64_7E4_id2 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 02
  CAN_frame KIA64_7E4_id3 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 03
  CAN_frame KIA64_7E4_id4 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 04
  CAN_frame KIA64_7E4_id5 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 05
  CAN_frame KIA64_7E4_id6 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 06
  CAN_frame KIA64_7E4_ack = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x7E4,
      .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Ack frame, correct PID is returned
};

#endif
