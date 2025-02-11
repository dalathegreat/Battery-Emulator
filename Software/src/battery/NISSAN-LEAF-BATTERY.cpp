#include "../include.h"
#ifdef NISSAN_LEAF_BATTERY
#include "NISSAN-LEAF-BATTERY.h"
#ifdef MQTT
#include "../devboard/mqtt/mqtt.h"
#endif
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send
static uint8_t mprun10r = 0;                 //counter 0-20 for 0x1F2 message
static uint8_t mprun10 = 0;                  //counter 0-3
static uint8_t mprun100 = 0;                 //counter 0-3

// These CAN messages need to be sent towards the battery to keep it alive
CAN_frame LEAF_1F2 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1F2,
                      .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame LEAF_50B = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x50B,
                      .data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
CAN_frame LEAF_50C = {.FD = false,
                      .ext_ID = false,
                      .DLC = 6,
                      .ID = 0x50C,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame LEAF_1D4 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1D4,
                      .data = {0x6E, 0x6E, 0x00, 0x04, 0x07, 0x46, 0xE0, 0x44}};
// Active polling messages
uint8_t PIDgroups[] = {0x01, 0x02, 0x04, 0x83, 0x84, 0x90};
uint8_t PIDindex = 0;
CAN_frame LEAF_GROUP_REQUEST = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x79B,
                                .data = {2, 0x21, 1, 0, 0, 0, 0, 0}};
