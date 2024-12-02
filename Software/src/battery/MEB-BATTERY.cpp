#include "../include.h"
#ifdef MEB_BATTERY
#include <algorithm>  // For std::min and std::max
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "MEB-BATTERY.h"

/*
TODO list
- Check value mappings on the PID polls
- Check all TODO:s in the code
- 0x1B000044 & 1B00008F seems to be missing from logs? (Classic CAN)
- Scaled remaining capacity, should take already scaled total capacity into account, or we 
should undo the scaling on the total capacity (which is calculated from the ah value now, 
which is scaled already).
- Investigate why opening and then closing contactors from webpage does not always work
- Invertigate why contactors don't close when lilygo and battery are powered on simultaneously -> timeout on can msgs triggers to late, reset when open contactors is executed
- Find out how to get the battery in balancing mode
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10ms = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis20ms = 0;   // will store last time a 20ms CAN Message was send
static unsigned long previousMillis40ms = 0;   // will store last time a 40ms CAN Message was send
static unsigned long previousMillis50ms = 0;   // will store last time a 50ms CAN Message was send
static unsigned long previousMillis100ms = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200ms = 0;  // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500ms = 0;  // will store last time a 200ms CAN Message was send
static unsigned long previousMillis1s = 0;     // will store last time a 1s CAN Message was send

static bool toggle = false;
static uint8_t counter_1000ms = 0;
static uint8_t counter_200ms = 0;
static uint8_t counter_100ms = 0;
static uint8_t counter_50ms = 0;
static uint8_t counter_40ms = 0;
static uint8_t counter_20ms = 0;
static uint8_t counter_10ms = 0;
static uint8_t counter_040 = 0;
static uint8_t counter_0F7 = 0;
static uint8_t counter_3b5 = 0;

static uint32_t poll_pid = PID_CELLVOLTAGE_CELL_85;  // We start here to quickly determine the cell size of the pack.
static bool nof_cells_determined = false;
static uint32_t pid_reply = 0;
static uint16_t battery_soc_polled = 0;
static uint16_t battery_voltage_polled = 1480;
static int16_t battery_current_polled = 0;
static int16_t battery_max_temp = 600;
static int16_t battery_min_temp = 600;
static uint16_t battery_max_charge_voltage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static uint16_t battery_allowed_charge_power = 0;
static uint16_t battery_allowed_discharge_power = 0;
static uint16_t cellvoltages_polled[108];
static uint16_t tempval = 0;
static uint8_t BMS_5A2_CRC = 0;
static uint8_t BMS_5CA_CRC = 0;
static uint8_t BMS_0CF_CRC = 0;
static uint8_t BMS_578_CRC = 0;
static uint8_t BMS_0C0_CRC = 0;
static uint8_t BMS_16A954A6_CRC = 0;
static uint8_t BMS_5A2_counter = 0;
static uint8_t BMS_5CA_counter = 0;
static uint8_t BMS_0CF_counter = 0;
static uint8_t BMS_0C0_counter = 0;
static uint8_t BMS_578_counter = 0;
static uint8_t BMS_16A954A6_counter = 0;
static bool BMS_ext_limits_active =
    false;  //The current current limits of the HV battery are expanded to start the combustion engine / confirmation of the request
static uint8_t BMS_mode =
    0x07;  //0: standby; Gates open; Communication active 1: Main contactor closed / HV network activated / normal driving operation
//2: assigned depending on the project (e.g. balancing, extended DC fast charging) //3: external charging
static uint8_t BMS_HVIL_status = 0;         //0 init, 1 seated, 2 open, 3 fault
static bool BMS_fault_performance = false;  //Error: Battery performance is limited (e.g. due to sensor or fan failure)
static uint16_t BMS_current = 16300;
static bool BMS_fault_emergency_shutdown_crash =
    false;  //Error: Safety-critical error (crash detection) Battery contactors are already opened / will be opened immediately Signal is read directly by the EMS and initiates an AKS of the PWR and an active discharge of the DC link
static uint32_t BMS_voltage_intermediate = 0;
static uint32_t BMS_voltage = 1480;
static uint8_t BMS_status_voltage_free =
    0;  //0=Init, 1=BMS intermediate circuit voltage-free (U_Zwkr < 20V), 2=BMS intermediate circuit not voltage-free (U_Zwkr >/= 25V, hysteresis), 3=Error
static bool BMS_OBD_MIL = false;
static uint8_t BMS_error_status =
    0x7;  //0 Component_IO, 1 Restricted_CompFkt_Isoerror_I, 2 Restricted_CompFkt_Isoerror_II, 3 Restricted_CompFkt_Interlock, 4 Restricted_CompFkt_SD, 5 Restricted_CompFkt_Performance red, 6 = No component function, 7 = Init
static uint16_t BMS_capacity_ah = 0;
static bool BMS_error_lamp_req = false;
static bool BMS_warning_lamp_req = false;
static uint8_t BMS_Kl30c_Status = 0;  // 0 init, 1 closed, 2 open, 3 fault
static bool service_disconnect_switch_missing = false;
static bool pilotline_open = false;
static bool balancing_request = false;
static uint8_t battery_diagnostic = 0;
static uint16_t battery_Wh_left = 0;
static uint16_t battery_Wh_max = 1000;
static uint8_t battery_potential_status = 0;
static uint8_t battery_temperature_warning = 0;
static uint16_t max_discharge_power_watt = 0;
static uint16_t max_discharge_current_amp = 0;
static uint16_t max_charge_power_watt = 0;
static uint16_t max_charge_current_amp = 0;
static uint16_t battery_SOC = 1;
static uint16_t usable_energy_amount_Wh = 0;
static uint8_t status_HV_line = 0;  //0 init, 1 No open HV line, 2 open HV line detected, 3 fault
static uint8_t warning_support = 0;
static bool battery_heating_active = false;
static uint16_t power_discharge_percentage = 0;
static uint16_t power_charge_percentage = 0;
static uint16_t actual_battery_voltage = 0;
static uint16_t regen_battery = 0;
static uint16_t energy_extracted_from_battery = 0;
static uint16_t max_fastcharging_current_amp = 0;  //maximum DC charging current allowed
static uint8_t BMS_Status_DCLS =
    0;  //Status of the voltage monitoring on the DC charging interface. 0 inactive, 1 I_O , 2 N_I_O , 3 active
static uint16_t DC_voltage_DCLS =
    0;  //Factor 1, DC voltage of the charging station. Measurement between the DC HV lines.
static uint16_t DC_voltage_chargeport =
    0;  //Factor 0.5,  Current voltage at the HV battery DC charging terminals; Outlet to the DC charging plug.
static uint8_t BMS_welded_contactors_status =
    0;  //0: Init no diagnostic result, 1: no contactor welded, 2: at least 1 contactor welded, 3: Protection status detection error
static bool BMS_error_shutdown_request =
    false;  // Fault: Fault condition, requires battery contactors to be opened internal battery error; Advance notification of an impending opening of the battery contactors by the BMS
static bool BMS_error_shutdown =
    false;  // Fault: Fault condition, requires battery contactors to be opened Internal battery error, battery contactors opened without notice by the BMS
static uint16_t power_battery_heating_watt = 0;
static uint16_t power_battery_heating_req_watt = 0;
static uint8_t cooling_request =
    0;  //0 = No cooling, 1 = Light cooling, cabin prio, 2= higher cooling, 3 = immediate cooling, 4 = emergency cooling
static uint8_t heating_request = 0;       //0 = init, 1= maintain temp, 2=higher demand, 3 = immediate heat demand
static uint8_t balancing_active = false;  //0 = init, 1 active, 2 not active
static bool charging_active = false;
static uint16_t max_energy_Wh = 0;
static uint16_t max_charge_percent = 0;
static uint16_t min_charge_percent = 0;
static uint16_t isolation_resistance_kOhm = 0;
static bool battery_heating_installed = false;
static bool error_NT_circuit = false;
static uint8_t pump_1_control = 0;             //0x0D not installed, 0x0E init, 0x0F fault
static uint8_t pump_2_control = 0;             //0x0D not installed, 0x0E init, 0x0F fault
static uint8_t target_flow_temperature_C = 0;  //*0,5 -40
static uint8_t return_temperature_C = 0;       //*0,5 -40
static uint8_t status_valve_1 = 0;             //0 not active, 1 active, 5 not installed, 6 init, 7 fault
static uint8_t status_valve_2 = 0;             //0 not active, 1 active, 5 not installed, 6 init, 7 fault
static uint8_t battery_temperature = 0;
static uint8_t temperature_request =
    0;  //0 high cooling, 1 medium cooling, 2 low cooling, 3 no temp requirement init, 4 low heating , 5 medium heating, 6 high heating, 7 circulation
static uint16_t performance_index_discharge_peak_temperature_percentage = 0;
static uint16_t performance_index_charge_peak_temperature_percentage = 0;
static uint8_t temperature_status_discharge =
    0;  //0 init, 1 temp under optimal, 2 temp optimal, 3 temp over optimal, 7 fault
static uint8_t temperature_status_charge =
    0;  //0 init, 1 temp under optimal, 2 temp optimal, 3 temp over optimal, 7 fault
static uint8_t isolation_fault =
    0;  //0 init, 1 no iso fault, 2 iso fault threshold1, 3 iso fault threshold2, 4 IWU defective
static uint8_t isolation_status =
    0;  // 0 init, 1 = larger threshold1, 2 = smaller threshold1 total, 3 = smaller threshold1 intern, 4 = smaller threshold2 total, 5 = smaller threshold2 intern, 6 = no measurement, 7 = measurement active
static uint8_t actual_temperature_highest_C = 0;    //*0,5 -40
static uint8_t actual_temperature_lowest_C = 0;     //*0,5 -40
static uint16_t actual_cellvoltage_highest_mV = 0;  //bias 1000
static uint16_t actual_cellvoltage_lowest_mV = 0;   //bias 1000
static uint16_t predicted_power_dyn_standard_watt = 0;
static uint8_t predicted_time_dyn_standard_minutes = 0;
static uint8_t mux = 0;
static int8_t celltemperature[56] = {0};  //Temperatures 1-56. Value is 0xFD if sensor not present
static uint16_t cellvoltages[160] = {0};
static uint16_t duration_discharge_power_watt = 0;
static uint16_t duration_charge_power_watt = 0;
static uint16_t maximum_voltage = 0;
static uint16_t minimum_voltage = 0;
static uint8_t battery_serialnumber[26];
static uint8_t realtime_overcurrent_monitor = 0;
static uint8_t realtime_CAN_communication_fault = 0;
static uint8_t realtime_overcharge_warning = 0;
static uint8_t realtime_SOC_too_high = 0;
static uint8_t realtime_SOC_too_low = 0;
static uint8_t realtime_SOC_jumping_warning = 0;
static uint8_t realtime_temperature_difference_warning = 0;
static uint8_t realtime_cell_overtemperature_warning = 0;
static uint8_t realtime_cell_undertemperature_warning = 0;
static uint8_t realtime_battery_overvoltage_warning = 0;
static uint8_t realtime_battery_undervoltage_warning = 0;
static uint8_t realtime_cell_overvoltage_warning = 0;
static uint8_t realtime_cell_undervoltage_warning = 0;
static uint8_t realtime_cell_imbalance_warning = 0;
static uint8_t realtime_warning_battery_unathorized = 0;
static bool component_protection_active = false;
static bool shutdown_active = false;
static bool transportation_mode_active = false;
static uint8_t KL15_mode = 0;
static uint8_t bus_knockout_timer = 0;
static uint8_t hybrid_wakeup_reason = 0;
static uint8_t wakeup_type = 0;
static bool instrumentation_cluster_request = false;
static uint8_t seconds = 0;
static uint32_t first_can_msg = 0;
static uint32_t last_can_msg_timestamp = 0;

#define TIME_YEAR 2024
#define TIME_MONTH 8
#define TIME_DAY 20
#define TIME_HOUR 10
#define TIME_MINUTE 5
#define TIME_SECOND 0

#define BMS_TARGET_HV_OFF 0
#define BMS_TARGET_HV_ON 1
#define BMS_TARGET_AC_CHARGING_EXT 3  //(HS + AC_2 contactors closed)
#define BMS_TARGET_AC_CHARGING 4      //(HS + AC contactors closed)
#define BMS_TARGET_DC_CHARGING 6      //(HS + DC contactors closed)
#define BMS_TARGET_INIT 7

#define DC_FASTCHARGE_NO_START_REQUEST 0x00
#define DC_FASTCHARGE_VEHICLE 0x40
#define DC_FASTCHARGE_LS1 0x80
#define DC_FASTCHARGE_LS2 0xC0

CAN_frame MEB_POLLING_FRAME = {.FD = true,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x1C40007B,  // SOC 02 8C
                               .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_ACK_FRAME = {.FD = true,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1C40007B,  // Ack
                           .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};
//Messages needed for contactor closing
CAN_frame MEB_040 = {.FD = true,  // Airbag
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x040,  //Frame5 has HV deactivate request. Needs to be 0x00
                     .data = {0x7E, 0x83, 0x00, 0x01, 0x00, 0x00, 0x15, 0x00}};
CAN_frame MEB_0C0 = {
    .FD = true,  // EM1 message
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x0C0,  //Frame 5-6 and maybe 7-8 important (external voltage at inverter)
    .data = {0x77, 0x0A, 0xFE, 0xE7, 0x7F, 0x10, 0x27, 0x00, 0xE0, 0x7F, 0xFF, 0xF3, 0x3F, 0xFF, 0xF3, 0x3F,
             0xFC, 0x0F, 0x00, 0x00, 0xC0, 0xFF, 0xFE, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_0FC = {
    .FD = true,  //
    .ext_ID = false,
    .DLC = 48,
    .ID = 0x0FC,  //This message contains emergency regen request?(byte19), battery needs to see this message
    .data = {0x07, 0x08, 0x00, 0x00, 0x7E, 0x00, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0xFE, 0xFE, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0xF4, 0x01, 0x40, 0xFF, 0xEB, 0x7F, 0x0A, 0x88, 0xE3, 0x81, 0xAF, 0x42}};
CAN_frame MEB_6B2 = {.FD = true,  // Diagnostics
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x6B2,
                     .data = {0x6A, 0xA7, 0x37, 0x80, 0xC9, 0xBD, 0xF6, 0xC2}};
CAN_frame MEB_17FC007B_poll = {.FD = true,  // Non period request
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x17FC007B,
                               .data = {0x03, 0x22, 0x1E, 0x3D, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_1A5555A6 = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1A5555A6,
                          .data = {0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_585 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x585,
    .data = {0xCF, 0x38, 0xAF, 0x5B, 0x25, 0x00, 0x00, 0x00}};  // CF 38 AF 5B 25 00 00 00 in start4.log
//                     .data = {0xCF, 0x38, 0x20, 0x02, 0x25, 0xF7, 0x30, 0x00}}; // CF 38 AF 5B 25 00 00 00 in start4.log
CAN_frame MEB_5F5 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x5F5,
                     .data = {0x23, 0x02, 0x39, 0xC0, 0x1B, 0x8B, 0xC8, 0x1B}};
CAN_frame MEB_641 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x641,
                     .data = {0x37, 0x18, 0x00, 0x00, 0xF0, 0x00, 0xAA, 0x70}};
CAN_frame MEB_3C0 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 4,
                     .ID = 0x3C0,
                     .data = {0x66, 0x00, 0x00, 0x00}};  // Klemmen_status_01
CAN_frame MEB_0FD = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x0FD,  //CRC and counter, otherwise static
                     .data = {0x5F, 0xD0, 0x1F, 0x81, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_16A954FB = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x16A954FB,
                          .data = {0x00, 0xC0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_1A555548 = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1A555548,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_1A55552B = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1A55552B,
                          .data = {0x00, 0x00, 0x00, 0xA0, 0x02, 0x04, 0x00, 0x30}};
CAN_frame MEB_569 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x569,  //HVEM
                     .data = {0x00, 0x00, 0x01, 0x3A, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_16A954B4 = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x16A954B4,  //eTM
                          .data = {0xFE, 0xB6, 0x0D, 0x00, 0x00, 0xD5, 0x48, 0xFD}};
CAN_frame MEB_1B000046 = {.FD = false,  // Not FD
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1B000046,  // Klima
                          .data = {0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_1B000010 = {.FD = false,  // Not FD
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1B000010,  // Gateway
                          .data = {0x00, 0x50, 0x08, 0x50, 0x01, 0xFF, 0x30, 0x00}};
CAN_frame MEB_1B0000B9 = {.FD = false,  // Not FD
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1B0000B9,  //DC/DC converter
                          .data = {0x00, 0x40, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00}};
CAN_frame MEB_153 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x153,  // Static content
                     .data = {0x00, 0x00, 0x00, 0xFF, 0xEF, 0xFE, 0xFF, 0xFF}};
CAN_frame MEB_5E1 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x5E1,  // Static content
                     .data = {0x7F, 0x2A, 0x00, 0x60, 0xFE, 0x00, 0x00, 0x00}};
CAN_frame MEB_3BE = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3BE,  // CRC, otherwise Static content
                     .data = {0x57, 0x0D, 0x00, 0x00, 0x00, 0x02, 0x04, 0x40}};
CAN_frame MEB_272 = {.FD = true,  //HVLM_14
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x272,  // Static content
                     .data = {0x00, 0x00, 0x00, 0x00, 0x48, 0x08, 0x00, 0x94}};
CAN_frame MEB_503 = {.FD = true,  //HVK_01
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x503,  // Content varies. Frame1 & 3 has HV req
                     .data = {0x5D, 0x61, 0x00, 0xFF, 0x7F, 0x80, 0xE3, 0x03}};
CAN_frame MEB_14C = {
    .FD = true,  //Motor message
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x14C,  //CRC needed, static content otherwise
    .data = {0x38, 0x0A, 0xFF, 0x01, 0x01, 0xFF, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
             0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x25, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE}};
/** Calculate the CRC checksum for VAG CAN Messages
 *
 * The method used is described in Chapter "7.2.1.2 8-bit 0x2F polynomial CRC Calculation".
 * CRC Parameters:
 *     0x2F - Polynomial
 *     0xFF - Initial Value
 *     0xFF - XOR Output
 * 
 * @see https://github.com/crasbe/VW-OnBoard-Charger
 * @see https://github.com/colinoflynn/crcbeagle for CRC hacking :)
 * @see https://github.com/commaai/opendbc/blob/master/can/common.cc#L110
 * @see https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
 * @see https://web.archive.org/web/20221105210302/https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
 */
