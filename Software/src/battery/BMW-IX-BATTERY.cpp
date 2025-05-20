#include "../include.h"
#ifdef BMW_IX_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-IX-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 600ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

#define ALIVE_MAX_VALUE 14  // BMW CAN messages contain alive counter, goes from 0...14

enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

static CmdState cmdState = SOC;

static bool battery_awake = false;

bool contactorCloseReq = false;

struct ContactorCloseRequestStruct {
  bool previous;
  bool present;
} ContactorCloseRequest = {false, false};

struct ContactorStateStruct {
  bool closed;
  bool open;
};
ContactorStateStruct ContactorState = {false, true};

struct InverterContactorCloseRequestStruct {
  bool previous;
  bool present;
};
InverterContactorCloseRequestStruct InverterContactorCloseRequest = {false, false};

/*
SME output:
  0x12B8D087         5000ms - Extended ID
  0x1D2       DLC8   1000ms
  0x20B       DLC8   1000ms
  0x2E2       DLC16  1000ms
  0x31F       DLC16  100ms  - 2 downward counters?
  0x3EA       DLC8
  0x453       DLC20  200ms
  0x486       DLC48  1000ms
  0x49C       DLC8   1000ms
  0x4A1       DLC8   1000ms
  0x4BB       DLC64  200ms  - seems multplexed on [0]
  0x4D0       DLC64  1000ms - some slow/flickering values - possible change during fault
  0x507       DLC8
  0x587       DLC8          - appears at startup   0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF , 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF,  0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF,   0x06 0x00 0x00 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x82 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF
  0x607                     - UDS response
  0x7AB       DLC64         - seen at startup
  0x8F        DLC48  10ms   - appears to have analog readings like volt/temp/current
  0xD0D087    DLC4

BDC output:
  0x276       DLC8          - vehicle condition
  0x2F1       DLC8   1000ms - during run: 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xF3, 0xFF - at startup  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF3, 0xFF.   Suspect byte [4] is a counter
  0x439       DLC4   1000ms - STATIC: byte0, byte1. byte3 changes, byte4 = 0xF2 OR 0xF3
  0x4EB       DLC8          - RSU condition
  0x510       DLC8   100ms  - FIXME:(update as this content is not seen in car logs:) STATIC 40 10 40 00 6F DF 19 00  during run -  Startup sends this once: 0x40 0x10 0x02 0x00 0x00 0x00 0x00 0x00
  0x6D        DLC8   1000ms - counters? counter on byte3
  0xC0        DLC2   200ms  - needs counter

SME asks for:
  0x125 (CCU)
  0x16E (CCU)
  0x188 (CCU)
  0x1A1 (DSC) - vehicle speed (not seen in car logs)
  0x1EA (KOMBI)
  0x1FC (FIXME:(add transmitter node.))
  0x21D (FIXME:(add transmitter node.))
  0x276 (BDC)
  0x2ED (FIXME:(add transmitter node.))
  0x340 (CCU)
  0x380 (FIXME:(add transmitter node.))
  0x442 (FIXME:(add transmitter node.))
  0x4EB (BDC)
  0x4F8 (CCU)
  0x91 (EME1)
  0xAA (EME2) - all wheel drive only
  0x?? Suspect there is a drive mode flag somewhere - balancing might only be active in some modes

TODO:
- Request batt serial number on F1 8C (already parsing RX)
*/

//Vehicle CAN START
CAN_frame BMWiX_125 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 20,
    .ID = 0x125,
    //.data = {TODO:, TODO:, TODO:, TODO:, 0xFE, 0x7F, 0xFE, 0x7F, TODO:, TODO:, TODO:, TODO:, TODO:, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF}
};  // CCU output

/* SME output
CAN_frame BMWiX_12B8D087 = {.FD = true,
                            .ext_ID = true,
                            .DLC = 2,
                            .ID = 0x12B8D087,
                            .data = {0xFC, 0xFF}};  // 5000ms SME output - Static values
*/