CAN_frame LEAF_NEXT_LINE_REQUEST = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x79B,
                                    .data = {0x30, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// The Li-ion battery controller only accepts a multi-message query. In fact, the LBC transmits many
// groups: the first one contains lots of High Voltage battery data as SOC, currents, and voltage; the second
// replies with all the battery’s cells voltages in millivolt, the third and the fifth one are still unknown, the
// fourth contains the four battery packs temperatures, and the last one tells which cell has the shunt active.
// There are also two more groups: group 61, which replies with lots of CAN messages (up to 48); here we
// found the SOH value, and group 84 that replies with the HV battery production serial.

static uint8_t crctable[256] = {
    0,   133, 143, 10,  155, 30,  20,  145, 179, 54,  60,  185, 40,  173, 167, 34,  227, 102, 108, 233, 120, 253,
    247, 114, 80,  213, 223, 90,  203, 78,  68,  193, 67,  198, 204, 73,  216, 93,  87,  210, 240, 117, 127, 250,
    107, 238, 228, 97,  160, 37,  47,  170, 59,  190, 180, 49,  19,  150, 156, 25,  136, 13,  7,   130, 134, 3,
    9,   140, 29,  152, 146, 23,  53,  176, 186, 63,  174, 43,  33,  164, 101, 224, 234, 111, 254, 123, 113, 244,
    214, 83,  89,  220, 77,  200, 194, 71,  197, 64,  74,  207, 94,  219, 209, 84,  118, 243, 249, 124, 237, 104,
    98,  231, 38,  163, 169, 44,  189, 56,  50,  183, 149, 16,  26,  159, 14,  139, 129, 4,   137, 12,  6,   131,
    18,  151, 157, 24,  58,  191, 181, 48,  161, 36,  46,  171, 106, 239, 229, 96,  241, 116, 126, 251, 217, 92,
    86,  211, 66,  199, 205, 72,  202, 79,  69,  192, 81,  212, 222, 91,  121, 252, 246, 115, 226, 103, 109, 232,
    41,  172, 166, 35,  178, 55,  61,  184, 154, 31,  21,  144, 1,   132, 142, 11,  15,  138, 128, 5,   148, 17,
    27,  158, 188, 57,  51,  182, 39,  162, 168, 45,  236, 105, 99,  230, 119, 242, 248, 125, 95,  218, 208, 85,
    196, 65,  75,  206, 76,  201, 195, 70,  215, 82,  88,  221, 255, 122, 112, 245, 100, 225, 235, 110, 175, 42,
    32,  165, 52,  177, 187, 62,  28,  153, 147, 22,  135, 2,   8,   141};

//Nissan LEAF battery parameters from constantly sent CAN
#define ZE0_BATTERY 0
#define AZE0_BATTERY 1
#define ZE1_BATTERY 2
static uint8_t LEAF_battery_Type = ZE0_BATTERY;
static bool battery_can_alive = false;
#define WH_PER_GID 77                               //One GID is this amount of Watt hours
static uint16_t battery_Discharge_Power_Limit = 0;  //Limit in kW
static uint16_t battery_Charge_Power_Limit = 0;     //Limit in kW
static int16_t battery_MAX_POWER_FOR_CHARGER = 0;   //Limit in kW
static int16_t battery_SOC = 500;                   //0 - 100.0 % (0-1000) The real SOC% in the battery
static uint16_t battery_TEMP = 0;                   //Temporary value used in status checks
static uint16_t battery_Wh_Remaining = 0;           //Amount of energy in battery, in Wh
static uint16_t battery_GIDS = 273;                 //Startup in 24kWh mode
static uint16_t battery_MAX = 0;
static uint16_t battery_Max_GIDS = 273;               //Startup in 24kWh mode
static uint16_t battery_StateOfHealth = 99;           //State of health %
static uint16_t battery_Total_Voltage2 = 740;         //Battery voltage (0-450V) [0.5V/bit, so actual range 0-800]
static int16_t battery_Current2 = 0;                  //Battery current (-400-200A) [0.5A/bit, so actual range -800-400]
static int16_t battery_HistData_Temperature_MAX = 6;  //-40 to 86*C
static int16_t battery_HistData_Temperature_MIN = 5;  //-40 to 86*C
static int16_t battery_AverageTemperature = 6;        //Only available on ZE0, in celcius, -40 to +55
static uint8_t battery_Relay_Cut_Request = 0;         //battery_FAIL
static uint8_t battery_Failsafe_Status = 0;           //battery_STATUS
static bool battery_Interlock =
    true;  //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
static bool battery_Full_CHARGE_flag = false;  //battery_FCHGEND , Goes to 1 if battery is fully charged
static bool battery_MainRelayOn_flag = false;  //No-Permission=0, Main Relay On Permission=1
static bool battery_Capacity_Empty = false;    //battery_EMPTY, , Goes to 1 if battery is empty
static bool battery_HeatExist = false;  //battery_HEATEXIST, Specifies if battery pack is equipped with heating elements
static bool battery_Heating_Stop = false;                   //When transitioning from 0->1, signals a STOP heat request
static bool battery_Heating_Start = false;                  //When transitioning from 1->0, signals a START heat request
static bool battery_Batt_Heater_Mail_Send_Request = false;  //Stores info when a heat request is happening

// Nissan LEAF battery data from polled CAN messages
static uint8_t battery_request_idx = 0;
static uint8_t group_7bb = 0;
static bool stop_battery_query = true;
static uint8_t hold_off_with_polling_10seconds = 2;  //Paused for 20 seconds on startup
static uint16_t battery_cell_voltages[97];           //array with all the cellvoltages
static uint8_t battery_cellcounter = 0;
static uint16_t battery_min_max_voltage[2];  //contains cell min[0] and max[1] values in mV
static uint16_t battery_HX = 0;              //Internal resistance
static uint16_t battery_insulation = 0;      //Insulation resistance
static uint16_t battery_temp_raw_1 = 0;
static uint8_t battery_temp_raw_2_highnibble = 0;
static uint16_t battery_temp_raw_2 = 0;
static uint16_t battery_temp_raw_3 = 0;
static uint16_t battery_temp_raw_4 = 0;
static uint16_t battery_temp_raw_max = 0;
static uint16_t battery_temp_raw_min = 0;
static int16_t battery_temp_polled_max = 0;
static int16_t battery_temp_polled_min = 0;
static uint8_t BatterySerialNumber[15] = {0};  // Stores raw HEX values for ASCII chars
static uint8_t BatteryPartNumber[7] = {0};     // Stores raw HEX values for ASCII chars
static uint8_t BMSIDcode[8] = {0};
#ifdef DOUBLE_BATTERY
static uint8_t LEAF_battery2_Type = ZE0_BATTERY;
static bool battery2_can_alive = false;
static uint16_t battery2_Discharge_Power_Limit = 0;  //Limit in kW
static uint16_t battery2_Charge_Power_Limit = 0;     //Limit in kW
static int16_t battery2_MAX_POWER_FOR_CHARGER = 0;   //Limit in kW
static int16_t battery2_SOC = 500;                   //0 - 100.0 % (0-1000) The real SOC% in the battery
static uint16_t battery2_TEMP = 0;                   //Temporary value used in status checks
static uint16_t battery2_Wh_Remaining = 0;           //Amount of energy in battery, in Wh
static uint16_t battery2_GIDS = 273;                 //Startup in 24kWh mode
static uint16_t battery2_MAX = 0;
static uint16_t battery2_Max_GIDS = 273;      //Startup in 24kWh mode
static uint16_t battery2_StateOfHealth = 99;  //State of health %
static uint16_t battery2_Total_Voltage2 = 0;  //Battery voltage (0-450V) [0.5V/bit, so actual range 0-800]
static int16_t battery2_Current2 = 0;         //Battery current (-400-200A) [0.5A/bit, so actual range -800-400]
static int16_t battery2_HistData_Temperature_MAX = 6;  //-40 to 86*C
static int16_t battery2_HistData_Temperature_MIN = 5;  //-40 to 86*C
static int16_t battery2_AverageTemperature = 6;        //Only available on ZE0, in celcius, -40 to +55
static uint8_t battery2_Relay_Cut_Request = 0;         //battery2_FAIL
static uint8_t battery2_Failsafe_Status = 0;           //battery2_STATUS
static bool battery2_Interlock =
    true;  //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
static bool battery2_Full_CHARGE_flag = false;  //battery2_FCHGEND , Goes to 1 if battery is fully charged
static bool battery2_MainRelayOn_flag = false;  //No-Permission=0, Main Relay On Permission=1
static bool battery2_Capacity_Empty = false;    //battery2_EMPTY, , Goes to 1 if battery is empty
static bool battery2_HeatExist =
    false;  //battery2_HEATEXIST, Specifies if battery pack is equipped with heating elements
static bool battery2_Heating_Stop = false;   //When transitioning from 0->1, signals a STOP heat request
static bool battery2_Heating_Start = false;  //When transitioning from 1->0, signals a START heat request
static bool battery2_Batt_Heater_Mail_Send_Request = false;  //Stores info when a heat request is happening
// Polled values
static uint8_t battery2_group_7bb = 0;
static uint8_t battery2_request_idx = 0;
static uint16_t battery2_cell_voltages[97];  //array with all the cellvoltages
static uint8_t battery2_cellcounter = 0;
static uint16_t battery2_min_max_voltage[2];  //contains cell min[0] and max[1] values in mV
static uint16_t battery2_HX = 0;              //Internal resistance
static uint16_t battery2_insulation = 0;      //Insulation resistance
static uint16_t battery2_temp_raw_1 = 0;
static uint8_t battery2_temp_raw_2_highnibble = 0;
static uint16_t battery2_temp_raw_2 = 0;
static uint16_t battery2_temp_raw_3 = 0;
static uint16_t battery2_temp_raw_4 = 0;
static uint16_t battery2_temp_raw_max = 0;
static uint16_t battery2_temp_raw_min = 0;
static int16_t battery2_temp_polled_max = 0;
static int16_t battery2_temp_polled_min = 0;
#endif  // DOUBLE_BATTERY

// Clear SOH values
static uint8_t stateMachineClearSOH = 0xFF;
static uint32_t incomingChallenge = 0xFFFFFFFF;
static uint8_t solvedChallenge[8];
static bool challengeFailed = false;

CAN_frame LEAF_CLEAR_SOH = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x79B,
                            .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

void update_values_battery() { /* This function maps all the values fetched via CAN to the correct parameters used for modbus */
  /* Start with mapping all values */

  datalayer.battery.status.soh_pptt = (battery_StateOfHealth * 100);  //Increase range from 99% -> 99.00%

  datalayer.battery.status.real_soc = (battery_SOC * 10);

  datalayer.battery.status.voltage_dV =
      (battery_Total_Voltage2 * 5);  //0.5V/bit, multiply by 5 to get Voltage+1decimal (350.5V = 701)

  datalayer.battery.status.current_dA =
      (battery_Current2 * 5);  //0.5A/bit, multiply by 5 to get Amp+1decimal (5,5A = 11)

  if (battery_Max_GIDS == 273) {  //battery_Max_GIDS is stuck at 273 on ZE0
    datalayer.battery.info.total_capacity_Wh = ((battery_Max_GIDS * WH_PER_GID * battery_StateOfHealth) / 100);
  } else {  //battery_Max_GIDS updates on newer generations, making for a total_capacity_Wh value that makes sense
    datalayer.battery.info.total_capacity_Wh = (battery_Max_GIDS * WH_PER_GID);
  }

  datalayer.battery.status.remaining_capacity_Wh = battery_Wh_Remaining;

  //Update temperature readings. Method depends on which generation LEAF battery is used
  if (LEAF_battery_Type == ZE0_BATTERY) {
    //Since we only have average value, send the minimum as -1.0 degrees below average
    datalayer.battery.status.temperature_min_dC =
        ((battery_AverageTemperature * 10) - 10);  //Increase range from C to C+1, remove 1.0C
    datalayer.battery.status.temperature_max_dC = (battery_AverageTemperature * 10);  //Increase range from C to C+1
  } else if (LEAF_battery_Type == AZE0_BATTERY) {
    //Use the value sent constantly via CAN in 5C0 (only available on AZE0)
    datalayer.battery.status.temperature_min_dC =
        (battery_HistData_Temperature_MIN * 10);  //Increase range from C to C+1
    datalayer.battery.status.temperature_max_dC =
        (battery_HistData_Temperature_MAX * 10);  //Increase range from C to C+1
  } else {  // ZE1 (TODO: Once the muxed value in 5C0 becomes known, switch to using that instead of this complicated polled value)
    if (battery_temp_raw_min != 0)  //We have a polled value available
    {
      battery_temp_polled_min = ((Temp_fromRAW_to_F(battery_temp_raw_min) - 320) * 5) / 9;  //Convert from F to C
      battery_temp_polled_max = ((Temp_fromRAW_to_F(battery_temp_raw_max) - 320) * 5) / 9;  //Convert from F to C
      if (battery_temp_polled_min < battery_temp_polled_max) {  //Catch any edge cases from Temp_fromRAW_to_F function
        datalayer.battery.status.temperature_min_dC = battery_temp_polled_min;
        datalayer.battery.status.temperature_max_dC = battery_temp_polled_max;
      } else {
        datalayer.battery.status.temperature_min_dC = battery_temp_polled_max;
        datalayer.battery.status.temperature_max_dC = battery_temp_polled_min;
      }
    }
  }

  datalayer.battery.status.max_discharge_power_W = (battery_Discharge_Power_Limit * 1000);  //kW to W

  datalayer.battery.status.max_charge_power_W = (battery_Charge_Power_Limit * 1000);  //kW to W

  //Allow contactors to close
  if (battery_can_alive) {
    datalayer.system.status.battery_allows_contactor_closing = true;
  }

  /*Extra safety functions below*/
  if (battery_GIDS < 10)  //700Wh left in battery!
  {                       //Battery is running abnormally low, some discharge logic might have failed. Zero it all out.
    set_event(EVENT_BATTERY_EMPTY, 0);
    datalayer.battery.status.real_soc = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
  }

  if (battery_Full_CHARGE_flag) {  //Battery reports that it is fully charged stop all further charging incase it hasn't already
    set_event(EVENT_BATTERY_FULL, 0);
    datalayer.battery.status.max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL);
  }

  if (battery_Capacity_Empty) {  //Battery reports that it is fully discharged. Stop all further discharging incase it hasn't already
    set_event(EVENT_BATTERY_EMPTY, 0);
    datalayer.battery.status.max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY);
  }

  if (battery_Total_Voltage2 == 0x3FF) {  //Battery reports critical measurement unavailable
    set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, 0);
  } else {
    clear_event(EVENT_BATTERY_VALUE_UNAVAILABLE);
  }

  if (battery_Relay_Cut_Request) {  //battery_FAIL, BMS requesting shutdown and contactors to be opened
    //Note, this is sometimes triggered during the night while idle, and the BMS recovers after a while. Removed latching from this scenario
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_power_W = 0;
  }

  if (battery_Failsafe_Status > 0)  // 0 is normal, start charging/discharging
  {
    switch (battery_Failsafe_Status) {
      case (1):
        //Normal Stop Request
        //This means that battery is fully discharged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (2):
        //Charging Mode Stop Request
        //This means that battery is fully charged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (3):
        //Charging Mode Stop Request & Normal Stop Request
        //Normal stop request. For stationary storage we don't disconnect contactors, so we ignore this.
        break;
      case (4):
        //Caution Lamp Request
        set_event(EVENT_BATTERY_CAUTION, 0);
        break;
      case (5):
        //Caution Lamp Request & Normal Stop Request
        set_event(EVENT_BATTERY_DISCHG_STOP_REQ, 0);
        break;
      case (6):
        //Caution Lamp Request & Charging Mode Stop Request
        set_event(EVENT_BATTERY_CHG_STOP_REQ, 0);
        break;
      case (7):
        //Caution Lamp Request & Charging Mode Stop Request & Normal Stop Request
        set_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 0);
        break;
      default:
        break;
    }
  } else {  //battery_Failsafe_Status == 0
    clear_event(EVENT_BATTERY_DISCHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ);
  }

