#include "../include.h"
#ifdef BMW_IX_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-IX-BATTERY.h"

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

/*
Suspected Vehicle comms required:
  0x06D DLC? 1000ms - counters?
  0x2F1 DLC? 1000ms  during run : 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xF3, 0xFF - at startup  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF3, 0xFF.   Suspect byte [4] is a counter
  0x439 DLC4 1000ms  STATIC 
  0x0C0 DLC2 200ms needs counter
  0x587 DLC8 appears at startup   0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF , 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF,  0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF,   0x06 0x00 0x00 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x82 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF

SME Output:
  0x08F DLC48  10ms    - Appears to have analog readings like volt/temp/current
  0x12B8D087 5000ms  - Extended ID
  0x1D2 DLC8  1000ms
  0x20B DLC8  1000ms
  0x2E2 DLC16 1000ms
  0x2F1 DLC8  1000ms
  0x31F DLC16 100ms - 2 downward counters?
  0x453 DLC20 200ms
  0x486 DLC48  1000ms
  0x49C DLC8 1000ms
  0x4A1 DLC8 1000ms
  0x4BB DLC64  200ms - seems multplexed on [0]
  0x4D0 DLC64 1000ms - some slow/flickering values - possible change during fault
  0x510 DLC8 100ms  STATIC 40 10 40 00 6F DF 19 00  during run -  Startup sends this once: 0x40 0x10 0x02 0x00 0x00 0x00 0x00 0x00
  0x607 UDS Response

No vehicle  log available, SME asks for:
  0x125 (CCU)
  0x16E (CCU)
  0x340 (CCU)
  0x4F8 (CCU)
  0x188 (CCU)
  0x91 (EME1)
  0xAA (EME2)
  0x?? Suspect there is a drive mode flag somewhere - balancing might only be active in some modes

TODO
- Request batt serial number on F1 8C (already parsing RX)

*/

//Vehicle CAN START
CAN_frame BMWiX_06D = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x06D,
    .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
        0xFF}};  // 1000ms BDC Output - [0] static [1,2][3,4] counter x2. 3,4 is 9 higher than 1,2 is needed? [5-7] static

CAN_frame BMWiX_0C0 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 2,
    .ID = 0x0C0,
    .data = {
        0xF0,
        0x08}};  // Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 08 static - MINIMUM ID TO KEEP SME AWAKE

CAN_frame BMWiX_276 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x476,
                       .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFC}};  // 5000ms BDC Output - Suspected keep alive Static CONFIRM NEEDED

CAN_frame BMWiX_2F1 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x2F1,
    .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xF3, 0xFF}};  // 1000ms BDC Output - Static values - varies at startup

CAN_frame BMWiX_439 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x439,
                       .data = {0xFF, 0xBF, 0xFF, 0xFF}};  // 1000ms BDC Output - Static values

CAN_frame
    BMWiX_486 =
        {
            .FD = true,
            .ext_ID = false,
            .DLC = 48,
            .ID = 0x486,
            .data =
                {
                    0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
                    0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF,
                    0xFE, 0xFF, 0xFF, 0x7F, 0x33, 0xFD, 0xFD, 0xFD, 0xFD, 0xC0, 0x41, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};  // 1000ms BDC Output - Suspected keep alive Static CONFIRM NEEDED

CAN_frame BMWiX_49C = {.FD = true,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x49C,
                       .data = {0xD2, 0xF2, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF}};  // 1000ms BDC Output - Suspected keep alive Static CONFIRM NEEDED

CAN_frame BMWiX_510 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x510,
                       .data = {0x40, 0x10, 0x40, 0x00, 0x6F, 0xDF, 0x19, 0x00}};  // 100ms BDC Output - Static values

CAN_frame BMWiX_12B8D087 = {.FD = true,
                            .ext_ID = true,
                            .DLC = 2,
                            .ID = 0x12B8D087,
                            .data = {0xFC, 0xFF}};  // 5000ms SME Output - Static values
//Vehicle CAN END

