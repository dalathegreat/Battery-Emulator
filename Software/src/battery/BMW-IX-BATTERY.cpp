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
  0x510 DLC8 100ms  STATIC 40 10 40 00 6F DF 19 00  during run -  Startup sends this once: 0x40 0x10 0x02 0x00 0x00 0x00 0x00 0x00
  0x587 DLC8 appears at startup   0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF , 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF,  0x78 0x07 0x00 0x00 0xFF 0xFF 0xFF 0xFF,   0x06 0x00 0x00 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x82 0xFF 0xFF 0xFF 0xFF 0xFF, 0x01 0x03 0x80 0xFF 0xFF 0xFF 0xFF 0xFF

No vehicle  log available, SME asks for:
  0x125 (CCU)
  0x16 (CCU)
  0x91 (EME1)
  0xAA (EME2)

SME Output:

  0x08F DLC48  10ms    - Appears to have analog readings like volt/temp/current
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
  0x4D0 DLC64 1000ms - some slow/flickering values
  0x607 UDS Response



UDS Map:
  69 - service disconnect (1 = closed)
  c7 - available energy - available energy charged
  ce - min avg max SOC
  61 len12 = current sensor
  53 - min and max cell voltage
  4d- main battery voltage
  4a - after contactor voltage
  a4 = charnging contactor temp
  A3 - MAIN CONTACTOR VOLTAGE
  f4 00 0a 62 DD = main battery temp
  A7 - T30 12v voltage (SME input voltage)
  B6 - T30C 12v voltage (12v pyro sensor)
  51 = release, switch contactors. 1 = control of switch contactors active
  CD = charge contactors

  TX 07 03 22 E5 54 - UDS request cell voltages
  RX F4 10 E3 62 E5 54 - 16bit cell voltages batch1 (29 cells)
  TX 07 30 00 02  - UDS request continue data
  RX F4 21 0F - 16bit cell voltages continue2 (31 cells)
  RX F4 22 0F - 16bit cell voltages continue3 (31 cells)
  RX F4 23 0F - 16bit cell voltages continue4 (17 cells)

  TX 07 03 22 E5 9A - UDS request cell SOC
  RX F4 10 E3 62 E5 9A?? - 16bit cell SOC batch1 (29 cells)

  TX 07 03 22 E5 CA - UDS request cell temps
  RX F4 10 81 62 E5 CA - 16bit cell temps batch1 
  RX F4 21 81 62 E5 CA - 16bit cell temps continue2

  TX 07 30 00 02 - UDS request continue data


*/



CAN_frame BMWiX_06D = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x06D,
                     .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFF}};  // 1000ms BDC Output - [0] static [1]-[4] counter x2? is needed? [5-7] static 


CAN_frame BMWiX_2F1 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x2F1,
                     .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xF3, 0xFF}};  // 1000ms BDC Output - Static values - varies at startup



CAN_frame BMWiX_439 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 4,
                     .ID = 0x439,
                     .data = {0xFF, 0xBF, 0xFF, 0xFF}};  // 1000ms BDC Output - Static values



CAN_frame BMWiX_510 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x510,
                     .data = {0x40, 0x10, 0x40, 0x00, 0x6F, 0xDF, 0x19, 0x00}};  // 100ms BDC Output - Static values



CAN_frame BMWiX_6F4 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 5,
                     .ID = 0x6F4,
                     .data = {0x07, 0x03, 0x22, 0xE5, 0xC7}};  // UDS Request data from SME. byte 4 selects requested value


CAN_frame BMWiX_0C0 = {.FD = true,
                     .ext_ID = false,
                     .DLC = 2,
                     .ID = 0x0C0,
                     .data = {0xF0, 0x08}};  // Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 08 static


CAN_frame BMWiX_6F4_CELL_VOLTAGE = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0xE5, 0x54}};
CAN_frame BMWiX_6F4_CELL_SOC = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0xE5, 0x9A}};
CAN_frame BMWiX_6F4_CELL_TEMP = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0xE5, 0xCA}};

CAN_frame BMWiX_6F4_CONTINUE_DATA = {.FD = true, .ext_ID = false, .DLC = 4, .ID = 0x6F4, .data = {0x07, 0x30, 0x00, 0x02}};