#ifdef INTERLOCK_REQUIRED
  if (!battery_Interlock) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
#endif

  if (battery_HeatExist) {
    if (battery_Heating_Stop) {
      set_event(EVENT_BATTERY_WARMED_UP, 0);
    }
    if (battery_Heating_Start) {
      set_event(EVENT_BATTERY_REQUESTS_HEAT, 0);
    }
  }

  // Update webserver datalayer
  memcpy(datalayer_extended.nissanleaf.BatterySerialNumber, BatterySerialNumber, sizeof(BatterySerialNumber));
  memcpy(datalayer_extended.nissanleaf.BatteryPartNumber, BatteryPartNumber, sizeof(BatteryPartNumber));
  memcpy(datalayer_extended.nissanleaf.BMSIDcode, BMSIDcode, sizeof(BMSIDcode));
  datalayer_extended.nissanleaf.LEAF_gen = LEAF_battery_Type;
  datalayer_extended.nissanleaf.GIDS = battery_GIDS;
  datalayer_extended.nissanleaf.ChargePowerLimit = battery_Charge_Power_Limit;
  datalayer_extended.nissanleaf.MaxPowerForCharger = battery_MAX_POWER_FOR_CHARGER;
  datalayer_extended.nissanleaf.Interlock = battery_Interlock;
  datalayer_extended.nissanleaf.Insulation = battery_insulation;
  datalayer_extended.nissanleaf.RelayCutRequest = battery_Relay_Cut_Request;
  datalayer_extended.nissanleaf.FailsafeStatus = battery_Failsafe_Status;
  datalayer_extended.nissanleaf.Full = battery_Full_CHARGE_flag;
  datalayer_extended.nissanleaf.Empty = battery_Capacity_Empty;
  datalayer_extended.nissanleaf.MainRelayOn = battery_MainRelayOn_flag;
  datalayer_extended.nissanleaf.HeatExist = battery_HeatExist;
  datalayer_extended.nissanleaf.HeatingStop = battery_Heating_Stop;
  datalayer_extended.nissanleaf.HeatingStart = battery_Heating_Start;
  datalayer_extended.nissanleaf.HeaterSendRequest = battery_Batt_Heater_Mail_Send_Request;
  datalayer_extended.nissanleaf.CryptoChallenge = incomingChallenge;
  datalayer_extended.nissanleaf.SolvedChallengeMSB =
      ((solvedChallenge[7] << 24) | (solvedChallenge[6] << 16) | (solvedChallenge[5] << 8) | solvedChallenge[4]);
  datalayer_extended.nissanleaf.SolvedChallengeLSB =
      ((solvedChallenge[3] << 24) | (solvedChallenge[2] << 16) | (solvedChallenge[1] << 8) | solvedChallenge[0]);
  datalayer_extended.nissanleaf.challengeFailed = challengeFailed;

  // Update requests from webserver datalayer
  if (datalayer_extended.nissanleaf.UserRequestSOHreset) {
    stateMachineClearSOH = 0;  //Start the statemachine
    datalayer_extended.nissanleaf.UserRequestSOHreset = false;
  }
}

#ifdef DOUBLE_BATTERY

