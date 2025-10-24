#ifndef RELION_BATTERY_H
#define RELION_BATTERY_H

#include "../system_settings.h"
#include "CanBattery.h"

class RelionBattery : public CanBattery {
 public:
  RelionBattery() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {}

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Relion LV protocol via 250kbps CAN";

 private:
  uint16_t estimateSOC(uint16_t pack_total_vol);

  static const int MAX_PACK_VOLTAGE_DV = 584;  //58.4V recommended charge voltage. BMS protection steps in at 60.8V
  static const int MIN_PACK_VOLTAGE_DV = 440;  //44.0V Recommended LV disconnect. BMS protection steps in at 40.0V
  static const int MAX_CELL_DEVIATION_MV = 300;
  static const int MAX_CELL_VOLTAGE_MV = 3750;
  static const int MIN_CELL_VOLTAGE_MV = 2800;

  static const int RAMPDOWN_SOC = 900;  // 90.0 SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int RAMPDOWNPOWERALLOWED = 1000;  // What power we ramp down from towards top balancing
  static const int FLOAT_MAX_POWER_W = 150;      // W, what power to allow for top balancing battery
  static const int FLOAT_START_MV = 20;          // mV, how many mV under overvoltage to start float charging

  const uint16_t SOC[101] = {10000, 9900, 9800, 9700, 9600, 9500, 9400, 9300, 9200, 9100, 9000, 8900, 8800, 8700, 8600,
                             8500,  8400, 8300, 8200, 8100, 8000, 7900, 7800, 7700, 7600, 7500, 7400, 7300, 7200, 7100,
                             7000,  6900, 6800, 6700, 6600, 6500, 6400, 6300, 6200, 6100, 6000, 5900, 5800, 5700, 5600,
                             5500,  5400, 5300, 5200, 5100, 5000, 4900, 4800, 4700, 4600, 4500, 4400, 4300, 4200, 4100,
                             4000,  3900, 3800, 3700, 3600, 3500, 3400, 3300, 3200, 3100, 3000, 2900, 2800, 2700, 2600,
                             2500,  2400, 2300, 2200, 2100, 2000, 1900, 1800, 1700, 1600, 1500, 1400, 1300, 1200, 1100,
                             1000,  900,  800,  700,  600,  500,  400,  300,  200,  100,  0};
  // Define the data points for %SOC depending on cell voltage
  const uint8_t numPoints = 100;
  const uint16_t voltage[101] = {
      5760, 5750, 5740, 5730, 5720, 5710, 5700, 5690, 5680, 5670, 5660, 5650, 5640, 5630, 5620, 5610, 5600,
      5590, 5580, 5570, 5560, 5550, 5540, 5530, 5520, 5510, 5500, 5490, 5480, 5470, 5460, 5450, 5440, 5430,
      5420, 5410, 5400, 5390, 5380, 5370, 5360, 5350, 5340, 5330, 5320, 5310, 5300, 5290, 5280, 5270, 5260,
      5250, 5240, 5230, 5220, 5210, 5200, 5190, 5180, 5170, 5160, 5150, 5140, 5130, 5120, 5110, 5100, 5090,
      5080, 5070, 5060, 5050, 5040, 5030, 5020, 5010, 5000, 4990, 4980, 4970, 4960, 4950, 4940, 4930, 4920,
      4910, 4900, 4890, 4880, 4870, 4860, 4850, 4840, 4830, 4820, 4810, 4800, 4790, 4780, 4770, 4500};

  uint16_t battery_total_voltage = 500;
  int16_t battery_total_current = 0;
  uint8_t system_state = 0;
  uint8_t battery_soc = 50;
  uint8_t battery_soh = 99;
  uint8_t most_serious_fault = 0;
  uint16_t max_cell_voltage = 3300;
  uint16_t min_cell_voltage = 3300;
  int16_t max_cell_temperature = 0;
  int16_t min_cell_temperature = 0;
  int16_t charge_current_A = 0;
  int16_t regen_charge_current_A = 0;
  int16_t discharge_current_A = 0;
};

#endif