CAN_frame BMWiX_16E = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x16E,
                       .data = {0x00,  // Almost any possible number in 0x00 and 0xFF
                                0xA0,  // Almost any possible number in 0xA0 and 0xAF
                                0xC9, 0xFF,
                                0x60,  // FIXME: find out what this value represents
                                0xC9,
                                0x3A,    // 0x3A to close contactors, 0x33 to open contactors
                                0xF7}};  // 0xF7 to close contactors, 0xF0 to open contactors // CCU output.

CAN_frame BMWiX_188 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x188,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x3C, 0xFF, 0xFF, 0xFF}};  // CCU output - values while driving

CAN_frame BMWiX_1EA = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x1EA,
    //.data = {TODO:km_least_significant, TODO:, TODO:, TODO:, TODO:km_most_significant, 0xFF, TODO:, TODO:}
};  // KOMBI output - kilometerstand

CAN_frame BMWiX_1FC = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x1FC,
    .data = {0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0xC0,
             0x00}};  // FIXME:(add transmitter node) output - heat management engine control - static values

CAN_frame BMWiX_21D = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x21D,
    //    .data = {TODO:, TODO:, TODO:, 0xFF, 0xFF, 0xFF, 0xFF, TODO:}
};  // FIXME:(add transmitter node) output - request heating and air conditioning system 1

CAN_frame BMWiX_276 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x276,
                       .data = {0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFD}};  // BDC output - vehicle condition. Used for contactor closing

CAN_frame BMWiX_2ED = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x2ED,
    .data = {
        0x75,
        0x75}};  // FIXME:(add transmitter node) output - ambient temperature (values seen in logs vary between 0x72 and 0x79)

CAN_frame BMWiX_2F1 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x2F1,
    .data = {0xFF, 0xFF, 0xD0, 0x39, 0x94, 0x00, 0xF3, 0xFF}};  // 1000ms BDC output - Static values - varies at startup

CAN_frame BMWiX_340 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 12,
    .ID = 0x340,
    //      .data = {TODO:, TODO:, TODO:, 0xFF, TODO:, TODO:, 0x00, 0x00, TODO:, TODO:, TODO:, 0xFF, 0xFF, }
};  // CCU output

CAN_frame BMWiX_380 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 7,
    .ID = 0x380,
    //      .data = {FIXME:(VIN_char1), FIXME:(VIN_char2), FIXME:(VIN_char3), FIXME:(VIN_char4), FIXME:(VIN_char5), FIXME:(VIN_char6), FIXME:(VIN_char7)}
};  // FIXME:(add transmitter node) output -  VIN: ASCII2HEX

/* Not requested by SME
CAN_frame BMWiX_439 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x439,
                       .data = {0xFF, 0x3F, 0xFF, 0xF3}};  // 1000ms BDC output
*/

CAN_frame BMWiX_442 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 6,
    .ID = 0x442,
    //                        .data = {TODO: relative time byte 0, TODO: relative time byte1, 0xA9, 0x00, 0xE0, 0x23}
};  // FIXME:(add transmitter node) output - relative time BN2020

/* SME output
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
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};  // 1000ms SME output - Suspected keep alive Static CONFIRM NEEDED
*/

/* SME output
CAN_frame BMWiX_49C = {.FD = true,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x49C,
                       .data = {0xD2, 0xF2, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF}};  // 1000ms SME output - Suspected keep alive Static CONFIRM NEEDED
*/

CAN_frame BMWiX_4EB = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x4EB,
    //  .data = {TODO:, TODO:, TODO: 0xE0 or 0xE5 (while driving), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};  // BDC output - RSU condition

CAN_frame BMWiX_4F8 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x4F8,
    //  .data = {0xFF, 0xFD, 0xFF, 0xFF, 0xFF, TODO:, TODO:, 0xC8, 0x00, 0x00, 0xF0, 0x40, 0xFE, 0xFF, 0xFD, 0xFF, TODO:, TODO:, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};  // CCU output

