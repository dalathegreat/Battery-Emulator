// SERIAL-LINK-RECEIVER-FROM-BATTERY.cpp

#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"

#define INVERTER_SEND_NUM_VARIABLES 1
#define INVERTER_RECV_NUM_VARIABLES 16

#ifdef INVERTER_SEND_NUM_VARIABLES
const uint8_t sendingNumVariables = INVERTER_SEND_NUM_VARIABLES;
#else
const uint8_t sendingNumVariables = 0;
#endif

//                                  txid,rxid,  num_send,num_recv
SerialDataLink dataLinkReceive(Serial2, 0, 0x01, sendingNumVariables,
                               INVERTER_RECV_NUM_VARIABLES);  // ...

void __getData() {
  SOC = (uint16_t)dataLinkReceive.getReceivedData(0);
  StateOfHealth = (uint16_t)dataLinkReceive.getReceivedData(1);
  battery_voltage = (uint16_t)dataLinkReceive.getReceivedData(2);
  battery_current = (uint16_t)dataLinkReceive.getReceivedData(3);
  capacity_Wh = (uint16_t)dataLinkReceive.getReceivedData(4);
  remaining_capacity_Wh = (uint16_t)dataLinkReceive.getReceivedData(5);
  max_target_discharge_power = (uint16_t)dataLinkReceive.getReceivedData(6);
  max_target_charge_power = (uint16_t)dataLinkReceive.getReceivedData(7);
  bms_status = (uint16_t)dataLinkReceive.getReceivedData(8);
  bms_char_dis_status = (uint16_t)dataLinkReceive.getReceivedData(9);
  stat_batt_power = (uint16_t)dataLinkReceive.getReceivedData(10);
  temperature_min = (uint16_t)dataLinkReceive.getReceivedData(11);
  temperature_max = (uint16_t)dataLinkReceive.getReceivedData(12);
  cell_max_voltage = (uint16_t)dataLinkReceive.getReceivedData(13);
  cell_min_voltage = (uint16_t)dataLinkReceive.getReceivedData(14);
  batteryAllowsContactorClosing = (uint16_t)dataLinkReceive.getReceivedData(15);
}

void updateData() {
  // --- update with fresh data
  dataLinkReceive.updateData(0, inverterAllowsContactorClosing);
  //dataLinkReceive.updateData(1,var2);
  //dataLinkReceive.updateData(2,var3);
}

/*
*  @ 9600bps, assume void manageSerialLinkReceiver()
*             is called every 1mS
*/

void manageSerialLinkReceiver() {
  static bool lasterror = false;
  static unsigned long last_minutesLost = 0;
  static unsigned long lastGood;
  static uint16_t lastGoodMaxCharge;
  static uint16_t lastGoodMaxDischarge;
  static bool initLink = false;

  unsigned long currentTime = millis();

  if (!initLink) {
    initLink = true;
    // sends variables every 5000mS even if no change
    dataLinkReceive.setUpdateInterval(5000);
#ifdef SERIALDATALINK_MUTEACK
    dataLinkReceive.muteACK(true);
#endif
  }
  dataLinkReceive.run();
  bool readError = dataLinkReceive.checkReadError(true);  // check for error & clear error flag

  if (readError) {
    Serial.print(currentTime);
    Serial.println(" - ERROR: Serial Data Link - Read Error");
    lasterror = true;
  } else {
    if (lasterror) {
      lasterror = false;
      Serial.print(currentTime);
      Serial.println(" - RECOVERY: Serial Data Link - Read GOOD");
    }
  }
  if (dataLinkReceive.checkNewData(true))  // true = clear Flag
  {
    __getData();
    lastGoodMaxCharge = max_target_charge_power;
    lastGoodMaxDischarge = max_target_discharge_power;
    lastGood = currentTime;
  }

  unsigned long minutesLost = (currentTime - lastGood) / 60000UL;
  if (minutesLost > 0 && lastGood > 0) {
    //   lose 25% each minute of data loss
    if (minutesLost < 4) {
      max_target_charge_power = (lastGoodMaxCharge * (4 - minutesLost)) / 4;
      max_target_discharge_power = (lastGoodMaxDischarge * (4 - minutesLost)) / 4;
    } else {
      max_target_charge_power = 0;
      max_target_discharge_power = 0;
      bms_status = 4;  //Fault state
      LEDcolor = RED;
      //----- Throw Error
    }
    // report Lost data & Max charge / Discharge reductions
    if (minutesLost != last_minutesLost) {
      last_minutesLost = minutesLost;
      Serial.print(currentTime);
      Serial.print(" - Minutes without data : ");
      Serial.print(minutesLost);
      Serial.print(", max Charge = ");
      Serial.print(max_target_charge_power);
      Serial.print(", max Discharge = ");
      Serial.println(max_target_discharge_power);
    }
  }

  static unsigned long updateTime = 0;

#ifdef INVERTER_SEND_NUM_VARIABLES
  if (currentTime - updateTime > 100) {
    updateTime = currentTime;
    dataLinkReceive.run();
    bool sendError = dataLinkReceive.checkTransmissionError(true);  // check for error & clear error flag
    updateData();
  }
#endif
}

void update_values_serial_link() {
  Serial.println("Values from battery: ");
  Serial.print("SOC: ");
  Serial.print(SOC);
  Serial.print(" SOH: ");
  Serial.print(StateOfHealth);
  Serial.print(" Voltage: ");
  Serial.print(battery_voltage);
  Serial.print(" Current: ");
  Serial.print(battery_current);
  Serial.print(" Capacity: ");
  Serial.print(capacity_Wh);
  Serial.print(" Remain cap: ");
  Serial.print(remaining_capacity_Wh);
  Serial.print(" Max discharge W: ");
  Serial.print(max_target_discharge_power);
  Serial.print(" Max charge W: ");
  Serial.print(max_target_charge_power);
  Serial.print(" BMS status: ");
  Serial.print(bms_status);
  Serial.print(" BMS status dis/cha: ");
  Serial.print(bms_char_dis_status);
  Serial.print(" Power: ");
  Serial.print(stat_batt_power);
  Serial.print(" Temp min: ");
  Serial.print(temperature_min);
  Serial.print(" Temp max: ");
  Serial.print(temperature_max);
  Serial.print(" Cell max: ");
  Serial.print(cell_max_voltage);
  Serial.print(" Cell min: ");
  Serial.print(cell_min_voltage);
  Serial.print(" batteryAllowsContactorClosing: ");
  Serial.print(batteryAllowsContactorClosing);
  Serial.print(" inverterAllowsContactorClosing: ");
  Serial.print(inverterAllowsContactorClosing);
}
