#ifndef KIA_HYUNDAI_64_BATTERY_H
#define KIA_HYUNDAI_64_BATTERY_H
#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class KiaHyundai64Battery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  KiaHyundai64Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* contactor_closing_allowed_ptr,
                      CAN_Interface targetCan)
      : UdsCanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    contactor_closing_allowed = contactor_closing_allowed_ptr;
    allows_contactor_closing = nullptr;
    dtc = &datalayer_battery->dtc;
  }

  // Use the default constructor to create the first or single battery.
  KiaHyundai64Battery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    contactor_closing_allowed = nullptr;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Kia/Hyundai 64/40kWh battery";

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "kia_hyundai64_dtc.json"; }

  bool supports_insulation_resistance() { return true; }

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  // If not null, this battery listens to this boolean to determine whether contactor closing is allowed
  bool* contactor_closing_allowed;

  void update_number_of_cells();
  void set_cell_voltages(uint8_t reading, uint8_t cellNumber);
  void process_cell_voltage_group(const uint8_t* data, uint8_t baseCell);

  static const int MAX_PACK_VOLTAGE_98S_DV = 4110;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_98S_DV = 2800;
  static const int MAX_PACK_VOLTAGE_90S_DV = 3870;
  static const int MIN_PACK_VOLTAGE_90S_DV = 2250;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2950;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10 = 0;   // will store last time a 10s CAN Message was send

  uint16_t soc_calculated = 0;
  uint16_t SOC_BMS = 0;
  uint16_t SOC_Display = 0;
  uint16_t batterySOH = 1000;
  uint16_t CellVoltMax_mV = 3700;
  uint16_t CellVoltMin_mV = 3700;
  uint16_t allowedDischargePower = 0;
  uint16_t allowedChargePower = 0;
  uint16_t batteryVoltage = 3700;
  uint16_t inverterVoltageFrameHigh = 0;
  uint16_t inverterVoltage = 0;
  uint16_t cellvoltages_mv[98];
  uint16_t leadAcidBatteryVoltage = 120;
  int16_t batteryAmps = 0;
  int16_t temperatureMax = 0;
  int16_t temperatureMin = 0;
  uint8_t poll_data_pid = 0;
  uint8_t open_state = 0;
  uint16_t pid_reply = 0;
  bool holdPidCounter = false;
  uint8_t CellVmaxNo = 0;
  uint8_t CellVminNo = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t BMS_ign = 0;
  uint8_t batteryRelay = 0;
  uint8_t waterleakageSensor = 164;
  uint8_t counter_200 = 0;
  int8_t temperature_water_inlet = 0;
  int8_t heatertemp = 0;
  int8_t powerRelayTemperature = 0;
  bool startedUp = false;
  uint8_t ecu_serial_number[16] = {0};
  uint8_t ecu_version_number[16] = {0};
  uint32_t cumulative_charge_current_ah = 0;
  uint32_t cumulative_discharge_current_ah = 0;
  uint32_t cumulative_energy_charged_kWh = 0;
  uint16_t cumulative_energy_discharged_HIGH_BYTE = 0;
  uint32_t cumulative_energy_discharged_kWh = 0;
  uint32_t powered_on_total_time = 0;
  uint16_t isolation_resistance_kOhm = 0;
  uint16_t number_of_standard_charging_sessions = 0;
  uint16_t number_of_fastcharging_sessions = 0;
  uint16_t accumulated_normal_charging_energy_kWh = 0;
  uint16_t accumulated_fastcharging_energy_kWh = 0;

  CAN_frame KIA_HYUNDAI_200 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x200,
                               .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};
  CAN_frame KIA_HYUNDAI_523 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x523,
                               .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}};
  CAN_frame KIA_HYUNDAI_524 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x524,
                               .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //553 Needed frame 200ms
  CAN_frame KIA64_553 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x553,
                         .data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
  //57F Needed frame 100ms
  CAN_frame KIA64_57F = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x57F,
                         .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
  //Needed frame 100ms
  CAN_frame KIA64_2A1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A1,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA64_7E4_OPEN_CONTACTOR_SEQUENCE = {.FD = false,
                                                 .ext_ID = false,
                                                 .DLC = 8,
                                                 .ID = 0x7E4,
                                                 .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  static const int POLL_GROUP_1 = 0x0101;
  static const int POLL_GROUP_2 = 0x0102;  //Cellvoltages 1-32
  static const int POLL_GROUP_3 = 0x0103;  //Cellvoltages 33-64
  static const int POLL_GROUP_4 = 0x0104;  //Cellvoltages 65-96 (Only to 90S on 40kWh battery)
  static const int POLL_GROUP_5 = 0x0105;  //Cellvoltages 97-98 + more
  static const int POLL_GROUP_6 = 0x0106;
  static const int POLL_GROUP_11 = 0x0111;  //Charging statistics
  static const int POLL_ECU_SERIAL = 0xF18C;
  static const int POLL_ECU_VERSION = 0xF191;
};

#endif
