#include "../include.h"
#ifdef TESLA_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For Advanced Battery Insights webpage
#include "../devboard/utils/events.h"
#include "TESLA-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
/* Credits: Most of the code comes from Per Carlen's bms_comms_tesla_model3.py (https://gitlab.com/pelle8/batt2gen24/) */

static unsigned long previousMillis30 = 0;  // will store last time a 30ms CAN Message was send

CAN_frame TESLA_221_1 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x221,
    .data = {0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96}};  //Contactor frame 221 - close contactors
CAN_frame TESLA_221_2 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x221,
    .data = {0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA}};  //Contactor Frame 221 - hv_up_for_drive

static uint32_t battery_total_discharge = 0;
static uint32_t battery_total_charge = 0;
static uint16_t battery_volts = 0;     // V
static int16_t battery_amps = 0;       // A
static uint16_t battery_raw_amps = 0;  // A
static int16_t battery_max_temp = 0;   // C*
static int16_t battery_min_temp = 0;   // C*
static uint16_t battery_energy_buffer = 0;
static uint16_t battery_energy_to_charge_complete = 0;
static uint16_t battery_expected_energy_remaining = 0;
static uint8_t battery_full_charge_complete = 0;
static uint16_t battery_ideal_energy_remaining = 0;
static uint16_t battery_nominal_energy_remaining = 0;
static uint16_t battery_nominal_full_pack_energy = 600;
static uint16_t battery_beginning_of_life = 600;
static uint16_t battery_charge_time_remaining = 0;  // Minutes
static uint16_t battery_regenerative_limit = 0;
static uint16_t battery_discharge_limit = 0;
static uint16_t battery_max_heat_park = 0;
static uint16_t battery_hvac_max_power = 0;
static uint16_t battery_max_discharge_current = 0;
static uint16_t battery_max_charge_current = 0;
static uint16_t battery_bms_max_voltage = 0;
static uint16_t battery_bms_min_voltage = 0;
static uint16_t battery_high_voltage = 0;
static uint16_t battery_low_voltage = 0;
static uint16_t battery_output_current = 0;
static uint16_t battery_soc_min = 0;
static uint16_t battery_soc_max = 0;
static uint16_t battery_soc_vi = 0;
static uint16_t battery_soc_ave = 0;
static uint16_t battery_cell_max_v = 3700;
static uint16_t battery_cell_min_v = 3700;
static uint16_t battery_cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static uint8_t battery_max_vno = 0;
static uint8_t battery_min_vno = 0;
static uint8_t battery_contactor = 0;  //State of contactor
static uint8_t battery_hvil_status = 0;
static uint8_t battery_packContNegativeState = 0;
static uint8_t battery_packContPositiveState = 0;
static uint8_t battery_packContactorSetState = 0;
static uint8_t battery_packCtrsClosingAllowed = 0;
static uint8_t battery_pyroTestInProgress = 0;
//Fault codes
static uint8_t battery_WatchdogReset = 0;   //Warns if the processor has experienced a reset due to watchdog reset.
static uint8_t battery_PowerLossReset = 0;  //Warns if the processor has experienced a reset due to power loss.
static uint8_t battery_SwAssertion = 0;     //An internal software assertion has failed.
static uint8_t battery_CrashEvent = 0;      //Warns if the crash signal is detected by HVP
static uint8_t battery_OverDchgCurrentFault = 0;  //Warns if the pack discharge is above max discharge current limit
static uint8_t battery_OverChargeCurrentFault =
    0;  //Warns if the pack discharge current is above max charge current limit
static uint8_t battery_OverCurrentFault =
    0;  //Warns if the pack current (discharge or charge) is above max current limit.
static uint8_t battery_OverTemperatureFault = 0;  //A pack module temperature is above maximum temperature limit
static uint8_t battery_OverVoltageFault = 0;      //A brick voltage is above maximum voltage limit
static uint8_t battery_UnderVoltageFault = 0;     //A brick voltage is below minimum voltage limit
static uint8_t battery_PrimaryBmbMiaFault =
    0;  //Warns if the voltage and temperature readings from primary BMB chain are mia
static uint8_t battery_SecondaryBmbMiaFault =
    0;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
static uint8_t battery_BmbMismatchFault =
    0;  //Warns if the primary and secondary BMB chain readings don't match with each other
static uint8_t battery_BmsHviMiaFault = 0;   //Warns if the BMS node is mia on HVS or HVI CAN
static uint8_t battery_CpMiaFault = 0;       //Warns if the CP node is mia on HVS CAN
static uint8_t battery_PcsMiaFault = 0;      //The PCS node is mia on HVS CAN
static uint8_t battery_BmsFault = 0;         //Warns if the BMS ECU has faulted
static uint8_t battery_PcsFault = 0;         //Warns if the PCS ECU has faulted
static uint8_t battery_CpFault = 0;          //Warns if the CP ECU has faulted
static uint8_t battery_ShuntHwMiaFault = 0;  //Warns if the shunt current reading is not available
static uint8_t battery_PyroMiaFault = 0;     //Warns if the pyro squib is not connected
static uint8_t battery_hvsMiaFault = 0;      //Warns if the pack contactor hw fault
static uint8_t battery_hviMiaFault = 0;      //Warns if the FC contactor hw fault
static uint8_t battery_Supply12vFault = 0;   //Warns if the low voltage (12V) battery is below minimum voltage threshold
static uint8_t battery_VerSupplyFault =
    0;                                 //Warns if the Energy reserve voltage supply is below minimum voltage threshold
static uint8_t battery_HvilFault = 0;  //Warn if a High Voltage Inter Lock fault is detected
static uint8_t battery_BmsHvsMiaFault = 0;  //Warns if the BMS node is mia on HVS or HVI CAN
static uint8_t battery_PackVoltMismatchFault =
    0;  //Warns if the pack voltage doesn't match approximately with sum of brick voltages
static uint8_t battery_EnsMiaFault = 0;         //Warns if the ENS line is not connected to HVC
static uint8_t battery_PackPosCtrArcFault = 0;  //Warns if the HVP detectes series arc at pack contactor
static uint8_t battery_packNegCtrArcFault = 0;  //Warns if the HVP detectes series arc at FC contactor
static uint8_t battery_ShuntHwAndBmsMiaFault = 0;
static uint8_t battery_fcContHwFault = 0;
static uint8_t battery_robinOverVoltageFault = 0;
static uint8_t battery_packContHwFault = 0;
static uint8_t battery_pyroFuseBlown = 0;
static uint8_t battery_pyroFuseFailedToBlow = 0;
static uint8_t battery_CpilFault = 0;
static uint8_t battery_PackContactorFellOpen = 0;
static uint8_t battery_FcContactorFellOpen = 0;
static uint8_t battery_packCtrCloseBlocked = 0;
static uint8_t battery_fcCtrCloseBlocked = 0;
static uint8_t battery_packContactorForceOpen = 0;
static uint8_t battery_fcContactorForceOpen = 0;
static uint8_t battery_dcLinkOverVoltage = 0;
static uint8_t battery_shuntOverTemperature = 0;
static uint8_t battery_passivePyroDeploy = 0;
static uint8_t battery_logUploadRequest = 0;
static uint8_t battery_packCtrCloseFailed = 0;
static uint8_t battery_fcCtrCloseFailed = 0;
static uint8_t battery_shuntThermistorMia = 0;

