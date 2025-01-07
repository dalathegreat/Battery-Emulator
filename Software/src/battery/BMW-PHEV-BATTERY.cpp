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

CAN_frame BMWPHEV_6F1_REQUEST_MINMAXCELLV = {
    .FD = false,
    .ext_ID = false,
    .DLC = 5,
    .ID = 0x6F1,
    .data = {
        0x07, 0x03, 0x22, 0xDF,
        0xA0}};  //Min and max cell voltage   6.55V = Qualifier Invalid?  Multi return frame - might be all cell voltages

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
static int16_t min_battery_temperature = 0;
static int16_t avg_battery_temperature = 0;
static int16_t max_battery_temperature = 0;
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

  datalayer_extended.bmwphev.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.T30_Voltage = terminal30_12v_voltage;

  datalayer_extended.bmwphev.hvil_status = hvil_status;

  datalayer_extended.bmwphev.bms_uptime = sme_uptime;

  datalayer_extended.bmwphev.pyro_status_pss1 = pyro_status_pss1;

  datalayer_extended.bmwphev.pyro_status_pss4 = pyro_status_pss4;

  datalayer_extended.bmwphev.pyro_status_pss6 = pyro_status_pss6;

  datalayer_extended.bmwphev.iso_safety_positive = iso_safety_positive;

  datalayer_extended.bmwphev.iso_safety_negative = iso_safety_negative;

  datalayer_extended.bmwphev.iso_safety_parallel = iso_safety_parallel;

  datalayer_extended.bmwphev.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwphev.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwphev.balancing_status = balancing_status;

  datalayer_extended.bmwphev.battery_voltage_after_contactor = battery_voltage_after_contactor;

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

      if (rx_frame.DLC = 8 && rx_frame.data.u8[4] == 0xDD &&
                         rx_frame.data.u8[5] == 0xC0) {  //Cell Temp Min - continue frame follows
        min_battery_temperature = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) / 10;
      }

      // if (rx_frame.DLC =
      //         7 &&
      //         rx_frame.data.u8[1] ==
      //             0x21) {  //Cell Temp Max/Avg - is continue frame - fingerprinting needs improving as 0xF1 0x21 is used by other continues
      //   max_battery_temperature = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]) / 10;
      //   avg_battery_temperature = (rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5]) / 10;
      // }
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  //if (battery_awake) { //We can always send CAN as the PHEV BMS will wake up on vehicle comms
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
    transmit_can_frame(&BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE,
                       can_config.battery);  // Attempt contactor close - experimental
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