CAN_frame BMW_6F4_CELL = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0xDD, 0xBF}};// gives error F4 03 7F 22 31
CAN_frame BMW_6F4_SOH = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0x63, 0x35}}; // gives error F4 03 7F 22 31
CAN_frame BMW_6F4_SOC = {.FD = true, .ext_ID = false, .DLC = 5, .ID = 0x6F4, .data = {0x07, 0x03, 0x22, 0xDD, 0xBC}}; // gives blanks

CAN_frame BMW_10B = {.FD = true,
                     .ext_ID = false,
                     .DLC = 3,
                     .ID = 0x10B,
                     .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command

CAN_frame BMW_6F4_CELL_VOLTAGE_AVG = {.FD = true,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F4,
                                      .data = {0x07, 0x03, 0x22, 0xDF, 0xA0}};
CAN_frame BMW_6F4_CELL_VOLTAGE_CELLNO = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 7,
                                         .ID = 0x6F4,
                                         .data = {0x07, 0x05, 0x31, 0x01, 0xAD, 0x6E, 0x01}}; // gives error
CAN_frame BMW_6F4_CELL_CONTINUE = {.FD = true,
                                   .ext_ID = false,
                                   .DLC = 6,
                                   .ID = 0x6F4,
                                   .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6E}};

static bool battery_awake = false;

//iX Intermediate vars
static int32_t battery_current = 0;
static int16_t battery_voltage = 0;
static int16_t battery_voltage_after_contactor = 0;
static int16_t min_soc_state = 0;
static int16_t avg_soc_state = 0;
static int16_t max_soc_state = 0;
static int16_t remaining_capacity = 0;
static int16_t max_capacity = 0;
static int16_t min_battery_temperature = 0;
static int16_t avg_battery_temperature = 0;
static int16_t max_battery_temperature = 0;
static int16_t main_contactor_temperature = 0;
static int16_t min_cell_voltage = 0;
static int16_t max_cell_voltage = 0;
static int16_t battery_power = 0;
static uint8_t uds_req_id_counter = 0;
static byte iX_0C0_counter = 0xF0; // Initialize to 0xF0

//End iX Intermediate vars

static uint8_t current_cell_polled = 0;

static uint8_t increment_uds_req_id_counter(uint8_t counter) {
  counter++;
  if (counter > 7) {
    counter = 0;
  }
  return counter;
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

  datalayer.battery.status.soh_pptt = 9900;

  datalayer.battery.status.max_discharge_power_W = 6000;

  datalayer.battery.status.max_charge_power_W = 6000;

  battery_power = (datalayer.battery.status.current_dA * (datalayer.battery.status.voltage_dV / 100));

  datalayer.battery.status.active_power_W = battery_power;

  datalayer.battery.status.temperature_min_dC = min_battery_temperature;

  datalayer.battery.status.temperature_max_dC = max_battery_temperature;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;
  
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  datalayer.battery.info.number_of_cells = 108; //Hardcoded for the SE26 battery until values found
}

