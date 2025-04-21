#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"

extern ACAN2517FD canfd;

#define ESTIMATE_SOC_FROM_CELLVOLTAGE

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 8064  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 4320
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2950  //Battery is put into emergency stop if one cell goes below this value
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
  static constexpr char* Name = "Kia/Hyundai EGMP platform";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
