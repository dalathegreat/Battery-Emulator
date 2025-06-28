#ifndef CMFA_EV_BATTERY_H
#define CMFA_EV_BATTERY_H
#include "../include.h"

#include "CMFA-EV-HTML.h"
#include "CanBattery.h"

#ifdef CMFA_EV_BATTERY
#define SELECTED_BATTERY_CLASS CmfaEvBattery
#endif

class CmfaEvBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  CmfaEvBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_CMFAEV* extended, CAN_Interface targetCan)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    datalayer_cmfa = extended;

    average_voltage_of_cells = 0;
  }

  // Use the default constructor to create the first or single battery.
  CmfaEvBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_cmfa = &datalayer_extended.CMFAEV;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "CMFA platform, 27 kWh battery";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  CmfaEvHtmlRenderer renderer;

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_CMFAEV* datalayer_cmfa;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  uint16_t rescale_raw_SOC(uint32_t raw_SOC);

  static const int MAX_PACK_VOLTAGE_DV = 3040;  // 5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2185;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  // Emergency stop if above
  static const int MIN_CELL_VOLTAGE_MV = 2700;  // Emergency stop if below

  // OBD2 PID polls
  static const int PID_POLL_SOCZ = 0x9001;
  static const int PID_POLL_USOC = 0x9002;
  static const int PID_POLL_SOH_AVERAGE = 0x9003;
  static const int PID_POLL_AVERAGE_VOLTAGE_OF_CELLS = 0x9006;
  static const int PID_POLL_HIGHEST_CELL_VOLTAGE = 0x9007;
  static const int PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE = 0x9008;
  static const int PID_POLL_LOWEST_CELL_VOLTAGE = 0x9009;
  static const int PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE = 0x900A;
  static const int PID_POLL_CURRENT_OFFSET = 0x900C;
  static const int PID_POLL_INSTANT_CURRENT = 0x900D;
  static const int PID_POLL_MAX_REGEN = 0x900E;
  static const int PID_POLL_MAX_DISCHARGE_POWER = 0x900F;
  static const int PID_POLL_12V_BATTERY = 0x9011;
  static const int PID_POLL_AVERAGE_TEMPERATURE = 0x9012;
  static const int PID_POLL_MIN_TEMPERATURE = 0x9013;
  static const int PID_POLL_MAX_TEMPERATURE = 0x9014;
  static const int PID_POLL_MAX_CHARGE_POWER = 0x9018;
  static const int PID_POLL_END_OF_CHARGE_FLAG = 0x9019;
  static const int PID_POLL_INTERLOCK_FLAG = 0x901A;
  static const int PID_POLL_BATTERY_IDENTIFICATION = 0x901B;

  static const int PID_POLL_CELL_1 = 0x9021;
  static const int PID_POLL_CELL_2 = 0x9022;
  static const int PID_POLL_CELL_3 = 0x9023;
  static const int PID_POLL_CELL_4 = 0x9024;
  static const int PID_POLL_CELL_5 = 0x9025;
  static const int PID_POLL_CELL_6 = 0x9026;
  static const int PID_POLL_CELL_7 = 0x9027;
  static const int PID_POLL_CELL_8 = 0x9028;
  static const int PID_POLL_CELL_9 = 0x9029;
  static const int PID_POLL_CELL_10 = 0x902A;
  static const int PID_POLL_CELL_11 = 0x902B;
  static const int PID_POLL_CELL_12 = 0x902C;
  static const int PID_POLL_CELL_13 = 0x902D;
  static const int PID_POLL_CELL_14 = 0x902E;
  static const int PID_POLL_CELL_15 = 0x902F;
  static const int PID_POLL_CELL_16 = 0x9030;
  static const int PID_POLL_CELL_17 = 0x9031;
  static const int PID_POLL_CELL_18 = 0x9032;
  static const int PID_POLL_CELL_19 = 0x9033;
  static const int PID_POLL_CELL_20 = 0x9034;
  static const int PID_POLL_CELL_21 = 0x9035;
  static const int PID_POLL_CELL_22 = 0x9036;
  static const int PID_POLL_CELL_23 = 0x9037;
  static const int PID_POLL_CELL_24 = 0x9038;
  static const int PID_POLL_CELL_25 = 0x9039;
  static const int PID_POLL_CELL_26 = 0x903A;
  static const int PID_POLL_CELL_27 = 0x903B;
  static const int PID_POLL_CELL_28 = 0x903C;
  static const int PID_POLL_CELL_29 = 0x903D;
  static const int PID_POLL_CELL_30 = 0x903E;
  static const int PID_POLL_CELL_31 = 0x903F;

  static const int PID_POLL_DIDS_SUPPORTED_IN_RANGE_9041_9060 = 0x9040;

  static const int PID_POLL_CELL_32 = 0x9041;
  static const int PID_POLL_CELL_33 = 0x9042;
  static const int PID_POLL_CELL_34 = 0x9043;
  static const int PID_POLL_CELL_35 = 0x9044;
  static const int PID_POLL_CELL_36 = 0x9045;
  static const int PID_POLL_CELL_37 = 0x9046;
  static const int PID_POLL_CELL_38 = 0x9047;
  static const int PID_POLL_CELL_39 = 0x9048;
  static const int PID_POLL_CELL_40 = 0x9049;
  static const int PID_POLL_CELL_41 = 0x904A;
  static const int PID_POLL_CELL_42 = 0x904B;
  static const int PID_POLL_CELL_43 = 0x904C;
  static const int PID_POLL_CELL_44 = 0x904D;
  static const int PID_POLL_CELL_45 = 0x904E;
  static const int PID_POLL_CELL_46 = 0x904F;
  static const int PID_POLL_CELL_47 = 0x9050;
  static const int PID_POLL_CELL_48 = 0x9051;
  static const int PID_POLL_CELL_49 = 0x9052;
  static const int PID_POLL_CELL_50 = 0x9053;
  static const int PID_POLL_CELL_51 = 0x9054;
  static const int PID_POLL_CELL_52 = 0x9055;
  static const int PID_POLL_CELL_53 = 0x9056;
  static const int PID_POLL_CELL_54 = 0x9057;
  static const int PID_POLL_CELL_55 = 0x9058;
  static const int PID_POLL_CELL_56 = 0x9059;
  static const int PID_POLL_CELL_57 = 0x905A;
  static const int PID_POLL_CELL_58 = 0x905B;
  static const int PID_POLL_CELL_59 = 0x905C;
  static const int PID_POLL_CELL_60 = 0x905D;
  static const int PID_POLL_CELL_61 = 0x905E;
  static const int PID_POLL_CELL_62 = 0x905F;

  static const int PID_POLL_DIDS_SUPPORTED_IN_RANGE_9061_9080 = 0x9060;

  static const int PID_POLL_CELL_63 = 0x9061;
  static const int PID_POLL_CELL_64 = 0x9062;
  static const int PID_POLL_CELL_65 = 0x9063;
  static const int PID_POLL_CELL_66 = 0x9064;
  static const int PID_POLL_CELL_67 = 0x9065;
  static const int PID_POLL_CELL_68 = 0x9066;
  static const int PID_POLL_CELL_69 = 0x9067;
  static const int PID_POLL_CELL_70 = 0x9068;
  static const int PID_POLL_CELL_71 = 0x9069;
  static const int PID_POLL_CELL_72 = 0x906A;

  static const int PID_POLL_SOH_AVAILABLE_POWER_CALCULATION = 0x91BC;
  static const int PID_POLL_SOH_GENERATED_POWER_CALCULATION = 0x91BD;

  static const int PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING = 0x9243;
  static const int PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING = 0x9245;
  static const int PID_POLL_CUMULATIVE_ENERGY_IN_REGEN = 0x9247;

  CAN_frame CMFA_1EA = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x1EA, .data = {0x00}};
  CAN_frame CMFA_125 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 7,
                        .ID = 0x125,
                        .data = {0x7D, 0x7D, 0x7D, 0x07, 0x82, 0x6A, 0x8A}};
  CAN_frame CMFA_134 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x134,
                        .data = {0x90, 0x8A, 0x7E, 0x3E, 0xB2, 0x4C, 0x80, 0x00}};
  CAN_frame CMFA_135 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x135, .data = {0xD5, 0x85, 0x38, 0x80, 0x01}};
  CAN_frame CMFA_3D3 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3D3,
                        .data = {0x47, 0x30, 0x00, 0x02, 0x5D, 0x80, 0x5D, 0xE7}};
  CAN_frame CMFA_59B = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x59B, .data = {0x00, 0x02, 0x00}};
  CAN_frame CMFA_ACK = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x79B,
                        .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CMFA_POLLING_FRAME = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x79B,
                                  .data = {0x03, 0x22, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00}};
  bool end_of_charge = false;
  bool interlock_flag = false;
  uint16_t soc_z = 0;
  uint16_t soc_u = 0;
  uint16_t max_regen_power = 0;
  uint16_t max_discharge_power = 0;
  int16_t average_temperature = 0;
  int16_t minimum_temperature = 0;
  int16_t maximum_temperature = 0;
  uint16_t maximum_charge_power = 0;
  uint16_t SOH_available_power = 0;
  uint16_t SOH_generated_power = 0;
  uint32_t average_voltage_of_cells = 270000;
  uint16_t highest_cell_voltage_mv = 3700;
  uint16_t lowest_cell_voltage_mv = 3700;
  uint16_t lead_acid_voltage = 12000;
  uint8_t highest_cell_voltage_number = 0;
  uint8_t lowest_cell_voltage_number = 0;
  uint64_t cumulative_energy_when_discharging = 0;
  uint64_t cumulative_energy_when_charging = 0;
  uint64_t cumulative_energy_in_regen = 0;
  uint16_t soh_average = 10000;
  uint16_t cellvoltages_mv[72];
  uint32_t poll_pid = PID_POLL_SOH_AVERAGE;
  uint16_t pid_reply = 0;

  uint8_t counter_10ms = 0;
  uint8_t content_125[16] = {0x07, 0x0C, 0x01, 0x06, 0x0B, 0x00, 0x05, 0x0A,
                             0x0F, 0x04, 0x09, 0x0E, 0x03, 0x08, 0x0D, 0x02};
  uint8_t content_135[16] = {0x85, 0xD5, 0x25, 0x75, 0xC5, 0x15, 0x65, 0xB5,
                             0x05, 0x55, 0xA5, 0xF5, 0x45, 0x95, 0xE5, 0x35};
  unsigned long previousMillis200ms = 0;
  unsigned long previousMillis100ms = 0;
  unsigned long previousMillis10ms = 0;

  static const int MAXSOC = 9000;  //90.00 Raw SOC displays this value when battery is at 100%
  static const int MINSOC = 500;   //5.00 Raw SOC displays this value when battery is at 0%

  uint8_t heartbeat = 0;   //Alternates between 0x55 and 0xAA every 5th frame
  uint8_t heartbeat2 = 0;  //Alternates between 0x55 and 0xAA every 5th frame
  uint32_t SOC_raw = 0;
  uint16_t SOH = 99;
  int16_t current = 0;
  uint16_t pack_voltage = 2700;
  int16_t highest_cell_temperature = 0;
  int16_t lowest_cell_temperature = 0;
  uint32_t discharge_power_w = 0;
  uint32_t charge_power_w = 0;
};

#endif