CAN_frame BMWiX_510 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x510,
    .data = {
        0x40, 0x10,
        0x04,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
        0x00, 0x00,
        0x80,  // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
        0x01,
        0x00}};  // 100ms BDC output - Values change in car logs, these bytes are the most common. Used for contactor closing

CAN_frame BMWiX_6D = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x6D,
    .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
        0xFF}};  // 1000ms BDC output - [0] static [1,2][3,4] counter x2. 3,4 is 9 higher than 1,2 is needed? [5-7] static

CAN_frame BMWiX_C0 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 2,
    .ID = 0xC0,
    .data = {
        0xF0,
        0x00}};  // BDC output - Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 00 static - MINIMUM ID TO KEEP SME AWAKE
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
//End iX Intermediate vars

static uint8_t current_cell_polled = 0;

static uint16_t counter_10ms = 0;  // max 65535 --> 655.35 seconds
static uint8_t counter_100ms = 0;  // max 255 --> 25.5 seconds

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

static byte increment_C0_counter(byte counter) {
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
    set_event(EVENT_STALE_VALUE, 0);
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
    case 0x12B8D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x20B:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2E2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x31F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x453:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x486:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x49C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4BB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x507:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x587:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
#endif  // DEBUG_LOG
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
    case 0x7AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x8F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xD0D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void transmit_can_battery(unsigned long currentMillis) {
  // We can always send CAN as the iX BMS will wake up on vehicle comms
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    ContactorCloseRequest.present = contactorCloseReq;
    // Detect edge
    if (ContactorCloseRequest.previous == false && ContactorCloseRequest.present == true) {
      // Rising edge detected
#ifdef DEBUG_LOG
      logging.println("Rising edge detected. Resetting 10ms counter.");
#endif                   // DEBUG_LOG
      counter_10ms = 0;  // reset counter
    } else if (ContactorCloseRequest.previous == true && ContactorCloseRequest.present == false) {
      // Dropping edge detected
#ifdef DEBUG_LOG
      logging.println("Dropping edge detected. Resetting 10ms counter.");
#endif                   // DEBUG_LOG
      counter_10ms = 0;  // reset counter
    }
    ContactorCloseRequest.previous = ContactorCloseRequest.present;
    HandleBmwIxCloseContactorsRequest(counter_10ms);
    HandleBmwIxOpenContactorsRequest(counter_10ms);
    counter_10ms++;

    // prevent counter overflow: 2^16-1 = 65535
    if (counter_10ms == 65535) {
      counter_10ms = 1;  // set to 1, to differentiate the counter being set to 0 by the functions above
    }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    HandleIncomingInverterRequest();

    //Loop through and send a different UDS request once the contactors are closed
    if (contactorCloseReq == true &&
        ContactorState.closed ==
            true) {  // Do not send unless the contactors are requested to be closed and are closed, as sending these does not allow the contactors to close
      uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
      transmit_can_frame(UDS_REQUESTS100MS[uds_req_id_counter],
                         can_config.battery);  // FIXME: sending these does not allow the contactors to close
    } else {  // FIXME: hotfix: If contactors are not requested to be closed, ensure the battery is reported as alive, even if no CAN messages are received
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    }

    // Keep contactors closed if needed
    BmwIxKeepContactorsClosed(counter_100ms);
    counter_100ms++;
    if (counter_100ms == 140) {
      counter_100ms = 0;  // reset counter every 14 seconds
    }

    //Send SME Keep alive values 100ms
    //transmit_can_frame(&BMWiX_510, can_config.battery);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    //Send SME Keep alive values 200ms
    //BMWiX_C0.data.u8[0] = increment_C0_counter(BMWiX_C0.data.u8[0]);  //Keep Alive 1
    //transmit_can_frame(&BMWiX_C0, can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    HandleIncomingUserRequest();
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START2, can_config.battery);
    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "BMW iX and i4-7 platform", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //Reset Battery at bootup
  //transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET, can_config.battery);

  //Before we have started up and detected which battery is in use, use 108S values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

void HandleIncomingUserRequest(void) {
  // Debug user request to open or close the contactors
#ifdef DEBUG_LOG
  logging.print("User request: contactor close: ");
  logging.print(datalayer_extended.bmwix.UserRequestContactorClose);
  logging.print("  User request: contactor open: ");
  logging.println(datalayer_extended.bmwix.UserRequestContactorOpen);
#endif  // DEBUG_LOG
  if ((datalayer_extended.bmwix.UserRequestContactorClose == false) &&
      (datalayer_extended.bmwix.UserRequestContactorOpen == false)) {
    // do nothing
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == true) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == false)) {
    BmwIxCloseContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorClose = false;
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == false) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == true)) {
    BmwIxOpenContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorOpen = false;
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == true) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == true)) {
    // these flasgs should not be true at the same time, therefore open contactors, as that is the safest state
    BmwIxOpenContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorClose = false;
    datalayer_extended.bmwix.UserRequestContactorOpen = false;
// print error, as both these flags shall not be true at the same time
#ifdef DEBUG_LOG
    logging.println(
        "Error: user requested contactors to close and open at the same time. Contactors have been opened.");
#endif  // DEBUG_LOG
  }
}

