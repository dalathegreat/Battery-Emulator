#include "../include.h"
#ifdef BMW_PHEV_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-PHEV-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

#define ALIVE_MAX_VALUE 14  // BMW CAN messages contain alive counter, goes from 0...14

enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

static CmdState cmdState = SOC;

const unsigned char crc8_table[256] =
    {  // CRC8_SAE_J1850_ZER0 formula,0x1D Poly,initial value 0x3F,Final XOR value varies
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0,
        0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
        0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C, 0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23,
        0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
        0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B,
        0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65, 0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
        0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8,
        0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC,
        0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
        0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7, 0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C,
        0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
        0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47,
        0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09, 0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
        0xE3, 0xFE, 0xD9, 0xC4};

/*
INFO

V0.1  very basic implementation reading Gen3/4 BMW PHEV SME. 
Supported:
-Pre/Post contactor voltage
-Min/ Cell Temp
-SOC

UDS MAP
22 D6 CF - CSC Temps
22 DD C0 - Min Max temps
22 DF A5 - All Cell voltages
22 E5 EA - Alternate all cell voltages
22 DE 7E - Voltage limits.   62 DD 73 9D 5A 69 26 = 269.18V - 402.82V
22 DD 7D - Current limits 62 DD 7D 08 20 0B EA = 305A max discharge 208A max charge
22 E5 E9 DD 7D - Individual cell SOC
22 DD 69 - Current in Amps 62 DD 69 00 00 00 00 = 0 Amps
22 DD 7B - SOH  62 DD 7B 62 = 98%
22 DD 62 - HVIL Status 62 DD 64 01 = OK/Closed
22 DD 6A - Isolation values  62 DD 6A 07 D0 07 D0 07 D0 01 01 01 = in operation plausible/2000kOhm, in follow up plausible/2000kohm, internal iso open contactors (measured on request) pluasible/2000kohm
31 03 AD 61 - Isolation measurement status  71 03 AD 61 00 FF = Nmeasurement status - not successful / fault satate - not defined
22 DF A0 - Cell voltage and temps summary including min/max/average, Ah, 
*/

//Vehicle CAN START

CAN_frame BMWiX_0C0 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 2,
    .ID = 0x0C0,
    .data = {
        0xF0,
        0x08}};  // Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 08 static - MINIMUM ID TO KEEP SME AWAKE

CAN_frame BMW_13E = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x13E,
                     .data = {0xFF, 0x31, 0xFA, 0xFA, 0xFA, 0xFA, 0x0C, 0x00}};

//Vehicle CAN END

//Request Data CAN START

CAN_frame BMWPHEV_6F1_REQUEST_SOC = {.FD = false,
                                     .ext_ID = false,
                                     .DLC = 5,
                                     .ID = 0x6F1,
                                     .data = {0x07, 0x03, 0x22, 0xDD, 0xC4}};  //  SOC%

CAN_frame BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xDD, 0xB4}};  //Main Battery Voltage (Pre Contactor)

CAN_frame BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xDD, 0x66}};  //Main Battery Voltage (After Contactor)

CAN_frame BMWPHEV_6F1_REQUEST_CELLSUMMARY = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {
        0x07, 0x03, 0x22, 0xDF,
        0xA0}};  //Min and max cell voltage + temps   6.55V = Qualifier Invalid?  Multi return frame - might be all cell voltages

CAN_frame BMWPHEV_6F1_REQUEST_CELL_TEMP = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {
        0x07, 0x03, 0x22, 0xDD,
        0xC0}};  // UDS Request Cell Temperatures min max avg. Has continue frame min in first, then max + avg in second frame

CAN_frame BMW_6F4_CELL_TEMP_CONTINUE = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 5,
                                        .ID = 0x6F1,
                                        .data = {0x07, 0x30, 0x03, 0x00, 0x00}};

CAN_frame BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x6F1,
    .data = {0x07, 0x04, 0x2E, 0xDD, 0x61, 0x01, 0x00, 0x00}};  // Request Contactors Close - Unconfirmed
CAN_frame BMWPHEV_6F1_REQUEST_CONTACTORS_OPEN = {
    .FD = false,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x6F1,
    .data = {0x07, 0x04, 0x2E, 0xDD, 0x61, 0x00, 0x00, 0x00}};  // Request Contactors Open - Unconfirmed

CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_STATUS = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x6F1,
    .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6B, 0x00,
             0x00}};  // Balancing status.  Response 7DLC F1 05 71 03 AD 6B 01   (01 = active)  (03 not active)

CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_START = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x6F1,
    .data = {0x07, 0x04, 0x31, 0x01, 0xAD, 0x6B, 0x00, 0x00}};  // Balancing start request

CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_STOP = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x6F1,
    .data = {0x07, 0x04, 0x31, 0x02, 0xAD, 0x6B, 0x00, 0x00}};  // Balancing stop request

//UNTESTED/UNCHANGED FROM IX BEYOND HERE

CAN_frame BMWPHEV_6F1 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xC7}};  // Generic UDS Request data from SME. byte 4 selects requested value
CAN_frame BMWPHEV_6F1_REQUEST_SLEEPMODE = {
    .FD = false,
    .ext_ID = false,
    .DLC = 4,
    .ID = 0x6F1,
    .data = {0x07, 0x02, 0x11, 0x04}};  // UDS Request  Request BMS/SME goes to Sleep Mode
CAN_frame BMWPHEV_6F1_REQUEST_HARD_RESET = {.FD = false,
                                            .ext_ID = false,
                                            .DLC = 4,
                                            .ID = 0x6F1,
                                            .data = {0x07, 0x02, 0x11, 0x01}};  // UDS Request  Hard reset of BMS/SME

CAN_frame BMWPHEV_6F1_REQUEST_CAPACITY = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xC7}};  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias

CAN_frame BMWPHEV_6F1_REQUEST_BATTERYCURRENT = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x61}};  //Current amps 32bit signed MSB. dA . negative is discharge
CAN_frame BMWPHEV_6F1_REQUEST_CELL_VOLTAGE = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x54}};  //MultiFrameIndividual Cell Voltages
CAN_frame BMWPHEV_6F1_REQUEST_T30VOLTAGE = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xA7}};  //Terminal 30 Voltage (12V SME supply)
CAN_frame BMWPHEV_6F1_REQUEST_EOL_ISO = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 5,
                                         .ID = 0x6F1,
                                         .data = {0x07, 0x03, 0x22, 0xA8, 0x60}};  //Request EOL Reading including ISO
CAN_frame BMWPHEV_6F1_REQUEST_SOH = {.FD = false,
                                     .ext_ID = false,
                                     .DLC = 5,
                                     .ID = 0x6F1,
                                     .data = {0x07, 0x03, 0x22, 0xE5, 0x45}};  //SOH Max Min Mean Request
CAN_frame BMWPHEV_6F1_REQUEST_DATASUMMARY = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {
        0x07, 0x03, 0x22, 0xE5,
        0x45}};  //MultiFrame Summary Request, includes SOC/SOH/MinMax/MaxCapac/RemainCapac/max v and t at last charge. slow refreshrate
CAN_frame BMWPHEV_6F1_REQUEST_PYRO = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F1,
                                      .data = {0x07, 0x03, 0x22, 0xAC, 0x93}};  //Pyro Status
CAN_frame BMWPHEV_6F1_REQUEST_UPTIME = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 5,
                                        .ID = 0x6F1,
                                        .data = {0x07, 0x03, 0x22, 0xE4, 0xC0}};  // Uptime and Vehicle Time Status
CAN_frame BMWPHEV_6F1_REQUEST_HVIL = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F1,
                                      .data = {0x07, 0x03, 0x22, 0xE5, 0x69}};  // Request HVIL State

CAN_frame BMWPHEV_6F1_REQUEST_MAX_CHARGE_DISCHARGE_AMPS = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x62}};  // Request allowable charge discharge amps
CAN_frame BMWPHEV_6F1_REQUEST_VOLTAGE_QUALIFIER_CHECK = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4B}};  // Request HV Voltage Qualifier

CAN_frame BMWPHEV_6F1_REQUEST_PACK_VOLTAGE_LIMITS = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4C}};  // Request pack voltage limits

CAN_frame BMWPHEV_6F1_CONTINUE_DATA = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 4,
                                       .ID = 0x6F1,
                                       .data = {0x07, 0x30, 0x00, 0x02}};

//Action Requests:
CAN_frame BMW_10B = {.FD = false,
                     .ext_ID = false,
                     .DLC = 3,
                     .ID = 0x10B,
                     .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command?

CAN_frame BMWPHEV_6F1_CELL_SOC = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 5,
                                  .ID = 0x6F1,
                                  .data = {0x07, 0x03, 0x22, 0xE5, 0x9A}};