uint8_t vw_crc_calc(uint8_t* inputBytes, uint8_t length, uint16_t address) {

  const uint8_t poly = 0x2F;
  const uint8_t xor_output = 0xFF;
  // VAG Magic Bytes
  const uint8_t MB0040[16] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                              0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
  const uint8_t MB00C0[16] = {0x2f, 0x44, 0x72, 0xd3, 0x07, 0xf2, 0x39, 0x09,
                              0x8d, 0x6f, 0x57, 0x20, 0x37, 0xf9, 0x9b, 0xfa};
  const uint8_t MB00FC[16] = {0x77, 0x5c, 0xa0, 0x89, 0x4b, 0x7c, 0xbb, 0xd6,
                              0x1f, 0x6c, 0x4f, 0xf6, 0x20, 0x2b, 0x43, 0xdd};
  const uint8_t MB00FD[16] = {0xb4, 0xef, 0xf8, 0x49, 0x1e, 0xe5, 0xc2, 0xc0,
                              0x97, 0x19, 0x3c, 0xc9, 0xf1, 0x98, 0xd6, 0x61};
  const uint8_t MB0097[16] = {0x3C, 0x54, 0xCF, 0xA3, 0x81, 0x93, 0x0B, 0xC7,
                              0x3E, 0xDF, 0x1C, 0xB0, 0xA7, 0x25, 0xD3, 0xD8};
  const uint8_t MB00F7[16] = {0x5F, 0xA0, 0x44, 0xD0, 0x63, 0x59, 0x5B, 0xA2,
                              0x68, 0x04, 0x90, 0x87, 0x52, 0x12, 0xB4, 0x9E};
  const uint8_t MB0124[16] = {0x12, 0x7E, 0x34, 0x16, 0x25, 0x8F, 0x8E, 0x35,
                              0xBA, 0x7F, 0xEA, 0x59, 0x4C, 0xF0, 0x88, 0x15};
  const uint8_t MB0153[16] = {0x03, 0x13, 0x23, 0x7a, 0x40, 0x51, 0x68, 0xba,
                              0xa8, 0xbe, 0x55, 0x02, 0x11, 0x31, 0x76, 0xec};
  const uint8_t MB014C[16] = {0x16, 0x35, 0x59, 0x15, 0x9a, 0x2a, 0x97, 0xb8,
                              0x0e, 0x4e, 0x30, 0xcc, 0xb3, 0x07, 0x01, 0xad};
  const uint8_t MB0187[16] = {0x7F, 0xED, 0x17, 0xC2, 0x7C, 0xEB, 0x44, 0x21,
                              0x01, 0xFA, 0xDB, 0x15, 0x4A, 0x6B, 0x23, 0x05};
  const uint8_t MB03A6[16] = {0xB6, 0x1C, 0xC1, 0x23, 0x6D, 0x8B, 0x0C, 0x51,
                              0x38, 0x32, 0x24, 0xA8, 0x3F, 0x3A, 0xA4, 0x02};
  const uint8_t MB03AF[16] = {0x94, 0x6A, 0xB5, 0x38, 0x8A, 0xB4, 0xAB, 0x27,
                              0xCB, 0x22, 0x88, 0xEF, 0xA3, 0xE1, 0xD0, 0xBB};
  const uint8_t MB03BE[16] = {0x1f, 0x28, 0xc6, 0x85, 0xe6, 0xf8, 0xb0, 0x19,
                              0x5b, 0x64, 0x35, 0x21, 0xe4, 0xf7, 0x9c, 0x24};
  const uint8_t MB03C0[16] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                              0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3};
  const uint8_t MB0503[16] = {0xed, 0xd6, 0x96, 0x63, 0xa5, 0x12, 0xd5, 0x9a,
                              0x1e, 0x0d, 0x24, 0xcd, 0x8c, 0xa6, 0x2f, 0x41};
  const uint8_t MB0578[16] = {0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
                              0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48};
  const uint8_t MB05CA[16] = {0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
                              0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43};
  const uint8_t MB0641[16] = {0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
                              0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47};
  const uint8_t MB06A3[16] = {0xC1, 0x8B, 0x38, 0xA8, 0xA4, 0x27, 0xEB, 0xC8,
                              0xEF, 0x05, 0x9A, 0xBB, 0x39, 0xF7, 0x80, 0xA7};
  const uint8_t MB06A4[16] = {0xC7, 0xD8, 0xF1, 0xC4, 0xE3, 0x5E, 0x9A, 0xE2,
                              0xA1, 0xCB, 0x02, 0x4F, 0x57, 0x4E, 0x8E, 0xE4};
  const uint8_t MB16A954A6[16] = {0x79, 0xB9, 0x67, 0xAD, 0xD5, 0xF7, 0x70, 0xAA,
                                  0x44, 0x61, 0x5A, 0xDC, 0x26, 0xB4, 0xD2, 0xC3};

  uint8_t crc = 0xFF;
  uint8_t magicByte = 0x00;
  uint8_t counter = inputBytes[1] & 0x0F;  // only the low byte of the couner is relevant

  switch (address) {
    case 0x0040:  // Airbag
      magicByte = MB0040[counter];
      break;
    case 0x00C0:  //
      magicByte = MB00C0[counter];
      break;
    case 0x00FC:
      magicByte = MB00FC[counter];
      break;
    case 0x00FD:
      magicByte = MB00FD[counter];
      break;
    case 0x0097:  // ??
      magicByte = MB0097[counter];
      break;
    case 0x00F7:  // ??
      magicByte = MB00F7[counter];
      break;
    case 0x0124:  // ??
      magicByte = MB0124[counter];
      break;
    case 0x014C:  // Motor
      magicByte = MB014C[counter];
      break;
    case 0x0153:  // HYB30
      magicByte = MB0153[counter];
      break;
    case 0x0187:  // EV_Gearshift "Gear" selection data for EVs with no gearbox
      magicByte = MB0187[counter];
      break;
    case 0x03A6:  // ??
      magicByte = MB03A6[counter];
      break;
    case 0x03AF:  // ??
      magicByte = MB03AF[counter];
      break;
    case 0x03BE:  // Motor
      magicByte = MB03BE[counter];
      break;
    case 0x03C0:  // Klemmen status
      magicByte = MB03C0[counter];
      break;
    case 0x0503:  // HVK
      magicByte = MB0503[counter];
      break;
    case 0x0578:  // BMS DC
      magicByte = MB0578[counter];
      break;
    case 0x05CA:  // BMS
      magicByte = MB05CA[counter];
      break;
    case 0x0641:  // Motor
      magicByte = MB0641[counter];
      break;
    case 0x06A3:  // ??
      magicByte = MB06A3[counter];
      break;
    case 0x06A4:  // ??
      magicByte = MB06A4[counter];
      break;
    case 0x16A954A6:
      magicByte = MB16A954A6[counter];
      break;
    default:  // this won't lead to correct CRC checksums
      magicByte = 0x00;
      break;
  }

  for (uint8_t i = 1; i < length + 1; i++) {
    // We skip the empty CRC position and start at the timer
    // The last element is the VAG magic byte for address 0x187 depending on the counter value.
    if (i < length)
      crc ^= inputBytes[i];
    else
      crc ^= magicByte;

    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ poly;
      else
        crc = (crc << 1);
    }
  }

  crc ^= xor_output;

  return crc;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = battery_SOC * 5;  //*0.05*100   battery_soc_polled * 10;
  //Alternatively use battery_SOC for more precision

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = BMS_voltage * 2.5;  // *0.25*10

  datalayer.battery.status.current_dA = (BMS_current / 10) - 1630;

  datalayer.battery.info.total_capacity_Wh =
      ((float)datalayer.battery.info.number_of_cells) * 3.6458 * ((float)BMS_capacity_ah) * 0.2;

  datalayer.battery.status.remaining_capacity_Wh = usable_energy_amount_Wh * 5;
  //Alternatively use battery_Wh_left

  datalayer.battery.status.max_charge_power_W = (max_charge_power_watt * 100);

  datalayer.battery.status.max_discharge_power_W = (max_discharge_power_watt * 100);

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.temperature_min_dC = (battery_min_temp - 350) / 2;

  datalayer.battery.status.temperature_max_dC = (battery_max_temp - 350) / 2;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_polled, 108 * sizeof(uint16_t));

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < 108; ++i) {
    uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
    if (voltage_mV != 0) {  // Skip unread values (0)
      min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
      max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
    }
  }
  // If all array values are 0, reset min/max to 3700
  if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
    min_cell_mv_value = 3700;
    max_cell_mv_value = 3700;
  }

  datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
  datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;
  //TODO, use actual_cellvoltage_lowest_mV instead to save performance

  if (service_disconnect_switch_missing) {
    set_event(EVENT_HVIL_FAILURE, 1);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (pilotline_open || BMS_HVIL_status == 2) {
    set_event(EVENT_HVIL_FAILURE, 2);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  // Update webserver datalayer for "More battery info" page
  datalayer_extended.meb.SDSW = service_disconnect_switch_missing;
  datalayer_extended.meb.pilotline = pilotline_open;
  datalayer_extended.meb.transportmode = transportation_mode_active;
  datalayer_extended.meb.componentprotection = component_protection_active;
  datalayer_extended.meb.shutdown_active = shutdown_active;
  datalayer_extended.meb.HVIL = BMS_HVIL_status;
  datalayer_extended.meb.BMS_mode = BMS_mode;
  datalayer_extended.meb.battery_diagnostic = battery_diagnostic;
  datalayer_extended.meb.status_HV_line = status_HV_line;
  datalayer_extended.meb.warning_support = warning_support;
  datalayer_extended.meb.BMS_status_voltage_free = BMS_status_voltage_free;
  datalayer_extended.meb.BMS_OBD_MIL = BMS_OBD_MIL;
  datalayer_extended.meb.BMS_error_status = BMS_error_status;
  datalayer_extended.meb.BMS_error_lamp_req = BMS_error_lamp_req;
  datalayer_extended.meb.BMS_warning_lamp_req = BMS_warning_lamp_req;
  datalayer_extended.meb.BMS_Kl30c_Status = BMS_Kl30c_Status;
  datalayer_extended.meb.BMS_voltage_intermediate_dV = (BMS_voltage_intermediate - 2000) * 10 / 2;
  datalayer_extended.meb.BMS_voltage_dV = BMS_voltage * 10 / 4;
  datalayer_extended.meb.isolation_resistance = isolation_resistance_kOhm * 5;
  datalayer_extended.meb.battery_heating = battery_heating_active;
  datalayer_extended.meb.rt_overcurrent = realtime_overcurrent_monitor;
  datalayer_extended.meb.rt_CAN_fault = realtime_CAN_communication_fault;
  datalayer_extended.meb.rt_overcharge = realtime_overcharge_warning;
  datalayer_extended.meb.rt_SOC_high = realtime_SOC_too_high;
  datalayer_extended.meb.rt_SOC_low = realtime_SOC_too_low;
  datalayer_extended.meb.rt_SOC_jumping = realtime_SOC_jumping_warning;
  datalayer_extended.meb.rt_temp_difference = realtime_temperature_difference_warning;
  datalayer_extended.meb.rt_cell_overtemp = realtime_cell_overtemperature_warning;
  datalayer_extended.meb.rt_cell_undertemp = realtime_cell_undertemperature_warning;
  datalayer_extended.meb.rt_battery_overvolt = realtime_battery_overvoltage_warning;
  datalayer_extended.meb.rt_battery_undervol = realtime_battery_undervoltage_warning;
  datalayer_extended.meb.rt_cell_overvolt = realtime_cell_overvoltage_warning;
  datalayer_extended.meb.rt_cell_undervol = realtime_cell_undervoltage_warning;
  datalayer_extended.meb.rt_cell_imbalance = realtime_cell_imbalance_warning;
  datalayer_extended.meb.rt_battery_unathorized = realtime_warning_battery_unathorized;
}

void receive_can_battery(CAN_frame rx_frame) {
  last_can_msg_timestamp = millis();
  if (first_can_msg == 0)
    first_can_msg = last_can_msg_timestamp;
  switch (rx_frame.ID) {
    case 0x17F0007B:  // BMS 500ms
      component_protection_active = (rx_frame.data.u8[0] & 0x01);
      shutdown_active = ((rx_frame.data.u8[0] & 0x02) >> 1);
      transportation_mode_active = ((rx_frame.data.u8[0] & 0x02) >> 1);
      KL15_mode = ((rx_frame.data.u8[0] & 0xF0) >> 4);
      //0 = communication only when terminal 15 = ON (no run-on, cannot be woken up)
      //1 = communication after terminal 15 = OFF (run-on, cannot be woken up)
      //2 = communication when terminal 15 = OFF (run-on, can be woken up)
      bus_knockout_timer = rx_frame.data.u8[5];
      hybrid_wakeup_reason = rx_frame.data.u8[6];  //(if several active, lowest wins)
      //0 = wakeup cause not known 1 = Bus wakeup2 = KL15 HW 3 = TPA active
      break;
    case 0x17FE007B:  // BMS - Offboard tester diag response
      break;
    case 0x1B00007B:  // BMS - 200ms
      wakeup_type =
          ((rx_frame.data.u8[1] & 0x10) >> 4);  //0 passive, SG has not woken up, 1 active, SG has woken up the network
      instrumentation_cluster_request = ((rx_frame.data.u8[1] & 0x40) >> 6);  //True/false
      break;
    case 0x12DD54D0:  // BMS Limits 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_discharge_power_watt =
          ((rx_frame.data.u8[6] & 0x07) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[4] & 0xC0) >> 6;  //*100
      max_discharge_current_amp =
          ((rx_frame.data.u8[3] & 0x01) << 12) | (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*0.2
      max_charge_power_watt = (rx_frame.data.u8[7] << 5) | (rx_frame.data.u8[6] >> 3);                     //*100
      max_charge_current_amp = ((rx_frame.data.u8[4] & 0x3F) << 7) | (rx_frame.data.u8[3] >> 1);           //*0.2
      break;
    case 0x12DD54D1:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[6] != 0xFE || rx_frame.data.u8[7] != 0xFF) {  // Init state, values below invalid
        battery_SOC = ((rx_frame.data.u8[3] & 0x0F) << 7) | (rx_frame.data.u8[2] >> 1);               //*0.05
        usable_energy_amount_Wh = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];                   //*5
        power_discharge_percentage = ((rx_frame.data.u8[4] & 0x3F) << 4) | rx_frame.data.u8[3] >> 4;  //*0.2
        power_charge_percentage = (rx_frame.data.u8[5] << 2) | rx_frame.data.u8[4] >> 6;              //*0.2
      }
      status_HV_line = ((rx_frame.data.u8[2] & 0x01) << 1) | rx_frame.data.u8[1] >> 7;
      warning_support = (rx_frame.data.u8[1] & 0x70) >> 4;
      break;
    case 0x12DD54D2:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_heating_active = (rx_frame.data.u8[4] & 0x40) >> 6;
      heating_request = (rx_frame.data.u8[5] & 0xE0) >> 5;
      cooling_request = (rx_frame.data.u8[5] & 0x1C) >> 2;
      power_battery_heating_watt = rx_frame.data.u8[6];
      power_battery_heating_req_watt = rx_frame.data.u8[7];
      break;
    case 0x1A555550:  // BMS 500ms
      balancing_active = (rx_frame.data.u8[1] & 0xC0) >> 6;
      charging_active = (rx_frame.data.u8[2] & 0x01);
      max_energy_Wh = ((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[5];                     //*40
      max_charge_percent = ((rx_frame.data.u8[7] << 3) | rx_frame.data.u8[6] >> 5);                  //*0.05
      min_charge_percent = ((rx_frame.data.u8[4] << 3) | rx_frame.data.u8[3] >> 5);                  //*0.05
      isolation_resistance_kOhm = (((rx_frame.data.u8[3] & 0x1F) << 7) | rx_frame.data.u8[2] >> 1);  //*5
      break;
    case 0x1A555551:  // BMS 500ms
      battery_heating_installed = (rx_frame.data.u8[1] & 0x20) >> 5;
      error_NT_circuit = (rx_frame.data.u8[1] & 0x40) >> 6;
      pump_1_control = rx_frame.data.u8[2] & 0x0F;         //*10, percent
      pump_2_control = (rx_frame.data.u8[2] & 0xF0) >> 4;  //*10, percent
      status_valve_1 = (rx_frame.data.u8[3] & 0x1C) >> 2;
      status_valve_2 = (rx_frame.data.u8[3] & 0xE0) >> 5;
      temperature_request = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      battery_temperature = rx_frame.data.u8[5];        //*0,5 -40
      target_flow_temperature_C = rx_frame.data.u8[6];  //*0,5 -40
      return_temperature_C = rx_frame.data.u8[7];       //*0,5 -40
      break;
    case 0x1A5555B2:  // BMS
      performance_index_discharge_peak_temperature_percentage =
          (((rx_frame.data.u8[3] & 0x07) << 6) | rx_frame.data.u8[2] >> 2);  //*0.2
      performance_index_charge_peak_temperature_percentage =
          (((rx_frame.data.u8[4] & 0x3F) << 3) | rx_frame.data.u8[3] >> 5);  //*0.2
      temperature_status_discharge = (rx_frame.data.u8[1] & 0x70) >> 4;
      temperature_status_charge = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      break;
    case 0x16A954A6:                                        // BMS
      BMS_16A954A6_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_16A954A6_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      isolation_fault = (rx_frame.data.u8[2] & 0xE0) >> 5;
      isolation_status = (rx_frame.data.u8[2] & 0x1E) >> 1;
      actual_temperature_highest_C = rx_frame.data.u8[3];  //*0,5 -40
      actual_temperature_lowest_C = rx_frame.data.u8[4];   //*0,5 -40
      actual_cellvoltage_highest_mV = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5]);
      actual_cellvoltage_lowest_mV = ((rx_frame.data.u8[7] << 4) | rx_frame.data.u8[6] >> 4);
      break;
    case 0x16A954F8:                                                                                // BMS
      predicted_power_dyn_standard_watt = ((rx_frame.data.u8[6] << 1) | rx_frame.data.u8[5] >> 7);  //*50
      predicted_time_dyn_standard_minutes = rx_frame.data.u8[7];
      break;
    case 0x16A954E8:  // BMS Temperature and cellvoltages - 180ms
      mux = (rx_frame.data.u8[0] & 0x0F);
      switch (mux) {
        case 0:  // Temperatures 1-56. Value is 0xFD if sensor not present
          for (uint8_t i = 0; i < 56; i++) {
            celltemperature[i] = (rx_frame.data.u8[i + 1] / 2) - 40;
          }
          break;
        case 1:  // Cellvoltages 1-42
          cellvoltages[0] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[1] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[2] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[3] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[4] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[5] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[6] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[7] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[8] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[9] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[10] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[11] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[12] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[13] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[14] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[15] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[16] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[17] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[18] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[19] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[20] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[21] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[22] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[23] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[24] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[25] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[26] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[27] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[28] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[29] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[30] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[31] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[32] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[33] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[34] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[35] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[36] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[37] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[38] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[39] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[40] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[41] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 2:  // Cellvoltages 43-84
          cellvoltages[42] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[43] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[44] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[45] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[46] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[47] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[48] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[49] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[50] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[51] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[52] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[53] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[54] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[55] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[56] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[57] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[58] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[59] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[60] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[61] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[62] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[63] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[64] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[65] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[66] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[67] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[68] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[69] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[70] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[71] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[72] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[73] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[74] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[75] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[76] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[77] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[78] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[79] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[80] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[81] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[82] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[83] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 3:  // Cellvoltages 85-126
          cellvoltages[84] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[85] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[86] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[87] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[88] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[89] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[90] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[91] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[92] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[93] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[94] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[95] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[96] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[97] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[98] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[99] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[100] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[101] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[102] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[103] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[104] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[105] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[106] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[107] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[108] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[109] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[110] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[111] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[112] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[113] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[114] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[115] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[116] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[117] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[118] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[119] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[120] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[121] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[122] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[123] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[124] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[125] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 4:  //Cellvoltages 127-160
          cellvoltages[126] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[127] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[128] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[129] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[130] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[131] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[132] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[133] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[134] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[135] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[136] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[137] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[138] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[139] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[140] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[141] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[142] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[143] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[144] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[145] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[146] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[147] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[148] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[149] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[150] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[151] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[152] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[153] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[154] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[155] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[156] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[157] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[158] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[159] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          break;
        default:  //Invalid mux
          //TODO: Add corrupted CAN message counter tick?
          break;
      }
      break;
    case 0x1C42017B:  // BMS - Non-Cyclic, TP_ISO
      //hybrid_01_response_fd_data (Whole frame)
      break;
    case 0x1A5555B0:  // BMS 1000ms cyclic
      duration_discharge_power_watt = ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5];
      duration_charge_power_watt = (rx_frame.data.u8[7] << 4) | rx_frame.data.u8[6] >> 4;
      maximum_voltage = ((rx_frame.data.u8[3] & 0x3F) << 4) | rx_frame.data.u8[2] >> 4;
      minimum_voltage = (rx_frame.data.u8[4] << 2) | rx_frame.data.u8[3] >> 6;
      break;
    case 0x1A5555B1:  // BMS 1000ms cyclic
      // All realtime_ have same enumeration, 0 = no fault, 1 = error level 1, 2 error level 2, 3 error level 3
      realtime_overcurrent_monitor = ((rx_frame.data.u8[3] & 0x01) << 2) | rx_frame.data.u8[2] >> 6;
      realtime_CAN_communication_fault = (rx_frame.data.u8[3] & 0x0E) >> 1;
      realtime_overcharge_warning = (rx_frame.data.u8[3] & 0x70) >> 4;
      realtime_SOC_too_high = ((rx_frame.data.u8[4] & 0x03) << 1) | rx_frame.data.u8[3] >> 7;
      realtime_SOC_too_low = (rx_frame.data.u8[4] & 0x1C) >> 2;
      realtime_SOC_jumping_warning = (rx_frame.data.u8[4] & 0xE0) >> 5;
      realtime_temperature_difference_warning = rx_frame.data.u8[5] & 0x07;
      realtime_cell_overtemperature_warning = (rx_frame.data.u8[5] & 0x38) >> 3;
      realtime_cell_undertemperature_warning = ((rx_frame.data.u8[6] & 0x01) << 2) | rx_frame.data.u8[5] >> 6;
      realtime_battery_overvoltage_warning = (rx_frame.data.u8[6] & 0x0E) >> 1;
      realtime_battery_undervoltage_warning = (rx_frame.data.u8[6] & 0x70) >> 4;
      realtime_cell_overvoltage_warning = ((rx_frame.data.u8[7] & 0x03) << 1) | rx_frame.data.u8[6] >> 7;
      realtime_cell_undervoltage_warning = (rx_frame.data.u8[7] & 0x1C) >> 2;
      realtime_cell_imbalance_warning = (rx_frame.data.u8[7] & 0xE0) >> 5;
      for (uint8_t i = 0; i < 26; i++) {  // Frame 9 to 34 is S/N for battery
        battery_serialnumber[i] = rx_frame.data.u8[i + 9];
      }
      realtime_warning_battery_unathorized = (rx_frame.data.u8[40] & 0x07);
      break;
    case 0x2AF:  // BMS 50ms
      actual_battery_voltage =
          ((rx_frame.data.u8[1] & 0x3F) << 8) | rx_frame.data.u8[0];  //*0.0625 // Seems to be 0.125 in logging
      regen_battery = ((rx_frame.data.u8[5] & 0x7F) << 8) | rx_frame.data.u8[4];
      energy_extracted_from_battery = ((rx_frame.data.u8[7] & 0x7F) << 8) | rx_frame.data.u8[6];
      break;
    case 0x578:                                        // BMS 100ms
      BMS_578_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_578_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      BMS_Status_DCLS = ((rx_frame.data.u8[1] & 0x30) >> 4);
      DC_voltage_DCLS = (rx_frame.data.u8[2] << 6) | (rx_frame.data.u8[1] >> 6);
      max_fastcharging_current_amp = ((rx_frame.data.u8[4] & 0x01) << 8) | rx_frame.data.u8[3];
      DC_voltage_chargeport = (rx_frame.data.u8[7] << 4) | (rx_frame.data.u8[6] >> 4);
      break;
    case 0x5A2:                                        // BMS 500ms normal, 100ms fast
      BMS_5A2_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_5A2_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      service_disconnect_switch_missing = (rx_frame.data.u8[1] & 0x20) >> 5;
      pilotline_open = (rx_frame.data.u8[1] & 0x10) >> 4;
      BMS_status_voltage_free = (rx_frame.data.u8[1] & 0xC0) >> 6;
      BMS_OBD_MIL = (rx_frame.data.u8[2] & 0x01);
      BMS_error_status = (rx_frame.data.u8[2] & 0x70) >> 4;
      BMS_error_lamp_req = (rx_frame.data.u8[4] & 0x04) >> 2;
      BMS_warning_lamp_req = (rx_frame.data.u8[4] & 0x08) >> 3;
      BMS_Kl30c_Status = (rx_frame.data.u8[4] & 0x30) >> 4;
      if (BMS_Kl30c_Status != 0) {  // init state
        BMS_capacity_ah = ((rx_frame.data.u8[4] & 0x03) << 9) | (rx_frame.data.u8[3] << 1) | (rx_frame.data.u8[2] >> 7);
      }
      break;
    case 0x5CA:                                               // BMS 500ms
      BMS_5CA_CRC = rx_frame.data.u8[0];                      // Can be used to check CAN signal integrity later on
      BMS_5CA_counter = (rx_frame.data.u8[1] & 0x0F);         // Can be used to check CAN signal integrity later on
      balancing_request = (rx_frame.data.u8[5] & 0x08) >> 3;  //True/False
      battery_diagnostic = (rx_frame.data.u8[3] & 0x07);
      battery_Wh_left =
          (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*50  ! Not usable, seems to always contain 0x7F0
      battery_potential_status =
          (rx_frame.data.u8[5] & 0x30) >> 4;  //0 = function not enabled, 1= no potential, 2 = potential on, 3 = fault
      battery_temperature_warning =
          (rx_frame.data.u8[7] & 0x0C) >> 2;  // 0 = no warning, 1 = temp level 1, 2=temp level 2
      battery_Wh_max =
          ((rx_frame.data.u8[5] & 0x07) << 8) | rx_frame.data.u8[4];  //*50  ! Not usable, seems to always contain 0x7F0
      break;
    case 0x0CF:                                        //BMS 10ms
      BMS_0CF_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_0CF_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      BMS_welded_contactors_status = (rx_frame.data.u8[1] & 0x60) >> 5;
      BMS_ext_limits_active = (rx_frame.data.u8[1] & 0x80) >> 7;
      BMS_mode = (rx_frame.data.u8[2] & 0x07);
      switch (BMS_mode) {
        case 1:
        case 3:
        case 4:
          datalayer.system.status.battery_allows_contactor_closing = true;
          break;
        default:
          datalayer.system.status.battery_allows_contactor_closing = false;
      }
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x18) >> 3;
      BMS_error_shutdown = (rx_frame.data.u8[2] & 0x20) >> 5;
      BMS_error_shutdown_request = (rx_frame.data.u8[2] & 0x40) >> 6;
      BMS_fault_performance = (rx_frame.data.u8[2] & 0x80) >> 7;
      BMS_fault_emergency_shutdown_crash = (rx_frame.data.u8[4] & 0x80) >> 7;
      if (BMS_mode != 7) {  // Init state, values below are invalid
        BMS_current = ((rx_frame.data.u8[4] & 0x7F) << 8) | rx_frame.data.u8[3];
        BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5]));
        BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4));
      }
      break;
    case 0x1C42007B:                      // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can(&MEB_ACK_FRAME, can_config.battery);
      }
      if (rx_frame.DLC == 8) {
        pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      } else {  //12 or 24bit message has reply in other location
        pid_reply = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
      }

      switch (pid_reply) {
        case PID_SOC:
          battery_soc_polled = rx_frame.data.u8[4] * 4;  // 135*4 = 54.0%
        case PID_VOLTAGE:
          battery_voltage_polled = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CURRENT:  // IDLE 0A: 00 08 62 1E 3D (00 02) 49 F0 39 AA AA
          battery_current_polled = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);  //TODO: right bits?
          break;
        case PID_MAX_TEMP:
          battery_max_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_TEMP:
          battery_min_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MAX_CHARGE_VOLTAGE:
          battery_max_charge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          battery_min_discharge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_CHARGE_POWER:
          battery_allowed_charge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          battery_allowed_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CELLVOLTAGE_CELL_1:
          cellvoltages_polled[0] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_2:
          cellvoltages_polled[1] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_3:
          cellvoltages_polled[2] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_4:
          cellvoltages_polled[3] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_5:
          cellvoltages_polled[4] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_6:
          cellvoltages_polled[5] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_7:
          cellvoltages_polled[6] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_8:
          cellvoltages_polled[7] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_9:
          cellvoltages_polled[8] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_10:
          cellvoltages_polled[9] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_11:
          cellvoltages_polled[10] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_12:
          cellvoltages_polled[11] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_13:
          cellvoltages_polled[12] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_14:
          cellvoltages_polled[13] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_15:
          cellvoltages_polled[14] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_16:
          cellvoltages_polled[15] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_17:
          cellvoltages_polled[16] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_18:
          cellvoltages_polled[17] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_19:
          cellvoltages_polled[18] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_20:
          cellvoltages_polled[19] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_21:
          cellvoltages_polled[20] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_22:
          cellvoltages_polled[21] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_23:
          cellvoltages_polled[22] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_24:
          cellvoltages_polled[23] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_25:
          cellvoltages_polled[24] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_26:
          cellvoltages_polled[25] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_27:
          cellvoltages_polled[26] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_28:
          cellvoltages_polled[27] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_29:
          cellvoltages_polled[28] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_30:
          cellvoltages_polled[29] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_31:
          cellvoltages_polled[30] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_32:
          cellvoltages_polled[31] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_33:
          cellvoltages_polled[32] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_34:
          cellvoltages_polled[33] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_35:
          cellvoltages_polled[34] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_36:
          cellvoltages_polled[35] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_37:
          cellvoltages_polled[36] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_38:
          cellvoltages_polled[37] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_39:
          cellvoltages_polled[38] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_40:
          cellvoltages_polled[39] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_41:
          cellvoltages_polled[40] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_42:
          cellvoltages_polled[41] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_43:
          cellvoltages_polled[42] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_44:
          cellvoltages_polled[43] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_45:
          cellvoltages_polled[44] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_46:
          cellvoltages_polled[45] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_47:
          cellvoltages_polled[46] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_48:
          cellvoltages_polled[47] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_49:
          cellvoltages_polled[48] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_50:
          cellvoltages_polled[49] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_51:
          cellvoltages_polled[50] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_52:
          cellvoltages_polled[51] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_53:
          cellvoltages_polled[52] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_54:
          cellvoltages_polled[53] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_55:
          cellvoltages_polled[54] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_56:
          cellvoltages_polled[55] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_57:
          cellvoltages_polled[56] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_58:
          cellvoltages_polled[57] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_59:
          cellvoltages_polled[58] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_60:
          cellvoltages_polled[59] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_61:
          cellvoltages_polled[60] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_62:
          cellvoltages_polled[61] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_63:
          cellvoltages_polled[62] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_64:
          cellvoltages_polled[63] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_65:
          cellvoltages_polled[64] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_66:
          cellvoltages_polled[65] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_67:
          cellvoltages_polled[66] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_68:
          cellvoltages_polled[67] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_69:
          cellvoltages_polled[68] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_70:
          cellvoltages_polled[69] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_71:
          cellvoltages_polled[70] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_72:
          cellvoltages_polled[71] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_73:
          cellvoltages_polled[72] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_74:
          cellvoltages_polled[73] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_75:
          cellvoltages_polled[74] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_76:
          cellvoltages_polled[75] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_77:
          cellvoltages_polled[76] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_78:
          cellvoltages_polled[77] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_79:
          cellvoltages_polled[78] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_80:
          cellvoltages_polled[79] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_81:
          cellvoltages_polled[80] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_82:
          cellvoltages_polled[81] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_83:
          cellvoltages_polled[82] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_84:
          cellvoltages_polled[83] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_85:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[84] = (tempval + 1000);
          } else {  // Cell 85 unavailable. We have a 84S battery (48kWh)
            datalayer.battery.info.number_of_cells = 84;
            nof_cells_determined = true;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_84S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;
          }
          break;
        case PID_CELLVOLTAGE_CELL_86:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[85] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_87:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[86] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_88:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[87] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_89:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[88] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_90:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[89] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_91:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[90] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_92:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[91] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_93:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[92] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_94:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[93] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_95:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[94] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_96:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[95] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_97:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[96] = (tempval + 1000);
          } else {  // Cell 97 unavailable. We have a 96S battery (55kWh) (Unless already specified as 84S)
            if (datalayer.battery.info.number_of_cells == 84) {
              // Do nothing, we already identified it as 84S
            } else {
              datalayer.battery.info.number_of_cells = 96;
              nof_cells_determined = true;
              datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
              datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
            }
          }
          break;
        case PID_CELLVOLTAGE_CELL_98:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[97] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_99:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[98] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_100:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[99] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_101:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[100] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_102:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[101] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_103:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[102] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_104:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[103] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_105:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[104] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_106:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[105] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_107:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[106] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_108:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          nof_cells_determined = true;  // This is placed outside of the if, to make
          // sure we only take the shortcuts to determine the number of cells once.
          if (tempval != 0xFFE) {
            cellvoltages_polled[107] = (tempval + 1000);
            datalayer.battery.info.number_of_cells = 108;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 10ms CAN Message
  if (currentMillis > last_can_msg_timestamp + 500) {
    first_can_msg = 0;
  }

  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10ms >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10ms));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10ms = currentMillis;

    MEB_0FC.data.u8[1] = ((MEB_0FC.data.u8[1] & 0xF0) | counter_10ms);
    MEB_0FC.data.u8[0] = vw_crc_calc(MEB_0FC.data.u8, MEB_0FC.DLC, MEB_0FC.ID);

    counter_10ms = (counter_10ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can(&MEB_0FC, can_config.battery);  // Required for contactor closing
  }
  // Send 20ms CAN Message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;

    MEB_0FD.data.u8[1] = ((MEB_0FD.data.u8[1] & 0xF0) | counter_20ms);
    MEB_0FD.data.u8[0] = vw_crc_calc(MEB_0FD.data.u8, MEB_0FD.DLC, MEB_0FD.ID);

    counter_20ms = (counter_20ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can(&MEB_0FD, can_config.battery);  // Required for contactor closing
  }
  // Send 40ms CAN Message
  if (currentMillis - previousMillis40ms >= INTERVAL_40_MS) {
    previousMillis40ms = currentMillis;

    /* Handle content for 0x040 message */
    /* Airbag message, needed for BMS to function */
    MEB_040.data.u8[7] = counter_040;
    MEB_040.data.u8[1] = ((MEB_040.data.u8[1] & 0xF0) | counter_40ms);
    MEB_040.data.u8[0] = vw_crc_calc(MEB_040.data.u8, MEB_040.DLC, MEB_040.ID);
    counter_40ms = (counter_40ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    if (toggle) {
      counter_040 = (counter_040 + 1) % 256;  // Increment only on every other pass
    }
    toggle = !toggle;  // Flip the toggle each time the code block is executed

    transmit_can(&MEB_040, can_config.battery);  // Airbag message - Needed for contactor closing
  }
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {
    previousMillis50ms = currentMillis;

    /* Handle content for 0x0C0 message */
    /* BMS needs to see this EM1 message. Content located in frame5&6 especially (can be static?)*/
    /* Also the voltage seen externally to battery is in frame 7&8. At least for the 62kWh ID3 version does not seem to matter, but we send it anyway. */
    MEB_0C0.data.u8[1] = ((MEB_0C0.data.u8[1] & 0xF0) | counter_50ms);
    MEB_0C0.data.u8[7] = ((datalayer.battery.status.voltage_dV / 10) * 4) & 0x00FF;
    MEB_0C0.data.u8[8] =
        ((MEB_0C0.data.u8[8] & 0xF0) | ((((datalayer.battery.status.voltage_dV / 10) * 4) >> 8) & 0x0F));
    MEB_0C0.data.u8[0] = vw_crc_calc(MEB_0C0.data.u8, MEB_0C0.DLC, MEB_0C0.ID);
    counter_50ms = (counter_50ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can(&MEB_0C0, can_config.battery);  //  Needed for contactor closing
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    //HV request and DC/DC control lies in 0x503
    MEB_503.data.u8[3] = 0x00;
    if (datalayer.battery.status.bms_status != FAULT && first_can_msg > 0 && currentMillis > first_can_msg + 2000) {
      MEB_503.data.u8[1] = 0xB0;
      MEB_503.data.u8[3] = BMS_TARGET_HV_ON;  //BMS_TARGET_AC_CHARGING;  //TODO, should we try AC_2 or DC charging?
      MEB_503.data.u8[5] = 0x82;              // Bordnetz Active
      MEB_503.data.u8[6] = 0xE0;              // Request emergency shutdown HV system == 0, false
    } else if (first_can_msg > 0 && currentMillis > first_can_msg + 2000) {  //FAULT STATE, open contactors
      MEB_503.data.u8[1] = 0x90;
      MEB_503.data.u8[3] = BMS_TARGET_HV_OFF;
      MEB_503.data.u8[5] = 0x80;  // Bordnetz Inactive
      MEB_503.data.u8[6] =
          0xE3;  // Request emergency shutdown HV system == init (3) (not sure if we dare activate this, this is done with 0xE1)
    }
    MEB_503.data.u8[1] = ((MEB_503.data.u8[1] & 0xF0) | counter_100ms);
    MEB_503.data.u8[0] = vw_crc_calc(MEB_503.data.u8, MEB_503.DLC, MEB_503.ID);

    //Bidirectional charging message
    MEB_272.data.u8[1] =
        0x00;  //0x80;  // Bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    MEB_272.data.u8[2] = 0x00;
    //0x01;  // High load bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    MEB_272.data.u8[5] = DC_FASTCHARGE_NO_START_REQUEST;  //DC_FASTCHARGE_VEHICLE;  //DC charging

    //Klemmen status
    MEB_3C0.data.u8[2] = 0x00;  //0x02;  //bit to signal that KL_15 is ON // Always 0 in start4.log
    MEB_3C0.data.u8[1] = ((MEB_3C0.data.u8[1] & 0xF0) | counter_100ms);
    MEB_3C0.data.u8[0] = vw_crc_calc(MEB_3C0.data.u8, MEB_3C0.DLC, MEB_3C0.ID);

    MEB_3BE.data.u8[1] = ((MEB_3BE.data.u8[1] & 0xF0) | counter_100ms);
    MEB_3BE.data.u8[0] = vw_crc_calc(MEB_3BE.data.u8, MEB_3BE.DLC, MEB_3BE.ID);

    MEB_14C.data.u8[1] = ((MEB_14C.data.u8[1] & 0xF0) | counter_100ms);
    MEB_14C.data.u8[0] = vw_crc_calc(MEB_14C.data.u8, MEB_14C.DLC, MEB_14C.ID);

    counter_100ms = (counter_100ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can(&MEB_503, can_config.battery);
    transmit_can(&MEB_272, can_config.battery);
    transmit_can(&MEB_3C0, can_config.battery);
    transmit_can(&MEB_3BE, can_config.battery);
    transmit_can(&MEB_14C, can_config.battery);
  }
  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    //TODO: 153 does not seem to need CRC even though it has it? Empty in some logs and still works

    //TODO: MEB_1B0000B9 & MEB_1B000010 & MEB_1B000046 has CAN sleep commands, static OK?

    transmit_can(&MEB_5E1, can_config.battery);
    transmit_can(&MEB_153, can_config.battery);
    transmit_can(&MEB_1B0000B9, can_config.battery);
    transmit_can(&MEB_1B000010, can_config.battery);
    transmit_can(&MEB_1B000046, can_config.battery);

    switch (poll_pid) {
      case PID_SOC:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_SOC >> 8);  // High byte
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_SOC;         // Low byte
        poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_VOLTAGE;
        poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CURRENT >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CURRENT;
        poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_TEMP;
        poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_TEMP;
        poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_CHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_CHARGE_VOLTAGE;
        poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_DISCHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_DISCHARGE_VOLTAGE;
        poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_CHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_CHARGE_POWER;
        poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_DISCHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_DISCHARGE_POWER;
        poll_pid = PID_CELLVOLTAGE_CELL_1;  // Start polling cell voltages
        break;
      // Cell Voltage Cases
      case PID_CELLVOLTAGE_CELL_1:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CELLVOLTAGE_CELL_1 >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_1;
        poll_pid = PID_CELLVOLTAGE_CELL_2;
        break;
      case PID_CELLVOLTAGE_CELL_2:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_2;
        poll_pid = PID_CELLVOLTAGE_CELL_3;
        break;
      case PID_CELLVOLTAGE_CELL_3:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_3;
        poll_pid = PID_CELLVOLTAGE_CELL_4;
        break;
      case PID_CELLVOLTAGE_CELL_4:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_4;
        poll_pid = PID_CELLVOLTAGE_CELL_5;
        break;
      case PID_CELLVOLTAGE_CELL_5:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_5;
        poll_pid = PID_CELLVOLTAGE_CELL_6;
        break;
      case PID_CELLVOLTAGE_CELL_6:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_6;
        poll_pid = PID_CELLVOLTAGE_CELL_7;
        break;
      case PID_CELLVOLTAGE_CELL_7:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_7;
        poll_pid = PID_CELLVOLTAGE_CELL_8;
        break;
      case PID_CELLVOLTAGE_CELL_8:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_8;
        poll_pid = PID_CELLVOLTAGE_CELL_9;
        break;
      case PID_CELLVOLTAGE_CELL_9:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_9;
        poll_pid = PID_CELLVOLTAGE_CELL_10;
        break;
      case PID_CELLVOLTAGE_CELL_10:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_10;
        poll_pid = PID_CELLVOLTAGE_CELL_11;
        break;
      case PID_CELLVOLTAGE_CELL_11:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_11;
        poll_pid = PID_CELLVOLTAGE_CELL_12;
        break;
      case PID_CELLVOLTAGE_CELL_12:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_12;
        poll_pid = PID_CELLVOLTAGE_CELL_13;
        break;
      case PID_CELLVOLTAGE_CELL_13:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_13;
        poll_pid = PID_CELLVOLTAGE_CELL_14;
        break;
      case PID_CELLVOLTAGE_CELL_14:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_14;
        poll_pid = PID_CELLVOLTAGE_CELL_15;
        break;
      case PID_CELLVOLTAGE_CELL_15:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_15;
        poll_pid = PID_CELLVOLTAGE_CELL_16;
        break;
      case PID_CELLVOLTAGE_CELL_16:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_16;
        poll_pid = PID_CELLVOLTAGE_CELL_17;
        break;
      case PID_CELLVOLTAGE_CELL_17:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_17;
        poll_pid = PID_CELLVOLTAGE_CELL_18;
        break;
      case PID_CELLVOLTAGE_CELL_18:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_18;
        poll_pid = PID_CELLVOLTAGE_CELL_19;
        break;
      case PID_CELLVOLTAGE_CELL_19:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_19;
        poll_pid = PID_CELLVOLTAGE_CELL_20;
        break;
      case PID_CELLVOLTAGE_CELL_20:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_20;
        poll_pid = PID_CELLVOLTAGE_CELL_21;
        break;
      case PID_CELLVOLTAGE_CELL_21:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_21;
        poll_pid = PID_CELLVOLTAGE_CELL_22;
        break;
      case PID_CELLVOLTAGE_CELL_22:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_22;
        poll_pid = PID_CELLVOLTAGE_CELL_23;
        break;
      case PID_CELLVOLTAGE_CELL_23:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_23;
        poll_pid = PID_CELLVOLTAGE_CELL_24;
        break;
      case PID_CELLVOLTAGE_CELL_24:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_24;
        poll_pid = PID_CELLVOLTAGE_CELL_25;
        break;
      case PID_CELLVOLTAGE_CELL_25:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_25;
        poll_pid = PID_CELLVOLTAGE_CELL_26;
        break;
      case PID_CELLVOLTAGE_CELL_26:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_26;
        poll_pid = PID_CELLVOLTAGE_CELL_27;
        break;
      case PID_CELLVOLTAGE_CELL_27:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_27;
        poll_pid = PID_CELLVOLTAGE_CELL_28;
        break;
      case PID_CELLVOLTAGE_CELL_28:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_28;
        poll_pid = PID_CELLVOLTAGE_CELL_29;
        break;
      case PID_CELLVOLTAGE_CELL_29:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_29;
        poll_pid = PID_CELLVOLTAGE_CELL_30;
        break;
      case PID_CELLVOLTAGE_CELL_30:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_30;
        poll_pid = PID_CELLVOLTAGE_CELL_31;
        break;
      case PID_CELLVOLTAGE_CELL_31:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_31;
        poll_pid = PID_CELLVOLTAGE_CELL_32;
        break;
      case PID_CELLVOLTAGE_CELL_32:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_32;
        poll_pid = PID_CELLVOLTAGE_CELL_33;
        break;
      case PID_CELLVOLTAGE_CELL_33:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_33;
        poll_pid = PID_CELLVOLTAGE_CELL_34;
        break;
      case PID_CELLVOLTAGE_CELL_34:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_34;
        poll_pid = PID_CELLVOLTAGE_CELL_35;
        break;
      case PID_CELLVOLTAGE_CELL_35:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_35;
        poll_pid = PID_CELLVOLTAGE_CELL_36;
        break;
      case PID_CELLVOLTAGE_CELL_36:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_36;
        poll_pid = PID_CELLVOLTAGE_CELL_37;
        break;
      case PID_CELLVOLTAGE_CELL_37:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_37;
        poll_pid = PID_CELLVOLTAGE_CELL_38;
        break;
      case PID_CELLVOLTAGE_CELL_38:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_38;
        poll_pid = PID_CELLVOLTAGE_CELL_39;
        break;
      case PID_CELLVOLTAGE_CELL_39:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_39;
        poll_pid = PID_CELLVOLTAGE_CELL_40;
        break;
      case PID_CELLVOLTAGE_CELL_40:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_40;
        poll_pid = PID_CELLVOLTAGE_CELL_41;
        break;
      case PID_CELLVOLTAGE_CELL_41:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_41;
        poll_pid = PID_CELLVOLTAGE_CELL_42;
        break;
      case PID_CELLVOLTAGE_CELL_42:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_42;
        poll_pid = PID_CELLVOLTAGE_CELL_43;
        break;
      case PID_CELLVOLTAGE_CELL_43:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_43;
        poll_pid = PID_CELLVOLTAGE_CELL_44;
        break;
      case PID_CELLVOLTAGE_CELL_44:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_44;
        poll_pid = PID_CELLVOLTAGE_CELL_45;
        break;
      case PID_CELLVOLTAGE_CELL_45:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_45;
        poll_pid = PID_CELLVOLTAGE_CELL_46;
        break;
      case PID_CELLVOLTAGE_CELL_46:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_46;
        poll_pid = PID_CELLVOLTAGE_CELL_47;
        break;
      case PID_CELLVOLTAGE_CELL_47:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_47;
        poll_pid = PID_CELLVOLTAGE_CELL_48;
        break;
      case PID_CELLVOLTAGE_CELL_48:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_48;
        poll_pid = PID_CELLVOLTAGE_CELL_49;
        break;
      case PID_CELLVOLTAGE_CELL_49:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_49;
        poll_pid = PID_CELLVOLTAGE_CELL_50;
        break;
      case PID_CELLVOLTAGE_CELL_50:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_50;
        poll_pid = PID_CELLVOLTAGE_CELL_51;
        break;
      case PID_CELLVOLTAGE_CELL_51:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_51;
        poll_pid = PID_CELLVOLTAGE_CELL_52;
        break;
      case PID_CELLVOLTAGE_CELL_52:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_52;
        poll_pid = PID_CELLVOLTAGE_CELL_53;
        break;
      case PID_CELLVOLTAGE_CELL_53:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_53;
        poll_pid = PID_CELLVOLTAGE_CELL_54;
        break;
      case PID_CELLVOLTAGE_CELL_54:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_54;
        poll_pid = PID_CELLVOLTAGE_CELL_55;
        break;
      case PID_CELLVOLTAGE_CELL_55:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_55;
        poll_pid = PID_CELLVOLTAGE_CELL_56;
        break;
      case PID_CELLVOLTAGE_CELL_56:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_56;
        poll_pid = PID_CELLVOLTAGE_CELL_57;
        break;
      case PID_CELLVOLTAGE_CELL_57:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_57;
        poll_pid = PID_CELLVOLTAGE_CELL_58;
        break;
      case PID_CELLVOLTAGE_CELL_58:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_58;
        poll_pid = PID_CELLVOLTAGE_CELL_59;
        break;
      case PID_CELLVOLTAGE_CELL_59:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_59;
        poll_pid = PID_CELLVOLTAGE_CELL_60;
        break;
      case PID_CELLVOLTAGE_CELL_60:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_60;
        poll_pid = PID_CELLVOLTAGE_CELL_61;
        break;
      case PID_CELLVOLTAGE_CELL_61:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_61;
        poll_pid = PID_CELLVOLTAGE_CELL_62;
        break;
      case PID_CELLVOLTAGE_CELL_62:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_62;
        poll_pid = PID_CELLVOLTAGE_CELL_63;
        break;
      case PID_CELLVOLTAGE_CELL_63:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_63;
        poll_pid = PID_CELLVOLTAGE_CELL_64;
        break;
      case PID_CELLVOLTAGE_CELL_64:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_64;
        poll_pid = PID_CELLVOLTAGE_CELL_65;
        break;
      case PID_CELLVOLTAGE_CELL_65:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_65;
        poll_pid = PID_CELLVOLTAGE_CELL_66;
        break;
      case PID_CELLVOLTAGE_CELL_66:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_66;
        poll_pid = PID_CELLVOLTAGE_CELL_67;
        break;
      case PID_CELLVOLTAGE_CELL_67:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_67;
        poll_pid = PID_CELLVOLTAGE_CELL_68;
        break;
      case PID_CELLVOLTAGE_CELL_68:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_68;
        poll_pid = PID_CELLVOLTAGE_CELL_69;
        break;
      case PID_CELLVOLTAGE_CELL_69:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_69;
        poll_pid = PID_CELLVOLTAGE_CELL_70;
        break;
      case PID_CELLVOLTAGE_CELL_70:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_70;
        poll_pid = PID_CELLVOLTAGE_CELL_71;
        break;
      case PID_CELLVOLTAGE_CELL_71:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_71;
        poll_pid = PID_CELLVOLTAGE_CELL_72;
        break;
      case PID_CELLVOLTAGE_CELL_72:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_72;
        poll_pid = PID_CELLVOLTAGE_CELL_73;
        break;
      case PID_CELLVOLTAGE_CELL_73:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_73;
        poll_pid = PID_CELLVOLTAGE_CELL_74;
        break;
      case PID_CELLVOLTAGE_CELL_74:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_74;
        poll_pid = PID_CELLVOLTAGE_CELL_75;
        break;
      case PID_CELLVOLTAGE_CELL_75:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_75;
        poll_pid = PID_CELLVOLTAGE_CELL_76;
        break;
      case PID_CELLVOLTAGE_CELL_76:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_76;
        poll_pid = PID_CELLVOLTAGE_CELL_77;
        break;
      case PID_CELLVOLTAGE_CELL_77:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_77;
        poll_pid = PID_CELLVOLTAGE_CELL_78;
        break;
      case PID_CELLVOLTAGE_CELL_78:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_78;
        poll_pid = PID_CELLVOLTAGE_CELL_79;
        break;
      case PID_CELLVOLTAGE_CELL_79:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_79;
        poll_pid = PID_CELLVOLTAGE_CELL_80;
        break;
      case PID_CELLVOLTAGE_CELL_80:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_80;
        poll_pid = PID_CELLVOLTAGE_CELL_81;
        break;
      case PID_CELLVOLTAGE_CELL_81:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_81;
        poll_pid = PID_CELLVOLTAGE_CELL_82;
        break;
      case PID_CELLVOLTAGE_CELL_82:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_82;
        poll_pid = PID_CELLVOLTAGE_CELL_83;
        break;
      case PID_CELLVOLTAGE_CELL_83:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_83;
        poll_pid = PID_CELLVOLTAGE_CELL_84;
        break;
      case PID_CELLVOLTAGE_CELL_84:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_84;
        if (datalayer.battery.info.number_of_cells > 84) {
          if (nof_cells_determined) {
            poll_pid = PID_CELLVOLTAGE_CELL_85;
          } else {
            poll_pid = PID_CELLVOLTAGE_CELL_97;
          }
        } else {
          poll_pid = PID_SOC;
        }
        break;
      case PID_CELLVOLTAGE_CELL_85:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_85;
        poll_pid = PID_CELLVOLTAGE_CELL_86;
        break;
      case PID_CELLVOLTAGE_CELL_86:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_86;
        poll_pid = PID_CELLVOLTAGE_CELL_87;
        break;
      case PID_CELLVOLTAGE_CELL_87:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_87;
        poll_pid = PID_CELLVOLTAGE_CELL_88;
        break;
      case PID_CELLVOLTAGE_CELL_88:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_88;
        poll_pid = PID_CELLVOLTAGE_CELL_89;
        break;
      case PID_CELLVOLTAGE_CELL_89:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_89;
        poll_pid = PID_CELLVOLTAGE_CELL_90;
        break;
      case PID_CELLVOLTAGE_CELL_90:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_90;
        poll_pid = PID_CELLVOLTAGE_CELL_91;
        break;
      case PID_CELLVOLTAGE_CELL_91:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_91;
        poll_pid = PID_CELLVOLTAGE_CELL_92;
        break;
      case PID_CELLVOLTAGE_CELL_92:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_92;
        poll_pid = PID_CELLVOLTAGE_CELL_93;
        break;
      case PID_CELLVOLTAGE_CELL_93:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_93;
        poll_pid = PID_CELLVOLTAGE_CELL_94;
        break;
      case PID_CELLVOLTAGE_CELL_94:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_94;
        poll_pid = PID_CELLVOLTAGE_CELL_95;
        break;
      case PID_CELLVOLTAGE_CELL_95:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_95;
        poll_pid = PID_CELLVOLTAGE_CELL_96;
        break;
      case PID_CELLVOLTAGE_CELL_96:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_96;
        if (datalayer.battery.info.number_of_cells > 96)
          poll_pid = PID_CELLVOLTAGE_CELL_97;
        else
          poll_pid = PID_SOC;
        break;
      case PID_CELLVOLTAGE_CELL_97:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_97;
        poll_pid = PID_CELLVOLTAGE_CELL_98;
        break;
      case PID_CELLVOLTAGE_CELL_98:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_98;
        poll_pid = PID_CELLVOLTAGE_CELL_99;
        break;
      case PID_CELLVOLTAGE_CELL_99:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_99;
        poll_pid = PID_CELLVOLTAGE_CELL_100;
        break;
      case PID_CELLVOLTAGE_CELL_100:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_100;
        poll_pid = PID_CELLVOLTAGE_CELL_101;
        break;
      case PID_CELLVOLTAGE_CELL_101:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_101;
        poll_pid = PID_CELLVOLTAGE_CELL_102;
        break;
      case PID_CELLVOLTAGE_CELL_102:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_102;
        poll_pid = PID_CELLVOLTAGE_CELL_103;
        break;
      case PID_CELLVOLTAGE_CELL_103:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_103;
        poll_pid = PID_CELLVOLTAGE_CELL_104;
        break;
      case PID_CELLVOLTAGE_CELL_104:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_104;
        poll_pid = PID_CELLVOLTAGE_CELL_105;
        break;
      case PID_CELLVOLTAGE_CELL_105:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_105;
        poll_pid = PID_CELLVOLTAGE_CELL_106;
        break;
      case PID_CELLVOLTAGE_CELL_106:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_106;
        poll_pid = PID_CELLVOLTAGE_CELL_107;
        break;
      case PID_CELLVOLTAGE_CELL_107:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_107;
        poll_pid = PID_CELLVOLTAGE_CELL_108;
        break;
      case PID_CELLVOLTAGE_CELL_108:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_108;
        poll_pid = PID_SOC;
        break;
      default:
        poll_pid = PID_SOC;
        break;
    }
    if (first_can_msg > 0 && currentMillis > first_can_msg + 2000) {
      transmit_can(&MEB_POLLING_FRAME, can_config.battery);
    }
  }

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can(&MEB_16A954B4, can_config.battery);  //eTM, Cooling valves and pumps for BMS
    transmit_can(&MEB_569, can_config.battery);       // Battery heating requests
    transmit_can(&MEB_1A55552B, can_config.battery);  //Climate, heatpump and priorities
    transmit_can(&MEB_1A555548, can_config.battery);  //ORU, OTA update message for reserving battery
    transmit_can(&MEB_16A954FB, can_config.battery);  //Climate, request to BMS for starting preconditioning
  }

  //Send 1s CANFD message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    MEB_641.data.u8[1] = ((MEB_641.data.u8[1] & 0xF0) | counter_1000ms);
    MEB_641.data.u8[0] = vw_crc_calc(MEB_641.data.u8, MEB_641.DLC, MEB_641.ID);

    MEB_1A5555A6.data.u8[2] = 0x7F;  //Outside temperature, factor 0.5, offset -50

    MEB_6B2.data.u8[0] =  //driving cycle counter, 0-254 wrap around. 255 = invalid value
        //MEB_6B2.data.u8[1-2-3b0-4] // Odometer, km (20 bits long)
        MEB_6B2.data.u8[3] = (uint8_t)((TIME_YEAR - 2000) << 4) | MEB_6B2.data.u8[3];
    MEB_6B2.data.u8[4] = (uint8_t)((TIME_DAY & 0x01) << 7 | TIME_MONTH << 3 | (TIME_YEAR - 2000) >> 4);
    MEB_6B2.data.u8[5] = (uint8_t)((TIME_HOUR & 0x0F) << 4 | TIME_DAY >> 1);
    MEB_6B2.data.u8[6] = (uint8_t)((seconds & 0x01) << 7 | TIME_MINUTE << 1 | TIME_HOUR >> 4);
    MEB_6B2.data.u8[7] = (uint8_t)((seconds & 0x3E) >> 1);
    seconds = (seconds + 1) % 60;

    counter_1000ms = (counter_1000ms + 1) % 16;       //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can(&MEB_6B2, can_config.battery);       // Diagnostics - Needed for contactor closing
    transmit_can(&MEB_641, can_config.battery);       // Motor - OBD
    transmit_can(&MEB_5F5, can_config.battery);       // Loading profile
    transmit_can(&MEB_585, can_config.battery);       // Systeminfo
    transmit_can(&MEB_1A5555A6, can_config.battery);  // Temperature QBit
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Volkswagen Group MEB platform via CAN-FD", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;  //Startup in 108S mode. We figure out the actual count later.
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Defined later to correct pack size
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;   //Defined later to correct pack size
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