#ifdef DOUBLE_BATTERY
static uint32_t battery2_total_discharge = 0;
static uint32_t battery2_total_charge = 0;
static uint16_t battery2_volts = 0;     // V
static int16_t battery2_amps = 0;       // A
static uint16_t battery2_raw_amps = 0;  // A
static int16_t battery2_max_temp = 0;   // C*
static int16_t battery2_min_temp = 0;   // C*
static uint16_t battery2_energy_buffer = 0;
static uint16_t battery2_energy_to_charge_complete = 0;
static uint16_t battery2_expected_energy_remaining = 0;
static uint8_t battery2_full_charge_complete = 0;
static uint16_t battery2_ideal_energy_remaining = 0;
static uint16_t battery2_nominal_energy_remaining = 0;
static uint16_t battery2_nominal_full_pack_energy = 600;
static uint16_t battery2_beginning_of_life = 600;
static uint16_t battery2_charge_time_remaining = 0;  // Minutes
static uint16_t battery2_regenerative_limit = 0;
static uint16_t battery2_discharge_limit = 0;
static uint16_t battery2_max_heat_park = 0;
static uint16_t battery2_hvac_max_power = 0;
static uint16_t battery2_max_discharge_current = 0;
static uint16_t battery2_max_charge_current = 0;
static uint16_t battery2_bms_max_voltage = 0;
static uint16_t battery2_bms_min_voltage = 0;
static uint16_t battery2_high_voltage = 0;
static uint16_t battery2_low_voltage = 0;
static uint16_t battery2_output_current = 0;
static uint16_t battery2_soc_min = 0;
static uint16_t battery2_soc_max = 0;
static uint16_t battery2_soc_vi = 0;
static uint16_t battery2_soc_ave = 0;
static uint16_t battery2_cell_max_v = 3700;
static uint16_t battery2_cell_min_v = 3700;
static uint16_t battery2_cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static uint8_t battery2_max_vno = 0;
static uint8_t battery2_min_vno = 0;
static uint8_t battery2_contactor = 0;  //State of contactor
static uint8_t battery2_hvil_status = 0;
static uint8_t battery2_packContNegativeState = 0;
static uint8_t battery2_packContPositiveState = 0;
static uint8_t battery2_packContactorSetState = 0;
static uint8_t battery2_packCtrsClosingAllowed = 0;
static uint8_t battery2_pyroTestInProgress = 0;
//Fault codes
static uint8_t battery2_WatchdogReset = 0;   //Warns if the processor has experienced a reset due to watchdog reset.
static uint8_t battery2_PowerLossReset = 0;  //Warns if the processor has experienced a reset due to power loss.
static uint8_t battery2_SwAssertion = 0;     //An internal software assertion has failed.
static uint8_t battery2_CrashEvent = 0;      //Warns if the crash signal is detected by HVP
static uint8_t battery2_OverDchgCurrentFault = 0;  //Warns if the pack discharge is above max discharge current limit
static uint8_t battery2_OverChargeCurrentFault =
    0;  //Warns if the pack discharge current is above max charge current limit
static uint8_t battery2_OverCurrentFault =
    0;  //Warns if the pack current (discharge or charge) is above max current limit.
static uint8_t battery2_OverTemperatureFault = 0;  //A pack module temperature is above maximum temperature limit
static uint8_t battery2_OverVoltageFault = 0;      //A brick voltage is above maximum voltage limit
static uint8_t battery2_UnderVoltageFault = 0;     //A brick voltage is below minimum voltage limit
static uint8_t battery2_PrimaryBmbMiaFault =
    0;  //Warns if the voltage and temperature readings from primary BMB chain are mia
static uint8_t battery2_SecondaryBmbMiaFault =
    0;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
static uint8_t battery2_BmbMismatchFault =
    0;  //Warns if the primary and secondary BMB chain readings don't match with each other
static uint8_t battery2_BmsHviMiaFault = 0;   //Warns if the BMS node is mia on HVS or HVI CAN
static uint8_t battery2_CpMiaFault = 0;       //Warns if the CP node is mia on HVS CAN
static uint8_t battery2_PcsMiaFault = 0;      //The PCS node is mia on HVS CAN
static uint8_t battery2_BmsFault = 0;         //Warns if the BMS ECU has faulted
static uint8_t battery2_PcsFault = 0;         //Warns if the PCS ECU has faulted
static uint8_t battery2_CpFault = 0;          //Warns if the CP ECU has faulted
static uint8_t battery2_ShuntHwMiaFault = 0;  //Warns if the shunt current reading is not available
static uint8_t battery2_PyroMiaFault = 0;     //Warns if the pyro squib is not connected
static uint8_t battery2_hvsMiaFault = 0;      //Warns if the pack contactor hw fault
static uint8_t battery2_hviMiaFault = 0;      //Warns if the FC contactor hw fault
static uint8_t battery2_Supply12vFault = 0;  //Warns if the low voltage (12V) battery is below minimum voltage threshold
static uint8_t battery2_VerSupplyFault =
    0;                                  //Warns if the Energy reserve voltage supply is below minimum voltage threshold
static uint8_t battery2_HvilFault = 0;  //Warn if a High Voltage Inter Lock fault is detected
static uint8_t battery2_BmsHvsMiaFault = 0;  //Warns if the BMS node is mia on HVS or HVI CAN
static uint8_t battery2_PackVoltMismatchFault =
    0;  //Warns if the pack voltage doesn't match approximately with sum of brick voltages
static uint8_t battery2_EnsMiaFault = 0;         //Warns if the ENS line is not connected to HVC
static uint8_t battery2_PackPosCtrArcFault = 0;  //Warns if the HVP detectes series arc at pack contactor
static uint8_t battery2_packNegCtrArcFault = 0;  //Warns if the HVP detectes series arc at FC contactor
static uint8_t battery2_ShuntHwAndBmsMiaFault = 0;
static uint8_t battery2_fcContHwFault = 0;
static uint8_t battery2_robinOverVoltageFault = 0;
static uint8_t battery2_packContHwFault = 0;
static uint8_t battery2_pyroFuseBlown = 0;
static uint8_t battery2_pyroFuseFailedToBlow = 0;
static uint8_t battery2_CpilFault = 0;
static uint8_t battery2_PackContactorFellOpen = 0;
static uint8_t battery2_FcContactorFellOpen = 0;
static uint8_t battery2_packCtrCloseBlocked = 0;
static uint8_t battery2_fcCtrCloseBlocked = 0;
static uint8_t battery2_packContactorForceOpen = 0;
static uint8_t battery2_fcContactorForceOpen = 0;
static uint8_t battery2_dcLinkOverVoltage = 0;
static uint8_t battery2_shuntOverTemperature = 0;
static uint8_t battery2_passivePyroDeploy = 0;
static uint8_t battery2_logUploadRequest = 0;
static uint8_t battery2_packCtrCloseFailed = 0;
static uint8_t battery2_fcCtrCloseFailed = 0;
static uint8_t battery2_shuntThermistorMia = 0;
#endif  //DOUBLE_BATTERY

static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                      "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                      "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
static const char* contactorState[] = {"SNA",        "OPEN",       "PRECHARGE",   "BLOCKED",
                                       "PULLED_IN",  "OPENING",    "ECONOMIZED",  "WELDED",
                                       "UNKNOWN(8)", "UNKNOWN(9)", "UNKNOWN(10)", "UNKNOWN(11)"};
