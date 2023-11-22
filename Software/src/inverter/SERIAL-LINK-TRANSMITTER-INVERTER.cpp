//SERIAL-LINK-TRANSMITTER-INVERTER.cpp

#include "SERIAL-LINK-TRANSMITTER-INVERTER.h"

/*
*         SerialDataLink
*           txid=1, rxid=0 gives this the startup transmit priority_queue
*           Will transmit max 16 int variable - receive none
*/

#define BATTERY_SEND_NUM_VARIABLES 16
//#define BATTERY_RECV_NUM_VARIABLES   3		//--- comment out if nothing to receive

#ifdef BATTERY_RECV_NUM_VARIABLES
const uint8_t receivingNumVariables = BATTERY_RECV_NUM_VARIABLES;
#else
const uint8_t receivingNumVariables = 0;
#endif

//                                     txid,rxid,num_tx,num_rx
SerialDataLink dataLinkTransmit(Serial2, 0x01, 0, BATTERY_SEND_NUM_VARIABLES, receivingNumVariables);

void _getData() {
  /*
	var1 = dataLinkTransmit.getReceivedData(0);
	var2 = dataLinkTransmit.getReceivedData(1);
	var3 = dataLinkTransmit.getReceivedData(2);
	*/
}

void manageSerialLinkTransmitter() {
  static bool initLink = false;
  static unsigned long updateTime = 0;
  static bool lasterror = false;

  dataLinkTransmit.run();

#ifdef BATTERY_RECV_NUM_VARIABLES
  bool readError = dataLinkTransmit.checkReadError(true);  // check for error & clear error flag
  if (dataLinkTransmit.checkNewData(true))                 // true = clear Flag
  {
    _getData();
  }
#endif

  if (millis() - updateTime > 100) {
    updateTime = millis();
    if (!initLink) {
      initLink = true;
      // sends variables every 5000mS even if no change
      dataLinkTransmit.setUpdateInterval(5000);
    }
    bool sendError = dataLinkTransmit.checkTransmissionError(true);
    LEDcolor = GREEN;
    if (sendError) {
      LEDcolor = RED;
      Serial.print(millis());
      Serial.println(" - ERROR: Serial Data Link - SEND Error");
      lasterror = true;
    } else {
      if (lasterror) {
        lasterror = false;
        Serial.print(millis());
        Serial.println(" - RECOVERY: Serial Data Link - Send GOOD");
      }
    }

    dataLinkTransmit.updateData(0, SOC);
    dataLinkTransmit.updateData(1, StateOfHealth);
    dataLinkTransmit.updateData(2, battery_voltage);
    dataLinkTransmit.updateData(3, battery_current);
    dataLinkTransmit.updateData(4, capacity_Wh);
    dataLinkTransmit.updateData(5, remaining_capacity_Wh);
    dataLinkTransmit.updateData(6, max_target_discharge_power);
    dataLinkTransmit.updateData(7, max_target_charge_power);
    dataLinkTransmit.updateData(8, bms_status);
    dataLinkTransmit.updateData(9, bms_char_dis_status);
    dataLinkTransmit.updateData(10, stat_batt_power);
    dataLinkTransmit.updateData(11, temperature_min);
    dataLinkTransmit.updateData(12, temperature_max);
    dataLinkTransmit.updateData(13, cell_max_voltage);
    dataLinkTransmit.updateData(14, cell_min_voltage);
    dataLinkTransmit.updateData(15, batteryAllowsContactorClosing);
  }
}
