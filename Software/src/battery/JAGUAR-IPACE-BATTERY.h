#ifndef JAGUAR_IPACE_BATTERY_H
#define JAGUAR_IPACE_BATTERY_H

#include "UdsCanBattery.h"

class JaguarIpaceBattery : public UdsCanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  JaguarIpaceBattery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Jaguar I-PACE";
  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "jaguar_ipace_dtc.json"; }

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  static const int MAX_PACK_VOLTAGE_DV = 4546;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3370;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillisKeepAlive = 0;

  uint8_t HVBattAvgSOC = 0;
  uint8_t HVBattFastChgCounter = 0;
  uint8_t HVBattTempColdCellID = 0;
  uint8_t HVBatTempHotCellID = 0;
  uint8_t HVBattVoltMaxCellID = 0;
  uint8_t HVBattVoltMinCellID = 0;
  uint8_t HVBattPwerGPCS = 0;
  uint8_t HVBattPwrGpCounter = 0;
  int8_t HVBattCurrentTR = 0;
  uint16_t HVBattCellVoltageMaxMv = 3700;
  uint16_t HVBattCellVoltageMinMv = 3700;
  uint16_t HVBattEnergyAvailable = 0;
  uint16_t HVBattEnergyUsableMax = 0;
  uint16_t HVBattTotalCapacityWhenNew = 0;
  uint16_t HVBattDischargeContiniousPowerLimit = 0;
  uint16_t HVBattDischargePowerLimitExt = 0;
  uint16_t HVBattDischargeVoltageLimit = 0;
  uint16_t HVBattVoltageExt = 0;
  uint16_t HVBatteryVoltageOC = 0;
  uint16_t HVBatteryChgCurrentLimit = 0;
  uint16_t HVBattChargeContiniousPowerLimit = 0;
  int16_t HVBattAverageTemperature = 0;
  int16_t HVBattCellTempAverage = 0;
  int16_t HVBattCellTempColdest = 0;
  int16_t HVBattCellTempHottest = 0;
  int16_t HVBattInletCoolantTemp = 0;
  bool HVBatteryContactorStatus = false;
  bool HVBatteryContactorStatusT = false;
  bool HVBattHVILError = false;
  bool HVILBattIsolationError = false;
  bool HVIsolationTestStatus = false;

  // UDS PIDs
  static const uint16_t POLL_ON_DEMAND_SELF_TEST = 0x0202;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_01 = 0x487e;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_02 = 0x487f;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_03 = 0x4880;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_04 = 0x4881;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_05 = 0x4882;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_06 = 0x4883;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_07 = 0x4884;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_08 = 0x4885;
  static const uint16_t POLL_CONTROL_MODULE_INTERNAL_DATA_09 = 0x4886;
  static const uint16_t POLL_INTERNAL_FW_VERSION = 0x48c2;
  static const uint16_t POLL_POSITIVE_CONTACTOR_B_VOLTAGE = 0x4900;  //Traction contactor voltage
  static const uint16_t POLL_POSITIVE_CONTACTOR_A_VOLTAGE = 0x4901;  //Aux contactor voltage
  static const uint16_t POLL_ISOLATION_RESISTANCE = 0x4902;
  static const uint16_t POLL_MAX_CELL_VOLTAGE = 0x4903;
  static const uint16_t POLL_MIN_CELL_VOLTAGE = 0x4904;
  static const uint16_t POLL_MAX_CELL_TEMPERATURE = 0x4905;
  static const uint16_t POLL_MIN_CELL_TEMPERATURE = 0x4906;
  static const uint16_t POLL_TEMPERATURE_UNKNOWN = 0x4907;
  static const uint16_t POLL_COOLALNT_OUTLET_TEMPERATURE = 0x4908;
  static const uint16_t POLL_POSITIVE_CONTACTOR_C_VOLTAGE = 0x4909;
  static const uint16_t POLL_TEMPERATURE_UNKNOWN_2 = 0x490a;
  static const uint16_t POLL_REAR_INVERTER_FUSE_VOLTAGE = 0x490b;
  static const uint16_t POLL_CURRENT_SENSOR_A = 0x490c;
  static const uint16_t POLL_FRONT_INVERTER_FUSE_VOLTAGE = 0x490d;
  static const uint16_t POLL_ANCILLARY_FUSE_VOLTAGE = 0x490e;
  static const uint16_t POLL_BATTERY_PACK_VOLTAGE = 0x490f;
  static const uint16_t POLL_AVERAGE_BATTERY_SOC = 0x4910;
  static const uint16_t POLL_MIN_BATTERY_SOC = 0x4911;
  static const uint16_t POLL_BATTERY_CURRENT_UNKNOWN = 0x4912;
  static const uint16_t POLL_MAX_REGEN_POWER = 0x4913;
  static const uint16_t POLL_MAX_BATTERY_SOC = 0x4914;
  static const uint16_t POLL_UNKNOWN_4915 = 0x4915;
  static const uint16_t POLL_UNKNOWN_4916 = 0x4916;
  static const uint16_t POLL_UNKNOWN_4917 = 0x4917;
  static const uint16_t POLL_UNKNOWN_4918 = 0x4918;
  static const uint16_t POLL_UNKNOWN_4919 = 0x4919;
  static const uint16_t POLL_UNKNOWN_491A = 0x491a;
  static const uint16_t POLL_NEGATIVE_CONTACTOR_TEMPERATURE = 0x491b;
  static const uint16_t POLL_POSITIVE_CONTACTOR_TEMPERATURE = 0x491c;
  static const uint16_t POLL_UNKNOWN_491D = 0x491d;
  static const uint16_t POLL_ANCILLARY_CONTACTOR_DIFF_VOLTAGE = 0x491e;
  static const uint16_t POLL_TRACTION_CONTACTOR_DIFF_VOLTAGE = 0x491f;
  static const uint16_t POLL_UNKNOWN_VOLTAGE_4920 = 0x4920;
  static const uint16_t POLL_NEGATIVE_CONTACTOR_DIFF_VOLTAGE = 0x4921;
  static const uint16_t POLL_FLAGS_4923 = 0x4923;
  static const uint16_t POLL_CSC_1_TEMPERATURE = 0x492b;
  static const uint16_t POLL_CSC_2_TEMPERATURE = 0x492c;
  static const uint16_t POLL_CSC_3_TEMPERATURE = 0x492d;
  static const uint16_t POLL_CSC_4_TEMPERATURE = 0x492e;
  static const uint16_t POLL_CSC_5_TEMPERATURE = 0x492f;
  static const uint16_t POLL_CSC_6_TEMPERATURE = 0x4930;
  static const uint16_t POLL_UNKNOWN_4931 = 0x4931;
  static const uint16_t POLL_UNKNOWN_4933 = 0x4933;
  static const uint16_t POLL_UNKNOWN_4934 = 0x4934;
  static const uint16_t POLL_UNKNOWN_4935 = 0x4935;
  static const uint16_t POLL_UNKNOWN_4936 = 0x4936;
  static const uint16_t POLL_UNKNOWN_4937 = 0x4937;
  static const uint16_t POLL_UNKNOWN_4938 = 0x4938;
  static const uint16_t POLL_UNKNOWN_4939 = 0x4939;
  static const uint16_t POLL_UNKNOWN_4941 = 0x4941;
  static const uint16_t POLL_FLAGS_4944 = 0x4944;
  static const uint16_t POLL_FLAGS_4945 = 0x4945;
  static const uint16_t POLL_REAR_INVERTER_CURRENT = 0x494e;
  static const uint16_t POLL_UNKNOWN_4970 = 0x4970;
  static const uint16_t POLL_UNKNOWN_ASCII_4971 = 0x4971;
  static const uint16_t POLL_UNKNOWN_VOLTAGE_497A = 0x497a;
  static const uint16_t POLL_UNKNOWN_VOLTAGE_497B = 0x497b;
  static const uint16_t POLL_UNKNOWN_VOLTAGE_497C = 0x497c;
  static const uint16_t POLL_UNKNOWN_497E = 0x497e;
  static const uint16_t POLL_UNKNOWN_497F = 0x497f;
  static const uint16_t POLL_UNKNOWN_4980 = 0x4980;
  static const uint16_t POLL_UNKNOWN_VOLTAGE_4981 = 0x4981;
  static const uint16_t POLL_UNKNOWN_498C = 0x498c;
  static const uint16_t POLL_UNKNOWN_D015 = 0xd015;
  static const uint16_t POLL_UNKNOWN_D018 = 0xd018;
  static const uint16_t POLL_UNKNOWN_D019 = 0xd019;
  static const uint16_t POLL_UNKNOWN_D020 = 0xd020;
  static const uint16_t POLL_UNKNOWN_D021 = 0xd021;
  static const uint16_t POLL_UNKNOWN_D05B = 0xd05b;
  static const uint16_t POLL_UNKNOWN_D100 = 0xd100;
  static const uint16_t POLL_FLAGS_D10E = 0xd10e;
  static const uint16_t POLL_UNKNOWN_D14D = 0xd14d;
  static const uint16_t POLL_LIVE_DATA_FAIL_CODE = 0xD703;
  static const uint16_t POLL_GLOBAL_REAL_TIME = 0xDD00;
  static const uint16_t POLL_TOTAL_DISTANCE = 0xDD01;
  static const uint16_t POLL_MAIN_ECU_VOLTAGE_SUPPLY = 0xDD02;
  static const uint16_t POLL_IN_CAR_TEMPERATURE = 0xDD04;
  static const uint16_t POLL_OUTSIDE_TEMPERATURE = 0xDD05;
  static const uint16_t POLL_POWER_MODE = 0xDD06;
  static const uint16_t POLL_UNKNOWN_DD08 = 0xDD08;
  static const uint16_t POLL_VEHICLE_SPEED = 0xDD09;
  static const uint16_t POLL_PRIMARY_BOOTLOADER_CONFIGURATION = 0xF101;
  static const uint16_t POLL_ECU_NETWORK_SIGNAL_CALIBRATION_NUMBER = 0xF108;
  static const uint16_t POLL_PRIMARY_BOOT_LOADER_PART_NUMBER = 0xF10E;
  static const uint16_t POLL_ECU_SERIAL_NUMBER = 0xF18C;
  static const uint16_t POLL_VIN = 0xF190;
  static const uint16_t POLL_UNKNOWN_FD01 = 0xFD01;
  static const uint16_t POLL_ECU_CORE_ASSEMBLY_NUMBER = 0xF111;
  static const uint16_t POLL_ECU_DELIVERY_ASSEMBLY_NUMBER = 0xF113;
  static const uint16_t POLL_ECU_SOFTWARE_2_PART_NUMBER = 0xF120;
  static const uint16_t POLL_ECU_SOFTWARE_3_PART_NUMBER = 0xF121;
  static const uint16_t POLL_ECU_CALIBRATION_DATA_1_NUMBER = 0xF124;
  static const uint16_t POLL_ECU_CALIBRATION_DATA_2_NUMBER = 0xF125;
  static const uint16_t POLL_SOFTWARE_DOWNLOAD_SPEC_VERSION = 0xF162;
  static const uint16_t POLL_UNKNOWN_F163 = 0xF163;
  static const uint16_t POLL_JLR_CONFIG_DATA_STORAGE_SPEC_VERSION = 0xF16A;
  static const uint16_t POLL_NETWORK_CONFIGURATION_IDENTIFIER = 0xF170;
  static const uint16_t POLL_BOOT_SOFTWARE_IDENTIFICATION = 0xF180;
  static const uint16_t POLL_ACTIVE_DIAGNOSIS_SESSION = 0xF186;
  static const uint16_t POLL_ECU_SOFTWARE_NUMBER = 0xF188;
  static const uint16_t POLL_ODX_FILE_DATA_IDENTIFIER = 0xF19D;

  uint32_t battery_vin = 0;  //temporary variable for undefined PIDs, to be removed when all PIDs are defined

  uint32_t positive_contactors_b_voltage = 0;
  uint32_t positive_contactors_a_voltage = 0;
  uint32_t isolation_resistance = 0;
  uint32_t coolant_outlet_temperature = 0;
  uint32_t positive_contactors_c_voltage = 0;
  uint32_t rear_inverter_fuse_voltage = 0;
  uint32_t current_sensor_a = 0;
  uint32_t front_inverter_fuse_voltage = 0;
  uint32_t ancillary_fuse_voltage = 0;
  uint32_t max_regen_power = 0;
  uint32_t negative_contactors_temperature = 0;
  uint32_t positive_contactors_temperature = 0;

  /* KeepAlive: PMZ_CAN_GWM_OSEK_NM_Pdu every 200ms.
 */
  CAN_frame ipace_keep_alive = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x51e,
                                .data = {0x22, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  /* KeepAlive: PMZ_CAN_NodeGWM_NM every 1s.
 * TODO: This may be needed for >2021 models.
 */
  /*CAN_frame ipace_keep_alive = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x59e,
                              .data = {0x9E, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};*/
};

#endif
