#ifndef NISSAN_LEAF_BATTERY_H
#define NISSAN_LEAF_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../include.h"
#include "CanBattery.h"
#include "NISSAN-LEAF-HTML.h"

#ifdef NISSAN_LEAF_BATTERY
#define SELECTED_BATTERY_CLASS NissanLeafBattery
#endif

class NissanLeafBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  NissanLeafBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_NISSAN_LEAF* extended,
                    CAN_Interface targetCan)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    datalayer_nissan = extended;

    battery_Total_Voltage2 = 0;
  }

  // Use the default constructor to create the first or single battery.
  NissanLeafBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_nissan = &datalayer_extended.nissanleaf;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_reset_SOH();
  void reset_SOH() { datalayer_extended.nissanleaf.UserRequestSOHreset = true; }

  bool soc_plausible() {
    // When pack voltage is close to max, and SOC% is still low, SOC is not plausible
    return !((datalayer.battery.status.voltage_dV > (datalayer.battery.info.max_design_voltage_dV - 100)) &&
             (datalayer.battery.status.real_soc < 6500));
  }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }
  static constexpr char* Name = "Nissan LEAF battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2600;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  NissanLeafHtmlRenderer renderer;

  bool is_message_corrupt(CAN_frame rx_frame);
  void clearSOH(void);

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_NISSAN_LEAF* datalayer_nissan;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send
  uint8_t mprun10r = 0;                 //counter 0-20 for 0x1F2 message
  uint8_t mprun10 = 0;                  //counter 0-3
  uint8_t mprun100 = 0;                 //counter 0-3

  static const int ZE0_BATTERY = 0;
  static const int AZE0_BATTERY = 1;
  static const int ZE1_BATTERY = 2;

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
  uint8_t PIDgroups[7] = {0x01, 0x02, 0x04, 0x06, 0x83, 0x84, 0x90};
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

  uint8_t crctable[256] = {
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
  uint8_t LEAF_battery_Type = ZE0_BATTERY;
  bool battery_can_alive = false;
#define WH_PER_GID 77                          //One GID is this amount of Watt hours
  uint16_t battery_Discharge_Power_Limit = 0;  //Limit in kW
  uint16_t battery_Charge_Power_Limit = 0;     //Limit in kW
  int16_t battery_MAX_POWER_FOR_CHARGER = 0;   //Limit in kW
  int16_t battery_SOC = 500;                   //0 - 100.0 % (0-1000) The real SOC% in the battery
  uint16_t battery_TEMP = 0;                   //Temporary value used in status checks
  uint16_t battery_Wh_Remaining = 0;           //Amount of energy in battery, in Wh
  uint16_t battery_GIDS = 273;                 //Startup in 24kWh mode
  uint16_t battery_MAX = 0;
  uint16_t battery_Max_GIDS = 273;               //Startup in 24kWh mode
  uint16_t battery_StateOfHealth = 99;           //State of health %
  uint16_t battery_Total_Voltage2 = 740;         //Battery voltage (0-450V) [0.5V/bit, so actual range 0-800]
  int16_t battery_Current2 = 0;                  //Battery current (-400-200A) [0.5A/bit, so actual range -800-400]
  int16_t battery_HistData_Temperature_MAX = 6;  //-40 to 86*C
  int16_t battery_HistData_Temperature_MIN = 5;  //-40 to 86*C
  int16_t battery_AverageTemperature = 6;        //Only available on ZE0, in celcius, -40 to +55
  uint8_t battery_Relay_Cut_Request = 0;         //battery_FAIL
  uint8_t battery_Failsafe_Status = 0;           //battery_STATUS
  bool battery_Interlock =
      true;  //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
  bool battery_Full_CHARGE_flag = false;  //battery_FCHGEND , Goes to 1 if battery is fully charged
  bool battery_MainRelayOn_flag = false;  //No-Permission=0, Main Relay On Permission=1
  bool battery_Capacity_Empty = false;    //battery_EMPTY, , Goes to 1 if battery is empty
  bool battery_HeatExist = false;      //battery_HEATEXIST, Specifies if battery pack is equipped with heating elements
  bool battery_Heating_Stop = false;   //When transitioning from 0->1, signals a STOP heat request
  bool battery_Heating_Start = false;  //When transitioning from 1->0, signals a START heat request
  bool battery_Batt_Heater_Mail_Send_Request = false;  //Stores info when a heat request is happening

  // Nissan LEAF battery data from polled CAN messages
  uint8_t battery_request_idx = 0;
  uint8_t group_7bb = 0;
  bool stop_battery_query = true;
  uint8_t hold_off_with_polling_10seconds = 2;  //Paused for 20 seconds on startup
  uint16_t battery_cell_voltages[96];           //array with all the cellvoltages
  bool battery_balancing_shunts[96];            //array with all the balancing resistors
  uint8_t battery_cellcounter = 0;
  uint16_t battery_min_max_voltage[2];  //contains cell min[0] and max[1] values in mV
  uint16_t battery_HX = 0;              //Internal resistance
  uint16_t battery_insulation = 0;      //Insulation resistance
  uint16_t battery_temp_raw_1 = 0;
  uint8_t battery_temp_raw_2_highnibble = 0;
  uint16_t battery_temp_raw_2 = 0;
  uint16_t battery_temp_raw_3 = 0;
  uint16_t battery_temp_raw_4 = 0;
  uint16_t battery_temp_raw_max = 0;
  uint16_t battery_temp_raw_min = 0;
  int16_t battery_temp_polled_max = 0;
  int16_t battery_temp_polled_min = 0;
  uint8_t BatterySerialNumber[15] = {0};  // Stores raw HEX values for ASCII chars
  uint8_t BatteryPartNumber[7] = {0};     // Stores raw HEX values for ASCII chars
  uint8_t BMSIDcode[8] = {0};

  // Clear SOH values
  uint8_t stateMachineClearSOH = 0xFF;
  uint32_t incomingChallenge = 0xFFFFFFFF;
  uint8_t solvedChallenge[8];
  bool challengeFailed = false;

  CAN_frame LEAF_CLEAR_SOH = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x79B,
                              .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
};

#endif