//Request Data CAN START
CAN_frame BMWiX_6F4 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xC7}};  // Generic UDS Request data from SME. byte 4 selects requested value
CAN_frame BMWiX_6F4_REQUEST_SLEEPMODE = {
    .FD = true,
    .ext_ID = false,
    .DLC = 4,
    .ID = 0x6F4,
    .data = {0x07, 0x02, 0x11, 0x04}};  // UDS Request  Request BMS/SME goes to Sleep Mode
CAN_frame BMWiX_6F4_REQUEST_HARD_RESET = {.FD = true,
                                          .ext_ID = false,
                                          .DLC = 4,
                                          .ID = 0x6F4,
                                          .data = {0x07, 0x02, 0x11, 0x01}};  // UDS Request  Hard reset of BMS/SME
CAN_frame BMWiX_6F4_REQUEST_CELL_TEMP = {.FD = true,
                                         .ext_ID = false,
                                         .DLC = 5,
                                         .ID = 0x6F4,
                                         .data = {0x07, 0x03, 0x22, 0xDD, 0xC0}};  // UDS Request Cell Temperatures
CAN_frame BMWiX_6F4_REQUEST_SOC = {.FD = true,
                                   .ext_ID = false,
                                   .DLC = 5,
                                   .ID = 0x6F4,
                                   .data = {0x07, 0x03, 0x22, 0xE5, 0xCE}};  // Min/Avg/Max SOC%
CAN_frame BMWiX_6F4_REQUEST_CAPACITY = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xC7}};  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
CAN_frame BMWiX_6F4_REQUEST_MINMAXCELLV = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x53}};  //Min and max cell voltage   10V = Qualifier Invalid
CAN_frame BMWiX_6F4_REQUEST_MAINVOLTAGE_POSTCONTACTOR = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4A}};  //Main Battery Voltage (After Contactor)
CAN_frame BMWiX_6F4_REQUEST_MAINVOLTAGE_PRECONTACTOR = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4D}};  //Main Battery Voltage (Pre Contactor)
CAN_frame BMWiX_6F4_REQUEST_BATTERYCURRENT = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x61}};  //Current amps 32bit signed MSB. dA . negative is discharge
CAN_frame BMWiX_6F4_REQUEST_CELL_VOLTAGE = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x54}};  //MultiFrameIndividual Cell Voltages
CAN_frame BMWiX_6F4_REQUEST_T30VOLTAGE = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0xA7}};  //Terminal 30 Voltage (12V SME supply)
CAN_frame BMWiX_6F4_REQUEST_EOL_ISO = {.FD = true,
                                       .ext_ID = false,
                                       .DLC = 5,
                                       .ID = 0x6F4,
                                       .data = {0x07, 0x03, 0x22, 0xA8, 0x60}};  //Request EOL Reading including ISO
CAN_frame BMWiX_6F4_REQUEST_SOH = {.FD = true,
                                   .ext_ID = false,
                                   .DLC = 5,
                                   .ID = 0x6F4,
                                   .data = {0x07, 0x03, 0x22, 0xE5, 0x45}};  //SOH Max Min Mean Request
CAN_frame BMWiX_6F4_REQUEST_DATASUMMARY = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {
        0x07, 0x03, 0x22, 0xE5,
        0x45}};  //MultiFrame Summary Request, includes SOC/SOH/MinMax/MaxCapac/RemainCapac/max v and t at last charge. slow refreshrate
CAN_frame BMWiX_6F4_REQUEST_PYRO = {.FD = true,
                                    .ext_ID = false,
                                    .DLC = 5,
                                    .ID = 0x6F4,
                                    .data = {0x07, 0x03, 0x22, 0xAC, 0x93}};  //Pyro Status
CAN_frame BMWiX_6F4_REQUEST_UPTIME = {.FD = true,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F4,
                                      .data = {0x07, 0x03, 0x22, 0xE4, 0xC0}};  // Uptime and Vehicle Time Status
CAN_frame BMWiX_6F4_REQUEST_HVIL = {.FD = true,
                                    .ext_ID = false,
                                    .DLC = 5,
                                    .ID = 0x6F4,
                                    .data = {0x07, 0x03, 0x22, 0xE5, 0x69}};  // Request HVIL State
