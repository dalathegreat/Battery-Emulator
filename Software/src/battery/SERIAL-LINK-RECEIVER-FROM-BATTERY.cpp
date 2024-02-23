#include "BATTERIES.h"
#ifdef SERIAL_LINK_RECEIVER
#include <Arduino.h>
#include "../devboard/utils/events.h"
#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"

#define INVERTER_SEND_NUM_VARIABLES 1
#define INVERTER_RECV_NUM_VARIABLES 16

#ifdef INVERTER_SEND_NUM_VARIABLES
const uint8_t sendingNumVariables = INVERTER_SEND_NUM_VARIABLES;
#else
const uint8_t sendingNumVariables = 0;
#endif

#ifdef TESTBENCH
// In the testbench environment, the receiver uses Serial3
#define SerialReceiver Serial3
#else
// In the production environment, the receiver uses Serial2
#define SerialReceiver Serial2
#endif

//                                          txid,rxid,  num_send,num_recv
SerialDataLink dataLinkReceive(SerialReceiver, 0, 0x01, sendingNumVariables,
                               INVERTER_RECV_NUM_VARIABLES);  // ...

static bool batteryFault = false;  // used locally - mainly to indicate Battery CAN failure

void __getData() {
  system_real_SOC_pptt = (uint16_t)dataLinkReceive.getReceivedData(0);
  system_SOH_pptt = (uint16_t)dataLinkReceive.getReceivedData(1);
  system_battery_voltage_dV = (uint16_t)dataLinkReceive.getReceivedData(2);
  system_battery_current_dA = (int16_t)dataLinkReceive.getReceivedData(3);
  system_capacity_Wh = (uint32_t)dataLinkReceive.getReceivedData(4);
  system_remaining_capacity_Wh = (uint32_t)dataLinkReceive.getReceivedData(5);
  system_max_discharge_power_W = (uint16_t)dataLinkReceive.getReceivedData(6);
  system_max_charge_power_W = (uint16_t)dataLinkReceive.getReceivedData(7);
  uint16_t _system_bms_status = (uint16_t)dataLinkReceive.getReceivedData(8);
  system_active_power_W = (uint16_t)dataLinkReceive.getReceivedData(9);
  system_temperature_min_dC = (int16_t)dataLinkReceive.getReceivedData(10);
  system_temperature_max_dC = (int16_t)dataLinkReceive.getReceivedData(11);
  system_cell_max_voltage_mV = (uint16_t)dataLinkReceive.getReceivedData(12);
  system_cell_min_voltage_mV = (uint16_t)dataLinkReceive.getReceivedData(13);
  system_LFP_Chemistry = (bool)dataLinkReceive.getReceivedData(14);
  batteryAllowsContactorClosing = (bool)dataLinkReceive.getReceivedData(15);

  batteryFault = false;
  if (_system_bms_status == FAULT) {
    batteryFault = true;
    set_event(EVENT_SERIAL_TRANSMITTER_FAILURE, 0);
  }
}

void updateData() {
  // --- update with fresh data
  dataLinkReceive.updateData(0, inverterAllowsContactorClosing);
  //dataLinkReceive.updateData(1,var2); // For future expansion,
  //dataLinkReceive.updateData(2,var3); // if inverter needs to send data to battery
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
  static unsigned long reportTime = 0;
  static uint16_t reads = 0;
  static uint16_t errors = 0;
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
    Serial.println(" - ERROR: SerialDataLink - Read Error");
    lasterror = true;
    errors++;
  }

  if (dataLinkReceive.checkNewData(true))  // true = clear Flag
  {
    __getData();
    reads++;
    lastGoodMaxCharge = system_max_charge_power_W;
    lastGoodMaxDischarge = system_max_discharge_power_W;
    //--- if BatteryFault then assume Data is stale
    if (!batteryFault)
      lastGood = currentTime;
    //bms_status = ACTIVE;  // just testing
    if (lasterror) {
      lasterror = false;
      Serial.print(currentTime);
      Serial.println(" - RECOVERY: SerialDataLink - Read GOOD");
    }
  }

  unsigned long minutesLost = (currentTime - lastGood) / 60000UL;
  if (minutesLost > 0 && lastGood > 0) {
    //   lose 25% each minute of data loss
    if (minutesLost < 4) {
      system_max_charge_power_W = (lastGoodMaxCharge * (4 - minutesLost)) / 4;
      system_max_discharge_power_W = (lastGoodMaxDischarge * (4 - minutesLost)) / 4;
      set_event(EVENT_SERIAL_RX_WARNING, minutesLost);
    } else {
      // Times Up -
      system_max_charge_power_W = 0;
      system_max_discharge_power_W = 0;
      set_event(EVENT_SERIAL_RX_FAILURE, uint8_t(min(minutesLost, 255uL)));
      //----- Throw Error
    }
    // report Lost data & Max charge / Discharge reductions
    if (minutesLost != last_minutesLost) {
      last_minutesLost = minutesLost;
      Serial.print(currentTime);
      if (batteryFault) {
        Serial.print("Battery Fault (minutes) : ");
      } else {
        Serial.print(" - Minutes without data : ");
      }
      Serial.print(minutesLost);
      Serial.print(", max Charge = ");
      Serial.print(system_max_charge_power_W);
      Serial.print(", max Discharge = ");
      Serial.println(system_max_discharge_power_W);
    }
  }

  if (currentTime - reportTime > 59999) {
    reportTime = currentTime;
    Serial.print(currentTime);
    Serial.print(" SerialDataLink-Receiver - NewData :");
    Serial.print(reads);
    Serial.print("   Errors : ");
    Serial.println(errors);
    reads = 0;
    errors = 0;

// --- printUsefullData();
//Serial.print("SOC = ");
//Serial.println(SOC);
#ifdef DEBUG_VIA_USB
    update_values_serial_link();
#endif
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
  Serial.print(system_real_SOC_pptt);
  Serial.print(" SOH: ");
  Serial.print(system_SOH_pptt);
  Serial.print(" Voltage: ");
  Serial.print(system_battery_voltage_dV);
  Serial.print(" Current: ");
  Serial.print(system_battery_current_dA);
  Serial.print(" Capacity: ");
  Serial.print(system_capacity_Wh);
  Serial.print(" Remain cap: ");
  Serial.print(system_remaining_capacity_Wh);
  Serial.print(" Max discharge W: ");
  Serial.print(system_max_discharge_power_W);
  Serial.print(" Max charge W: ");
  Serial.print(system_max_charge_power_W);
  Serial.print(" BMS status: ");
  Serial.print(system_bms_status);
  Serial.print(" Power: ");
  Serial.print(system_active_power_W);
  Serial.print(" Temp min: ");
  Serial.print(system_temperature_min_dC);
  Serial.print(" Temp max: ");
  Serial.print(system_temperature_max_dC);
  Serial.print(" Cell max: ");
  Serial.print(system_cell_max_voltage_mV);
  Serial.print(" Cell min: ");
  Serial.print(system_cell_min_voltage_mV);
  Serial.print(" LFP : ");
  Serial.print(system_LFP_Chemistry);
  Serial.print(" batteryAllowsContactorClosing: ");
  Serial.print(batteryAllowsContactorClosing);
  Serial.print(" inverterAllowsContactorClosing: ");
  Serial.print(inverterAllowsContactorClosing);

  Serial.println("");
}

void setup_battery(void) {
  Serial.println("SERIAL_DATA_LINK_RECEIVER selected");
}
void update_values_battery() {}
void send_can_battery() {}

#endif