static const char* hvilStatusState[] = {"NOT OK",
                                        "STATUS_OK",
                                        "CURRENT_SOURCE_FAULT",
                                        "INTERNAL_OPEN_FAULT",
                                        "VEHICLE_OPEN_FAULT",
                                        "PENTHOUSE_LID_OPEN_FAULT",
                                        "UNKNOWN_LOCATION_OPEN_FAULT",
                                        "VEHICLE_NODE_FAULT",
                                        "NO_12V_SUPPLY",
                                        "VEHICLE_OR_PENTHOUSE_LID_OPENFAULT",
                                        "UNKNOWN(10)",
                                        "UNKNOWN(11)",
                                        "UNKNOWN(12)",
                                        "UNKNOWN(13)",
                                        "UNKNOWN(14)",
                                        "UNKNOWN(15)"};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery.status.soh_pptt = 9900;  //Tesla batteries do not send a SOH% value on bus. Hardcode to 99%

  datalayer.battery.status.real_soc = (battery_soc_vi * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = (battery_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery.status.current_dA = battery_amps;  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  // Define the allowed discharge power
  datalayer.battery.status.max_discharge_power_W = (battery_max_discharge_current * battery_volts);
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery.status.max_discharge_power_W > MAXDISCHARGEPOWERALLOWED) {
    datalayer.battery.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery_soc_vi > 990) {
    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery_soc_vi >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        RAMPDOWNPOWERALLOWED * (1 - (battery_soc_vi - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    } else {  //NCM/A
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }

  datalayer.battery.status.active_power_W = ((battery_volts / 10) * battery_amps);

  datalayer.battery.status.temperature_min_dC = battery_min_temp;

  datalayer.battery.status.temperature_max_dC = battery_max_temp;

  datalayer.battery.status.cell_max_voltage_mV = battery_cell_max_v;

  datalayer.battery.status.cell_min_voltage_mV = battery_cell_min_v;

  battery_cell_deviation_mV = (battery_cell_max_v - battery_cell_min_v);

  /* Value mapping is completed. Start to check all safeties */

  if (battery_hvil_status ==
      3) {  //INTERNAL_OPEN_FAULT - Someone disconnected a high voltage cable while battery was in use
    set_event(EVENT_INTERNAL_OPEN_FAULT, 0);
  } else {
    clear_event(EVENT_INTERNAL_OPEN_FAULT);
  }

#ifdef TESLA_MODEL_3Y_BATTERY
  // Autodetect algoritm for chemistry on 3/Y packs.
  // NCM/A batteries have 96s, LFP has 102-106s
  // Drawback with this check is that it takes 3-5minutes before all cells have been counted!
  if (datalayer.battery.info.number_of_cells > 101) {
    datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  }

  //Once cell chemistry is determined, set maximum and minimum total pack voltage safety limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
  } else {  // NCM/A chemistry
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
  }
#endif  // TESLA_MODEL_3Y_BATTERY

  // Update webserver datalayer
  datalayer_extended.tesla.status_contactor = battery_contactor;
  datalayer_extended.tesla.hvil_status = battery_hvil_status;
  datalayer_extended.tesla.packContNegativeState = battery_packContNegativeState;
  datalayer_extended.tesla.packContPositiveState = battery_packContPositiveState;
  datalayer_extended.tesla.packContactorSetState = battery_packContactorSetState;
  datalayer_extended.tesla.packCtrsClosingAllowed = battery_packCtrsClosingAllowed;
  datalayer_extended.tesla.pyroTestInProgress = battery_pyroTestInProgress;

#ifdef DEBUG_VIA_USB

  printFaultCodesIfActive();

  Serial.print("STATUS: Contactor: ");
  Serial.print(contactorText[battery_contactor]);  //Display what state the contactor is in
  Serial.print(", HVIL: ");
  Serial.print(hvilStatusState[battery_hvil_status]);
  Serial.print(", NegativeState: ");
  Serial.print(contactorState[battery_packContNegativeState]);
  Serial.print(", PositiveState: ");
  Serial.print(contactorState[battery_packContPositiveState]);
  Serial.print(", setState: ");
  Serial.print(contactorState[battery_packContactorSetState]);
  Serial.print(", close allowed: ");
  Serial.print(battery_packCtrsClosingAllowed);
  Serial.print(", Pyrotest: ");
  Serial.println(battery_pyroTestInProgress);

  Serial.print("Battery values: ");
  Serial.print("Real SOC: ");
  Serial.print(battery_soc_vi / 10.0, 1);
  print_int_with_units(", Battery voltage: ", battery_volts, "V");
  print_int_with_units(", Battery HV current: ", (battery_amps * 0.1), "A");
  Serial.print(", Fully charged?: ");
  if (battery_full_charge_complete)
    Serial.print("YES, ");
  else
    Serial.print("NO, ");
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    Serial.print("LFP chemistry detected!");
  }
  Serial.println("");
  Serial.print("Cellstats, Max: ");
  Serial.print(battery_cell_max_v);
  Serial.print("mV (cell ");
  Serial.print(battery_max_vno);
  Serial.print("), Min: ");
  Serial.print(battery_cell_min_v);
  Serial.print("mV (cell ");
  Serial.print(battery_min_vno);
  Serial.print("), Imbalance: ");
  Serial.print(battery_cell_deviation_mV);
  Serial.println("mV.");

  print_int_with_units("High Voltage Output Pins: ", battery_high_voltage, "V");
  Serial.print(", ");
  print_int_with_units("Low Voltage: ", battery_low_voltage, "V");
  Serial.println("");
  print_int_with_units("DC/DC 12V current: ", battery_output_current, "A");
  Serial.println("");

  Serial.println("Values passed to the inverter: ");
  print_SOC(" SOC: ", datalayer.battery.status.reported_soc);
  print_int_with_units(" Max discharge power: ", datalayer.battery.status.max_discharge_power_W, "W");
  Serial.print(", ");
  print_int_with_units(" Max charge power: ", datalayer.battery.status.max_charge_power_W, "W");
  Serial.println("");
  print_int_with_units(" Max temperature: ", ((int16_t)datalayer.battery.status.temperature_min_dC * 0.1), "°C");
  Serial.print(", ");
  print_int_with_units(" Min temperature: ", ((int16_t)datalayer.battery.status.temperature_max_dC * 0.1), "°C");
  Serial.println("");
#endif  //DEBUG_VIA_USB
}

