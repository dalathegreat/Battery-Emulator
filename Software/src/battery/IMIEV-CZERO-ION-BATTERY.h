#ifndef IMIEV_CZERO_ION_BATTERY_H
#define IMIEV_CZERO_ION_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef IMIEV_CZERO_ION_BATTERY
#define SELECTED_BATTERY_CLASS ImievCZeroIonBattery
#endif

class ImievCZeroIonBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "I-Miev / C-Zero / Ion Triplet";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 3696;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3160;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4150;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2750;  //Battery is put into emergency stop if one cell goes below this value

  uint8_t errorCode = 0;  //stores if we have an error code active from battery control logic
  uint8_t BMU_Detected = 0;
  uint8_t CMU_Detected = 0;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent

  int pid_index = 0;
  int cmu_id = 0;
  int voltage_index = 0;
  int temp_index = 0;
  uint8_t BMU_SOC = 0;
  int temp_value = 0;
  double temp1 = 0;
  double temp2 = 0;
  double temp3 = 0;
  double voltage1 = 0;
  double voltage2 = 0;
  double BMU_Current = 0;
  double BMU_PackVoltage = 0;
  double BMU_Power = 0;
  double cell_voltages[88];      //array with all the cellvoltages
  double cell_temperatures[88];  //array with all the celltemperatures
  double max_volt_cel = 3.70;
  double min_volt_cel = 3.70;
  double max_temp_cel = 20.00;
  double min_temp_cel = 19.00;
};

#endif