void HandleIncomingInverterRequest(void) {
  InverterContactorCloseRequest.present = datalayer.system.status.inverter_allows_contactor_closing;
  // Detect edge
  if (InverterContactorCloseRequest.previous == false && InverterContactorCloseRequest.present == true) {
// Rising edge detected
#ifdef DEBUG_LOG
    logging.println("Inverter requests to close contactors");
#endif  // DEBUG_LOG
    BmwIxCloseContactors();
  } else if (InverterContactorCloseRequest.previous == true && InverterContactorCloseRequest.present == false) {
// Falling edge detected
#ifdef DEBUG_LOG
    logging.println("Inverter requests to open contactors");
#endif  // DEBUG_LOG
    BmwIxOpenContactors();
  }  // else: do nothing

  // Update state
  InverterContactorCloseRequest.previous = InverterContactorCloseRequest.present;
}

void BmwIxCloseContactors(void) {
#ifdef DEBUG_LOG
  logging.println("Closing contactors");
#endif  // DEBUG_LOG
  contactorCloseReq = true;
}

void BmwIxOpenContactors(void) {
#ifdef DEBUG_LOG
  logging.println("Opening contactors");
#endif  // DEBUG_LOG
  contactorCloseReq = false;
  counter_100ms = 0;  // reset counter, such that keep contactors closed message sequence starts from the beginning
}

void HandleBmwIxCloseContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == true) {  // Only when contactor close request is set to true
    if (ContactorState.closed == false &&
        ContactorState.open ==
            true) {  // Only when the following commands have not been completed yet, because it shall not be run when commands have already been run, AND only when contactor open commands have finished
      // Initially 0x510[2] needs to be 0x02, and 0x510[5] needs to be 0x00
      BMWiX_510.data = {0x40, 0x10,
                        0x02,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                        0x00, 0x00,
                        0x00,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                        0x01,   // 0x01 at contactor closing
                        0x00};  // Explicit declaration, to prevent modification by other functions
      BMWiX_16E.data = {
          0x00,  // Almost any possible number in 0x00 and 0xFF
          0xA0,  // Almost any possible number in 0xA0 and 0xAF
          0xC9, 0xFF, 0x60,
          0xC9, 0x3A, 0xF7};  // Explicit declaration of default values, to prevent modification by other functions

      if (counter_10ms == 0) {
        // @0 ms
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 1/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 5) {
        // @50 ms
        transmit_can_frame(&BMWiX_276, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x276 - 2/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 10) {
        // @100 ms
        BMWiX_510.data.u8[2] = 0x04;  // TODO: check if needed
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 3/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 20) {
        // @200 ms
        BMWiX_510.data.u8[2] = 0x10;  // TODO: check if needed
        BMWiX_510.data.u8[5] = 0x80;  // needed to close contactors
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 4/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 30) {
        // @300 ms
        BMWiX_16E.data.u8[0] = 0x6A;
        BMWiX_16E.data.u8[1] = 0xAD;
        transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x16E - 5/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 50) {
        // @500 ms
        BMWiX_16E.data.u8[0] = 0x03;
        BMWiX_16E.data.u8[1] = 0xA9;
        transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x16E - 6/6");
#endif  // DEBUG_LOG
        ContactorState.closed = true;
        ContactorState.open = false;
      }
    }
  }
}