void receive_can_battery(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  static uint16_t temp = 0;

  switch (rx_frame.ID) {
    case 0x352:
      //SOC
      battery_nominal_full_pack_energy =
          (((rx_frame.data.u8[1] & 0x0F) << 8) | (rx_frame.data.u8[0]));  //Example 752 (75.2kWh)
      battery_nominal_energy_remaining = (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0xF8) >> 3)) *
                                         0.1;  //Example 1247 * 0.1 = 124.7kWh
      battery_expected_energy_remaining = (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) |
                                           ((rx_frame.data.u8[2] & 0xC0) >> 6));  //Example 622 (62.2kWh)
      battery_ideal_energy_remaining = (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0xFE) >> 1)) *
                                       0.1;  //Example 311 * 0.1 = 31.1kWh
      battery_energy_to_charge_complete = (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) *
                                          0.1;  //Example 147 * 0.1 = 14.7kWh
      battery_energy_buffer =
          (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x80) >> 7)) * 0.1;  //Example 1 * 0.1 = 0
      battery_full_charge_complete = ((rx_frame.data.u8[7] & 0x80) >> 7);
      break;
    case 0x20A:
      //Contactor state
      battery_packContNegativeState = (rx_frame.data.u8[0] & 0x07);
      battery_packContPositiveState = (rx_frame.data.u8[0] & 0x38) >> 3;
      battery_contactor = (rx_frame.data.u8[1] & 0x0F);
      battery_packContactorSetState = (rx_frame.data.u8[1] & 0x0F);
      battery_packCtrsClosingAllowed = (rx_frame.data.u8[4] & 0x08) >> 3;
      battery_pyroTestInProgress = (rx_frame.data.u8[4] & 0x20) >> 5;
      battery_hvil_status = (rx_frame.data.u8[5] & 0x0F);
      break;
    case 0x252:
      //Limits
      battery_regenerative_limit =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 4715 * 0.01 = 47.15kW
      battery_discharge_limit =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.013;  //Example 2009 * 0.013 = 26.117???
      battery_max_heat_park =
          (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[4]) * 0.01;  //Example 500 * 0.01 = 5kW
      battery_hvac_max_power =
          (((rx_frame.data.u8[7] << 6) | ((rx_frame.data.u8[6] & 0xFC) >> 2))) * 0.02;  //Example 1000 * 0.02 = 20kW?
      break;
    case 0x132:
      //battery amps/volts
      battery_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 37030mv * 0.01 = 370V
      battery_amps = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);          //Example 65492 (-4.3A) OR 225 (22.5A)
      battery_raw_amps = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05;  //Example 10425 * -0.05 = ?
      battery_charge_time_remaining =
          (((rx_frame.data.u8[7] & 0x0F) << 8) | rx_frame.data.u8[6]) * 0.1;  //Example 228 * 0.1 = 22.8min
      if (battery_charge_time_remaining == 4095) {
        battery_charge_time_remaining = 0;
      }

      break;
    case 0x3D2:
      // total charge/discharge kwh
      battery_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                 (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                                0.001;
      battery_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                              rx_frame.data.u8[4]) *
                             0.001;
      break;
    case 0x332:
      //min/max hist values
      mux = (rx_frame.data.u8[0] & 0x03);

      if (mux == 1)  //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        battery_cell_max_v = temp * 2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        battery_cell_min_v = temp * 2;
        battery_max_vno = 1 + (rx_frame.data.u8[4] & 0x7F);  //This cell has highest voltage
        battery_min_vno = 1 + (rx_frame.data.u8[5] & 0x7F);  //This cell has lowest voltage
      }
      if (mux == 0)  //Temperature sensors
      {
        battery_max_temp = (rx_frame.data.u8[2] * 5) - 400;  //Temperature values have 40.0*C offset, 0.5*C /bit
        battery_min_temp =
            (rx_frame.data.u8[3] * 5) - 400;  //Multiply by 5 and remove offset to get C+1 (0x61*5=485-400=8.5*C)
      }
      break;
    case 0x401:  // Cell stats
      mux = (rx_frame.data.u8[0]);

      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

      if (rx_frame.data.u8[1] == 0x2A)  // status byte must be 0x2A to read cellvoltages
      {
        // Example, frame3=0x89,frame2=0x1D = 35101 / 10 = 3510mV
        volts = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 10;
        datalayer.battery.status.cell_voltages_mV[mux * 3] = volts;
        volts = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) / 10;
        datalayer.battery.status.cell_voltages_mV[1 + mux * 3] = volts;
        volts = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) / 10;
        datalayer.battery.status.cell_voltages_mV[2 + mux * 3] = volts;

        // Track the max value of mux. If we've seen two 0 values for mux, we've probably gathered all
        // cell voltages. Then, 2 + mux_max * 3 + 1 is the number of cell voltages.
        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = 2 + 3 * mux_max + 1;
            // Increase the counter arbitrarily another time to make the initial if-statement evaluate to false
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x2d2:
      //Min / max limits
      battery_bms_min_voltage =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01 * 2;  //Example 24148mv * 0.01 = 241.48 V
      battery_bms_max_voltage =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.01 * 2;  //Example 40282mv * 0.01 = 402.82 V
      battery_max_charge_current =
          (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1;  //Example 1301? * 0.1 = 130.1?
      battery_max_discharge_current =
          (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128;  //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:
      battery_low_voltage = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]) * 0.0390625;
      battery_high_voltage = ((((rx_frame.data.u8[2] & 0x3F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2))) * 0.146484;
      battery_output_current = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]) / 100;
      break;
    case 0x292:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  //We are getting CAN messages from the BMS
      battery_beginning_of_life = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[5]);
      battery_soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      battery_soc_vi = (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));
      battery_soc_max = (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));
      battery_soc_ave = ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));
      break;
    case 0x3aa:  //HVP_alertMatrix1
      battery_WatchdogReset = (rx_frame.data.u8[0] & 0x01);
      battery_PowerLossReset = ((rx_frame.data.u8[0] & 0x02) >> 1);
      battery_SwAssertion = ((rx_frame.data.u8[0] & 0x04) >> 2);
      battery_CrashEvent = ((rx_frame.data.u8[0] & 0x08) >> 3);
      battery_OverDchgCurrentFault = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery_OverChargeCurrentFault = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery_OverCurrentFault = ((rx_frame.data.u8[0] & 0x40) >> 6);
      battery_OverTemperatureFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery_OverVoltageFault = (rx_frame.data.u8[1] & 0x01);
      battery_UnderVoltageFault = ((rx_frame.data.u8[1] & 0x02) >> 1);
      battery_PrimaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x04) >> 2);
      battery_SecondaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x08) >> 3);
      battery_BmbMismatchFault = ((rx_frame.data.u8[1] & 0x10) >> 4);
      battery_BmsHviMiaFault = ((rx_frame.data.u8[1] & 0x20) >> 5);
      battery_CpMiaFault = ((rx_frame.data.u8[1] & 0x40) >> 6);
      battery_PcsMiaFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery_BmsFault = (rx_frame.data.u8[2] & 0x01);
      battery_PcsFault = ((rx_frame.data.u8[2] & 0x02) >> 1);
      battery_CpFault = ((rx_frame.data.u8[2] & 0x04) >> 2);
      battery_ShuntHwMiaFault = ((rx_frame.data.u8[2] & 0x08) >> 3);
      battery_PyroMiaFault = ((rx_frame.data.u8[2] & 0x10) >> 4);
      battery_hvsMiaFault = ((rx_frame.data.u8[2] & 0x20) >> 5);
      battery_hviMiaFault = ((rx_frame.data.u8[2] & 0x40) >> 6);
      battery_Supply12vFault = ((rx_frame.data.u8[2] & 0x80) >> 7);
      battery_VerSupplyFault = (rx_frame.data.u8[3] & 0x01);
      battery_HvilFault = ((rx_frame.data.u8[3] & 0x02) >> 1);
      battery_BmsHvsMiaFault = ((rx_frame.data.u8[3] & 0x04) >> 2);
      battery_PackVoltMismatchFault = ((rx_frame.data.u8[3] & 0x08) >> 3);
      battery_EnsMiaFault = ((rx_frame.data.u8[3] & 0x10) >> 4);
      battery_PackPosCtrArcFault = ((rx_frame.data.u8[3] & 0x20) >> 5);
      battery_packNegCtrArcFault = ((rx_frame.data.u8[3] & 0x40) >> 6);
      battery_ShuntHwAndBmsMiaFault = ((rx_frame.data.u8[3] & 0x80) >> 7);
      battery_fcContHwFault = (rx_frame.data.u8[4] & 0x01);
      battery_robinOverVoltageFault = ((rx_frame.data.u8[4] & 0x02) >> 1);
      battery_packContHwFault = ((rx_frame.data.u8[4] & 0x04) >> 2);
      battery_pyroFuseBlown = ((rx_frame.data.u8[4] & 0x08) >> 3);
      battery_pyroFuseFailedToBlow = ((rx_frame.data.u8[4] & 0x10) >> 4);
      battery_CpilFault = ((rx_frame.data.u8[4] & 0x20) >> 5);
      battery_PackContactorFellOpen = ((rx_frame.data.u8[4] & 0x40) >> 6);
      battery_FcContactorFellOpen = ((rx_frame.data.u8[4] & 0x80) >> 7);
      battery_packCtrCloseBlocked = (rx_frame.data.u8[5] & 0x01);
      battery_fcCtrCloseBlocked = ((rx_frame.data.u8[5] & 0x02) >> 1);
      battery_packContactorForceOpen = ((rx_frame.data.u8[5] & 0x04) >> 2);
      battery_fcContactorForceOpen = ((rx_frame.data.u8[5] & 0x08) >> 3);
      battery_dcLinkOverVoltage = ((rx_frame.data.u8[5] & 0x10) >> 4);
      battery_shuntOverTemperature = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery_passivePyroDeploy = ((rx_frame.data.u8[5] & 0x40) >> 6);
      battery_logUploadRequest = ((rx_frame.data.u8[5] & 0x80) >> 7);
      battery_packCtrCloseFailed = (rx_frame.data.u8[6] & 0x01);
      battery_fcCtrCloseFailed = ((rx_frame.data.u8[6] & 0x02) >> 1);
      battery_shuntThermistorMia = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;
    default:
      break;
  }
}