CAN_frame BMWiX_6F4_REQUEST_BALANCINGSTATUS = {.FD = true,
                                               .ext_ID = false,
                                               .DLC = 5,
                                               .ID = 0x6F4,
                                               .data = {0x07, 0x03, 0x22, 0xE4, 0xCA}};  // Request Balancing Data
CAN_frame BMWiX_6F4_REQUEST_MAX_CHARGE_DISCHARGE_AMPS = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x62}};  // Request allowable charge discharge amps
CAN_frame BMWiX_6F4_REQUEST_VOLTAGE_QUALIFIER_CHECK = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4B}};  // Request HV Voltage Qualifier
CAN_frame BMWiX_6F4_REQUEST_CONTACTORS_CLOSE = {
    .FD = true,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x51, 0x01}};  // Request Contactors Close - Unconfirmed
CAN_frame BMWiX_6F4_REQUEST_CONTACTORS_OPEN = {
    .FD = true,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x51, 0x01}};  // Request Contactors Open - Unconfirmed
CAN_frame BMWiX_6F4_REQUEST_BALANCING_START = {
    .FD = true,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x6F4,
    .data = {0xF4, 0x04, 0x71, 0x01, 0xAE, 0x77}};  // Request Balancing command?
CAN_frame BMWiX_6F4_REQUEST_BALANCING_START2 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x6F4,
    .data = {0xF4, 0x04, 0x31, 0x01, 0xAE, 0x77}};  // Request Balancing command?
CAN_frame BMWiX_6F4_REQUEST_PACK_VOLTAGE_LIMITS = {
    .FD = true,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F4,
    .data = {0x07, 0x03, 0x22, 0xE5, 0x4C}};  // Request pack voltage limits

CAN_frame BMWiX_6F4_CONTINUE_DATA = {.FD = true,
                                     .ext_ID = false,
                                     .DLC = 4,
                                     .ID = 0x6F4,
                                     .data = {0x07, 0x30, 0x00, 0x02}};