void BmwIxKeepContactorsClosed(uint8_t counter_100ms) {
  if ((ContactorState.closed == true) && (ContactorState.open == false)) {
    BMWiX_510.data = {0x40, 0x10,
                      0x04,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                      0x00, 0x00,
                      0x80,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                      0x01,   // 0x01 at contactor closing
                      0x00};  // Explicit declaration, to prevent modification by other functions
    BMWiX_16E.data = {0x00,   // Almost any possible number in 0x00 and 0xFF
                      0xA0,   // Almost any possible number in 0xA0 and 0xAF
                      0xC9, 0xFF, 0x60,
                      0xC9, 0x3A, 0xF7};  // Explicit declaration, to prevent modification by other functions

    if (counter_100ms == 0) {
#ifdef DEBUG_LOG
      logging.println("Sending keep contactors closed messages started");
#endif  // DEBUG_LOG
      // @0 ms
      transmit_can_frame(&BMWiX_510, can_config.battery);
    } else if (counter_100ms == 7) {
      // @ 730 ms
      BMWiX_16E.data.u8[0] = 0x8C;
      BMWiX_16E.data.u8[1] = 0xA0;
      transmit_can_frame(&BMWiX_16E, can_config.battery);
    } else if (counter_100ms == 24) {
      // @2380 ms
      transmit_can_frame(&BMWiX_510, can_config.battery);
    } else if (counter_100ms == 29) {
      // @ 2900 ms
      BMWiX_16E.data.u8[0] = 0x02;
      BMWiX_16E.data.u8[1] = 0xA7;
      transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
      logging.println("Sending keep contactors closed messages finished");
#endif  // DEBUG_LOG
    } else if (counter_100ms == 140) {
      // @14000 ms
      // reset counter (outside of this function)
    }
  }
}

void HandleBmwIxOpenContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == false) {  // if contactors are not requested to be closed, they are requested to be opened
    if (ContactorState.open == false) {  // only if contactors are not open yet
      // message content to quickly open contactors
      if (counter_10ms == 0) {
        // @0 ms (0.00) RX0 510 [8] 40 10 00 00 00 80 00 00
        BMWiX_510.data = {0x40, 0x10, 0x00, 0x00,
                          0x00, 0x80, 0x00, 0x00};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_510, can_config.battery);
        // set back to default values
        BMWiX_510.data = {0x40, 0x10, 0x04, 0x00, 0x00, 0x80, 0x01, 0x00};  // default values
      } else if (counter_10ms == 6) {
        // @60 ms  (0.06) RX0 16E [8] E6 A4 C8 FF 60 C9 33 F0
        BMWiX_16E.data = {0xE6, 0xA4, 0xC8, 0xFF,
                          0x60, 0xC9, 0x33, 0xF0};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_16E, can_config.battery);
        // set back to default values
        BMWiX_16E.data = {0x00, 0xA0, 0xC9, 0xFF, 0x60, 0xC9, 0x3A, 0xF7};  // default values
        ContactorState.closed = false;
        ContactorState.open = true;
      }
    }
  }
}

#endif  // BMW_IX_BATTERY