#ifdef DOUBLE_BATTERY

void receive_can_battery2(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  static uint16_t temp = 0;

  switch (rx_frame.ID) {
    case 0x352:
      //SOC
      battery2_nominal_full_pack_energy =
          (((rx_frame.data.u8[1] & 0x0F) << 8) | (rx_frame.data.u8[0]));  //Example 752 (75.2kWh)
      battery2_nominal_energy_remaining = (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0xF8) >> 3)) *
                                          0.1;  //Example 1247 * 0.1 = 124.7kWh
      battery2_expected_energy_remaining = (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) |
                                            ((rx_frame.data.u8[2] & 0xC0) >> 6));  //Example 622 (62.2kWh)
      battery2_ideal_energy_remaining = (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0xFE) >> 1)) *
                                        0.1;  //Example 311 * 0.1 = 31.1kWh
      battery2_energy_to_charge_complete = (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) *
                                           0.1;  //Example 147 * 0.1 = 14.7kWh
      battery2_energy_buffer =
          (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x80) >> 7)) * 0.1;  //Example 1 * 0.1 = 0
      battery2_full_charge_complete = ((rx_frame.data.u8[7] & 0x80) >> 7);
      break;
    case 0x20A:
      //Contactor state
      battery2_packContNegativeState = (rx_frame.data.u8[0] & 0x07);
      battery2_packContPositiveState = (rx_frame.data.u8[0] & 0x38) >> 3;
      battery2_contactor = (rx_frame.data.u8[1] & 0x0F);
      battery2_packContactorSetState = (rx_frame.data.u8[1] & 0x0F);
      battery2_packCtrsClosingAllowed = (rx_frame.data.u8[4] & 0x08) >> 3;
      battery2_pyroTestInProgress = (rx_frame.data.u8[4] & 0x20) >> 5;
      battery2_hvil_status = (rx_frame.data.u8[5] & 0x0F);
      break;
    case 0x252:
      //Limits
      battery2_regenerative_limit =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 4715 * 0.01 = 47.15kW
      battery2_discharge_limit =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.013;  //Example 2009 * 0.013 = 26.117???
      battery2_max_heat_park =
          (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[4]) * 0.01;  //Example 500 * 0.01 = 5kW
      battery2_hvac_max_power =
          (((rx_frame.data.u8[7] << 6) | ((rx_frame.data.u8[6] & 0xFC) >> 2))) * 0.02;  //Example 1000 * 0.02 = 20kW?
      break;
    case 0x132:
      //battery amps/volts
      battery2_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 37030mv * 0.01 = 370V
      battery2_amps = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);  //Example 65492 (-4.3A) OR 225 (22.5A)
      battery2_raw_amps = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05;  //Example 10425 * -0.05 = ?
      battery2_charge_time_remaining =
          (((rx_frame.data.u8[7] & 0x0F) << 8) | rx_frame.data.u8[6]) * 0.1;  //Example 228 * 0.1 = 22.8min
      if (battery2_charge_time_remaining == 4095) {
        battery2_charge_time_remaining = 0;
      }

      break;
    case 0x3D2:
      // total charge/discharge kwh
      battery2_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                  (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                                 0.001;
      battery2_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                               rx_frame.data.u8[4]) *
                              0.001;
      break;
    case 0x332:
      //min/max hist values
      mux = (rx_frame.data.u8[0] & 0x03);

      if (mux == 1)  //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        battery2_cell_max_v = temp * 2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        battery2_cell_min_v = temp * 2;
        battery2_max_vno = 1 + (rx_frame.data.u8[4] & 0x7F);  //This cell has highest voltage
        battery2_min_vno = 1 + (rx_frame.data.u8[5] & 0x7F);  //This cell has lowest voltage
      }
      if (mux == 0)  //Temperature sensors
      {
        battery2_max_temp = (rx_frame.data.u8[2] * 5) - 400;  //Temperature values have 40.0*C offset, 0.5*C /bit
        battery2_min_temp =
            (rx_frame.data.u8[3] * 5) - 400;  //Multiply by 5 and remove offset to get C+1 (0x61*5=485-400=8.5*C)
      }
      break;
    case 0x401:  // Cell stats
      mux = (rx_frame.data.u8[0]);

      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

      if (rx_frame.data.u8[1] == 0x2A)  // status byte must be 0x2A to read cellvoltages
      {
        // Example, frame3=0x89,frame2=0x1D = 35101 / 10 = 3510mV
        volts = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 10;
        datalayer.battery.status.cell_voltages_mV[mux * 3] = volts;
        volts = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) / 10;
        datalayer.battery.status.cell_voltages_mV[1 + mux * 3] = volts;
        volts = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) / 10;
        datalayer.battery.status.cell_voltages_mV[2 + mux * 3] = volts;

        // Track the max value of mux. If we've seen two 0 values for mux, we've probably gathered all
        // cell voltages. Then, 2 + mux_max * 3 + 1 is the number of cell voltages.
        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = 2 + 3 * mux_max + 1;
            // Increase the counter arbitrarily another time to make the initial if-statement evaluate to false
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x2d2:
      //Min / max limits
      battery2_bms_min_voltage =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01 * 2;  //Example 24148mv * 0.01 = 241.48 V
      battery2_bms_max_voltage =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.01 * 2;  //Example 40282mv * 0.01 = 402.82 V
      battery2_max_charge_current =
          (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1;  //Example 1301? * 0.1 = 130.1?
      battery2_max_discharge_current =
          (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128;  //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:
      battery2_low_voltage = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]) * 0.0390625;
      battery2_high_voltage = ((((rx_frame.data.u8[2] & 0x3F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2))) * 0.146484;
      battery2_output_current = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]) / 100;
      break;
    case 0x292:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  //We are getting CAN messages from the BMS
      battery2_beginning_of_life = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[5]);
      battery2_soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      battery2_soc_vi = (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));
      battery2_soc_max = (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));
      battery2_soc_ave = ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));
      break;
    case 0x3aa:  //HVP_alertMatrix1
      battery2_WatchdogReset = (rx_frame.data.u8[0] & 0x01);
      battery2_PowerLossReset = ((rx_frame.data.u8[0] & 0x02) >> 1);
      battery2_SwAssertion = ((rx_frame.data.u8[0] & 0x04) >> 2);
      battery2_CrashEvent = ((rx_frame.data.u8[0] & 0x08) >> 3);
      battery2_OverDchgCurrentFault = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery2_OverChargeCurrentFault = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery2_OverCurrentFault = ((rx_frame.data.u8[0] & 0x40) >> 6);
      battery2_OverTemperatureFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery2_OverVoltageFault = (rx_frame.data.u8[1] & 0x01);
      battery2_UnderVoltageFault = ((rx_frame.data.u8[1] & 0x02) >> 1);
      battery2_PrimaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x04) >> 2);
      battery2_SecondaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x08) >> 3);
      battery2_BmbMismatchFault = ((rx_frame.data.u8[1] & 0x10) >> 4);
      battery2_BmsHviMiaFault = ((rx_frame.data.u8[1] & 0x20) >> 5);
      battery2_CpMiaFault = ((rx_frame.data.u8[1] & 0x40) >> 6);
      battery2_PcsMiaFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery2_BmsFault = (rx_frame.data.u8[2] & 0x01);
      battery2_PcsFault = ((rx_frame.data.u8[2] & 0x02) >> 1);
      battery2_CpFault = ((rx_frame.data.u8[2] & 0x04) >> 2);
      battery2_ShuntHwMiaFault = ((rx_frame.data.u8[2] & 0x08) >> 3);
      battery2_PyroMiaFault = ((rx_frame.data.u8[2] & 0x10) >> 4);
      battery2_hvsMiaFault = ((rx_frame.data.u8[2] & 0x20) >> 5);
      battery2_hviMiaFault = ((rx_frame.data.u8[2] & 0x40) >> 6);
      battery2_Supply12vFault = ((rx_frame.data.u8[2] & 0x80) >> 7);
      battery2_VerSupplyFault = (rx_frame.data.u8[3] & 0x01);
      battery2_HvilFault = ((rx_frame.data.u8[3] & 0x02) >> 1);
      battery2_BmsHvsMiaFault = ((rx_frame.data.u8[3] & 0x04) >> 2);
      battery2_PackVoltMismatchFault = ((rx_frame.data.u8[3] & 0x08) >> 3);
      battery2_EnsMiaFault = ((rx_frame.data.u8[3] & 0x10) >> 4);
      battery2_PackPosCtrArcFault = ((rx_frame.data.u8[3] & 0x20) >> 5);
      battery2_packNegCtrArcFault = ((rx_frame.data.u8[3] & 0x40) >> 6);
      battery2_ShuntHwAndBmsMiaFault = ((rx_frame.data.u8[3] & 0x80) >> 7);
      battery2_fcContHwFault = (rx_frame.data.u8[4] & 0x01);
      battery2_robinOverVoltageFault = ((rx_frame.data.u8[4] & 0x02) >> 1);
      battery2_packContHwFault = ((rx_frame.data.u8[4] & 0x04) >> 2);
      battery2_pyroFuseBlown = ((rx_frame.data.u8[4] & 0x08) >> 3);
      battery2_pyroFuseFailedToBlow = ((rx_frame.data.u8[4] & 0x10) >> 4);
      battery2_CpilFault = ((rx_frame.data.u8[4] & 0x20) >> 5);
      battery2_PackContactorFellOpen = ((rx_frame.data.u8[4] & 0x40) >> 6);
      battery2_FcContactorFellOpen = ((rx_frame.data.u8[4] & 0x80) >> 7);
      battery2_packCtrCloseBlocked = (rx_frame.data.u8[5] & 0x01);
      battery2_fcCtrCloseBlocked = ((rx_frame.data.u8[5] & 0x02) >> 1);
      battery2_packContactorForceOpen = ((rx_frame.data.u8[5] & 0x04) >> 2);
      battery2_fcContactorForceOpen = ((rx_frame.data.u8[5] & 0x08) >> 3);
      battery2_dcLinkOverVoltage = ((rx_frame.data.u8[5] & 0x10) >> 4);
      battery2_shuntOverTemperature = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery2_passivePyroDeploy = ((rx_frame.data.u8[5] & 0x40) >> 6);
      battery2_logUploadRequest = ((rx_frame.data.u8[5] & 0x80) >> 7);
      battery2_packCtrCloseFailed = (rx_frame.data.u8[6] & 0x01);
      battery2_fcCtrCloseFailed = ((rx_frame.data.u8[6] & 0x02) >> 1);
      battery2_shuntThermistorMia = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;
    default:
      break;
  }
}

