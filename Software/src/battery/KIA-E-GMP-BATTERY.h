#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include "CanBattery.h"
#include "KIA-E-GMP-HTML.h"

#define ESTIMATE_SOC_FROM_CELLVOLTAGE

#ifdef KIA_E_GMP_BATTERY
#define SELECTED_BATTERY_CLASS KiaEGmpBattery
#endif

class KiaEGmpBattery : public CanBattery {
 public:
  KiaEGmpBattery() : renderer(*this) {}
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Kia/Hyundai EGMP platform";
  BatteryHtmlRenderer& get_status_renderer() { return renderer; }
  // Getter implementations for HTML renderer
  int get_battery_12V() const;
  int get_waterleakageSensor() const;
  int get_temperature_water_inlet() const;
  int get_powerRelayTemperature() const;
  int get_batteryManagementMode() const;
  int get_BMS_ign() const;
  int get_batRelay() const;

 private:
  KiaEGMPHtmlRenderer renderer;
  uint16_t estimateSOC(uint16_t packVoltage, uint16_t cellCount, int16_t currentAmps);
  void set_voltage_minmax_limits();

  static const int MAX_PACK_VOLTAGE_DV = 8064;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 4320;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2950;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAXCHARGEPOWERALLOWED = 10000;
  static const int MAXDISCHARGEPOWERALLOWED = 10000;
  static const int RAMPDOWN_SOC = 9000;  // 90.00 SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int RAMPDOWNPOWERALLOWED = 10000;  // What power we ramp down from towards top balancing

  // Used for SoC compensation - Define internal resistance value in milliohms for the entire pack
  // How to calculate: voltage_drop_under_known_load [Volts] / load [Amps] = Resistance
  static const int PACK_INTERNAL_RESISTANCE_MOHM = 200;  // 200 milliohms for the whole pack

  unsigned long previousMillis200ms = 0;  // will store last time a 200ms CAN Message was send
  unsigned long previousMillis10s = 0;    // will store last time a 10s CAN Message was send

  uint16_t inverterVoltageFrameHigh = 0;
  uint16_t inverterVoltage = 0;
  uint16_t soc_calculated = 500;
  uint16_t SOC_BMS = 500;
  uint16_t SOC_Display = 500;
  uint16_t SOC_estimated_lowest = 0;
  uint16_t SOC_estimated_highest = 0;
  uint16_t batterySOH = 1000;
  uint16_t CellVoltMax_mV = 3700;
  uint16_t CellVoltMin_mV = 3700;
  uint16_t batteryVoltage = 6700;
  int16_t leadAcidBatteryVoltage = 120;
  int16_t batteryAmps = 0;
  int16_t temperatureMax = 20;
  int16_t temperatureMin = 20;
  int16_t allowedDischargePower = 0;
  int16_t allowedChargePower = 0;
  int16_t poll_data_pid = 0;
  uint8_t CellVmaxNo = 0;
  uint8_t CellVminNo = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t BMS_ign = 0xff;
  uint8_t batteryRelay = 0;
  uint8_t waterleakageSensor = 164;
  bool startedUp = false;
  bool ok_start_polling_battery = false;
  uint8_t counter_200 = 0;
  uint8_t KIA_7E4_COUNTER = 0x01;
  int8_t temperature_water_inlet = 20;
  int8_t powerRelayTemperature = 10;
  int8_t heatertemp = 20;
  bool set_voltage_limits = false;
  uint8_t ticks_200ms_counter = 0;
};

#endif
