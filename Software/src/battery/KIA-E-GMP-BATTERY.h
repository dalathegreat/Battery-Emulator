#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"

extern ACAN2517FD canfd;

#define ESTIMATE_SOC_FROM_CELLVOLTAGE

#define BATTERY_SELECTED
#define MAXCHARGEPOWERALLOWED 10000
#define MAXDISCHARGEPOWERALLOWED 10000
#define RAMPDOWN_SOC 9000           // 90.00 SOC% to start ramping down from max charge power towards 0 at 100.00%
#define RAMPDOWNPOWERALLOWED 10000  // What power we ramp down from towards top balancing

// Used for SoC compensation - Define internal resistance value in milliohms for the entire pack
// How to calculate: voltage_drop_under_known_load [Volts] / load [Amps] = Resistance
#define PACK_INTERNAL_RESISTANCE_MOHM 200  // 200 milliohms for the whole pack

class KiaEGmpBattery : public CanBattery {
 public:
  KiaEGmpBattery() : CanBattery(KiaEGmp) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Kia/Hyundai EGMP platform";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 8064; }
  virtual uint16_t min_pack_voltage_dv() { return 4320; }
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2950; }
};

#endif