void update_values_battery2() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery2.status.soh_pptt = 9900;  //Tesla batteries do not send a SOH% value on bus. Hardcode to 99%

  datalayer.battery2.status.real_soc = (battery2_soc_vi * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery2.status.voltage_dV = (battery2_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery2.status.current_dA = battery2_amps;  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery2.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery2.status.real_soc) / 10000) * datalayer.battery2.info.total_capacity_Wh);

  // Define the allowed discharge power
  datalayer.battery2.status.max_discharge_power_W = (battery2_max_discharge_current * battery2_volts);
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery2.status.max_discharge_power_W > MAXDISCHARGEPOWERALLOWED) {
    datalayer.battery2.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery2_soc_vi > 990) {
    datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery2_soc_vi >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery2.status.max_charge_power_W =
        MAXCHARGEPOWERALLOWED * (1 - (battery2_soc_vi - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
      if (battery2_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
        datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    } else {  //NCM/A
      if (battery2_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
        datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery2.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }

  datalayer.battery2.status.active_power_W = ((battery2_volts / 10) * battery2_amps);

  datalayer.battery2.status.temperature_min_dC = battery2_min_temp;

  datalayer.battery2.status.temperature_max_dC = battery2_max_temp;

  datalayer.battery2.status.cell_max_voltage_mV = battery2_cell_max_v;

  datalayer.battery2.status.cell_min_voltage_mV = battery2_cell_min_v;

  battery2_cell_deviation_mV = (battery2_cell_max_v - battery2_cell_min_v);

  /* Value mapping is completed. Start to check all safeties */

  if (battery2_hvil_status ==
      3) {  //INTERNAL_OPEN_FAULT - Someone disconnected a high voltage cable while battery was in use
    set_event(EVENT_INTERNAL_OPEN_FAULT, 2);
  } else {
    clear_event(EVENT_INTERNAL_OPEN_FAULT);
  }

#ifdef TESLA_MODEL_3Y_BATTERY
  // Autodetect algoritm for chemistry on 3/Y packs.
  // NCM/A batteries have 96s, LFP has 102-106s
  // Drawback with this check is that it takes 3-5minutes before all cells have been counted!
  if (datalayer.battery2.info.number_of_cells > 101) {
    datalayer.battery2.info.chemistry = battery_chemistry_enum::LFP;
  }

  //Once cell chemistry is determined, set maximum and minimum total pack voltage safety limits
  if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
    datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
    datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
  } else {  // NCM/A chemistry
    datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
  }
#endif  // TESLA_MODEL_3Y_BATTERY

#ifdef DEBUG_VIA_USB

  printFaultCodesIfActive_battery2();

  Serial.print("STATUS: Contactor: ");
  Serial.print(contactorText[battery2_contactor]);  //Display what state the contactor is in
  Serial.print(", HVIL: ");
  Serial.print(hvilStatusState[battery2_hvil_status]);
  Serial.print(", NegativeState: ");
  Serial.print(contactorState[battery2_packContNegativeState]);
  Serial.print(", PositiveState: ");
  Serial.print(contactorState[battery2_packContPositiveState]);
  Serial.print(", setState: ");
  Serial.print(contactorState[battery2_packContactorSetState]);
  Serial.print(", close allowed: ");
  Serial.print(battery2_packCtrsClosingAllowed);
  Serial.print(", Pyrotest: ");
  Serial.println(battery2_pyroTestInProgress);

  Serial.print("Battery2 values: ");
  Serial.print("Real SOC: ");
  Serial.print(battery2_soc_vi / 10.0, 1);
  print_int_with_units(", Battery2 voltage: ", battery2_volts, "V");
  print_int_with_units(", Battery2 HV current: ", (battery2_amps * 0.1), "A");
  Serial.print(", Fully charged?: ");
  if (battery2_full_charge_complete)
    Serial.print("YES, ");
  else
    Serial.print("NO, ");
  if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
    Serial.print("LFP chemistry detected!");
  }
  Serial.println("");
  Serial.print("Cellstats, Max: ");
  Serial.print(battery2_cell_max_v);
  Serial.print("mV (cell ");
  Serial.print(battery2_max_vno);
  Serial.print("), Min: ");
  Serial.print(battery2_cell_min_v);
  Serial.print("mV (cell ");
  Serial.print(battery2_min_vno);
  Serial.print("), Imbalance: ");
  Serial.print(battery2_cell_deviation_mV);
  Serial.println("mV.");

  print_int_with_units("High Voltage Output Pins: ", battery2_high_voltage, "V");
  Serial.print(", ");
  print_int_with_units("Low Voltage: ", battery2_low_voltage, "V");
  Serial.println("");
  print_int_with_units("DC/DC 12V current: ", battery2_output_current, "A");
  Serial.println("");

  Serial.println("Values passed to the inverter: ");
  print_SOC(" SOC: ", datalayer.battery2.status.reported_soc);
  print_int_with_units(" Max discharge power: ", datalayer.battery2.status.max_discharge_power_W, "W");
  Serial.print(", ");
  print_int_with_units(" Max charge power: ", datalayer.battery2.status.max_charge_power_W, "W");
  Serial.println("");
  print_int_with_units(" Max temperature: ", ((int16_t)datalayer.battery2.status.temperature_min_dC * 0.1), "°C");
  Serial.print(", ");
  print_int_with_units(" Min temperature: ", ((int16_t)datalayer.battery2.status.temperature_max_dC * 0.1), "°C");
  Serial.println("");
#endif  // DEBUG_VIA_USB
}

#endif  //DOUBLE_BATTERY

void send_can_battery() {
  /*From bielec: My fist 221 message, to close the contactors is 0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96 and then, 
to cause "hv_up_for_drive" I send an additional 221 message 0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA  so 
two 221 messages are being continuously transmitted.   When I want to shut down, I stop the second message and only send 
the first, for a few cycles, then stop all  messages which causes the contactor to open. */

  unsigned long currentMillis = millis();
  //Send 30ms message
  if (currentMillis - previousMillis30 >= INTERVAL_30_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis30 >= INTERVAL_30_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis30));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis30 = currentMillis;

    if (datalayer.system.status.inverter_allows_contactor_closing) {
      transmit_can(&TESLA_221_1, can_config.battery);
      transmit_can(&TESLA_221_2, can_config.battery);
#ifdef DOUBLE_BATTERY
      if (datalayer.system.status.battery2_allows_contactor_closing) {
        transmit_can(&TESLA_221_1, can_config.battery_double);  // CAN2 connected to battery 2
        transmit_can(&TESLA_221_2, can_config.battery_double);
      }
#endif  //DOUBLE_BATTERY
    }
  }
}