void update_values_battery2() {  // Handle the values coming in from battery #2
  /* Start with mapping all values */

  datalayer.battery2.status.soh_pptt = (battery2_StateOfHealth * 100);  //Increase range from 99% -> 99.00%

  datalayer.battery2.status.real_soc = (battery2_SOC * 10);

  datalayer.battery2.status.voltage_dV =
      (battery2_Total_Voltage2 * 5);  //0.5V/bit, multiply by 5 to get Voltage+1decimal (350.5V = 701)

  datalayer.battery2.status.current_dA =
      (battery2_Current2 * 5);  //0.5A/bit, multiply by 5 to get Amp+1decimal (5,5A = 11)

  if (battery2_Max_GIDS == 273) {  //battery2_Max_GIDS is stuck at 273 on 24kWh packs
    datalayer.battery2.info.total_capacity_Wh = ((battery2_Max_GIDS * WH_PER_GID * battery2_StateOfHealth) / 100);
  } else {  //battery_Max_GIDS updates on newer generations, making for a total_capacity_Wh value that makes sense
    datalayer.battery2.info.total_capacity_Wh = (battery2_Max_GIDS * WH_PER_GID);
  }

  datalayer.battery2.status.remaining_capacity_Wh = battery2_Wh_Remaining;

  //Update temperature readings. Method depends on which generation LEAF battery is used
  if (LEAF_battery2_Type == ZE0_BATTERY) {
    //Since we only have average value, send the minimum as -1.0 degrees below average
    datalayer.battery2.status.temperature_min_dC =
        ((battery2_AverageTemperature * 10) - 10);  //Increase range from C to C+1, remove 1.0C
    datalayer.battery2.status.temperature_max_dC = (battery2_AverageTemperature * 10);  //Increase range from C to C+1
  } else if (LEAF_battery2_Type == AZE0_BATTERY) {
    //Use the value sent constantly via CAN in 5C0 (only available on AZE0)
    datalayer.battery2.status.temperature_min_dC =
        (battery2_HistData_Temperature_MIN * 10);  //Increase range from C to C+1
    datalayer.battery2.status.temperature_max_dC =
        (battery2_HistData_Temperature_MAX * 10);  //Increase range from C to C+1
  } else {  // ZE1 (TODO: Once the muxed value in 5C0 becomes known, switch to using that instead of this complicated polled value)
    if (battery2_temp_raw_min != 0)  //We have a polled value available
    {
      battery2_temp_polled_min = ((Temp_fromRAW_to_F(battery2_temp_raw_min) - 320) * 5) / 9;  //Convert from F to C
      battery2_temp_polled_max = ((Temp_fromRAW_to_F(battery2_temp_raw_max) - 320) * 5) / 9;  //Convert from F to C
      if (battery2_temp_polled_min < battery2_temp_polled_max) {  //Catch any edge cases from Temp_fromRAW_to_F function
        datalayer.battery2.status.temperature_min_dC = battery2_temp_polled_min;
        datalayer.battery2.status.temperature_max_dC = battery2_temp_polled_max;
      } else {
        datalayer.battery2.status.temperature_min_dC = battery2_temp_polled_max;
        datalayer.battery2.status.temperature_max_dC = battery2_temp_polled_min;
      }
    }
  }

  datalayer.battery2.status.max_discharge_power_W = (battery2_Discharge_Power_Limit * 1000);  //kW to W

  datalayer.battery2.status.max_charge_power_W = (battery2_Charge_Power_Limit * 1000);  //kW to W

  /*Extra safety functions below*/
  if (battery2_GIDS < 10)  //700Wh left in battery!
  {                        //Battery is running abnormally low, some discharge logic might have failed. Zero it all out.
    set_event(EVENT_BATTERY_EMPTY, 0);
    datalayer.battery2.status.real_soc = 0;
    datalayer.battery2.status.max_discharge_power_W = 0;
  }

  if (battery2_Full_CHARGE_flag) {  //Battery reports that it is fully charged stop all further charging incase it hasn't already
    set_event(EVENT_BATTERY_FULL, 0);
    datalayer.battery2.status.max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL);
  }

  if (battery2_Capacity_Empty) {  //Battery reports that it is fully discharged. Stop all further discharging incase it hasn't already
    set_event(EVENT_BATTERY_EMPTY, 0);
    datalayer.battery2.status.max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY);
  }

  if (battery2_Total_Voltage2 == 0x3FF) {  //Battery reports critical measurement unavailable
    set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, 0);
  } else {
    clear_event(EVENT_BATTERY_VALUE_UNAVAILABLE);
  }

  if (battery2_Relay_Cut_Request) {  //battery2_FAIL, BMS requesting shutdown and contactors to be opened
    //Note, this is sometimes triggered during the night while idle, and the BMS recovers after a while. Removed latching from this scenario
    datalayer.battery2.status.max_discharge_power_W = 0;
    datalayer.battery2.status.max_charge_power_W = 0;
  }

  if (battery2_Failsafe_Status > 0)  // 0 is normal, start charging/discharging
  {
    switch (battery2_Failsafe_Status) {
      case (1):
        //Normal Stop Request
        //This means that battery is fully discharged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (2):
        //Charging Mode Stop Request
        //This means that battery is fully charged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (3):
        //Charging Mode Stop Request & Normal Stop Request
        //Normal stop request. For stationary storage we don't disconnect contactors, so we ignore this.
        break;
      case (4):
        //Caution Lamp Request
        set_event(EVENT_BATTERY_CAUTION, 2);
        break;
      case (5):
        //Caution Lamp Request & Normal Stop Request
        set_event(EVENT_BATTERY_DISCHG_STOP_REQ, 2);
        break;
      case (6):
        //Caution Lamp Request & Charging Mode Stop Request
        set_event(EVENT_BATTERY_CHG_STOP_REQ, 2);
        break;
      case (7):
        //Caution Lamp Request & Charging Mode Stop Request & Normal Stop Request
        set_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 2);
        break;
      default:
        break;
    }
  } else {  //battery2_Failsafe_Status == 0
    clear_event(EVENT_BATTERY_DISCHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ);
  }