void receive_can_battery(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x112:
      break;
    case 0x607:  //SME responds to UDS requests on 0x607

      if (rx_frame.DLC > 6 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x10 && rx_frame.data.u8[2] == 0xE3 && rx_frame.data.u8[3] == 0x62  && rx_frame.data.u8[4] == 0xE5){
            //First of multi frame data - Parse the first frame
            if (rx_frame.DLC = 64 && rx_frame.data.u8[5] == 0x54) { //Individual Cell Voltages - First Frame
                  int start_index = 6; //Data starts here
                  int voltage_index = 0; //Start cell ID
                  int num_voltages = 29;  //  number of voltage readings to get
                  for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
                      uint16_t voltage = (rx_frame.data.u8[i]  <<8) | rx_frame.data.u8[i + 1];
                      datalayer.battery.status.cell_voltages_mV[voltage_index++] = voltage;
                   }
            }
     
        //Frame has continued data  - so request it
        transmit_can(&BMWiX_6F4_CONTINUE_DATA, can_config.battery);
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x21){ //Individual Cell Voltages - 1st Continue frame
                  int start_index = 2; //Data starts here
                  int voltage_index = 29; //Start cell ID
                  int num_voltages = 31;  //  number of voltage readings to get
                  for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
                      uint16_t voltage = (rx_frame.data.u8[i]  <<8) | rx_frame.data.u8[i + 1];
                      datalayer.battery.status.cell_voltages_mV[voltage_index++] = voltage;
                   }
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x22){ //Individual Cell Voltages - 2nd Continue frame
                  int start_index = 2; //Data starts here
                  int voltage_index = 60; //Start cell ID
                  int num_voltages = 31;  //  number of voltage readings to get
                  for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
                      uint16_t voltage = (rx_frame.data.u8[i]  <<8) | rx_frame.data.u8[i + 1];
                      datalayer.battery.status.cell_voltages_mV[voltage_index++] = voltage;
                   }
      }  

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x23){ //Individual Cell Voltages - 3rd Continue frame
                  int start_index = 2; //Data starts here
                  int voltage_index = 91; //Start cell ID
                  int num_voltages = 17;  //  number of voltage readings to get
                  for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
                      uint16_t voltage = (rx_frame.data.u8[i]  <<8) | rx_frame.data.u8[i + 1];
                      datalayer.battery.status.cell_voltages_mV[voltage_index++] = voltage;
                   }
      }    
       if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4D) { //Main Battery Voltage (Pre Contactor)
        battery_voltage = (rx_frame.data.u8[5] <<8 | rx_frame.data.u8[6])/10;
       }

       if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4A) { //Main Battery Voltage (After Contactor)
        battery_voltage_after_contactor = (rx_frame.data.u8[5] <<8 | rx_frame.data.u8[6]);
       }


       if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x61) { //Current amps 32bit signed MSB. dA . negative is discharge
        battery_current = (int32_t)((rx_frame.data.u8[5] << 24) |
                                (rx_frame.data.u8[6] << 16) |
                                (rx_frame.data.u8[7] << 8) |
                                rx_frame.data.u8[8]);
       }

       if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0xCE) { //Min/Avg/Max SOC%
        min_soc_state = (rx_frame.data.u8[8] <<8 | rx_frame.data.u8[9]);
        avg_soc_state = (rx_frame.data.u8[6] <<8 | rx_frame.data.u8[7]);
        max_soc_state = (rx_frame.data.u8[10] <<8 |rx_frame.data.u8[11]);
       }

       if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0xC7) { //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
        remaining_capacity = ((rx_frame.data.u8[6] <<8 | rx_frame.data.u8[7]) *1) -5000;
        max_capacity = ((rx_frame.data.u8[8] <<8 | rx_frame.data.u8[9]) *1) -5000; //uint16 limits to 65
                #ifdef DEBUG_VIA_USB
                Serial.print("Remaining Capacty: ");
                Serial.println( remaining_capacity);
                Serial.print("Max Capacty: ");
                Serial.println(max_capacity);
                #endif  //DEBUG_VIA_USB
       }

       if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x53) { //Min and max cell voltage 
        min_cell_voltage = (rx_frame.data.u8[6] <<8 | rx_frame.data.u8[7]);
        max_cell_voltage = (rx_frame.data.u8[8] <<8 | rx_frame.data.u8[9]);
       }

       if (rx_frame.DLC = 16 && rx_frame.data.u8[4] == 0xDD && rx_frame.data.u8[5] == 0xC0) { //Battery Temperature
        min_battery_temperature = (rx_frame.data.u8[6] <<8 | rx_frame.data.u8[7])/10;
        avg_battery_temperature = (rx_frame.data.u8[10] <<8 | rx_frame.data.u8[11])/10;
        max_battery_temperature = (rx_frame.data.u8[8] <<8 | rx_frame.data.u8[9])/10;
       }
       if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xA3) { //Main Contactor Temperature CHECK FINGERPRINT 2 LEVEL
        main_contactor_temperature = (rx_frame.data.u8[5] <<8 | rx_frame.data.u8[6]);

       }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  if (battery_awake) {
    //Send 20ms message
    if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
      // Check if sending of CAN messages has been delayed too much.
      if ((currentMillis - previousMillis20 >= INTERVAL_20_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
        set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis20));
      } else {
        clear_event(EVENT_CAN_OVERRUN);
      }
      previousMillis20 = currentMillis;
    }
    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;
      
      uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
      switch (uds_req_id_counter){
      case 0:
      //BMWiX_6F4.data.u8[3] = 0x22;
      BMWiX_6F4.data.u8[3] = 0xDD;
      BMWiX_6F4.data.u8[4] = 0xC0;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 1:
      BMWiX_6F4.data.u8[3] = 0xE5;
      BMWiX_6F4.data.u8[4] = 0xCE;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 2:
      BMWiX_6F4.data.u8[4] = 0xC7;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 3:
      BMWiX_6F4.data.u8[4] = 0x53;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 4:
      BMWiX_6F4.data.u8[4] = 0x4A;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 5:
      BMWiX_6F4.data.u8[4] = 0x4D;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 6:
      BMWiX_6F4.data.u8[4] = 0x61;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      case 7:
      BMWiX_6F4.data.u8[4] = 0x54;
      transmit_can(&BMWiX_6F4, can_config.battery);
      break;
      }


      //Send SME Keep alive values 100ms
      transmit_can(&BMWiX_510, can_config.battery);

    }
    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;
  
      //Send SME Keep alive values 200ms
      BMWiX_0C0.data.u8[0] =  increment_0C0_counter(BMWiX_0C0.data.u8[0]); //Keep Alive 1
      transmit_can(&BMWiX_0C0, can_config.battery);
    }
    // Send 500ms CAN Message
    if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
      previousMillis500 = currentMillis;
    }
    // Send 640ms CAN Message
    if (currentMillis - previousMillis640 >= INTERVAL_640_MS) {
      previousMillis640 = currentMillis;
    }
    // Send 1000ms CAN Message
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      //Send SME Keep alive values 1000ms
      transmit_can(&BMWiX_06D, can_config.battery);
      transmit_can(&BMWiX_2F1, can_config.battery);
      transmit_can(&BMWiX_439, can_config.battery);

      switch (cmdState) {
        case SOC:
          transmit_can(&BMW_6F4_CELL, can_config.battery);
          cmdState = CELL_VOLTAGE_MINMAX;
          break;
        case CELL_VOLTAGE_MINMAX:
          transmit_can(&BMW_6F4_SOH, can_config.battery);
          cmdState = SOH;
          break;
        case SOH:
          transmit_can(&BMW_6F4_CELL_VOLTAGE_AVG, can_config.battery);
          cmdState = CELL_VOLTAGE_CELLNO;
          current_cell_polled = 0;

          break;
        case CELL_VOLTAGE_CELLNO:
          current_cell_polled++;
          if (current_cell_polled > 96) {
            datalayer.battery.info.number_of_cells = 97;
            cmdState = CELL_VOLTAGE_CELLNO_LAST;
          } else {
            cmdState = CELL_VOLTAGE_CELLNO;

            BMW_6F4_CELL_VOLTAGE_CELLNO.data.u8[6] = current_cell_polled;
            transmit_can(&BMW_6F4_CELL_VOLTAGE_CELLNO, can_config.battery);
          }
          break;
        case CELL_VOLTAGE_CELLNO_LAST:
          transmit_can(&BMW_6F4_SOC, can_config.battery);
          cmdState = SOC;
          break;
      }
    }
    // Send 5000ms CAN Message
    if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
      previousMillis5000 = currentMillis;
    }
    // Send 10000ms CAN Message
    if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
      previousMillis10000 = currentMillis;
    }
  } else {
    previousMillis20 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    previousMillis500 = currentMillis;
    previousMillis640 = currentMillis;
    previousMillis1000 = currentMillis;
    previousMillis5000 = currentMillis;
    previousMillis10000 = currentMillis;
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("BMW iX battery selected");
#endif  //DEBUG_VIA_USB

  //Before we have started up and detected which battery is in use, use 60AH values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  pinMode(WUP_PIN, OUTPUT);
  digitalWrite(WUP_PIN, HIGH);  // Wake up the battery

      //Wake Battery
      //Send SME Keep alive values 100ms
      transmit_can(&BMWiX_510, can_config.battery);
      //Send SME Keep alive values 200ms
      BMWiX_0C0.data.u8[0] =  increment_0C0_counter(BMWiX_0C0.data.u8[0]); //Keep Alive 1
      transmit_can(&BMWiX_0C0, can_config.battery);
      //Send SME Keep alive values 1000ms
      transmit_can(&BMWiX_06D, can_config.battery);
      transmit_can(&BMWiX_2F1, can_config.battery);
      transmit_can(&BMWiX_439, can_config.battery);
}

#endif
