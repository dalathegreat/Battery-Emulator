// SERIAL-LINK-RECEIVER-FROM-BATTERY.cpp

#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"

#define INVERTER_SEND_NUM_VARIABLES   3		//---  comment out if nothing to send
#define INVERTER_RECV_NUM_VARIABLES   16

#ifdef INVERTER_SEND_NUM_VARIABLES
    const uint8_t sendingNumVariables = INVERTER_SEND_NUM_VARIABLES;
#else
    const uint8_t sendingNumVariables = 0;
#endif

//                                  txid,rxid,  num_send,num_recv
SerialDataLink dataLinkReceive(Serial2, 0, 0x01, sendingNumVariables,
                                                 INVERTER_RECV_NUM_VARIABLES); // ... 




void __getData()
{
	SOC 						= (uint16_t) dataLinkReceive.getReceivedData(0);
	StateOfHealth 			= (uint16_t) dataLinkReceive.getReceivedData(1);
	battery_voltage 			= (uint16_t) dataLinkReceive.getReceivedData(2);
	battery_current 			= (uint16_t) dataLinkReceive.getReceivedData(3);
	capacity_Wh 				= (uint16_t) dataLinkReceive.getReceivedData(4);
	remaining_capacity_Wh 	= (uint16_t) dataLinkReceive.getReceivedData(5);
	max_target_discharge_power = (uint16_t) dataLinkReceive.getReceivedData(6);
	max_target_charge_power 	= (uint16_t) dataLinkReceive.getReceivedData(7);
	bms_status 				= (uint16_t) dataLinkReceive.getReceivedData(8);
	bms_char_dis_status 		= (uint16_t) dataLinkReceive.getReceivedData(9);
	stat_batt_power 			= (uint16_t) dataLinkReceive.getReceivedData(10);
	temperature_min 			= (uint16_t) dataLinkReceive.getReceivedData(11);
	temperature_max 			= (uint16_t) dataLinkReceive.getReceivedData(12);
	cell_max_voltage 			= (uint16_t) dataLinkReceive.getReceivedData(13);
	cell_min_voltage 			= (uint16_t) dataLinkReceive.getReceivedData(14);
	batteryAllowsContactorClosing = (uint16_t) dataLinkReceive.getReceivedData(15);
}

void updateData()
{
	// --- update with fresh data
	/*
	dataLinkReceive.updateData(0,var1);
	dataLinkReceive.updateData(1,var2);
	dataLinkReceive.updateData(2,var3);
	*/
}


/*
*  @ 9600bps, assume void manageSerialLinkReceiver()
*             is called every 1mS
*/

void manageSerialLinkReceiver()
{
	dataLinkReceive.run();
	bool readError = dataLinkReceive.checkReadError(true); // check for error & clear error flag
	if (dataLinkReceive.checkNewData(true)) 			   // true = clear Flag
	{
		__getData();
	}
	
	
	
	#ifdef INVERTER_SEND_NUM_VARIABLES
		static bool  initLink=false;
		static unsigned long updateTime = 0;
		if (! initLink)
		{
			initLink = true;
			// sends variables every 5000mS even if no change
			dataLinkReceive.setUpdateInterval(5000); 
		}
		unsigned long currentTime = millis();
		if (currentTime - updateTime > 100)
		{
			updateTime = currentTime;
			dataLinkReceive.run();
			bool sendError = dataLinkReceive.checkTransmissionError(true); // check for error & clear error flag
			updateData();
		}
	#endif
}