#ifdef INTERLOCK_REQUIRED
  if (!battery2_Interlock) {
    set_event(EVENT_HVIL_FAILURE, 2);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
#endif

  if (battery2_HeatExist) {
    if (battery2_Heating_Stop) {
      set_event(EVENT_BATTERY_WARMED_UP, 2);
    }
    if (battery2_Heating_Start) {
      set_event(EVENT_BATTERY_REQUESTS_HEAT, 2);
    }
  }
}
void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1DB:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery2_Current2 = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
      if (battery2_Current2 & 0x0400) {
        // negative so extend the sign bit
        battery2_Current2 |= 0xf800;
      }  //BatteryCurrentSignal , 2s comp, 1lSB = 0.5A/bit

      battery2_TEMP = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6);  //0.5V/bit
      if (battery2_TEMP != 0x3ff) {  //3FF is unavailable value. Can happen directly on reboot.
        battery2_Total_Voltage2 = battery2_TEMP;
      }

      //Collect various data from the BMS
      battery2_Relay_Cut_Request = ((rx_frame.data.u8[1] & 0x18) >> 3);
      battery2_Failsafe_Status = (rx_frame.data.u8[1] & 0x07);
      battery2_MainRelayOn_flag = (bool)((rx_frame.data.u8[3] & 0x20) >> 5);
      //battery2_allows_contactor_closing written by check_interconnect_available();
      battery2_Full_CHARGE_flag = (bool)((rx_frame.data.u8[3] & 0x10) >> 4);
      battery2_Interlock = (bool)((rx_frame.data.u8[3] & 0x08) >> 3);
      break;
    case 0x1DC:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery2_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
      battery2_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 4 | rx_frame.data.u8[2] >> 4) / 4.0);
      battery2_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
      break;
    case 0x55B:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery2_TEMP = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
      if (battery2_TEMP != 0x3ff) {  //3FF is unavailable value
        battery2_SOC = battery2_TEMP;
      }
      battery2_Capacity_Empty = (bool)((rx_frame.data.u8[6] & 0x80) >> 7);
      break;
    case 0x5BC:
      battery2_can_alive = true;
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN

      battery2_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
      if (battery2_MAX) {
        battery2_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        //Max gids active, do nothing
        //Only the 30/40/62kWh packs have this mux
      } else {  //Normal current GIDS value is transmitted
        battery2_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        battery2_Wh_Remaining = (battery2_GIDS * WH_PER_GID);
      }

      if (LEAF_battery2_Type == ZE0_BATTERY) {
        battery2_AverageTemperature = (rx_frame.data.u8[3] - 40);  //In celcius, -40 to +55
      }

      battery2_TEMP = (rx_frame.data.u8[4] >> 1);
      if (battery2_TEMP != 0) {
        battery2_StateOfHealth = battery2_TEMP;  //Collect state of health from battery
      }
      break;
    case 0x5C0:
      //This temperature only works for 2013-2017 AZE0 LEAF packs, the mux is different on other generations
      if (LEAF_battery2_Type == AZE0_BATTERY) {
        if ((rx_frame.data.u8[0] >> 6) ==
            1) {  // Battery MAX temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery2_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
        }
        if ((rx_frame.data.u8[0] >> 6) ==
            3) {  // Battery MIN temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery2_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
        }
      }

      battery2_HeatExist = (rx_frame.data.u8[4] & 0x01);
      battery2_Heating_Stop = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery2_Heating_Start = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery2_Batt_Heater_Mail_Send_Request = (rx_frame.data.u8[1] & 0x01);

      break;
    case 0x59E:
      //AZE0 2013-2017 or ZE1 2018-2023 battery detected
      //Only detect as AZE0 if not already set as ZE1
      if (LEAF_battery2_Type != ZE1_BATTERY) {
        LEAF_battery2_Type = AZE0_BATTERY;
      }
      break;
    case 0x1ED:
    case 0x1C2:
      //ZE1 2018-2023 battery detected!
      LEAF_battery2_Type = ZE1_BATTERY;
      break;
    case 0x79B:
      stop_battery_query = true;             //Someone is trying to read data with Leafspy, stop our own polling!
      hold_off_with_polling_10seconds = 10;  //Polling is paused for 100s
      break;
    case 0x7BB:

      if (stop_battery_query) {  //Leafspy/Service request is active, stop our own polling
        break;
      }

      //First check which group data we are getting
      if (rx_frame.data.u8[0] == 0x10) {  //First message of a group
        battery2_group_7bb = rx_frame.data.u8[3];
      }

      transmit_can_frame(&LEAF_NEXT_LINE_REQUEST, can_config.battery_double);

      if (battery2_group_7bb == 0x01)  //High precision SOC, Current, voltages etc.
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame
          //High precision battery2_current_1 resides here, but has been deemed unusable by 62kWh owners
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame
          //High precision battery2_current_2 resides here, but has been deemed unusable by 62kWh owners
        }

        if (rx_frame.data.u8[0] == 0x23) {  // Fourth frame
          battery2_insulation = (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        }

        if (rx_frame.data.u8[0] == 0x24) {  // Fifth frame
          battery2_HX = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 102.4;
        }
      }

      if (battery2_group_7bb == 0x02)  //Cell Voltages
      {
        if (rx_frame.data.u8[0] == 0x10) {  //first frame is anomalous
          battery2_request_idx = 0;
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          break;
        }
        if (rx_frame.data.u8[6] == 0xFF && rx_frame.data.u8[0] == 0x2C) {  //Last frame
          //Last frame does not contain any cell data, calculate the result

          //Map all cell voltages to the global array
          memcpy(datalayer.battery2.status.cell_voltages_mV, battery2_cell_voltages, 96 * sizeof(uint16_t));

          //calculate min/max voltages
          battery2_min_max_voltage[0] = 9999;
          battery2_min_max_voltage[1] = 0;
          for (battery2_cellcounter = 0; battery2_cellcounter < 96; battery2_cellcounter++) {
            if (battery2_min_max_voltage[0] > battery2_cell_voltages[battery2_cellcounter])
              battery2_min_max_voltage[0] = battery2_cell_voltages[battery2_cellcounter];
            if (battery2_min_max_voltage[1] < battery2_cell_voltages[battery2_cellcounter])
              battery2_min_max_voltage[1] = battery2_cell_voltages[battery2_cellcounter];
          }

          datalayer.battery2.status.cell_max_voltage_mV = battery2_min_max_voltage[1];
          datalayer.battery2.status.cell_min_voltage_mV = battery2_min_max_voltage[0];

          break;
        }

        if ((rx_frame.data.u8[0] % 2) == 0) {  //even frames
          battery2_cell_voltages[battery2_request_idx++] |= rx_frame.data.u8[1];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        } else {  //odd frames
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery2_cell_voltages[battery2_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          battery2_cell_voltages[battery2_request_idx] = (rx_frame.data.u8[7] << 8);
        }
      }

      if (battery2_group_7bb == 0x04) {     //Temperatures
        if (rx_frame.data.u8[0] == 0x10) {  //First message
          battery2_temp_raw_1 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery2_temp_raw_2_highnibble = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second message
          battery2_temp_raw_2 = (battery2_temp_raw_2_highnibble << 8) | rx_frame.data.u8[1];
          battery2_temp_raw_3 = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery2_temp_raw_4 = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third message
          //All values read, let's figure out the min/max!

          if (battery2_temp_raw_3 == 65535) {  //We are on a 2013+ pack that only has three temp sensors.
            //Start with finding max value
            battery2_temp_raw_max = battery2_temp_raw_1;
            if (battery2_temp_raw_2 > battery2_temp_raw_max) {
              battery2_temp_raw_max = battery2_temp_raw_2;
            }
            if (battery2_temp_raw_4 > battery2_temp_raw_max) {
              battery2_temp_raw_max = battery2_temp_raw_4;
            }
            //Then find min
            battery2_temp_raw_min = battery2_temp_raw_1;
            if (battery2_temp_raw_2 < battery2_temp_raw_min) {
              battery2_temp_raw_min = battery2_temp_raw_2;
            }
            if (battery2_temp_raw_4 < battery2_temp_raw_min) {
              battery2_temp_raw_min = battery2_temp_raw_4;
            }
          } else {  //All 4 temp sensors available on 2011-2012
            //Start with finding max value
            battery2_temp_raw_max = battery2_temp_raw_1;
            if (battery2_temp_raw_2 > battery2_temp_raw_max) {
              battery2_temp_raw_max = battery2_temp_raw_2;
            }
            if (battery2_temp_raw_3 > battery2_temp_raw_max) {
              battery2_temp_raw_max = battery2_temp_raw_3;
            }
            if (battery2_temp_raw_4 > battery2_temp_raw_max) {
              battery2_temp_raw_max = battery2_temp_raw_4;
            }
            //Then find min
            battery2_temp_raw_min = battery2_temp_raw_1;
            if (battery2_temp_raw_2 < battery2_temp_raw_min) {
              battery2_temp_raw_min = battery2_temp_raw_2;
            }
            if (battery2_temp_raw_3 < battery2_temp_raw_min) {
              battery2_temp_raw_min = battery2_temp_raw_2;
            }
            if (battery2_temp_raw_4 < battery2_temp_raw_min) {
              battery2_temp_raw_min = battery2_temp_raw_4;
            }
          }
        }
      }

      break;
    default:
      break;
  }
}
#endif  // DOUBLE_BATTERY

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1DB:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_Current2 = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
      if (battery_Current2 & 0x0400) {
        // negative so extend the sign bit
        battery_Current2 |= 0xf800;
      }  //BatteryCurrentSignal , 2s comp, 1lSB = 0.5A/bit

      battery_TEMP = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6);  //0.5V/bit
      if (battery_TEMP != 0x3ff) {  //3FF is unavailable value. Can happen directly on reboot.
        battery_Total_Voltage2 = battery_TEMP;
      }

      //Collect various data from the BMS
      battery_Relay_Cut_Request = ((rx_frame.data.u8[1] & 0x18) >> 3);
      battery_Failsafe_Status = (rx_frame.data.u8[1] & 0x07);
      battery_MainRelayOn_flag = (bool)((rx_frame.data.u8[3] & 0x20) >> 5);
      battery_Full_CHARGE_flag = (bool)((rx_frame.data.u8[3] & 0x10) >> 4);
      battery_Interlock = (bool)((rx_frame.data.u8[3] & 0x08) >> 3);
      break;
    case 0x1DC:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
      battery_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 4 | rx_frame.data.u8[2] >> 4) / 4.0);
      battery_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
      break;
    case 0x55B:
      if (is_message_corrupt(rx_frame)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_TEMP = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
      if (battery_TEMP != 0x3ff) {  //3FF is unavailable value
        battery_SOC = battery_TEMP;
      }
      battery_Capacity_Empty = (bool)((rx_frame.data.u8[6] & 0x80) >> 7);
      break;
    case 0x5BC:
      battery_can_alive = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN

      battery_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
      if (battery_MAX) {
        battery_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        //Max gids active, do nothing
        //Only the 30/40/62kWh packs have this mux
      } else {  //Normal current GIDS value is transmitted
        battery_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        battery_Wh_Remaining = (battery_GIDS * WH_PER_GID);
      }

      if (LEAF_battery_Type == ZE0_BATTERY) {
        battery_AverageTemperature = (rx_frame.data.u8[3] - 40);  //In celcius, -40 to +55
      }

      battery_TEMP = (rx_frame.data.u8[4] >> 1);
      if (battery_TEMP != 0) {
        battery_StateOfHealth = (uint8_t)battery_TEMP;  //Collect state of health from battery
      }
      break;
    case 0x5C0:
      //This temperature only works for 2013-2017 AZE0 LEAF packs, the mux is different on other generations
      if (LEAF_battery_Type == AZE0_BATTERY) {
        if ((rx_frame.data.u8[0] >> 6) ==
            1) {  // Battery MAX temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
        }
        if ((rx_frame.data.u8[0] >> 6) ==
            3) {  // Battery MIN temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
        }
      }

      battery_HeatExist = (rx_frame.data.u8[4] & 0x01);
      battery_Heating_Stop = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery_Heating_Start = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery_Batt_Heater_Mail_Send_Request = (rx_frame.data.u8[1] & 0x01);

      break;
    case 0x59E:
      //AZE0 2013-2017 or ZE1 2018-2023 battery detected
      //Only detect as AZE0 if not already set as ZE1
      if (LEAF_battery_Type != ZE1_BATTERY) {
        LEAF_battery_Type = AZE0_BATTERY;
      }
      break;
    case 0x1ED:
    case 0x1C2:
      //ZE1 2018-2023 battery detected!
      LEAF_battery_Type = ZE1_BATTERY;
      break;
    case 0x79B:
      stop_battery_query = true;             //Someone is trying to read data with Leafspy, stop our own polling!
      hold_off_with_polling_10seconds = 10;  //Polling is paused for 100s
      break;
    case 0x7BB:

      // This section checks if we are doing a SOH reset towards BMS. If we do, all 7BB handling is halted
      if (stateMachineClearSOH < 255) {
        //Intercept the messages based on state machine
        if (rx_frame.data.u8[0] == 0x06) {  // Incoming challenge data!
                                            // BMS should reply with (challenge) 06 67 65 (02 DD 86 43) FF
          incomingChallenge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) |
                               rx_frame.data.u8[6]);
        }
        //Error checking
        if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F)) {
          challengeFailed = true;
        }
        break;
      }

      if (stop_battery_query) {  //Leafspy is active, stop our own polling
        break;
      }

      //First check which group data we are getting
      if (rx_frame.data.u8[0] == 0x10) {  //First message of a group
        group_7bb = rx_frame.data.u8[3];
      }

      transmit_can_frame(&LEAF_NEXT_LINE_REQUEST, can_config.battery);  //Request the next frame for the group

      if (group_7bb == 0x01)  //High precision SOC, Current, voltages etc.
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame
          //High precision Battery_current_1 resides here, but has been deemed unusable by 62kWh owners
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame
          //High precision Battery_current_2 resides here, but has been deemed unusable by 62kWh owners
        }

        if (rx_frame.data.u8[0] == 0x23) {  // Fourth frame
          battery_insulation = (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        }

        if (rx_frame.data.u8[0] == 0x24) {  // Fifth frame
          battery_HX = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 102.4;
        }
      }

      if (group_7bb == 0x02)  //Cell Voltages
      {
        if (rx_frame.data.u8[0] == 0x10) {  //first frame is anomalous
          battery_request_idx = 0;
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          break;
        }
        if (rx_frame.data.u8[6] == 0xFF && rx_frame.data.u8[0] == 0x2C) {  //Last frame
          //Last frame does not contain any cell data, calculate the result

          //Map all cell voltages to the global array
          memcpy(datalayer.battery.status.cell_voltages_mV, battery_cell_voltages, 96 * sizeof(uint16_t));

          //calculate min/max voltages
          battery_min_max_voltage[0] = 9999;
          battery_min_max_voltage[1] = 0;
          for (battery_cellcounter = 0; battery_cellcounter < 96; battery_cellcounter++) {
            if (battery_min_max_voltage[0] > battery_cell_voltages[battery_cellcounter])
              battery_min_max_voltage[0] = battery_cell_voltages[battery_cellcounter];
            if (battery_min_max_voltage[1] < battery_cell_voltages[battery_cellcounter])
              battery_min_max_voltage[1] = battery_cell_voltages[battery_cellcounter];
          }

          datalayer.battery.status.cell_max_voltage_mV = battery_min_max_voltage[1];
          datalayer.battery.status.cell_min_voltage_mV = battery_min_max_voltage[0];

          break;
        }

        if ((rx_frame.data.u8[0] % 2) == 0) {  //even frames
          battery_cell_voltages[battery_request_idx++] |= rx_frame.data.u8[1];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        } else {  //odd frames
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          battery_cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        }
      }

      if (group_7bb == 0x04) {              //Temperatures
        if (rx_frame.data.u8[0] == 0x10) {  //First message
          battery_temp_raw_1 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_temp_raw_2_highnibble = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second message
          battery_temp_raw_2 = (battery_temp_raw_2_highnibble << 8) | rx_frame.data.u8[1];
          battery_temp_raw_3 = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery_temp_raw_4 = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third message
          //All values read, let's figure out the min/max!

          if (battery_temp_raw_3 == 65535) {  //We are on a 2013+ pack that only has three temp sensors.
            //Start with finding max value
            battery_temp_raw_max = battery_temp_raw_1;
            if (battery_temp_raw_2 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_4;
            }
            //Then find min
            battery_temp_raw_min = battery_temp_raw_1;
            if (battery_temp_raw_2 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_4;
            }
          } else {  //All 4 temp sensors available on 2011-2012
            //Start with finding max value
            battery_temp_raw_max = battery_temp_raw_1;
            if (battery_temp_raw_2 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_2;
            }
            if (battery_temp_raw_3 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_3;
            }
            if (battery_temp_raw_4 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_4;
            }
            //Then find min
            battery_temp_raw_min = battery_temp_raw_1;
            if (battery_temp_raw_2 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_3 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_4;
            }
          }
        }
      }

      if (group_7bb == 0x83)  //BatteryPartNumber
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (101A6183334E4B32)
          BatteryPartNumber[0] = rx_frame.data.u8[4];
          BatteryPartNumber[1] = rx_frame.data.u8[5];
          BatteryPartNumber[2] = rx_frame.data.u8[6];
          BatteryPartNumber[3] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame (2141524205170000)
          BatteryPartNumber[4] = rx_frame.data.u8[1];
          BatteryPartNumber[5] = rx_frame.data.u8[2];
          BatteryPartNumber[6] = rx_frame.data.u8[3];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third frame (2200000002101311)
        }

        if (rx_frame.data.u8[0] == 0x23) {  //Fourth frame (23000000000080FF)
        }
      }
      if (group_7bb == 0x84) {              //BatterySerialNumber
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (10 16 61 84 32 33 30 55)
          BatterySerialNumber[0] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame (21 4B 31 31 39 32 45 30)
          BatterySerialNumber[1] = rx_frame.data.u8[1];
          BatterySerialNumber[2] = rx_frame.data.u8[2];
          BatterySerialNumber[3] = rx_frame.data.u8[3];
          BatterySerialNumber[4] = rx_frame.data.u8[4];
          BatterySerialNumber[5] = rx_frame.data.u8[5];
          BatterySerialNumber[6] = rx_frame.data.u8[6];
          BatterySerialNumber[7] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third frame (22 30 31 34 38 32 20 A0)
          BatterySerialNumber[8] = rx_frame.data.u8[1];
          BatterySerialNumber[9] = rx_frame.data.u8[2];
          BatterySerialNumber[10] = rx_frame.data.u8[3];
          BatterySerialNumber[11] = rx_frame.data.u8[4];
          BatterySerialNumber[12] = rx_frame.data.u8[5];
          BatterySerialNumber[13] = rx_frame.data.u8[6];
          BatterySerialNumber[14] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x23) {  //Fourth frame (23 00 00 00 00 00 00 00)
        }
      }

      if (group_7bb == 0x90) {              //BMSIDcode
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (100A619044434131)
          BMSIDcode[0] = rx_frame.data.u8[4];
          BMSIDcode[1] = rx_frame.data.u8[5];
          BMSIDcode[2] = rx_frame.data.u8[6];
          BMSIDcode[3] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame (2130303535FFFFFF)
          BMSIDcode[4] = rx_frame.data.u8[1];
          BMSIDcode[5] = rx_frame.data.u8[2];
          BMSIDcode[6] = rx_frame.data.u8[3];
          BMSIDcode[7] = rx_frame.data.u8[4];
        }
      }

      break;
    default:
      break;
  }
}
void transmit_can_battery() {

  unsigned long currentMillis = millis();

  if (datalayer.system.status.BMS_reset_in_progress) {
    // Transmitting towards battery is halted while BMS is being reset
    // Reset sending counters to avoid overrun messages when reset is over
    previousMillis10 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis10s = currentMillis;
    return;
  }

  if (battery_can_alive) {

    //Send 10ms message
    if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
      // Check if sending of CAN messages has been delayed too much.
      if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
        set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
      } else {
        clear_event(EVENT_CAN_OVERRUN);
      }
      previousMillis10 = currentMillis;

      switch (mprun10) {
        case 0:
          LEAF_1D4.data.u8[4] = 0x07;
          LEAF_1D4.data.u8[7] = 0x12;
          break;
        case 1:
          LEAF_1D4.data.u8[4] = 0x47;
          LEAF_1D4.data.u8[7] = 0xD5;
          break;
        case 2:
          LEAF_1D4.data.u8[4] = 0x87;
          LEAF_1D4.data.u8[7] = 0x19;
          break;
        case 3:
          LEAF_1D4.data.u8[4] = 0xC7;
          LEAF_1D4.data.u8[7] = 0xDE;
          break;
      }
      transmit_can_frame(&LEAF_1D4, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&LEAF_1D4, can_config.battery_double);
#endif  // DOUBLE_BATTERY

      switch (mprun10r) {
        case (0):
          LEAF_1F2.data.u8[3] = 0xB0;
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (1):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x80;
          break;
        case (2):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (3):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x82;
          break;
        case (4):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (5):  // Set 2
          LEAF_1F2.data.u8[3] = 0xB4;
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (6):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x85;
          break;
        case (7):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        case (8):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x83;
          break;
        case (9):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (10):  // Set 3
          LEAF_1F2.data.u8[3] = 0xB0;
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (11):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x82;
          break;
        case (12):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (13):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x80;
          break;
        case (14):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (15):  // Set 4
          LEAF_1F2.data.u8[3] = 0xB4;
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        case (16):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x83;
          break;
        case (17):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (18):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x85;
          break;
        case (19):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        default:
          break;
      }

//Only send this message when NISSANLEAF_CHARGER is not defined (otherwise it will collide!)
#ifndef NISSANLEAF_CHARGER
      transmit_can_frame(&LEAF_1F2, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&LEAF_1F2, can_config.battery_double);
#endif  // DOUBLE_BATTERY
#endif

      mprun10r = (mprun10r + 1) % 20;  // 0x1F2 patter repeats after 20 messages. 0-1..19-0

      mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    }

    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      if (stateMachineClearSOH < 255) {  // Enter the ClearSOH statemachine only if we request it
        clearSOH();
      }

      //When battery requests heating pack status change, ack this
      if (battery_Batt_Heater_Mail_Send_Request) {
        LEAF_50B.data.u8[6] = 0x20;  //Batt_Heater_Mail_Send_OK
      } else {
        LEAF_50B.data.u8[6] = 0x00;  //Batt_Heater_Mail_Send_NG
      }

      // VCM message, containing info if battery should sleep or stay awake
      transmit_can_frame(&LEAF_50B, can_config.battery);  // HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&LEAF_50B, can_config.battery_double);