void print_int_with_units(char* header, int value, char* units) {
  Serial.print(header);
  Serial.print(value);
  Serial.print(units);
}

void print_SOC(char* header, int SOC) {
  Serial.print(header);
  Serial.print(SOC / 100);
  Serial.print(".");
  int hundredth = SOC % 100;
  if (hundredth < 10)
    Serial.print(0);
  Serial.print(hundredth);
  Serial.println("%");
}

void printFaultCodesIfActive() {
  if (battery_packCtrsClosingAllowed == 0) {
    Serial.println(
        "ERROR: Check high voltage connectors and interlock circuit! Closing contactor not allowed! Values: ");
  }
  if (battery_pyroTestInProgress == 1) {
    Serial.println("ERROR: Please wait for Pyro Connection check to finish, HV cables successfully seated!");
  }
  if (datalayer.system.status.inverter_allows_contactor_closing == false) {
    Serial.println(
        "ERROR: Solar inverter does not allow for contactor closing. Check communication connection to the inverter OR "
        "disable the inverter protocol to proceed with contactor closing");
  }
  // Check each symbol and print debug information if its value is 1
  printDebugIfActive(battery_WatchdogReset, "ERROR: The processor has experienced a reset due to watchdog reset");
  printDebugIfActive(battery_PowerLossReset, "ERROR: The processor has experienced a reset due to power loss");
  printDebugIfActive(battery_SwAssertion, "ERROR: An internal software assertion has failed");
  printDebugIfActive(battery_CrashEvent, "ERROR: crash signal is detected by HVP");
  printDebugIfActive(battery_OverDchgCurrentFault,
                     "ERROR: Pack discharge current is above the safe max discharge current limit!");
  printDebugIfActive(battery_OverChargeCurrentFault,
                     "ERROR: Pack charge current is above the safe max charge current limit!");
  printDebugIfActive(battery_OverCurrentFault, "ERROR: Pack current (discharge or charge) is above max current limit!");
  printDebugIfActive(battery_OverTemperatureFault,
                     "ERROR: A pack module temperature is above the max temperature limit!");
  printDebugIfActive(battery_OverVoltageFault, "ERROR: A brick voltage is above maximum voltage limit");
  printDebugIfActive(battery_UnderVoltageFault, "ERROR: A brick voltage is below minimum voltage limit");
  printDebugIfActive(battery_PrimaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from primary BMB chain are mia");
  printDebugIfActive(battery_SecondaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from secondary BMB chain are mia");
  printDebugIfActive(battery_BmbMismatchFault,
                     "ERROR: primary and secondary BMB chain readings don't match with each other");
  printDebugIfActive(battery_BmsHviMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  //printDebugIfActive(battery_CpMiaFault, "ERROR: CP node is mia on HVS CAN"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsMiaFault, "ERROR: PCS node is mia on HVS CAN");
  //printDebugIfActive(battery_BmsFault, "ERROR: BmsFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsFault, "ERROR: PcsFault is active");
  //printDebugIfActive(battery_CpFault, "ERROR: CpFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_ShuntHwMiaFault, "ERROR: shunt current reading is not available");
  printDebugIfActive(battery_PyroMiaFault, "ERROR: pyro squib is not connected");
  printDebugIfActive(battery_hvsMiaFault, "ERROR: pack contactor hw fault");
  printDebugIfActive(battery_hviMiaFault, "ERROR: FC contactor hw fault");
  printDebugIfActive(battery_Supply12vFault, "ERROR: Low voltage (12V) battery is below minimum voltage threshold");
  printDebugIfActive(battery_VerSupplyFault, "ERROR: Energy reserve voltage supply is below minimum voltage threshold");
  printDebugIfActive(battery_HvilFault, "ERROR: High Voltage Inter Lock fault is detected");
  printDebugIfActive(battery_BmsHvsMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  printDebugIfActive(battery_PackVoltMismatchFault,
                     "ERROR: Pack voltage doesn't match approximately with sum of brick voltages");
  //printDebugIfActive(battery_EnsMiaFault, "ERROR: ENS line is not connected to HVC"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackPosCtrArcFault, "ERROR: HVP detectes series arc at pack contactor");
  printDebugIfActive(battery_packNegCtrArcFault, "ERROR: HVP detectes series arc at FC contactor");
  printDebugIfActive(battery_ShuntHwAndBmsMiaFault, "ERROR: ShuntHwAndBmsMiaFault is active");
  printDebugIfActive(battery_fcContHwFault, "ERROR: fcContHwFault is active");
  printDebugIfActive(battery_robinOverVoltageFault, "ERROR: robinOverVoltageFault is active");
  printDebugIfActive(battery_packContHwFault, "ERROR: packContHwFault is active");
  printDebugIfActive(battery_pyroFuseBlown, "ERROR: pyroFuseBlown is active");
  printDebugIfActive(battery_pyroFuseFailedToBlow, "ERROR: pyroFuseFailedToBlow is active");
  //printDebugIfActive(battery_CpilFault, "ERROR: CpilFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackContactorFellOpen, "ERROR: PackContactorFellOpen is active");
  printDebugIfActive(battery_FcContactorFellOpen, "ERROR: FcContactorFellOpen is active");
  printDebugIfActive(battery_packCtrCloseBlocked, "ERROR: packCtrCloseBlocked is active");
  printDebugIfActive(battery_fcCtrCloseBlocked, "ERROR: fcCtrCloseBlocked is active");
  printDebugIfActive(battery_packContactorForceOpen, "ERROR: packContactorForceOpen is active");
  printDebugIfActive(battery_fcContactorForceOpen, "ERROR: fcContactorForceOpen is active");
  printDebugIfActive(battery_dcLinkOverVoltage, "ERROR: dcLinkOverVoltage is active");
  printDebugIfActive(battery_shuntOverTemperature, "ERROR: shuntOverTemperature is active");
  printDebugIfActive(battery_passivePyroDeploy, "ERROR: passivePyroDeploy is active");
  printDebugIfActive(battery_logUploadRequest, "ERROR: logUploadRequest is active");
  printDebugIfActive(battery_packCtrCloseFailed, "ERROR: packCtrCloseFailed is active");
  printDebugIfActive(battery_fcCtrCloseFailed, "ERROR: fcCtrCloseFailed is active");
  printDebugIfActive(battery_shuntThermistorMia, "ERROR: shuntThermistorMia is active");
}

#ifdef DOUBLE_BATTERY
void printFaultCodesIfActive_battery2() {
  if (battery2_packCtrsClosingAllowed == 0) {
    Serial.println(
        "ERROR: Check high voltage connectors and interlock circuit! Closing contactor not allowed! Values: ");
  }
  if (battery2_pyroTestInProgress == 1) {
    Serial.println("ERROR: Please wait for Pyro Connection check to finish, HV cables successfully seated!");
  }
  if (datalayer.system.status.inverter_allows_contactor_closing == false) {
    Serial.println(
        "ERROR: Solar inverter does not allow for contactor closing. Check communication connection to the inverter OR "
        "disable the inverter protocol to proceed with contactor closing");
  }
  // Check each symbol and print debug information if its value is 1
  printDebugIfActive(battery2_WatchdogReset, "ERROR: The processor has experienced a reset due to watchdog reset");
  printDebugIfActive(battery2_PowerLossReset, "ERROR: The processor has experienced a reset due to power loss");
  printDebugIfActive(battery2_SwAssertion, "ERROR: An internal software assertion has failed");
  printDebugIfActive(battery2_CrashEvent, "ERROR: crash signal is detected by HVP");
  printDebugIfActive(battery2_OverDchgCurrentFault,
                     "ERROR: Pack discharge current is above the safe max discharge current limit!");
  printDebugIfActive(battery2_OverChargeCurrentFault,
                     "ERROR: Pack charge current is above the safe max charge current limit!");
  printDebugIfActive(battery2_OverCurrentFault,
                     "ERROR: Pack current (discharge or charge) is above max current limit!");
  printDebugIfActive(battery2_OverTemperatureFault,
                     "ERROR: A pack module temperature is above the max temperature limit!");
  printDebugIfActive(battery2_OverVoltageFault, "ERROR: A brick voltage is above maximum voltage limit");
  printDebugIfActive(battery2_UnderVoltageFault, "ERROR: A brick voltage is below minimum voltage limit");
  printDebugIfActive(battery2_PrimaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from primary BMB chain are mia");
  printDebugIfActive(battery2_SecondaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from secondary BMB chain are mia");
  printDebugIfActive(battery2_BmbMismatchFault,
                     "ERROR: primary and secondary BMB chain readings don't match with each other");
  printDebugIfActive(battery2_BmsHviMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  //printDebugIfActive(battery2_CpMiaFault, "ERROR: CP node is mia on HVS CAN"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PcsMiaFault, "ERROR: PCS node is mia on HVS CAN");
  //printDebugIfActive(battery2_BmsFault, "ERROR: BmsFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PcsFault, "ERROR: PcsFault is active");
  //printDebugIfActive(battery2_CpFault, "ERROR: CpFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_ShuntHwMiaFault, "ERROR: shunt current reading is not available");
  printDebugIfActive(battery2_PyroMiaFault, "ERROR: pyro squib is not connected");
  printDebugIfActive(battery2_hvsMiaFault, "ERROR: pack contactor hw fault");
  printDebugIfActive(battery2_hviMiaFault, "ERROR: FC contactor hw fault");
  printDebugIfActive(battery2_Supply12vFault, "ERROR: Low voltage (12V) battery is below minimum voltage threshold");
  printDebugIfActive(battery2_VerSupplyFault,
                     "ERROR: Energy reserve voltage supply is below minimum voltage threshold");
  printDebugIfActive(battery2_HvilFault, "ERROR: High Voltage Inter Lock fault is detected");
  printDebugIfActive(battery2_BmsHvsMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  printDebugIfActive(battery2_PackVoltMismatchFault,
                     "ERROR: Pack voltage doesn't match approximately with sum of brick voltages");
  //printDebugIfActive(battery2_EnsMiaFault, "ERROR: ENS line is not connected to HVC"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PackPosCtrArcFault, "ERROR: HVP detectes series arc at pack contactor");
  printDebugIfActive(battery2_packNegCtrArcFault, "ERROR: HVP detectes series arc at FC contactor");
  printDebugIfActive(battery2_ShuntHwAndBmsMiaFault, "ERROR: ShuntHwAndBmsMiaFault is active");
  printDebugIfActive(battery2_fcContHwFault, "ERROR: fcContHwFault is active");
  printDebugIfActive(battery2_robinOverVoltageFault, "ERROR: robinOverVoltageFault is active");
  printDebugIfActive(battery2_packContHwFault, "ERROR: packContHwFault is active");
  printDebugIfActive(battery2_pyroFuseBlown, "ERROR: pyroFuseBlown is active");
  printDebugIfActive(battery2_pyroFuseFailedToBlow, "ERROR: pyroFuseFailedToBlow is active");
  //printDebugIfActive(battery2_CpilFault, "ERROR: CpilFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PackContactorFellOpen, "ERROR: PackContactorFellOpen is active");
  printDebugIfActive(battery2_FcContactorFellOpen, "ERROR: FcContactorFellOpen is active");
  printDebugIfActive(battery2_packCtrCloseBlocked, "ERROR: packCtrCloseBlocked is active");
  printDebugIfActive(battery2_fcCtrCloseBlocked, "ERROR: fcCtrCloseBlocked is active");
  printDebugIfActive(battery2_packContactorForceOpen, "ERROR: packContactorForceOpen is active");
  printDebugIfActive(battery2_fcContactorForceOpen, "ERROR: fcContactorForceOpen is active");
  printDebugIfActive(battery2_dcLinkOverVoltage, "ERROR: dcLinkOverVoltage is active");
  printDebugIfActive(battery2_shuntOverTemperature, "ERROR: shuntOverTemperature is active");
  printDebugIfActive(battery2_passivePyroDeploy, "ERROR: passivePyroDeploy is active");
  printDebugIfActive(battery2_logUploadRequest, "ERROR: logUploadRequest is active");
  printDebugIfActive(battery2_packCtrCloseFailed, "ERROR: packCtrCloseFailed is active");
  printDebugIfActive(battery2_fcCtrCloseFailed, "ERROR: fcCtrCloseFailed is active");
  printDebugIfActive(battery2_shuntThermistorMia, "ERROR: shuntThermistorMia is active");
}
#endif  //DOUBLE_BATTERY

void printDebugIfActive(uint8_t symbol, const char* message) {
  if (symbol == 1) {
    Serial.println(message);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Tesla Model S/3/X/Y battery selected");
#endif

  datalayer.system.status.battery_allows_contactor_closing = true;

#ifdef TESLA_MODEL_SX_BATTERY  // Always use NCM/A mode on S/X packs
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#endif  // DOUBLE_BATTERY
#endif  // TESLA_MODEL_SX_BATTERY

#ifdef TESLA_MODEL_3Y_BATTERY  // Model 3/Y can be either LFP or NCM/A
#ifdef LFP_CHEMISTRY
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
#endif  // DOUBLE_BATTERY
#else   // Startup in NCM/A mode
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#endif  // DOUBLE_BATTERY
#endif  // !LFP_CHEMISTRY
#endif  // TESLA_MODEL_3Y_BATTERY
}

#endif  // TESLA_BATTERY
