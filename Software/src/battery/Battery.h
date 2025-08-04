#ifndef BATTERY_H
#define BATTERY_H

#include <vector>
#include "../../src/devboard/utils/types.h"
#include "../../src/devboard/webserver/BatteryHtmlRenderer.h"

enum class BatteryType {
  None = 0,
  BmwI3 = 2,
  BmwIx = 3,
  BoltAmpera = 4,
  BydAtto3 = 5,
  CellPowerBms = 6,
  Chademo = 7,
  CmfaEv = 8,
  Foxess = 9,
  GeelyGeometryC = 10,
  OrionBms = 11,
  Sono = 12,
  StellantisEcmp = 13,
  ImievCZeroIon = 14,
  JaguarIpace = 15,
  KiaEGmp = 16,
  KiaHyundai64 = 17,
  KiaHyundaiHybrid = 18,
  Meb = 19,
  Mg5 = 20,
  NissanLeaf = 21,
  Pylon = 22,
  DalyBms = 23,
  RjxzsBms = 24,
  RangeRoverPhev = 25,
  RenaultKangoo = 26,
  RenaultTwizy = 27,
  RenaultZoe1 = 28,
  RenaultZoe2 = 29,
  SantaFePhev = 30,
  SimpBms = 31,
  TeslaModel3Y = 32,
  TeslaModelSX = 33,
  TestFake = 34,
  VolvoSpa = 35,
  VolvoSpaHybrid = 36,
  MgHsPhev = 37,
  SamsungSdiLv = 38,
  HyundaiIoniq28 = 39,
  Highest
};

extern std::vector<BatteryType> supported_battery_types();
extern const char* name_for_battery_type(BatteryType type);
extern const char* name_for_chemistry(battery_chemistry_enum chem);
extern const char* name_for_comm_interface(comm_interface comm);

extern BatteryType user_selected_battery_type;
extern bool user_selected_second_battery;

extern battery_chemistry_enum user_selected_battery_chemistry;

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
class Battery {
 public:
  virtual void setup(void) = 0;
  virtual void update_values() = 0;

  // The name of the comm interface the battery is using.
  virtual const char* interface_name() = 0;

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
  virtual bool supports_chademo_restart() { return false; }
  virtual bool supports_chademo_stop() { return false; }

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
  virtual void chademo_restart() {}
  virtual void chademo_stop() {}

  virtual void set_fake_voltage(float v) {}
  virtual float get_voltage();

  // This allows for battery specific SOC plausibility calculations to be performed.
  virtual bool soc_plausible() { return true; }

  // Battery reports total_charged_battery_Wh and total_discharged_battery_Wh
  virtual bool supports_charged_energy() { return false; }

  virtual BatteryHtmlRenderer& get_status_renderer() { return defaultRenderer; }

 private:
  BatteryDefaultRenderer defaultRenderer;
};

#endif
