#ifndef BMW_SBOX_CONTROL_H
#define BMW_SBOX_CONTROL_H
#include "../include.h"
#define CAN_SHUNT_SELECTED
void transmit_can(CAN_frame* tx_frame, int interface);

/** Minimum input voltage required to enable relay control **/
#define MINIMUM_INPUT_VOLTAGE 250

/** Minimum required percentage of input voltage at the output port to engage the positive relay. **/
/** Prevents engagement of the positive contactor if the precharge resistor is faulty. **/
#define MAX_PRECHARGE_RESISTOR_VOLTAGE_PERCENT 0.99

/* NOTE: modify the T2 time constant below to account for the resistance and capacitance of the target system.
 *      t=3RC at minimum, t=5RC ideally
 */

#define CONTACTOR_CONTROL_T1 5000  // Time before negative contactor engages and precharging starts
#define CONTACTOR_CONTROL_T2 5000  // Precharge time before precharge resistor is bypassed by positive contactor
#define CONTACTOR_CONTROL_T3 2000  // Precharge relay lead time after positive contactor has been engaged

class BMWSboxBattery : public CanBattery {
 public:
  BMWSboxBattery() : CanBattery(BMWSBox) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "BMW SBOX";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
