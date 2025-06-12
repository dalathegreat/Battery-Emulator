#ifndef BATTERY_H
#define BATTERY_H

#include "../datalayer/datalayer.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
class Battery {
 public:
  virtual void setup(void) = 0;
  virtual void update_values() = 0;

  // The name of the comm interface the battery is using.
  virtual String interface_name() = 0;

  // These are commands from external I/O (UI, MQTT etc.)
  // Override in battery if it supports them. Otherwise they are NOP.

  virtual bool supports_clear_isolation() { return false; }
  virtual bool supports_reset_BMS() { return false; }
  virtual bool supports_reset_crash() { return false; }
  virtual bool supports_reset_NVROL() { return false; }
  virtual bool supports_reset_DTC() { return false; }
  virtual bool supports_read_DTC() { return false; }
  virtual bool supports_reset_SOH() { return false; }
  virtual bool supports_reset_BECM() { return false; }
  virtual bool supports_contactor_close() { return false; }
  virtual bool supports_contactor_reset() { return false; }
  virtual bool supports_set_fake_voltage() { return false; }
  virtual bool supports_manual_balancing() { return false; }
  virtual bool supports_real_BMS_status() { return false; }
  virtual bool supports_toggle_SOC_method() { return false; }
  virtual bool supports_factory_mode_method() { return false; }

  virtual void clear_isolation() {}
  virtual void reset_BMS() {}
  virtual void reset_crash() {}
  virtual void reset_contactor() {}
  virtual void reset_NVROL() {}
  virtual void reset_DTC() {}
  virtual void read_DTC() {}
  virtual void reset_SOH() {}
  virtual void reset_BECM() {}
  virtual void request_open_contactors() {}
  virtual void request_close_contactors() {}
  virtual void toggle_SOC_method() {}
  virtual void set_factory_mode() {}

  virtual void set_fake_voltage(float v) {}
  virtual float get_voltage() { return static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0; }

  // This allows for battery specific SOC plausibility calculations to be performed.
  virtual bool soc_plausible() { return true; }

  // Battery reports total_charged_battery_Wh and total_discharged_battery_Wh
  virtual bool supports_charged_energy() { return false; }

  virtual BatteryHtmlRenderer& get_status_renderer() { return defaultRenderer; }

 private:
  BatteryDefaultRenderer defaultRenderer;
};

#endif
