#include "../include.h"
#ifdef NISSAN_LEAF_BATTERY
#include "NISSAN-LEAF-BATTERY.h"
#ifdef MQTT
#include "../devboard/mqtt/mqtt.h"
#endif
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send
static uint16_t CANerror = 0;                //counter on how many CAN errors encountered
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive
static uint8_t mprun10r = 0;                 //counter 0-20 for 0x1F2 message
static uint8_t mprun10 = 0;                  //counter 0-3
static uint8_t mprun100 = 0;                 //counter 0-3

CAN_frame_t LEAF_1F2 = {.FIR = {.B =
                                    {
                                        .DLC = 8,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x1F2,
                        .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t LEAF_50B = {.FIR = {.B =
                                    {
                                        .DLC = 7,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x50B,
                        .data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
CAN_frame_t LEAF_50C = {.FIR = {.B =
                                    {
                                        .DLC = 6,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x50C,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t LEAF_1D4 = {.FIR = {.B =
                                    {
                                        .DLC = 8,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x1D4,
                        .data = {0x6E, 0x6E, 0x00, 0x04, 0x07, 0x46, 0xE0, 0x44}};
//These CAN messages need to be sent towards the battery to keep it alive

CAN_frame_t LEAF_GROUP_REQUEST = {.FIR = {.B =
                                              {
                                                  .DLC = 8,
                                                  .FF = CAN_frame_std,
                                              }},
                                  .MsgID = 0x79B,
                                  .data = {2, 0x21, 1, 0, 0, 0, 0, 0}};
const CAN_frame_t LEAF_NEXT_LINE_REQUEST = {.FIR = {.B =
                                                        {
                                                            .DLC = 8,
                                                            .FF = CAN_frame_std,
                                                        }},
                                            .MsgID = 0x79B,
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
static uint8_t LEAF_Battery_Type = ZE0_BATTERY;
#define MAX_CELL_VOLTAGE 4250                  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2700                  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION 500                 //LED turns yellow on the board if mv delta exceeds this value
#define WH_PER_GID 77                          //One GID is this amount of Watt hours
static uint16_t LB_Discharge_Power_Limit = 0;  //Limit in kW
static uint16_t LB_Charge_Power_Limit = 0;     //Limit in kW
static int16_t LB_MAX_POWER_FOR_CHARGER = 0;   //Limit in kW
static int16_t LB_SOC = 500;                   //0 - 100.0 % (0-1000) The real SOC% in the battery
static uint16_t LB_TEMP = 0;                   //Temporary value used in status checks
static uint16_t LB_Wh_Remaining = 0;           //Amount of energy in battery, in Wh
static uint16_t LB_GIDS = 273;                 //Startup in 24kWh mode
static uint16_t LB_MAX = 0;
static uint16_t LB_Max_GIDS = 273;               //Startup in 24kWh mode
static uint16_t LB_StateOfHealth = 99;           //State of health %
static uint16_t LB_Total_Voltage2 = 740;         //Battery voltage (0-450V) [0.5V/bit, so actual range 0-800]
static int16_t LB_Current2 = 0;                  //Battery current (-400-200A) [0.5A/bit, so actual range -800-400]
static int16_t LB_Power = 0;                     //Watts going in/out of battery
static int16_t LB_HistData_Temperature_MAX = 6;  //-40 to 86*C
static int16_t LB_HistData_Temperature_MIN = 5;  //-40 to 86*C
static int16_t LB_AverageTemperature = 6;        //Only available on ZE0, in celcius, -40 to +55
static uint8_t LB_Relay_Cut_Request = 0;         //LB_FAIL
static uint8_t LB_Failsafe_Status = 0;           //LB_STATUS = 000b = normal start Request
                                                 //001b = Main Relay OFF Request
                                                 //010b = Charging Mode Stop Request
                                                 //011b =  Main Relay OFF Request
                                                 //100b = Caution Lamp Request
                                                 //101b = Caution Lamp Request & Main Relay OFF Request
                                                 //110b = Caution Lamp Request & Charging Mode Stop Request
                                                 //111b = Caution Lamp Request & Main Relay OFF Request
static bool LB_Interlock =
    true;  //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
static bool LB_Full_CHARGE_flag = false;  //LB_FCHGEND , Goes to 1 if battery is fully charged
static bool LB_MainRelayOn_flag = false;  //No-Permission=0, Main Relay On Permission=1
static bool LB_Capacity_Empty = false;    //LB_EMPTY, , Goes to 1 if battery is empty
static bool LB_HeatExist = false;         //LB_HEATEXIST, Specifies if battery pack is equipped with heating elements
static bool LB_Heating_Stop = false;      //When transitioning from 0->1, signals a STOP heat request
static bool LB_Heating_Start = false;     //When transitioning from 1->0, signals a START heat request
static bool Batt_Heater_Mail_Send_Request = false;  //Stores info when a heat request is happening

// Nissan LEAF battery data from polled CAN messages
static uint8_t battery_request_idx = 0;
static uint8_t group_7bb = 0;
static uint8_t group = 1;
static uint8_t stop_battery_query = 1;
static uint8_t hold_off_with_polling_10seconds = 10;
static uint16_t cell_voltages[97];  //array with all the cellvoltages
static uint8_t cellcounter = 0;
static uint16_t min_max_voltage[2];     //contains cell min[0] and max[1] values in mV
static uint16_t cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static uint16_t HX = 0;                 //Internal resistance
static uint16_t insulation = 0;         //Insulation resistance
static int32_t Battery_current_1 =
    0;  //High Voltage battery current; it’s positive if discharged, negative when charging
static int32_t Battery_current_2 =
    0;  //High Voltage battery current; it’s positive if discharged, negative when charging (unclear why two values exist)
static uint16_t temp_raw_1 = 0;
static uint8_t temp_raw_2_highnibble = 0;
static uint16_t temp_raw_2 = 0;
static uint16_t temp_raw_3 = 0;
static uint16_t temp_raw_4 = 0;
static uint16_t temp_raw_max = 0;
static uint16_t temp_raw_min = 0;
static int16_t temp_polled_max = 0;
static int16_t temp_polled_min = 0;

void print_with_units(char* header, int value, char* units) {
  Serial.print(header);
  Serial.print(value);
  Serial.print(units);
}

void update_values_battery() { /* This function maps all the values fetched via CAN to the correct parameters used for modbus */
  /* Start with mapping all values */

  datalayer.battery.status.soh_pptt = (LB_StateOfHealth * 100);  //Increase range from 99% -> 99.00%

  system_real_SOC_pptt = (LB_SOC * 10);

  datalayer.battery.status.voltage_dV =
      (LB_Total_Voltage2 * 5);  //0.5V/bit, multiply by 5 to get Voltage+1decimal (350.5V = 701)

  datalayer.battery.status.current_dA = (LB_Current2 * 5);  //0.5A/bit, multiply by 5 to get Amp+1decimal (5,5A = 11)

  datalayer.battery.info.total_capacity_Wh = (LB_Max_GIDS * WH_PER_GID);

  datalayer.battery.status.remaining_capacity_Wh = LB_Wh_Remaining;

  LB_Power =
      ((LB_Total_Voltage2 * LB_Current2) / 4);  //P = U * I (Both values are 0.5 per bit so the math is non-intuitive)

  datalayer.battery.status.active_power_W = LB_Power;

  //Update temperature readings. Method depends on which generation LEAF battery is used
  if (LEAF_Battery_Type == ZE0_BATTERY) {
    //Since we only have average value, send the minimum as -1.0 degrees below average
    datalayer.battery.status.temperature_min_dC =
        ((LB_AverageTemperature * 10) - 10);  //Increase range from C to C+1, remove 1.0C
    datalayer.battery.status.temperature_max_dC = (LB_AverageTemperature * 10);  //Increase range from C to C+1
  } else if (LEAF_Battery_Type == AZE0_BATTERY) {
    //Use the value sent constantly via CAN in 5C0 (only available on AZE0)
    datalayer.battery.status.temperature_min_dC = (LB_HistData_Temperature_MIN * 10);  //Increase range from C to C+1
    datalayer.battery.status.temperature_max_dC = (LB_HistData_Temperature_MAX * 10);  //Increase range from C to C+1
  } else {  // ZE1 (TODO: Once the muxed value in 5C0 becomes known, switch to using that instead of this complicated polled value)
    if (temp_raw_min != 0)  //We have a polled value available
    {
      temp_polled_min = ((Temp_fromRAW_to_F(temp_raw_min) - 320) * 5) / 9;  //Convert from F to C
      temp_polled_max = ((Temp_fromRAW_to_F(temp_raw_max) - 320) * 5) / 9;  //Convert from F to C
      if (temp_polled_min < temp_polled_max) {  //Catch any edge cases from Temp_fromRAW_to_F function
        datalayer.battery.status.temperature_min_dC = temp_polled_min;
        datalayer.battery.status.temperature_max_dC = temp_polled_max;
      } else {
        datalayer.battery.status.temperature_min_dC = temp_polled_max;
        datalayer.battery.status.temperature_max_dC = temp_polled_min;
      }
    }
  }

  // Define power able to be discharged from battery
  if (LB_Discharge_Power_Limit > 60) {     //if >60kW can be pulled from battery
    system_max_discharge_power_W = 60000;  //cap value so we don't go over uint16 value
  } else {
    system_max_discharge_power_W = (LB_Discharge_Power_Limit * 1000);  //kW to W
  }
  if (system_scaled_SOC_pptt == 0) {  //Scaled SOC% value is 0.00%, we should not discharge battery further
    system_max_discharge_power_W = 0;
  }

  // Define power able to be put into the battery
  if (LB_Charge_Power_Limit > 60) {     //if >60kW can be put into the battery
    system_max_charge_power_W = 60000;  //cap value so we don't go over uint16 value
  } else {
    system_max_charge_power_W = (LB_Charge_Power_Limit * 1000);  //kW to W
  }
  if (system_scaled_SOC_pptt == 10000)  //Scaled SOC% value is 100.00%
  {
    system_max_charge_power_W = 0;  //No need to charge further, set max power to 0
  }

  //Map all cell voltages to the global array
  for (int i = 0; i < 96; ++i) {
    system_cellvoltages_mV[i] = cell_voltages[i];
  }

  /*Extra safety functions below*/
  if (LB_GIDS < 10)  //700Wh left in battery!
  {                  //Battery is running abnormally low, some discharge logic might have failed. Zero it all out.
    set_event(EVENT_BATTERY_EMPTY, 0);
    system_real_SOC_pptt = 0;
    system_max_discharge_power_W = 0;
  }

  //Check if SOC% is plausible
  if (datalayer.battery.status.voltage_dV >
      (datalayer.battery.info.max_design_voltage_dV -
       100)) {  // When pack voltage is close to max, and SOC% is still low, raise FAULT
    if (LB_SOC < 650) {
      set_event(EVENT_SOC_PLAUSIBILITY_ERROR, LB_SOC / 10);  // Set event with the SOC as data
    } else {
      clear_event(EVENT_SOC_PLAUSIBILITY_ERROR);
    }
  }

  if (LB_Full_CHARGE_flag) {  //Battery reports that it is fully charged stop all further charging incase it hasn't already
    set_event(EVENT_BATTERY_FULL, 0);
    system_max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL);
  }

  if (LB_Capacity_Empty) {  //Battery reports that it is fully discharged. Stop all further discharging incase it hasn't already
    set_event(EVENT_BATTERY_EMPTY, 0);
    system_max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY);
  }

  if (LB_Relay_Cut_Request) {  //LB_FAIL, BMS requesting shutdown and contactors to be opened
#ifdef DEBUG_VIA_USB
    Serial.println("Battery requesting immediate shutdown and contactors to be opened!");
#endif
    //Note, this is sometimes triggered during the night while idle, and the BMS recovers after a while. Removed latching from this scenario
    system_max_discharge_power_W = 0;
    system_max_charge_power_W = 0;
  }

  if (LB_Failsafe_Status > 0)  // 0 is normal, start charging/discharging
  {
    switch (LB_Failsafe_Status) {
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
#ifdef DEBUG_VIA_USB
        Serial.println("ERROR: Battery raised caution indicator. Inspect battery status!");
#endif
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
  } else {  //LB_Failsafe_Status == 0
    clear_event(EVENT_BATTERY_DISCHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_STOP_REQ);
    clear_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ);
  }

  if (LB_StateOfHealth < 25) {    //Battery is extremely degraded, not fit for secondlifestorage. Zero it all out.
    if (LB_StateOfHealth != 0) {  //Extra check to see that we actually have a SOH Value available
      set_event(EVENT_LOW_SOH, LB_StateOfHealth);
    } else {
      clear_event(EVENT_LOW_SOH);
    }
  }

#ifdef INTERLOCK_REQUIRED
  if (!LB_Interlock) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
#endif

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }
  if (CANerror >
      MAX_CAN_FAILURES)  //Also check if we have recieved too many malformed CAN messages. If so, signal via LED
  {
    set_event(EVENT_CAN_RX_WARNING, 0);
  }

  if (system_bms_status == FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    system_max_charge_power_W = 0;
    system_max_discharge_power_W = 0;
  }

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_VIA_USB
  Serial.println("Values going to inverter");
  print_with_units("SOH%: ", (datalayer.battery.status.soh_pptt * 0.01), "% ");
  print_with_units(", SOC% scaled: ", (system_scaled_SOC_pptt * 0.01), "% ");
  print_with_units(", Voltage: ", (datalayer.battery.status.voltage_dV * 0.1), "V ");
  print_with_units(", Max discharge power: ", system_max_discharge_power_W, "W ");
  print_with_units(", Max charge power: ", system_max_charge_power_W, "W ");
  print_with_units(", Max temp: ", (datalayer.battery.status.temperature_max_dC * 0.1), "°C ");
  print_with_units(", Min temp: ", (datalayer.battery.status.temperature_min_dC * 0.1), "°C ");
  Serial.println("");
  Serial.print("BMS Status: ");
  if (system_bms_status == 3) {
    Serial.print("Active, ");
  } else {
    Serial.print("FAULT, ");
  }
  print_with_units(", Power: ", LB_Power, "W ");
  Serial.println("");
  Serial.println("Values from battery");
  print_with_units("Real SOC%: ", (LB_SOC * 0.1), "% ");
  print_with_units(", GIDS: ", LB_GIDS, " (x77Wh) ");
  print_with_units(", Battery gen: ", LEAF_Battery_Type, " ");
  print_with_units(", Has heater: ", LB_HeatExist, " ");
  print_with_units(", Max cell voltage: ", min_max_voltage[1], "mV ");
  print_with_units(", Min cell voltage: ", min_max_voltage[0], "mV ");
  print_with_units(", Cell deviation: ", cell_deviation_mV, "mV ");
  if (LB_Heating_Stop) {
    Serial.println("Battery requesting heating pads to stop. The battery is now warm enough.");
  }
  if (LB_Heating_Start) {
    Serial.println("COLD BATTERY! Battery requesting heating pads to activate");
  }

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0x1DB:
      if (is_message_corrupt(rx_frame)) {
        CANerror++;
        break;  //Message content malformed, abort reading data from it
      }
      LB_Current2 = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
      if (LB_Current2 & 0x0400) {
        // negative so extend the sign bit
        LB_Current2 |= 0xf800;
      }  //BatteryCurrentSignal , 2s comp, 1lSB = 0.5A/bit

      LB_Total_Voltage2 = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6);  //0.5V/bit

      //Collect various data from the BMS
      LB_Relay_Cut_Request = ((rx_frame.data.u8[1] & 0x18) >> 3);
      LB_Failsafe_Status = (rx_frame.data.u8[1] & 0x07);
      LB_MainRelayOn_flag = (bool)((rx_frame.data.u8[3] & 0x20) >> 5);
      if (LB_MainRelayOn_flag) {
        batteryAllowsContactorClosing = true;
      } else {
        batteryAllowsContactorClosing = false;
      }
      LB_Full_CHARGE_flag = (bool)((rx_frame.data.u8[3] & 0x10) >> 4);
      LB_Interlock = (bool)((rx_frame.data.u8[3] & 0x08) >> 3);
      break;
    case 0x1DC:
      if (is_message_corrupt(rx_frame)) {
        CANerror++;
        break;  //Message content malformed, abort reading data from it
      }
      LB_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
      LB_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 4 | rx_frame.data.u8[2] >> 4) / 4.0);
      LB_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
      break;
    case 0x55B:
      if (is_message_corrupt(rx_frame)) {
        CANerror++;
        break;  //Message content malformed, abort reading data from it
      }
      LB_TEMP = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
      if (LB_TEMP != 0x3ff) {  //3FF is unavailable value
        LB_SOC = LB_TEMP;
      }
      LB_Capacity_Empty = (bool)((rx_frame.data.u8[6] & 0x80) >> 7);
      break;
    case 0x5BC:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS

      LB_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
      if (LB_MAX) {
        LB_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        //Max gids active, do nothing
        //Only the 30/40/62kWh packs have this mux
      } else {  //Normal current GIDS value is transmitted
        LB_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        LB_Wh_Remaining = (LB_GIDS * WH_PER_GID);
      }

      if (LEAF_Battery_Type == ZE0_BATTERY) {
        LB_AverageTemperature = (rx_frame.data.u8[3] - 40);  //In celcius, -40 to +55
      }

      LB_TEMP = (rx_frame.data.u8[4] >> 1);
      if (LB_TEMP != 0) {
        LB_StateOfHealth = LB_TEMP;  //Collect state of health from battery
      }
      break;
    case 0x5C0:
      //This temperature only works for 2013-2017 AZE0 LEAF packs, the mux is different on other generations
      if (LEAF_Battery_Type == AZE0_BATTERY) {
        if ((rx_frame.data.u8[0] >> 6) ==
            1) {  // Battery MAX temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          LB_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
        }
        if ((rx_frame.data.u8[0] >> 6) ==
            3) {  // Battery MIN temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          LB_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
        }
      }

      LB_HeatExist = (rx_frame.data.u8[4] & 0x01);
      LB_Heating_Stop = ((rx_frame.data.u8[0] & 0x10) >> 4);
      LB_Heating_Start = ((rx_frame.data.u8[0] & 0x20) >> 5);
      Batt_Heater_Mail_Send_Request = (rx_frame.data.u8[1] & 0x01);

      break;
    case 0x59E:
      //AZE0 2013-2017 or ZE1 2018-2023 battery detected
      //Only detect as AZE0 if not already set as ZE1
      if (LEAF_Battery_Type != ZE1_BATTERY) {
        LEAF_Battery_Type = AZE0_BATTERY;
      }
      break;
    case 0x1ED:
    case 0x1C2:
      //ZE1 2018-2023 battery detected!
      LEAF_Battery_Type = ZE1_BATTERY;
      break;
    case 0x79B:
      stop_battery_query = 1;                //Someone is trying to read data with Leafspy, stop our own polling!
      hold_off_with_polling_10seconds = 10;  //Polling is paused for 100s
      break;
    case 0x7BB:
      //First check which group data we are getting
      if (rx_frame.data.u8[0] == 0x10) {  //First message of a group
        group_7bb = rx_frame.data.u8[3];
        if (group_7bb != 1 && group_7bb != 2 && group_7bb != 4) {  //We are only interested in groups 1,2 and 4
          break;
        }
      }

      if (stop_battery_query) {  //Leafspy is active, stop our own polling
        break;
      }

      ESP32Can.CANWriteFrame(&LEAF_NEXT_LINE_REQUEST);  //Request the next frame for the group

      if (group_7bb == 1)  //High precision SOC, Current, voltages etc.
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame
          Battery_current_1 = (rx_frame.data.u8[4] << 24) |
                              (rx_frame.data.u8[5] << 16 | ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]));
          if (Battery_current_1 & 0x8000000 == 0x8000000) {
            Battery_current_1 = ((Battery_current_1 | -0x100000000) / 1024);
          } else {
            Battery_current_1 = (Battery_current_1 / 1024);
          }
        }

        if (rx_frame.data.u8[0] == 0x21) {  //Second frame
          Battery_current_2 = (rx_frame.data.u8[3] << 24) |
                              (rx_frame.data.u8[4] << 16 | ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]));
          if (Battery_current_2 & 0x8000000 == 0x8000000) {
            Battery_current_2 = ((Battery_current_2 | -0x100000000) / 1024);
          } else {
            Battery_current_2 = (Battery_current_2 / 1024);
          }
        }

        if (rx_frame.data.u8[0] == 0x23) {  // Fourth frame
          insulation = (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        }

        if (rx_frame.data.u8[0] == 0x24) {  // Fifth frame
          HX = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 102.4;
        }
      }

      if (group_7bb == 2)  //Cell Voltages
      {
        if (rx_frame.data.u8[0] == 0x10) {  //first frame is anomalous
          battery_request_idx = 0;
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          break;
        }
        if (rx_frame.data.u8[6] == 0xFF && rx_frame.data.u8[0] == 0x2C) {  //Last frame
          //Last frame does not contain any cell data, calculate the result
          min_max_voltage[0] = 9999;
          min_max_voltage[1] = 0;
          for (cellcounter = 0; cellcounter < 96; cellcounter++) {
            if (min_max_voltage[0] > cell_voltages[cellcounter])
              min_max_voltage[0] = cell_voltages[cellcounter];
            if (min_max_voltage[1] < cell_voltages[cellcounter])
              min_max_voltage[1] = cell_voltages[cellcounter];
          }

          cell_deviation_mV = (min_max_voltage[1] - min_max_voltage[0]);

          system_cell_max_voltage_mV = min_max_voltage[1];
          system_cell_min_voltage_mV = min_max_voltage[0];

          if (cell_deviation_mV > MAX_CELL_DEVIATION) {
            set_event(EVENT_CELL_DEVIATION_HIGH, 0);
          }

          if (min_max_voltage[1] >= MAX_CELL_VOLTAGE) {
            set_event(EVENT_CELL_OVER_VOLTAGE, 0);
          }
          if (min_max_voltage[0] <= MIN_CELL_VOLTAGE) {
            set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
          }
          break;
        }

        if ((rx_frame.data.u8[0] % 2) == 0) {  //even frames
          cell_voltages[battery_request_idx++] |= rx_frame.data.u8[1];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        } else {  //odd frames
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        }
      }

      if (group_7bb == 4) {                 //Temperatures
        if (rx_frame.data.u8[0] == 0x10) {  //First message
          temp_raw_1 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          temp_raw_2_highnibble = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second message
          temp_raw_2 = (temp_raw_2_highnibble << 8) | rx_frame.data.u8[1];
          temp_raw_3 = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          temp_raw_4 = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third message
          //All values read, let's figure out the min/max!

          if (temp_raw_3 == 65535) {  //We are on a 2013+ pack that only has three temp sensors.
            //Start with finding max value
            temp_raw_max = temp_raw_1;
            if (temp_raw_2 > temp_raw_max) {
              temp_raw_max = temp_raw_2;
            }
            if (temp_raw_4 > temp_raw_max) {
              temp_raw_max = temp_raw_4;
            }
            //Then find min
            temp_raw_min = temp_raw_1;
            if (temp_raw_2 < temp_raw_min) {
              temp_raw_min = temp_raw_2;
            }
            if (temp_raw_4 < temp_raw_min) {
              temp_raw_min = temp_raw_4;
            }
          } else {  //All 4 temp sensors available on 2011-2012
            //Start with finding max value
            temp_raw_max = temp_raw_1;
            if (temp_raw_2 > temp_raw_max) {
              temp_raw_max = temp_raw_2;
            }
            if (temp_raw_3 > temp_raw_max) {
              temp_raw_max = temp_raw_3;
            }
            if (temp_raw_4 > temp_raw_max) {
              temp_raw_max = temp_raw_4;
            }
            //Then find min
            temp_raw_min = temp_raw_1;
            if (temp_raw_2 < temp_raw_min) {
              temp_raw_min = temp_raw_2;
            }
            if (temp_raw_3 < temp_raw_min) {
              temp_raw_min = temp_raw_2;
            }
            if (temp_raw_4 < temp_raw_min) {
              temp_raw_min = temp_raw_4;
            }
          }
        }
      }

      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
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
    ESP32Can.CANWriteFrame(&LEAF_1D4);

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
    ESP32Can.CANWriteFrame(&LEAF_1F2);  //Contains (CHG_STA_RQ == 1 == Normal Charge)
#endif

    mprun10r = (mprun10r + 1) % 20;  // 0x1F2 patter repeats after 20 messages. 0-1..19-0

    mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //When battery requests heating pack status change, ack this
    if (Batt_Heater_Mail_Send_Request) {
      LEAF_50B.data.u8[6] = 0x20;  //Batt_Heater_Mail_Send_OK
    } else {
      LEAF_50B.data.u8[6] = 0x00;  //Batt_Heater_Mail_Send_NG
    }

    // VCM message, containing info if battery should sleep or stay awake
    ESP32Can.CANWriteFrame(&LEAF_50B);  // HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1

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
    ESP32Can.CANWriteFrame(&LEAF_50C);

    mprun100 = (mprun100 + 1) % 4;  // mprun100 cycles between 0-1-2-3-0-1...
  }

  //Send 10s CAN messages
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    //Every 10s, ask diagnostic data from the battery. Don't ask if someone is already polling on the bus (Leafspy?)
    if (!stop_battery_query) {
      group = (group == 1) ? 2 : (group == 2) ? 4 : 1;
      // Cycle between group 1, 2, and 4 using ternary operation
      LEAF_GROUP_REQUEST.data.u8[2] = group;
      ESP32Can.CANWriteFrame(&LEAF_GROUP_REQUEST);
    }

    if (hold_off_with_polling_10seconds > 0) {
      hold_off_with_polling_10seconds--;
    } else {
      stop_battery_query = 0;
    }
  }
}

bool is_message_corrupt(CAN_frame_t rx_frame) {
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

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Nissan LEAF battery selected");
#endif

  system_number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 2600;  // 260.0V under this, discharging further is disabled
}

#endif
