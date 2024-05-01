#include "../include.h"
#ifdef SERIAL_LINK_TRANSMITTER

#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "SERIAL-LINK-TRANSMITTER-INVERTER.h"

/*
*         SerialDataLink
*           txid=1, rxid=0 gives this the startup transmit priority_queue
*           Will transmit max 16 int variable - receive none
*/

#define BATTERY_SEND_NUM_VARIABLES 16
#define BATTERY_RECV_NUM_VARIABLES 1

#ifdef BATTERY_RECV_NUM_VARIABLES
const uint8_t receivingNumVariables = BATTERY_RECV_NUM_VARIABLES;
#else
const uint8_t receivingNumVariables = 0;
#endif

//                                     txid,rxid,num_tx,num_rx
SerialDataLink dataLinkTransmit(Serial2, 0x01, 0, BATTERY_SEND_NUM_VARIABLES, receivingNumVariables);

void printSendingValues();

void _getData() {
  datalayer.system.status.inverter_allows_contactor_closing = dataLinkTransmit.getReceivedData(0);
  //var2 = dataLinkTransmit.getReceivedData(1); // For future expansion,
  //var3 = dataLinkTransmit.getReceivedData(2); // if inverter needs to send data to battery
}

void manageSerialLinkTransmitter() {
  static bool initLink = false;
  static unsigned long updateTime = 0;
  static bool lasterror = false;
  //static unsigned long lastNoError = 0;
  static unsigned long transmitGoodSince = 0;
  static unsigned long lastGood = 0;

  unsigned long currentTime = millis();

  dataLinkTransmit.run();

#ifdef BATTERY_RECV_NUM_VARIABLES
  bool readError = dataLinkTransmit.checkReadError(true);  // check for error & clear error flag
  if (dataLinkTransmit.checkNewData(true))                 // true = clear Flag
  {
    _getData();
  }
#endif

  if (currentTime - updateTime > INTERVAL_100_MS) {
    updateTime = currentTime;
    if (!initLink) {
      initLink = true;
      transmitGoodSince = currentTime;
      dataLinkTransmit.setUpdateInterval(INTERVAL_10_S);
    }
    bool sendError = dataLinkTransmit.checkTransmissionError(true);
    if (sendError) {
      Serial.print(currentTime);
      Serial.println(" - ERROR: Serial Data Link - SEND Error");
      lasterror = true;
      transmitGoodSince = currentTime;
    }

    /* new feature */
    /* @getLastAcknowledge(bool resetFlag)
     *    - returns: 
     *      -2 NACK received from receiver
     *      -1 no ACK received 
     *      0  no activity 
     *      1  ACK received 
     *      resetFlag = true will clear to 0
     */

    int ackReceived = dataLinkTransmit.getLastAcknowledge(true);
    if (ackReceived > 0)
      lastGood = currentTime;

    if (lasterror && (ackReceived > 0)) {
      lasterror = false;
      Serial.print(currentTime);
      Serial.println(" - RECOVERY: Serial Data Link - Send GOOD");
    }

    //--- reporting every 60 seconds that transmission is good
    if (currentTime - transmitGoodSince > INTERVAL_60_S) {
      transmitGoodSince = currentTime;
      Serial.print(currentTime);
      Serial.println(" - Transmit Good");
//  printUsefullData();
#ifdef DEBUG_VIA_USB
      void printSendingValues();
#endif
    }

    //--- report that Errors been ocurring for > 60 seconds
    if (currentTime - lastGood > INTERVAL_60_S) {
      lastGood = currentTime;
      Serial.print(currentTime);
      Serial.println(" - Transmit Failed : 60 seconds");
      //  print the max_ data
      Serial.println("SerialDataLink : bms_status=4");
      Serial.println("SerialDataLink : LEDcolor = RED");
      Serial.println("SerialDataLink : max_target_discharge_power = 0");
      Serial.println("SerialDataLink : max_target_charge_power = 0");

      datalayer.battery.status.max_discharge_power_W = 0;
      datalayer.battery.status.max_charge_power_W = 0;
      set_event(EVENT_SERIAL_TX_FAILURE, 0);
      // throw error
    }
    /*
		//  lastMessageReceived from CAN bus (Battery)
		if (currentTime - lastMessageReceived > (5 * 60000) )  // 5 minutes
		{
			Serial.print(millis());
			Serial.println(" - Data Stale : 5 minutes");
			// throw error
			
			// stop transmitting until fresh
		}
		*/

    static unsigned long updateDataTime = 0;

    if (currentTime - updateDataTime > INTERVAL_1_S) {
      updateDataTime = currentTime;
      dataLinkTransmit.updateData(0, datalayer.battery.status.real_soc);
      dataLinkTransmit.updateData(1, datalayer.battery.status.soh_pptt);
      dataLinkTransmit.updateData(2, datalayer.battery.status.voltage_dV);
      dataLinkTransmit.updateData(3, datalayer.battery.status.current_dA);
      dataLinkTransmit.updateData(4, datalayer.battery.info.total_capacity_Wh / 10);  //u32, remove .0 to fit 16bit
      dataLinkTransmit.updateData(5,
                                  datalayer.battery.status.remaining_capacity_Wh / 10);  //u32, remove .0 to fit 16bit
      dataLinkTransmit.updateData(6,
                                  datalayer.battery.status.max_discharge_power_W / 10);  //u32, remove .0 to fit 16bit
      dataLinkTransmit.updateData(7, datalayer.battery.status.max_charge_power_W / 10);  //u32, remove .0 to fit 16bit
      dataLinkTransmit.updateData(8, datalayer.battery.status.bms_status);
      dataLinkTransmit.updateData(9, datalayer.battery.status.active_power_W / 10);  //u32, remove .0 to fit 16bit
      dataLinkTransmit.updateData(10, datalayer.battery.status.temperature_min_dC);
      dataLinkTransmit.updateData(11, datalayer.battery.status.temperature_max_dC);
      dataLinkTransmit.updateData(12, datalayer.battery.status.cell_max_voltage_mV);
      dataLinkTransmit.updateData(13, datalayer.battery.status.cell_min_voltage_mV);
      dataLinkTransmit.updateData(14, (int16_t)datalayer.battery.info.chemistry);
      dataLinkTransmit.updateData(15, datalayer.system.status.battery_allows_contactor_closing);
    }
  }
}