#endif  // DOUBLE_BATTERY

      LEAF_50C.data.u8[3] = mprun100;
      switch (mprun100) {
        case 0:
          LEAF_50C.data.u8[4] = 0x5D;
          LEAF_50C.data.u8[5] = 0xC8;
          break;
        case 1:
          LEAF_50C.data.u8[4] = 0xB2;
          LEAF_50C.data.u8[5] = 0x31;
          break;
        case 2:
          LEAF_50C.data.u8[4] = 0x5D;
          LEAF_50C.data.u8[5] = 0x63;
          break;
        case 3:
          LEAF_50C.data.u8[4] = 0xB2;
          LEAF_50C.data.u8[5] = 0x9A;
          break;
      }
      transmit_can_frame(&LEAF_50C, can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&LEAF_50C, can_config.battery_double);
#endif  // DOUBLE_BATTERY

      mprun100 = (mprun100 + 1) % 4;  // mprun100 cycles between 0-1-2-3-0-1...
    }

    //Send 10s CAN messages
    if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
      previousMillis10s = currentMillis;

      //Every 10s, ask diagnostic data from the battery. Don't ask if someone is already polling on the bus (Leafspy?)
      if (!stop_battery_query) {

        // Move to the next group
        PIDindex = (PIDindex + 1) % 6;  // 6 = amount of elements in the PIDgroups[]
        LEAF_GROUP_REQUEST.data.u8[2] = PIDgroups[PIDindex];

        transmit_can_frame(&LEAF_GROUP_REQUEST, can_config.battery);
#ifdef DOUBLE_BATTERY
        transmit_can_frame(&LEAF_GROUP_REQUEST, can_config.battery_double);
#endif  // DOUBLE_BATTERY
      }

      if (hold_off_with_polling_10seconds > 0) {
        hold_off_with_polling_10seconds--;
      } else {
        stop_battery_query = false;
      }
    }
  }
}