CAN_frame BMWPHEV_6F1_CELL_TEMP = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 5,
                                   .ID = 0x6F1,
                                   .data = {0x07, 0x03, 0x22, 0xE5, 0xCA}};
//Request Data CAN End

static bool battery_awake = false;

//Setup UDS values to poll for
CAN_frame* UDS_REQUESTS100MS[] = {&BMWPHEV_6F1_REQUEST_SOC, &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR,
                                  &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR, &BMWPHEV_6F1_REQUEST_CELL_TEMP,
                                  &BMWPHEV_6F1_REQUEST_BALANCING_STATUS};
int numUDSreqs = sizeof(UDS_REQUESTS100MS) / sizeof(UDS_REQUESTS100MS[0]);  // Number of elements in the array

//PHEV intermediate vars

static uint16_t battery_max_charge_voltage = 0;
static int16_t battery_max_charge_amperage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static int16_t battery_max_discharge_amperage = 0;

static uint8_t startup_counter_contactor = 0;
static uint8_t alive_counter_20ms = 0;
static uint8_t BMW_13E_counter = 0;

static uint32_t battery_BEV_available_power_shortterm_charge = 0;
static uint32_t battery_BEV_available_power_shortterm_discharge = 0;
static uint32_t battery_BEV_available_power_longterm_charge = 0;
static uint32_t battery_BEV_available_power_longterm_discharge = 0;

static uint16_t battery_predicted_energy_charge_condition = 0;
static uint16_t battery_predicted_energy_charging_target = 0;

static uint8_t battery_status_error_isolation_external_Bordnetz = 0;
static uint8_t battery_status_error_isolation_internal_Bordnetz = 0;
static uint8_t battery_request_cooling = 0;
static uint8_t battery_status_valve_cooling = 0;
static uint8_t battery_status_error_locking = 0;
static uint8_t battery_status_precharge_locked = 0;
static uint8_t battery_status_disconnecting_switch = 0;
static uint8_t battery_status_emergency_mode = 0;
static uint8_t battery_request_service = 0;
static uint8_t battery_error_emergency_mode = 0;
static uint8_t battery_status_error_disconnecting_switch = 0;
static uint8_t battery_status_warning_isolation = 0;
static uint8_t battery_status_cold_shutoff_valve = 0;
static int16_t battery_temperature_HV = 0;
static int16_t battery_temperature_heat_exchanger = 0;
static int16_t battery_temperature_max = 0;
static int16_t battery_temperature_min = 0;

//iX Intermediate vars
static bool battery_info_available = false;
static uint32_t battery_serial_number = 0;
static int32_t battery_current = 0;
static int16_t battery_voltage = 370;
static int16_t terminal30_12v_voltage = 0;
static int16_t battery_voltage_after_contactor = 0;
static int16_t min_soc_state = 50;
static int16_t avg_soc_state = 50;
static int16_t max_soc_state = 50;
static int16_t min_soh_state = 99;  // Uses E5 45, also available in 78 73
static int16_t avg_soh_state = 99;  // Uses E5 45, also available in 78 73
static int16_t max_soh_state = 99;  // Uses E5 45, also available in 78 73
static uint16_t max_design_voltage = 0;
static uint16_t min_design_voltage = 0;
static int32_t remaining_capacity = 0;
static int32_t max_capacity = 0;

static int16_t main_contactor_temperature = 0;
static int16_t min_cell_voltage = 0;
static int16_t max_cell_voltage = 0;
static unsigned long min_cell_voltage_lastchanged = 0;
static unsigned long max_cell_voltage_lastchanged = 0;
static unsigned min_cell_voltage_lastreceived = 0;
static unsigned max_cell_voltage_lastreceived = 0;
static uint32_t sme_uptime = 0;               //Uses E4 C0
static int16_t allowable_charge_amps = 0;     //E5 62
static int16_t allowable_discharge_amps = 0;  //E5 62
static int32_t iso_safety_positive = 0;       //Uses A8 60
static int32_t iso_safety_negative = 0;       //Uses A8 60
static int32_t iso_safety_parallel = 0;       //Uses A8 60
static int16_t count_full_charges = 0;        //TODO  42
static int16_t count_charges = 0;             //TODO  42
static int16_t hvil_status = 0;
static int16_t voltage_qualifier_status = 0;    //0 = Valid, 1 = Invalid
static int16_t balancing_status = 0;            //4 = not active
static uint8_t contactors_closed = 0;           //TODO  E5 BF  or E5 51
static uint8_t contactor_status_precharge = 0;  //TODO E5 BF
static uint8_t contactor_status_negative = 0;   //TODO E5 BF
static uint8_t contactor_status_positive = 0;   //TODO E5 BF
static uint8_t uds_req_id_counter = 0;
static uint8_t detected_number_of_cells = 108;
const unsigned long STALE_PERIOD =
    STALE_PERIOD_CONFIG;  // Time in milliseconds to check for staleness (e.g., 5000 ms = 5 seconds)

