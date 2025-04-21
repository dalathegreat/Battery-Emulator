#ifndef BMW_IX_BATTERY_H
#define BMW_IX_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"

#define BATTERY_SELECTED

#define MAX_PACK_VOLTAGE_DV 4650  //4650 = 465.0V
#define MIN_PACK_VOLTAGE_DV 3000
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 4300  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2800  //Battery is put into emergency stop if one cell goes below this value
#define MAX_DISCHARGE_POWER_ALLOWED_W 10000
#define MAX_CHARGE_POWER_ALLOWED_W 10000
#define MAX_CHARGE_POWER_WHEN_TOPBALANCING_W 500
#define RAMPDOWN_SOC 9000  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%
#define STALE_PERIOD_CONFIG \
  900000;  //Number of milliseconds before critical values are classed as stale/stuck 900000 = 900 seconds

class BMWIXBattery : public CanBattery {
 public:
  BMWIXBattery() : CanBattery(BMWIX) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "BMW iX and i4-7 platform";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

 private:
  uint8_t increment_uds_req_id_counter(uint8_t index);

  unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
  unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
  unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
  unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
  unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
  unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

#define ALIVE_MAX_VALUE 14  // BMW CAN messages contain alive counter, goes from 0...14

  enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

  CmdState cmdState = SOC;

  /*
  Suspected Vehicle comms required:
    0x06D DLC? 1000ms - counters?
    0x2F1 DLC? 1000ms  during run : 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xF3, 0xFF - at startup  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF3, 0xFF.   Suspect byte [4] is a counter
    0x439 DLC4 1000ms  
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
    0x510 DLC8 100ms  40 10 40 00 6F DF 19 00  during run -  Startup sends this once: 0x40 0x10 0x02 0x00 0x00 0x00 0x00 0x00
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
          0xFF}};  // 1000ms BDC Output - [0] [1,2][3,4] counter x2. 3,4 is 9 higher than 1,2 is needed? [5-7] static

  CAN_frame BMWiX_0C0 = {
      .FD = true,
      .ext_ID = false,
      .DLC = 2,
      .ID = 0x0C0,
      .data = {
          0xF0,
          0x00}};  // Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 00 - MINIMUM ID TO KEEP SME AWAKE

  CAN_frame BMWiX_276 = {.FD = true,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x476,
                         .data = {0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFC}};  // 5000ms BDC Output - Suspected keep alive Static CONFIRM NEEDED

  CAN_frame BMWiX_2F1 = {.FD = true,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2F1,
                         .data = {0xFF, 0xFF, 0xD0, 0x39, 0x94, 0x00, 0xF3,
                                  0xFF}};  // 1000ms BDC Output - Static values - varies at startup

  CAN_frame BMWiX_439 = {.FD = true,
                         .ext_ID = false,
                         .DLC = 4,
                         .ID = 0x439,
                         .data = {0xFF, 0x3F, 0xFF, 0xF3}};  // 1000ms BDC Output

  CAN_frame
      BMWiX_486 =
          {.FD = true,
           .ext_ID = false,
           .DLC = 48,
           .ID = 0x486,
           .data = {0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
                    0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF,
                    0xFE, 0xFF, 0xFF, 0x7F, 0x33, 0xFD, 0xFD, 0xFD, 0xFD, 0xC0, 0x41, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};  // 1000ms BDC Output - Suspected keep alive CONFIRM NEEDED

  CAN_frame BMWiX_49C = {.FD = true,
                         .ext_ID = false,
                         .DLC = 4,
                         .ID = 0x49C,
                         .data = {0xD2, 0xF2, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFF}};  // 1000ms BDC Output - Suspected keep alive CONFIRM NEEDED

  CAN_frame BMWiX_510 = {
      .FD = true,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x510,
      .data = {0x40, 0x10, 0x00, 0x00, 0x00, 0x80, 0x00,
               0x00}};  // 100ms BDC Output - Values change in car logs, these bytes are the most common

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
      .data = {0x07, 0x03, 0x22, 0xE5,
               0xC7}};  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
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

  bool battery_awake = false;

  //Setup UDS values to poll for
  CAN_frame* UDS_REQUESTS100MS[17] = {&BMWiX_6F4_REQUEST_CELL_TEMP,
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
  bool battery_info_available = false;
  uint32_t battery_serial_number = 0;
  int32_t battery_current = 0;
  int16_t battery_voltage = 370;  //Startup with valid values - needs fixing in future
  int16_t terminal30_12v_voltage = 0;
  int16_t battery_voltage_after_contactor = 0;
  int16_t min_soc_state = 5000;
  int16_t avg_soc_state = 5000;
  int16_t max_soc_state = 5000;
  int16_t min_soh_state = 9900;  // Uses E5 45, also available in 78 73
  int16_t avg_soh_state = 9900;  // Uses E5 45, also available in 78 73
  int16_t max_soh_state = 9900;  // Uses E5 45, also available in 78 73
  uint16_t max_design_voltage = 0;
  uint16_t min_design_voltage = 0;
  int32_t remaining_capacity = 0;
  int32_t max_capacity = 0;
  int16_t min_battery_temperature = 0;
  int16_t avg_battery_temperature = 0;
  int16_t max_battery_temperature = 0;
  int16_t main_contactor_temperature = 0;
  int16_t min_cell_voltage = 3700;  //Startup with valid values - needs fixing in future
  int16_t max_cell_voltage = 3700;  //Startup with valid values - needs fixing in future
  unsigned long min_cell_voltage_lastchanged = 0;
  unsigned long max_cell_voltage_lastchanged = 0;
  unsigned min_cell_voltage_lastreceived = 0;
  unsigned max_cell_voltage_lastreceived = 0;
  uint32_t sme_uptime = 0;               //Uses E4 C0
  int16_t allowable_charge_amps = 0;     //E5 62
  int16_t allowable_discharge_amps = 0;  //E5 62
  int32_t iso_safety_positive = 0;       //Uses A8 60
  int32_t iso_safety_negative = 0;       //Uses A8 60
  int32_t iso_safety_parallel = 0;       //Uses A8 60
  int16_t count_full_charges = 0;        //TODO  42
  int16_t count_charges = 0;             //TODO  42
  int16_t hvil_status = 0;
  int16_t voltage_qualifier_status = 0;    //0 = Valid, 1 = Invalid
  int16_t balancing_status = 0;            //4 = not active
  uint8_t contactors_closed = 0;           //TODO  E5 BF  or E5 51
  uint8_t contactor_status_precharge = 0;  //TODO E5 BF
  uint8_t contactor_status_negative = 0;   //TODO E5 BF
  uint8_t contactor_status_positive = 0;   //TODO E5 BF
  uint8_t pyro_status_pss1 = 0;            //Using AC 93
  uint8_t pyro_status_pss4 = 0;            //Using AC 93
  uint8_t pyro_status_pss6 = 0;            //Using AC 93
  uint8_t uds_req_id_counter = 0;
  uint8_t detected_number_of_cells = 108;

  byte iX_0C0_counter = 0xF0;  // Initialize to 0xF0

  //End iX Intermediate vars

  uint8_t current_cell_polled = 0;
};

#endif