bool is_message_corrupt(CAN_frame rx_frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc != rx_frame.data.u8[7];
}

uint16_t Temp_fromRAW_to_F(uint16_t temperature) {  //This function feels horrible, but apparently works well
  if (temperature == 1021) {
    return 10;
  } else if (temperature >= 589) {
    return static_cast<uint16_t>(1620 - temperature * 1.81);
  } else if (temperature >= 569) {
    return static_cast<uint16_t>(572 + (579 - temperature) * 1.80);
  } else if (temperature >= 558) {
    return static_cast<uint16_t>(608 + (558 - temperature) * 1.6363636363636364);
  } else if (temperature >= 548) {
    return static_cast<uint16_t>(626 + (548 - temperature) * 1.80);
  } else if (temperature >= 537) {
    return static_cast<uint16_t>(644 + (537 - temperature) * 1.6363636363636364);
  } else if (temperature >= 447) {
    return static_cast<uint16_t>(662 + (527 - temperature) * 1.8);
  } else if (temperature >= 438) {
    return static_cast<uint16_t>(824 + (438 - temperature) * 2);
  } else if (temperature >= 428) {
    return static_cast<uint16_t>(842 + (428 - temperature) * 1.80);
  } else if (temperature >= 365) {
    return static_cast<uint16_t>(860 + (419 - temperature) * 2.0);
  } else if (temperature >= 357) {
    return static_cast<uint16_t>(986 + (357 - temperature) * 2.25);
  } else if (temperature >= 348) {
    return static_cast<uint16_t>(1004 + (348 - temperature) * 2);
  } else if (temperature >= 316) {
    return static_cast<uint16_t>(1022 + (340 - temperature) * 2.25);
  }
  return static_cast<uint16_t>(1094 + (309 - temperature) * 2.5714285714285715);
}

