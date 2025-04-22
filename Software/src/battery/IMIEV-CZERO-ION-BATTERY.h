#ifndef IMIEV_CZERO_ION_BATTERY_H
#define IMIEV_CZERO_ION_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class ImievCzeroIonBattery : public CanBattery {
 public:
  ImievCzeroIonBattery() : CanBattery(ImievCzeroIon) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "I-Miev / C-Zero / Ion Triplet";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 3696; }
  virtual uint16_t min_pack_voltage_dv() { return 3160; }
  virtual uint16_t max_cell_deviation_mv() { return 250; }
  virtual uint16_t max_cell_voltage_mv() { return 4150; }
  virtual uint16_t min_cell_voltage_mv() { return 2750; }

 private:
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
