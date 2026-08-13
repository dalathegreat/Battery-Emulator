#ifndef STELLANTIS_PRO_ONE_BATTERY_H
#define STELLANTIS_PRO_ONE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class StellantisProOneBattery : public UdsCanBattery {
 public:
  StellantisProOneBattery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis Pro One 110kWh (E-Ducato/ProMaster/Proace)";

  String get_uds_info_html() override;

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;  //TODO SET
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  CAN_frame ONE_15A = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x15A, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_1D7 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D7,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_175 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x175,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_108 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x108,
                       .data = {0x00, 0x00, 0x00, 0x3D, 0x09, 0x00, 0x00, 0x00}};
  CAN_frame ONE_1D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D8,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  uint8_t expectedCRC = 0;
  uint8_t counter_10ms = 0;        //Counter for the 10ms CAN message, goes from 0-0xF and starts over
  uint8_t counter_20ms = 0;        //Counter for the 20ms CAN message, goes from 0-0xF and starts over
  uint8_t sent_10ms_messages = 0;  //Counter for the number of 10ms messages sent, goes from 0-0xFF and starts over
  uint8_t sent_20ms_messages = 0;  //Counter for the number of 10ms messages sent, goes from 0-0xFF and starts over

  uint16_t lead_acid_voltage = 0;
  uint16_t pid_pack_voltage = 0;
  int16_t pid_lowest_temperature = 0;
  int16_t pid_highest_temperature = 0;

  static const uint16_t PID_WELD_CHECK = 0xD814;
  static const uint16_t PID_CONT_REASON_OPEN = 0xD812;
  static const uint16_t PID_CONTACTOR_STATUS = 0xD813;
  static const uint16_t PID_NEG_CONT_CONTROL = 0xD44F;
  static const uint16_t PID_NEG_CONT_STATUS = 0xD453;
  static const uint16_t PID_POS_CONT_CONTROL = 0xD44E;
  static const uint16_t PID_POS_CONT_STATUS = 0xD452;
  static const uint16_t PID_CONTACTOR_NEGATIVE = 0xD44C;
  static const uint16_t PID_CONTACTOR_POSITIVE = 0xD44D;
  static const uint16_t PID_PRECHARGE_RELAY_CONTROL = 0xD44B;
  static const uint16_t PID_PRECHARGE_RELAY_STATUS = 0xD451;
  static const uint16_t PID_RECHARGE_STATUS = 0xD864;
  static const uint16_t PID_DELTA_TEMPERATURE = 0xD878;
  static const uint16_t PID_COLDEST_MODULE = 0xD446;
  static const uint16_t PID_LOWEST_TEMPERATURE = 0xD87D;
  static const uint16_t PID_AVERAGE_TEMPERATURE = 0xD877;
  static const uint16_t PID_HIGHEST_TEMPERATURE = 0xD817;
  static const uint16_t PID_HOTTEST_MODULE = 0xD445;
  static const uint16_t PID_AVG_CELL_VOLTAGE = 0xD43D;
  static const uint16_t PID_CURRENT = 0xD816;
  static const uint16_t PID_INSULATION_NEG = 0xD87C;
  static const uint16_t PID_INSULATION_POS = 0xD87B;
  static const uint16_t PID_MAX_CURRENT_10S = 0xD876;
  static const uint16_t PID_MAX_DISCHARGE_10S = 0xD873;
  static const uint16_t PID_MAX_DISCHARGE_30S = 0xD874;
  static const uint16_t PID_MAX_CHARGE_10S = 0xD871;
  static const uint16_t PID_MAX_CHARGE_30S = 0xD872;
  static const uint16_t PID_ENERGY_CAPACITY = 0xD860;
  static const uint16_t PID_HIGH_CELL_NUM = 0xD43B;
  static const uint16_t PID_LOW_CELL_NUM = 0xD43C;
  static const uint16_t PID_SUM_OF_CELLS = 0xD438;
  static const uint16_t PID_CELL_MIN_CAPACITY = 0xD413;
  static const uint16_t PID_CELL_VOLTAGE_MEAS_STATUS = 0xD48A;
  static const uint16_t PID_INSULATION_RES = 0xD47A;
  static const uint16_t PID_PACK_VOLTAGE = 0xD815;
  static const uint16_t PID_HIGH_CELL_VOLTAGE = 0xD870;
  static const uint16_t PID_ALL_CELL_VOLTAGES = 0xD440;  //Multi-frame
  static const uint16_t PID_LOW_CELL_VOLTAGE = 0xD86F;
  static const uint16_t PID_BATTERY_ENERGY = 0xD865;
  static const uint16_t PID_CELLBALANCE_STATUS = 0xD46F;      //Multi-frame?
  static const uint16_t PID_CELLBALANCE_HWERR_MASK = 0xD470;  //Multi-frame
  static const uint16_t PID_CRASH_COUNTER = 0xD42F;
  static const uint16_t PID_WIRE_CRASH = 0xD87F;
  static const uint16_t PID_CAN_CRASH = 0xD48D;
  static const uint16_t PID_HISTORY_DATA = 0xD465;
  static const uint16_t PID_LOWSOC_COUNTER = 0xD492;
  static const uint16_t PID_LAST_CAN_FAILURE_DETAIL = 0xD89E;
  static const uint16_t PID_HW_VERSION_NUM = 0xF193;
  static const uint16_t PID_SW_VERSION_NUM = 0xF195;
  static const uint16_t PID_FACTORY_MODE_CONTROL = 0xD900;
  static const uint16_t PID_BATTERY_SERIAL = 0xD901;
  static const uint16_t PID_ALL_CELL_SOH = 0xD4B5;
  static const uint16_t PID_AUX_FUSE_STATE = 0xD86C;
  static const uint16_t PID_BATTERY_STATE = 0xD811;
  static const uint16_t PID_PRECHARGE_SHORT_CIRCUIT = 0xD4D8;
  static const uint16_t PID_ESERVICE_PLUG_STATE = 0xD86A;
  static const uint16_t PID_MAINFUSE_STATE = 0xD86B;
  static const uint16_t PID_MOST_CRITICAL_FAULT = 0xD481;
  static const uint16_t PID_CURRENT_TIME = 0xD47F;
  static const uint16_t PID_TIME_SENT_BY_CAR = 0xD4CA;
  static const uint16_t PID_12V = 0xD822;
  static const uint16_t PID_12V_ABNORMAL = 0xD42B;
  static const uint16_t PID_HVIL_IN_VOLTAGE = 0xD46B;
  static const uint16_t PID_HVIL_OUT_VOLTAGE = 0xD46A;
  static const uint16_t PID_HVIL_STATE = 0xD869;
  static const uint16_t PID_BMS_STATE = 0xD45A;
  static const uint16_t PID_VEHICLE_SPEED = 0xD802;
  static const uint16_t PID_TIME_SPENT_OVER_55C = 0xE082;
  static const uint16_t PID_CONTACTOR_CLOSING_COUNTER = 0xD416;
  static const uint16_t PID_DATE_OF_MANUFACTURE = 0xF18B;
};

#endif