//Action Requests:
CAN_frame BMW_10B = {.FD = true,
                     .ext_ID = false,
                     .DLC = 3,
                     .ID = 0x10B,
                     .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command?

CAN_frame BMWiX_6F4_CELL_SOC = {.FD = true,
                                .ext_ID = false,
                                .DLC = 5,
                                .ID = 0x6F4,
                                .data = {0x07, 0x03, 0x22, 0xE5, 0x9A}};
CAN_frame BMWiX_6F4_CELL_TEMP = {.FD = true,
                                 .ext_ID = false,
                                 .DLC = 5,
                                 .ID = 0x6F4,
                                 .data = {0x07, 0x03, 0x22, 0xE5, 0xCA}};
//Request Data CAN End

static bool battery_awake = false;

//Setup UDS values to poll for
CAN_frame* UDS_REQUESTS100MS[] = {&BMWiX_6F4_REQUEST_CELL_TEMP,
                                  &BMWiX_6F4_REQUEST_SOC,
                                  &BMWiX_6F4_REQUEST_CAPACITY,
                                  &BMWiX_6F4_REQUEST_MINMAXCELLV,
                                  &BMWiX_6F4_REQUEST_MAINVOLTAGE_POSTCONTACTOR,
                                  &BMWiX_6F4_REQUEST_MAINVOLTAGE_PRECONTACTOR,
                                  &BMWiX_6F4_REQUEST_BATTERYCURRENT,
                                  &BMWiX_6F4_REQUEST_CELL_VOLTAGE,
                                  &BMWiX_6F4_REQUEST_T30VOLTAGE,
                                  &BMWiX_6F4_REQUEST_SOH,
                                  &BMWiX_6F4_REQUEST_UPTIME,
                                  &BMWiX_6F4_REQUEST_PYRO,
                                  &BMWiX_6F4_REQUEST_EOL_ISO,
                                  &BMWiX_6F4_REQUEST_HVIL,
                                  &BMWiX_6F4_REQUEST_MAX_CHARGE_DISCHARGE_AMPS,
                                  &BMWiX_6F4_REQUEST_BALANCINGSTATUS,
                                  &BMWiX_6F4_REQUEST_PACK_VOLTAGE_LIMITS};
int numUDSreqs = sizeof(UDS_REQUESTS100MS) / sizeof(UDS_REQUESTS100MS[0]);  // Number of elements in the array

//iX Intermediate vars
static bool battery_info_available = false;
static uint32_t battery_serial_number = 0;
static int32_t battery_current = 0;
static int16_t battery_voltage = 370;  //Startup with valid values - needs fixing in future
static int16_t terminal30_12v_voltage = 0;
static int16_t battery_voltage_after_contactor = 0;
static int16_t min_soc_state = 5000;
static int16_t avg_soc_state = 5000;
static int16_t max_soc_state = 5000;
static int16_t min_soh_state = 9900;  // Uses E5 45, also available in 78 73
static int16_t avg_soh_state = 9900;  // Uses E5 45, also available in 78 73
static int16_t max_soh_state = 9900;  // Uses E5 45, also available in 78 73
static uint16_t max_design_voltage = 0;
static uint16_t min_design_voltage = 0;
static int32_t remaining_capacity = 0;
static int32_t max_capacity = 0;
static int16_t min_battery_temperature = 0;
static int16_t avg_battery_temperature = 0;
static int16_t max_battery_temperature = 0;
static int16_t main_contactor_temperature = 0;
static int16_t min_cell_voltage = 3700;  //Startup with valid values - needs fixing in future
static int16_t max_cell_voltage = 3700;  //Startup with valid values - needs fixing in future
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
static uint8_t pyro_status_pss1 = 0;            //Using AC 93
static uint8_t pyro_status_pss4 = 0;            //Using AC 93
static uint8_t pyro_status_pss6 = 0;            //Using AC 93
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

  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity;

  datalayer.battery.status.soh_pptt = min_soh_state;

  datalayer.battery.status.max_discharge_power_W = MAX_DISCHARGE_POWER_ALLOWED_W;

  //datalayer.battery.status.max_charge_power_W = 3200; //10000; //Aux HV Port has 100A Fuse  Moved to Ramping

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        MAX_CHARGE_POWER_ALLOWED_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_ALLOWED_W;
  }

  datalayer.battery.status.temperature_min_dC = min_battery_temperature;

  datalayer.battery.status.temperature_max_dC = max_battery_temperature;

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

  datalayer_extended.bmwix.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwix.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  datalayer_extended.bmwix.T30_Voltage = terminal30_12v_voltage;

  datalayer_extended.bmwix.hvil_status = hvil_status;

  datalayer_extended.bmwix.bms_uptime = sme_uptime;

  datalayer_extended.bmwix.pyro_status_pss1 = pyro_status_pss1;

  datalayer_extended.bmwix.pyro_status_pss4 = pyro_status_pss4;

  datalayer_extended.bmwix.pyro_status_pss6 = pyro_status_pss6;

  datalayer_extended.bmwix.iso_safety_positive = iso_safety_positive;

  datalayer_extended.bmwix.iso_safety_negative = iso_safety_negative;

  datalayer_extended.bmwix.iso_safety_parallel = iso_safety_parallel;

  datalayer_extended.bmwix.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwix.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwix.balancing_status = balancing_status;

  datalayer_extended.bmwix.battery_voltage_after_contactor = battery_voltage_after_contactor;

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
    case 0x607:  //SME responds to UDS requests on 0x607

      if (rx_frame.DLC > 6 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x10 &&
          rx_frame.data.u8[2] == 0xE3 && rx_frame.data.u8[3] == 0x62 && rx_frame.data.u8[4] == 0xE5) {
        //First of multi frame data - Parse the first frame
        if (rx_frame.DLC = 64 && rx_frame.data.u8[5] == 0x54) {  //Individual Cell Voltages - First Frame
          int start_index = 6;                                   //Data starts here
          int voltage_index = 0;                                 //Start cell ID
          int num_voltages = 29;                                 //  number of voltage readings to get
          for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
            uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
            if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
              datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
            }
            voltage_index++;
          }
        }

        //Frame has continued data  - so request it
        transmit_can_frame(&BMWiX_6F4_CONTINUE_DATA, can_config.battery);
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x21) {  //Individual Cell Voltages - 1st Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 29;                          //Start cell ID
        int num_voltages = 31;                           //  number of voltage readings to get
        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x22) {  //Individual Cell Voltages - 2nd Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 60;                          //Start cell ID
        int num_voltages = 31;                           //  number of voltage readings to get
        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x23) {  //Individual Cell Voltages - 3rd Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 91;                          //Start cell ID
        int num_voltages;
        if (rx_frame.data.u8[12] == 0xFF && rx_frame.data.u8[13] == 0xFF) {  //97th cell is blank - assume 96S Battery
          num_voltages = 5;  //  number of voltage readings to get - 6 more to get on 96S
          detected_number_of_cells = 96;
        } else {              //We have data in 97th cell, assume 108S Battery
          num_voltages = 17;  //  number of voltage readings to get - 17 more to get on 108S
          detected_number_of_cells = 108;
        }

        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4D) {  //Main Battery Voltage (Pre Contactor)
        battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4A) {  //Main Battery Voltage (After Contactor)
        battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x61) {  //Current amps 32bit signed MSB. dA . negative is discharge
        battery_current = ((int32_t)((rx_frame.data.u8[6] << 24) | (rx_frame.data.u8[7] << 16) |
                                     (rx_frame.data.u8[8] << 8) | rx_frame.data.u8[9])) *
                          0.1;
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[4] == 0xE4 && rx_frame.data.u8[5] == 0xCA) {  //Balancing Data
        balancing_status = (rx_frame.data.u8[6]);  //4 = No symmetry mode active, invalid qualifier
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0xCE) {  //Min/Avg/Max SOC%
        min_soc_state = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        avg_soc_state = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        max_soc_state = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]);
      }

      if (rx_frame.DLC =
              12 && rx_frame.data.u8[4] == 0xE5 &&
              rx_frame.data.u8[5] == 0xC7) {  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
        remaining_capacity = ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) * 10) - 50000;
        max_capacity = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) * 10) - 50000;
      }

      if (rx_frame.DLC = 20 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x45) {  //SOH Max Min Mean Request
        min_soh_state = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]));
        avg_soh_state = ((rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]));
        max_soh_state = ((rx_frame.data.u8[12] << 8 | rx_frame.data.u8[13]));
      }

      if (rx_frame.DLC = 10 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x62) {  //Max allowed charge and discharge current - Signed 16bit
        allowable_charge_amps = (int16_t)((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7])) / 10;
        allowable_discharge_amps = (int16_t)((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9])) / 10;
      }

      if (rx_frame.DLC = 9 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x4B) {    //Max allowed charge and discharge current - Signed 16bit
        voltage_qualifier_status = (rx_frame.data.u8[8]);  // Request HV Voltage Qualifier
      }

      if (rx_frame.DLC =
              48 && rx_frame.data.u8[4] == 0xA8 && rx_frame.data.u8[5] == 0x60) {  // Safety Isolation Measurements
        iso_safety_positive = (rx_frame.data.u8[34] << 24) | (rx_frame.data.u8[35] << 16) |
                              (rx_frame.data.u8[36] << 8) | rx_frame.data.u8[37];  //Assuming 32bit
        iso_safety_negative = (rx_frame.data.u8[38] << 24) | (rx_frame.data.u8[39] << 16) |
                              (rx_frame.data.u8[40] << 8) | rx_frame.data.u8[41];  //Assuming 32bit
        iso_safety_parallel = (rx_frame.data.u8[42] << 24) | (rx_frame.data.u8[43] << 16) |
                              (rx_frame.data.u8[44] << 8) | rx_frame.data.u8[45];  //Assuming 32bit
      }

      if (rx_frame.DLC =
              48 && rx_frame.data.u8[4] == 0xE4 && rx_frame.data.u8[5] == 0xC0) {  // Uptime and Vehicle Time Status
        sme_uptime = (rx_frame.data.u8[10] << 24) | (rx_frame.data.u8[11] << 16) | (rx_frame.data.u8[12] << 8) |
                     rx_frame.data.u8[13];  //Assuming 32bit
      }

      if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xAC && rx_frame.data.u8[4] == 0x93) {  // Pyro Status
        pyro_status_pss1 = (rx_frame.data.u8[5]);
        pyro_status_pss4 = (rx_frame.data.u8[6]);
        pyro_status_pss6 = (rx_frame.data.u8[7]);
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x53) {  //Min and max cell voltage   10V = Qualifier Invalid

        datalayer.battery.status.CAN_battery_still_alive =
            CAN_STILL_ALIVE;  //This is the most important safety values, if we receive this we reset CAN alive counter.

        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) == 10000 ||
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) == 10000) {  //Qualifier Invalid Mode - Request Reboot
#ifdef DEBUG_LOG
          logging.println("Cell MinMax Qualifier Invalid - Requesting BMS Reset");
#endif
          //set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, (millis())); //Eventually need new Info level event type
          transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET, can_config.battery);
        } else {  //Only ingest values if they are not the 10V Error state
          min_cell_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
          max_cell_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        }
      }

      if (rx_frame.DLC = 16 && rx_frame.data.u8[4] == 0xDD && rx_frame.data.u8[5] == 0xC0) {  //Battery Temperature
        min_battery_temperature = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) / 10;
        avg_battery_temperature = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]) / 10;
        max_battery_temperature = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) / 10;
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xA3) {  //Main Contactor Temperature CHECK FINGERPRINT 2 LEVEL
        main_contactor_temperature = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xA7) {  //Terminal 30 Voltage (12V SME supply)
        terminal30_12v_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if (rx_frame.DLC = 6 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x04 &&
                         rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xE5 &&
                         rx_frame.data.u8[4] == 0x69) {  //HVIL Status
        hvil_status = (rx_frame.data.u8[5]);
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[2] == 0x07 && rx_frame.data.u8[3] == 0x62 &&
                         rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x4C) {  //Pack Voltage Limits
        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) < 4700 &&
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) > 2600) {  //Make sure values are plausible
          battery_info_available = true;
          max_design_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
          min_design_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        }
      }

      if (rx_frame.DLC = 16 && rx_frame.data.u8[3] == 0xF1 && rx_frame.data.u8[4] == 0x8C) {  //Battery Serial Number
        //Convert hex bytes to ASCII characters and combine them into a string
        char numberString[11];  // 10 characters + null terminator
        for (int i = 0; i < 10; i++) {
          numberString[i] = char(rx_frame.data.u8[i + 6]);
        }
        numberString[10] = '\0';  // Null-terminate the string
        // Step 3: Convert the string to an unsigned long integer
        battery_serial_number = strtoul(numberString, NULL, 10);
      }
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  //if (battery_awake) { //We can always send CAN as the iX BMS will wake up on vehicle comms
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //Loop through and send a different UDS request each cycle
    uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
    transmit_can_frame(UDS_REQUESTS100MS[uds_req_id_counter], can_config.battery);

    //Send SME Keep alive values 100ms
    transmit_can_frame(&BMWiX_510, can_config.battery);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    //Send SME Keep alive values 200ms
    BMWiX_0C0.data.u8[0] = increment_0C0_counter(BMWiX_0C0.data.u8[0]);  //Keep Alive 1
    transmit_can_frame(&BMWiX_0C0, can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //Send SME Keep alive values 1000ms
    //Don't believe this is needed: transmit_can_frame(&BMWiX_06D, can_config.battery);
    //Don't believe this is needed: transmit_can_frame(&BMWiX_2F1, can_config.battery);
    //Don't believe this is needed: transmit_can_frame(&BMWiX_439, can_config.battery);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START2, can_config.battery);
    transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START, can_config.battery);
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
  strncpy(datalayer.system.info.battery_protocol, "BMW iX and i4-7 platform", 63);
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
