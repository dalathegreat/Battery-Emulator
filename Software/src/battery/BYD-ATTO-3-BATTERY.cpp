#include "../include.h"
#ifdef BYD_ATTO_3_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "BYD-ATTO-3-BATTERY.h"

/* TODO: 
- Map all values from battery CAN messages
  -SOC% still not found (Lets take it from PID poll, not working right yet)
  -SOC% is now ESTIMATED. This is bad, and should be fixed as soon as possible with the real value from CAN
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
static uint8_t counter_50ms = 0;
static uint8_t counter_100ms = 0;
static uint8_t frame6_counter = 0xB;
static uint8_t frame7_counter = 0x5;

static int16_t temperature_ambient = 0;
static int16_t daughterboard_temperatures[10];
static int16_t lowest_temperature = 0;
static int16_t highest_temperature = 0;
static int16_t calc_min_temperature = 0;
static int16_t calc_max_temperature = 0;

static uint16_t BMS_SOC = 0;
static uint16_t BMS_voltage = 0;
static int16_t BMS_current = 0;
static int16_t BMS_lowest_cell_temperature = 0;
static int16_t BMS_highest_cell_temperature = 0;
static int16_t BMS_average_cell_temperature = 0;
static uint16_t BMS_lowest_cell_voltage_mV = 3300;
static uint16_t BMS_highest_cell_voltage_mV = 3300;

#define POLL_FOR_BATTERY_SOC 0x05
#define POLL_FOR_BATTERY_VOLTAGE 0x08
#define POLL_FOR_BATTERY_CURRENT 0x09
#define POLL_FOR_LOWEST_TEMP_CELL 0x2f
#define POLL_FOR_HIGHEST_TEMP_CELL 0x31
#define POLL_FOR_BATTERY_PACK_AVG_TEMP 0x32
#define POLL_FOR_BATTERY_CELL_MV_MAX 0x2D
#define POLL_FOR_BATTERY_CELL_MV_MIN 0x2B
#define UNKNOWN_POLL_1 0xFC
static uint16_t poll_state = POLL_FOR_BATTERY_SOC;

CAN_frame ATTO_3_12D = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x12D,
                        .data = {0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71, 0xCF, 0x49}};
CAN_frame ATTO_3_441 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x441,
                        .data = {0x98, 0x3A, 0x88, 0x13, 0x07, 0x00, 0xFF, 0x8C}};
CAN_frame ATTO_3_7E7_POLL = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E7,  //Poll PID 03 22 00 05 (POLL_FOR_BATTERY_SOC)
                             .data = {0x03, 0x22, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}};

// Define the data points for %SOC depending on pack voltage
const uint8_t numPoints = 14;
const uint16_t SOC[numPoints] = {10000, 9970, 9490, 8470, 7750, 6790, 5500, 4900, 3910, 3000, 2280, 1600, 480, 0};
const uint16_t voltage[numPoints] = {4400, 4230, 4180, 4171, 4169, 4160, 4130,
                                     4121, 4119, 4100, 4070, 4030, 3950, 3800};

uint16_t estimateSOC(uint16_t packVoltage) {  // Linear interpolation function
  if (packVoltage >= voltage[0]) {
    return SOC[0];
  }
  if (packVoltage <= voltage[numPoints - 1]) {
    return SOC[numPoints - 1];
  }

  for (int i = 1; i < numPoints; ++i) {
    if (packVoltage >= voltage[i]) {
      double t = (packVoltage - voltage[i]) / (voltage[i - 1] - voltage[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  if (BMS_voltage > 0) {
    datalayer.battery.status.voltage_dV = BMS_voltage * 10;
  }

  //datalayer.battery.status.real_soc = BMS_SOC * 100;  //TODO: This is not yet found!
  // We instead estimate the SOC% based on the battery voltage
  // This is a very bad solution, and as soon as an usable SOC% value has been found on CAN, we should switch to that!
  datalayer.battery.status.real_soc = estimateSOC(datalayer.battery.status.voltage_dV);

  datalayer.battery.status.current_dA = -BMS_current;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = 10000;  //TODO: Map from CAN later on

  datalayer.battery.status.max_charge_power_W = 10000;  //TODO: Map from CAN later on

  datalayer.battery.status.active_power_W =
      (datalayer.battery.status.current_dA * (datalayer.battery.status.voltage_dV / 100));

  datalayer.battery.status.cell_max_voltage_mV = BMS_highest_cell_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV = BMS_lowest_cell_voltage_mV;

  // Initialize min and max variables
  calc_min_temperature = daughterboard_temperatures[0];
  calc_max_temperature = daughterboard_temperatures[0];

  // Loop through the array of daughterboard temps to find the smallest and largest values
  for (int i = 1; i < 10; i++) {
    if (daughterboard_temperatures[i] < calc_min_temperature) {
      calc_min_temperature = daughterboard_temperatures[i];
    }
    if (daughterboard_temperatures[i] > calc_max_temperature) {
      calc_max_temperature = daughterboard_temperatures[i];
    }
  }

  datalayer.battery.status.temperature_min_dC = calc_min_temperature * 10;  // Add decimals
  datalayer.battery.status.temperature_max_dC = calc_max_temperature * 10;

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {  //Log values taken with 422V from battery
    case 0x244:           //00,00,00,04,41,0F,20,8B - Static, values never changes between logs
      break;
    case 0x245:  //01,00,02,19,3A,25,90,F4 Seems to have a mux in frame0
                 //02,00,90,01,79,79,90,EA // Point of interest, went from 7E,75 to 7B,7C when discharging
                 //03,C6,88,12,FD,48,90,5C
                 //04,00,FF,FF,00,00,90,6D
      if (rx_frame.data.u8[0] == 0x01) {
        temperature_ambient = (rx_frame.data.u8[4] - 40);  // TODO, check if this is actually temperature_ambient
      }
      break;
    case 0x286:  //01,FF,FF,FF,FF,FF,FF,04 - Static, values never changes between logs
      break;
    case 0x334:  //FF,FF,FF,FC,3F,00,F0,D7 - Static, values never changes between logs
      break;
    case 0x338:  //01,52,02,00,88,13,00,0F
                 //01,51,02,00,88,13,00,10 407.5V 18deg
                 //01,4F,02,00,88,13,00,12 408.5V 14deg
      break;
    case 0x344:  //00,52,02,CC,1F,FF,04,BD
      break;
    case 0x345:  //27,0B,00,00,00,E0,01,EC - Static, values never changes between logs
      break;
    case 0x347:  //FF,00,00,F9,FF,FF,FF,0A - Static, values never changes between logs
      break;
    case 0x34A:  //00,52,02,CC,1F,FF,04,BD
                 //00,51,02,CC,1F,FF,04,BE //407.5V 18deg
                 //00,4F,02,CC,1F,FF,04,C0 //408.5V 14deg
      break;
    case 0x35E:  //01,00,C8,32,00,63,00,A1 - Flickering between A0 and A1, Could be temperature?
                 //01,00,64,01,10,63,00,26 //407.5V 18deg
                 //01,00,64,1C,10,63,00,0B //408.5V 14deg
      break;
    case 0x360:  //30,19,DE,D1,0B,C3,4B,EE - Static, values never changes between logs, Last and first byte has F-0 counters
      break;
    case 0x36C:  //01,57,13,DC,08,70,17,29 Seems to have a mux in frame0 , first message is static, never changes between logs
                 //02,03,DC,05,C0,0F,0F,3B - Static, values never changes between logs
                 //03,86,01,40,06,5C,02,D1 - Static, values never changes between logs
                 //04,57,13,73,04,01,FF,1A - Static, values never changes between logs
                 //05,FF,FF,FF,FF,FF,FF,00 - Static, values never changes between logs
      break;
    case 0x438:  //55,55,01,F6,47,2E,10,D9 - 0x10D9 = 4313
                 //55,55,01,F6,47,FD,0F,0B //407.5V 18deg
                 //55,55,01,F6,47,15,10,F2 //408.5V 14deg
      break;
    case 0x43A:  //7E,0A,B0,1C,63,E1,03,64
                 //7E,0A,E0,1E,63,E1,03,32 //407.5V 18deg
                 //7E,0A,66,1C,63,E1,03,AE //408.5V 14deg
      break;
    case 0x43B:  //01,3B,06,39,FF,64,64,BD
                 //01,3B,06,38,FF,64,64,BE
      break;
    case 0x43C:  // Daughterboard temperatures reside in this CAN message
      if (rx_frame.data.u8[0] == 0x00) {
        daughterboard_temperatures[0] = (rx_frame.data.u8[1] - 40);
        daughterboard_temperatures[1] = (rx_frame.data.u8[2] - 40);
        daughterboard_temperatures[2] = (rx_frame.data.u8[3] - 40);
        daughterboard_temperatures[3] = (rx_frame.data.u8[4] - 40);
        daughterboard_temperatures[4] = (rx_frame.data.u8[5] - 40);
        daughterboard_temperatures[5] = (rx_frame.data.u8[6] - 40);
      }
      if (rx_frame.data.u8[0] == 0x01) {
        daughterboard_temperatures[6] = (rx_frame.data.u8[1] - 40);
        daughterboard_temperatures[7] = (rx_frame.data.u8[2] - 40);
        daughterboard_temperatures[8] = (rx_frame.data.u8[3] - 40);
        daughterboard_temperatures[9] = (rx_frame.data.u8[4] - 40);
      }
      break;
    case 0x43D:  //Varies a lot
      break;
    case 0x444:  //9E,01,88,13,64,64,98,65
                 //9A,01,B6,13,64,64,98,3B //407.5V 18deg
                 //9B,01,B8,13,64,64,98,38 //408.5V 14deg
      break;
    case 0x445:  //00,98,FF,FF,63,20,4E,98 - Static, values never changes between logs
      break;
    case 0x446:  //2C,D4,0C,4D,21,DC,0C,9D - 0,1,7th frame varies a lot
      break;
    case 0x447:                                          // Seems to contain more temperatures, highest and lowest?
                                                         //06,38,01,3B,E0,03,39,69
                                                         //06,36,02,36,E0,03,36,72,
      lowest_temperature = (rx_frame.data.u8[1] - 40);   //Best guess for now
      highest_temperature = (rx_frame.data.u8[3] - 40);  //Best guess for now
      break;
    case 0x47B:  //01,FF,FF,FF,FF,FF,FF,FF - Static, values never changes between logs
      break;
    case 0x524:  //24,40,00,00,00,00,00,9B - Static, values never changes between logs
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN
      break;
    case 0x7EF:  //OBD2 PID reply from battery
      switch (rx_frame.data.u8[3]) {
        case POLL_FOR_BATTERY_SOC:
          BMS_SOC = rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_VOLTAGE:
          BMS_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CURRENT:
          BMS_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 5000;
          break;
        case POLL_FOR_LOWEST_TEMP_CELL:
          BMS_lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_HIGHEST_TEMP_CELL:
          BMS_highest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_PACK_AVG_TEMP:
          BMS_average_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_CELL_MV_MAX:
          BMS_highest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CELL_MV_MIN:
          BMS_lowest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        default:  //Unrecognized reply
          break;
      }
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 50ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis50 >= INTERVAL_50_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis50));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis50 = currentMillis;

    // Set close contactors to allowed (Useful for crashed packs, started via contactor control thru GPIO)
    if (datalayer.battery.status.bms_status == ACTIVE) {
      datalayer.system.status.battery_allows_contactor_closing = true;
    } else {  // Fault state, open contactors!
      datalayer.system.status.battery_allows_contactor_closing = false;
    }

    counter_50ms++;
    if (counter_50ms > 23) {
      ATTO_3_12D.data.u8[2] = 0x00;  // Goes from 02->00
      ATTO_3_12D.data.u8[3] = 0x22;  // Goes from A0->22
      ATTO_3_12D.data.u8[5] = 0x31;  // Goes from 71->31
      // TODO: handle more variations after more seconds have passed if needed
    }

    // Update the counters in frame 6 & 7 (they are not in sync)
    if (frame6_counter == 0x0) {
      frame6_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame6_counter--;  // Decrement the counter
    }
    if (frame7_counter == 0x0) {
      frame7_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame7_counter--;  // Decrement the counter
    }

    ATTO_3_12D.data.u8[6] = (0x0F | (frame6_counter << 4));
    ATTO_3_12D.data.u8[7] = (0x09 | (frame7_counter << 4));

    transmit_can(&ATTO_3_12D, can_config.battery);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (counter_100ms < 100) {
      counter_100ms++;
    }

    if (counter_100ms > 3) {

      ATTO_3_441.data.u8[4] = 0x9D;
      ATTO_3_441.data.u8[5] = 0x01;
      ATTO_3_441.data.u8[6] = 0xFF;
      ATTO_3_441.data.u8[7] = 0xF5;
    }

    transmit_can(&ATTO_3_441, can_config.battery);
  }
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    switch (poll_state) {
      case POLL_FOR_BATTERY_SOC:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_SOC;
        poll_state = POLL_FOR_BATTERY_VOLTAGE;
        break;
      case POLL_FOR_BATTERY_VOLTAGE:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_VOLTAGE;
        poll_state = POLL_FOR_BATTERY_CURRENT;
        break;
      case POLL_FOR_BATTERY_CURRENT:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_CURRENT;
        poll_state = POLL_FOR_LOWEST_TEMP_CELL;
        break;
      case POLL_FOR_LOWEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_LOWEST_TEMP_CELL;
        poll_state = POLL_FOR_HIGHEST_TEMP_CELL;
        break;
      case POLL_FOR_HIGHEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_HIGHEST_TEMP_CELL;
        poll_state = POLL_FOR_BATTERY_PACK_AVG_TEMP;
        break;
      case POLL_FOR_BATTERY_PACK_AVG_TEMP:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_PACK_AVG_TEMP;
        poll_state = POLL_FOR_BATTERY_CELL_MV_MAX;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MAX:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_CELL_MV_MAX;
        poll_state = POLL_FOR_BATTERY_CELL_MV_MIN;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MIN:
        ATTO_3_7E7_POLL.data.u8[3] = POLL_FOR_BATTERY_CELL_MV_MIN;
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
      default:
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
    }

    transmit_can(&ATTO_3_7E7_POLL, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("BYD Atto 3 battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4410;  // Over this charging is not possible
  datalayer.battery.info.min_design_voltage_dV = 3800;  // Under this discharging is disabled
}

#endif
