#ifndef _SHUNT_H
#define _SHUNT_H

#include "../../src/communication/Transmitter.h"
#include "../../src/communication/can/CanReceiver.h"
#include "../../src/communication/can/comm_can.h"
#include "../../src/devboard/safety/safety.h"
#include "../../src/devboard/utils/types.h"
#include "../../src/inverter/InverterProtocol.h"
#include "Arduino.h"

#include <vector>

#ifndef SMALL_FLASH_DEVICE
enum class ShuntType { None = 0, BmwSbox = 1, Inverter = 2, CustomClamp = 3, Qnhck2_16 = 4, Highest };
#else
// The QNHCK2-16 (4) is left out of the small flash devices
enum class ShuntType { None = 0, BmwSbox = 1, Inverter = 2, CustomClamp = 3, Highest };
#endif  // SMALL_FLASH_DEVICE
enum class BatteryType;

#ifndef SMALL_FLASH_DEVICE
// A shunt the emulator runs itself. How it reaches the hardware, CAN or an ADC pin, is up to
// the subclass.
class Shunt {
 public:
  virtual void setup() = 0;

  // The name of the interface the shunt is read through, for the settings page.
  virtual const char* interface_name() = 0;

  // Takes what the shunt reads right now as its zero current point. reading_mV is what was
  // read, 0 when there is no reading yet. Returns false when the shunt has no such calibration
  // or the reading cannot be its zero point.
  virtual bool calibrate_zero(uint16_t& reading_mV) {
    reading_mV = 0;
    return false;
  }
};

class CanShunt : public Shunt, public Transmitter, CanReceiver {
#else
class CanShunt : public Transmitter, CanReceiver {
#endif  // SMALL_FLASH_DEVICE
 public:
  virtual void setup() = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;

  // The name of the comm interface the shunt is using.
  virtual const char* interface_name() { return getCANInterfaceName(can_config.shunt); }

  void transmit(unsigned long currentMillis) {
    if (allowed_to_send_CAN) {
      transmit_can(currentMillis);
    }
  }

  void receive_can_frame(CAN_frame* frame) { handle_incoming_can_frame(*frame); }

 protected:
  CAN_Interface can_interface;

  CanShunt() {
    can_interface = can_config.shunt;
    register_transmitter(this);
    register_can_receiver(this, can_interface);
  }

  void transmit_can_frame(CAN_frame* frame) { transmit_can_frame_to_interface(frame, can_interface); }
};

#ifdef SMALL_FLASH_DEVICE
// Without the QNHCK2-16, every shunt the emulator runs itself is a CAN one
using Shunt = CanShunt;
#endif  // SMALL_FLASH_DEVICE
extern Shunt* shunt;
// Whether a shunt type can work with the selected battery resp. inverter. "Custom Clamp" is only
// read by the CHAdeMO integration, "Using inverter values" needs an inverter that reports the pack
// voltage and current. The settings page hides the unusable types and the save handler resets them.
extern bool shunt_type_supported_by_battery(ShuntType type, BatteryType battery_type);
extern bool shunt_type_supported_by_inverter(ShuntType type, InverterProtocolType inverter_type);
extern const char* name_for_shunt_type(ShuntType type);
extern ShuntType user_selected_shunt_type;

// Updateable parameters for the Chademo CT Clamp shunt type. Stored in NVM and modifiable via the webserver.
extern float ct_clamp_offset_mV;
extern uint16_t ct_clamp_nominal_voltage_dV;
extern uint16_t ct_clamp_nominal_current_A;
enum class adc_attenuation_enum { ADC_0db = 0, ADC_2_5db, ADC_6db, ADC_11db, Highest };
extern adc_attenuation_enum ct_clamp_pin_atten;
extern const char* name_for_adc_attenuation(adc_attenuation_enum type);
extern bool ct_invert_current;

#endif