static byte iX_0C0_counter = 0xF0;  // Initialize to 0xF0

//End iX Intermediate vars

static uint8_t current_cell_polled = 0;

// Function to check if a value has gone stale over a specified time period
bool isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
  unsigned long currentTime = millis();

  // Check if the value has changed
  if (currentValue != lastValue) {
    // Update the last change time and value
    lastChangeTime = currentTime;
    lastValue = currentValue;
    return false;  // Value is fresh because it has changed
  }

  // Check if the value has stayed the same for the specified staleness period
  return (currentTime - lastChangeTime >= STALE_PERIOD);
}

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

static uint8_t increment_uds_req_id_counter(uint8_t index) {
  index++;
  if (index >= numUDSreqs) {
    index = 0;
  }
  return index;
}

static uint8_t increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

static byte increment_0C0_counter(byte counter) {
  counter++;
  // Reset to 0xF0 if it exceeds 0xFE
  if (counter > 0xFE) {
    counter = 0xF0;
  }
  return counter;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = avg_soc_state;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh = max_capacity;

  datalayer.battery.status.remaining_capacity_Wh = battery_predicted_energy_charge_condition;

  datalayer.battery.status.soh_pptt = min_soh_state;

  datalayer.battery.status.max_discharge_power_W = battery_BEV_available_power_longterm_discharge;

  //datalayer.battery.status.max_charge_power_W = 3200; //10000; //Aux HV Port has 100A Fuse  Moved to Ramping

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        battery_BEV_available_power_longterm_charge *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = battery_BEV_available_power_longterm_charge;
  }

  datalayer.battery.status.temperature_min_dC = battery_temperature_min * 10;  // Add a decimal

  datalayer.battery.status.temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  //Check stale values. As values dont change much during idle only consider stale if both parts of this message freeze.
  bool isMinCellVoltageStale =
      isStale(min_cell_voltage, datalayer.battery.status.cell_min_voltage_mV, min_cell_voltage_lastchanged);
  bool isMaxCellVoltageStale =
      isStale(max_cell_voltage, datalayer.battery.status.cell_max_voltage_mV, max_cell_voltage_lastchanged);

  if (isMinCellVoltageStale && isMaxCellVoltageStale) {
    datalayer.battery.status.cell_min_voltage_mV = 9999;  //Stale values force stop
    datalayer.battery.status.cell_max_voltage_mV = 9999;  //Stale values force stop
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;  //Value is alive
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;  //Value is alive
  }

  datalayer.battery.info.max_design_voltage_dV = max_design_voltage;

  datalayer.battery.info.min_design_voltage_dV = min_design_voltage;

  datalayer.battery.info.number_of_cells = detected_number_of_cells;

  datalayer_extended.bmwphev.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.T30_Voltage = terminal30_12v_voltage;

  datalayer_extended.bmwphev.hvil_status = hvil_status;

  datalayer_extended.bmwphev.bms_uptime = sme_uptime;

  datalayer_extended.bmwphev.iso_safety_positive = iso_safety_positive;

  datalayer_extended.bmwphev.iso_safety_negative = iso_safety_negative;

  datalayer_extended.bmwphev.iso_safety_parallel = iso_safety_parallel;

  datalayer_extended.bmwphev.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwphev.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwphev.balancing_status = balancing_status;

  datalayer_extended.bmwphev.battery_voltage_after_contactor = battery_voltage_after_contactor;

  // Update webserver datalayer

  datalayer_extended.bmwphev.ST_iso_ext = battery_status_error_isolation_external_Bordnetz;
  datalayer_extended.bmwphev.ST_iso_int = battery_status_error_isolation_internal_Bordnetz;
  datalayer_extended.bmwphev.ST_valve_cooling = battery_status_valve_cooling;
  datalayer_extended.bmwphev.ST_interlock = battery_status_error_locking;
  datalayer_extended.bmwphev.ST_precharge = battery_status_precharge_locked;
  datalayer_extended.bmwphev.ST_DCSW = battery_status_disconnecting_switch;
  datalayer_extended.bmwphev.ST_EMG = battery_status_emergency_mode;
  datalayer_extended.bmwphev.ST_WELD = battery_status_error_disconnecting_switch;
  datalayer_extended.bmwphev.ST_isolation = battery_status_warning_isolation;
  datalayer_extended.bmwphev.ST_cold_shutoff_valve = battery_status_cold_shutoff_valve;

  if (battery_info_available) {
    // If we have data from battery - override the defaults to suit
    datalayer.battery.info.max_design_voltage_dV = max_design_voltage;
    datalayer.battery.info.min_design_voltage_dV = min_design_voltage;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  }
}
void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x112:
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x239:                                                                                      //BMS [200ms]
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x607:  //SME responds to UDS requests on 0x607

      if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xDD && rx_frame.data.u8[4] == 0xC4) {  // SOC%
        avg_soc_state = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }

      if (rx_frame.DLC =
              8 && rx_frame.data.u8[3] == 0xDD && rx_frame.data.u8[4] == 0xB4) {  //Main Battery Voltage (Pre Contactor)
        battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC = 7 && rx_frame.data.u8[3] == 0xDD &&
                         rx_frame.data.u8[4] == 0x66) {  //Main Battery Voltage (Post Contactor)
        battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC =
              7 && rx_frame.data.u8[1] == 0x05 && rx_frame.data.u8[2] == 0x71 && rx_frame.data.u8[3] == 0x03 &&
              rx_frame.data.u8[4] == 0xAD) {  //Balancing Status  01 Active 03 Not Active    7DLC F1 05 71 03 AD 6B 01
        balancing_status = (rx_frame.data.u8[6]);
      }

      //moved away from UDS - using SME default sent message
      //if (rx_frame.DLC = 8 && rx_frame.data.u8[4] == 0xDD &&
      //                   rx_frame.data.u8[5] == 0xC0) {  //Cell Temp Min - continue frame follows
      //  min_battery_temperature = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) / 10;
      //}

      // if (rx_frame.DLC =
      //         7 &&
      //         rx_frame.data.u8[1] ==
      //             0x21) {  //Cell Temp Max/Avg - is continue frame - fingerprinting needs improving as 0xF1 0x21 is used by other continues
      //   max_battery_temperature = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]) / 10;
      //   avg_battery_temperature = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 10;
      // }
      break;
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
      battery_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery_request_service = (rx_frame.data.u8[2] & 0x03);
      battery_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      battery_temperature_HV = (rx_frame.data.u8[4] - 50);
      battery_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      battery_temperature_min = (rx_frame.data.u8[6] - 50);
      battery_temperature_max = (rx_frame.data.u8[7] - 50);
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  //if (battery_awake) { //We can always send CAN as the PHEV BMS will wake up on vehicle comms

  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    if (startup_counter_contactor < 160) {
      startup_counter_contactor++;
    } else {                      //After 160 messages, turn on the request
      BMW_10B.data.u8[1] = 0x10;  // Close contactors
    }

    BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
    BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

    alive_counter_20ms = increment_alive_counter(alive_counter_20ms);

    BMW_13E_counter++;
    BMW_13E.data.u8[4] = BMW_13E_counter;

    //if (datalayer.battery.status.bms_status == FAULT) {  //ALLOW ANY TIME - TEST ONLY
    //}  //If battery is not in Fault mode, allow contactor to close by sending 10B
    //else {
    transmit_can_frame(&BMW_10B, can_config.battery);
    //}
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //Loop through and send a different UDS request each cycle
    uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
    transmit_can_frame(UDS_REQUESTS100MS[uds_req_id_counter], can_config.battery);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
    // transmit_can_frame(&BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE,
    //                    can_config.battery);  // Attempt contactor close - experimental
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_START,
                       can_config.battery);  // Enable Balancing
  }
}
//We can always send CAN as the iX BMS will wake up on vehicle comms
// else {
//   previousMillis20 = currentMillis;
//   previousMillis100 = currentMillis;
//   previousMillis200 = currentMillis;
//   previousMillis500 = currentMillis;
//   previousMillis640 = currentMillis;
//   previousMillis1000 = currentMillis;
//   previousMillis5000 = currentMillis;
//   previousMillis10000 = currentMillis;
// }
//} //We can always send CAN as the iX BMS will wake up on vehicle comms

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "BMW PHEV Battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //Before we have started up and detected which battery is in use, use 108S values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