void clearSOH(void) {
  stop_battery_query = true;
  hold_off_with_polling_10seconds = 10;  // Active battery polling is paused for 100 seconds

  switch (stateMachineClearSOH) {
    case 0:  // Wait until polling actually stops
      challengeFailed = false;
      stateMachineClearSOH = 1;
      break;
    case 1:  // Set CAN_PROCESS_FLAG to 0xC0
      LEAF_CLEAR_SOH.data = {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // BMS should reply 02 50 C0 FF FF FF FF FF
      stateMachineClearSOH = 2;
      break;
    case 2:  // Set something ?
      LEAF_CLEAR_SOH.data = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // BMS should reply 7E FF FF FF FF FF FF
      stateMachineClearSOH = 3;
      break;
    case 3:  // Request challenge to solve
      LEAF_CLEAR_SOH.data = {0x02, 0x27, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // BMS should reply with (challenge) 06 67 65 (02 DD 86 43) FF
      stateMachineClearSOH = 4;
      break;
    case 4:  // Send back decoded challenge data
      decodeChallengeData(incomingChallenge, solvedChallenge);
      LEAF_CLEAR_SOH.data = {
          0x10, 0x0A, 0x27, 0x66, solvedChallenge[0], solvedChallenge[1], solvedChallenge[2], solvedChallenge[3]};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // BMS should reply 7BB 8 30 01 00 FF FF FF FF FF // Proceed with more data (PID ACK)
      stateMachineClearSOH = 5;
      break;
    case 5:  // Reply with even more decoded challenge data
      LEAF_CLEAR_SOH.data = {
          0x21, solvedChallenge[4], solvedChallenge[5], solvedChallenge[6], solvedChallenge[7], 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // BMS should reply 02 67 66 FF FF FF FF FF // Thank you for the data
      stateMachineClearSOH = 6;
      break;
    case 6:  // Check if solved data was OK
      LEAF_CLEAR_SOH.data = {0x03, 0x31, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      //7BB 8 03 71 03 01 FF FF FF FF // If all is well, BMS replies with 03 71 03 01.
      //Incase you sent wrong challenge, you get 03 7f 31 12
      stateMachineClearSOH = 7;
      break;
    case 7:  // Reset SOH% request
      LEAF_CLEAR_SOH.data = {0x03, 0x31, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      //7BB 8 03 71 03 02 FF FF FF FF // 03 71 03 02 means that BMS accepted command.
      //7BB 03 7f 31 12 means your challenge was wrong, so command ignored
      stateMachineClearSOH = 8;
      break;
    case 8:  // Please proceed with resetting SOH
      LEAF_CLEAR_SOH.data = {0x02, 0x10, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH, can_config.battery);
      // 7BB 8 02 50 81 FF FF FF FF FF // SOH reset OK
      stateMachineClearSOH = 255;
      break;
    default:
      break;
  }
}

unsigned int CyclicXorHash16Bit(unsigned int param_1, unsigned int param_2) {
  bool bVar1;
  unsigned int uVar2, uVar3, uVar4, uVar5, uVar6, uVar7, uVar8, uVar9, uVar10, uVar11, iVar12;

  param_1 = param_1 & 0xffff;
  param_2 = param_2 & 0xffff;
  uVar10 = 0xffff;
  iVar12 = 2;
  do {
    uVar2 = param_2;
    if ((param_1 & 1) == 1) {
      uVar2 = param_1 >> 1;
    }
    uVar3 = param_2;
    if ((param_1 >> 1 & 1) == 1) {
      uVar3 = param_1 >> 2;
    }
    uVar4 = param_2;
    if ((param_1 >> 2 & 1) == 1) {
      uVar4 = param_1 >> 3;
    }
    uVar5 = param_2;
    if ((param_1 >> 3 & 1) == 1) {
      uVar5 = param_1 >> 4;
    }
    uVar6 = param_2;
    if ((param_1 >> 4 & 1) == 1) {
      uVar6 = param_1 >> 5;
    }
    uVar7 = param_2;
    if ((param_1 >> 5 & 1) == 1) {
      uVar7 = param_1 >> 6;
    }
    uVar11 = param_1 >> 7;
    uVar8 = param_2;
    if ((param_1 >> 6 & 1) == 1) {
      uVar8 = uVar11;
    }
    param_1 = param_1 >> 8;
    uVar9 = param_2;
    if ((uVar11 & 1) == 1) {
      uVar9 = param_1;
    }
    uVar10 =
        (((((((((((((((uVar10 & 0x7fff) << 1 ^ uVar2) & 0x7fff) << 1 ^ uVar3) & 0x7fff) << 1 ^ uVar4) & 0x7fff) << 1 ^
                uVar5) &
               0x7fff)
                  << 1 ^
              uVar6) &
             0x7fff)
                << 1 ^
            uVar7) &
           0x7fff)
              << 1 ^
          uVar8) &
         0x7fff)
            << 1 ^
        uVar9;
    bVar1 = iVar12 != 1;
    iVar12 = iVar12 + -1;
  } while (bVar1);
  return uVar10;
}
unsigned int ComputeMaskedXorProduct(unsigned int param_1, unsigned int param_2, unsigned int param_3) {
  return (param_3 ^ 0x7F88 | param_2 ^ 0x8FE7) * ((param_1 & 0xffff) >> 8 ^ param_1 & 0xff) & 0xffff;
}

short ShortMaskedSumAndProduct(short param_1, short param_2) {
  unsigned short uVar1;

  uVar1 = param_2 + param_1 * 0x0006 & 0xff;
  return (uVar1 + param_1) * (uVar1 + param_2);
}

unsigned int MaskedBitwiseRotateMultiply(unsigned int param_1, unsigned int param_2) {
  unsigned int uVar1;

  param_1 = param_1 & 0xffff;
  param_2 = param_2 & 0xffff;
  uVar1 = param_2 & (param_1 | 0x0006) & 0xf;
  return ((unsigned int)param_1 >> uVar1 | param_1 << (0x10 - uVar1 & 0x1f)) *
             (param_2 << uVar1 | (unsigned int)param_2 >> (0x10 - uVar1 & 0x1f)) &
         0xffff;
}

unsigned int CryptAlgo(unsigned int param_1, unsigned int param_2, unsigned int param_3) {
  unsigned int uVar1, uVar2, iVar3, iVar4;

  uVar1 = MaskedBitwiseRotateMultiply(param_2, param_3);
  uVar2 = ShortMaskedSumAndProduct(param_2, param_3);
  uVar1 = ComputeMaskedXorProduct(param_1, uVar1, uVar2);
  uVar2 = ComputeMaskedXorProduct(param_1, uVar2, uVar1);
  iVar3 = CyclicXorHash16Bit(uVar1, 0x8421);
  iVar4 = CyclicXorHash16Bit(uVar2, 0x8421);
  return iVar4 + iVar3 * 0x10000;
}

void decodeChallengeData(unsigned int incomingChallenge, unsigned char* solvedChallenge) {
  unsigned int uVar1, uVar2;

  uVar1 = CryptAlgo(0x54e9, 0x3afd, incomingChallenge >> 0x10);
  uVar2 = CryptAlgo(incomingChallenge & 0xffff, incomingChallenge >> 0x10, 0x54e9);
  *solvedChallenge = (unsigned char)uVar1;
  solvedChallenge[1] = (unsigned char)uVar2;
  solvedChallenge[2] = (unsigned char)((unsigned int)uVar2 >> 8);
  solvedChallenge[3] = (unsigned char)((unsigned int)uVar1 >> 8);
  solvedChallenge[4] = (unsigned char)((unsigned int)uVar2 >> 0x10);
  solvedChallenge[5] = (unsigned char)((unsigned int)uVar1 >> 0x10);
  solvedChallenge[6] = (unsigned char)((unsigned int)uVar2 >> 0x18);
  solvedChallenge[7] = (unsigned char)((unsigned int)uVar1 >> 0x18);
  return;
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Nissan LEAF battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.number_of_cells = datalayer.battery.info.number_of_cells;
  datalayer.battery2.info.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  datalayer.battery2.info.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  datalayer.battery2.info.max_cell_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  datalayer.battery2.info.min_cell_voltage_mV = datalayer.battery.info.min_cell_voltage_mV;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
#endif  //DOUBLE_BATTERY
}

#endif  //NISSAN_LEAF_BATTERY