void printSendingValues() {
  Serial.println("Values from battery: ");
  Serial.print("SOC: ");
  Serial.print(datalayer.battery.status.real_soc);
  Serial.print(" SOH: ");
  Serial.print(datalayer.battery.status.soh_pptt);
  Serial.print(" Voltage: ");
  Serial.print(datalayer.battery.status.voltage_dV);
  Serial.print(" Current: ");
  Serial.print(datalayer.battery.status.current_dA);
  Serial.print(" Capacity: ");
  Serial.print(datalayer.battery.info.total_capacity_Wh);
  Serial.print(" Remain cap: ");
  Serial.print(datalayer.battery.status.remaining_capacity_Wh);
  Serial.print(" Max discharge W: ");
  Serial.print(datalayer.battery.status.max_discharge_power_W);
  Serial.print(" Max charge W: ");
  Serial.print(datalayer.battery.status.max_charge_power_W);
  Serial.print(" BMS status: ");
  Serial.print(datalayer.battery.status.bms_status);
  Serial.print(" Power: ");
  Serial.print(datalayer.battery.status.active_power_W);
  Serial.print(" Temp min: ");
  Serial.print(datalayer.battery.status.temperature_min_dC);
  Serial.print(" Temp max: ");
  Serial.print(datalayer.battery.status.temperature_max_dC);
  Serial.print(" Cell max: ");
  Serial.print(datalayer.battery.status.cell_max_voltage_mV);
  Serial.print(" Cell min: ");
  Serial.print(datalayer.battery.status.cell_min_voltage_mV);
  Serial.print(" LFP : ");
  Serial.print(datalayer.battery.info.chemistry);
  Serial.print(" Battery Allows Contactor Closing: ");
  Serial.print(datalayer.system.status.battery_allows_contactor_closing);
  Serial.print(" Inverter Allows Contactor Closing: ");
  Serial.print(datalayer.system.status.inverter_allows_contactor_closing);

  Serial.println("");
}
#endif
