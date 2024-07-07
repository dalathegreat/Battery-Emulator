#include "../include.h"
#ifdef JAGUAR_IPACE_BATTERY
#include "../datalayer/datalayer.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "TEST-FAKE-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send

static uint8_t HVBattAvgSOC = 0;
static uint8_t HVBattFastChgCounter = 0;
static uint8_t HVBattTempColdCellID = 0;
static uint8_t HVBatTempHotCellID = 0;
static uint8_t HVBattVoltMaxCellID = 0;
static uint8_t HVBattVoltMinCellID = 0;
static uint16_t HVBattCellVoltageMaxMv = 3700;
static uint16_t HVBattCellVoltageMinMv = 3700;
static uint16_t HVBattEnergyAvailable = 0;
static uint16_t HVBattEnergyUsableMax = 0;
static uint16_t HVBattTotalCapacityWhenNew = 0;
static int16_t HVBattAverageTemperature = 0;
static int16_t HVBattCellTempAverage = 0;
static int16_t HVBattCellTempColdest = 0;
static int16_t HVBattCellTempHottest = 0;
static int16_t HVBattInletCoolantTemp = 0;

/*


You mentioned earlier that the modules use LG 3.7V 60AH HW001 cells? Is this still the current understanding?
Reason I ask is that I'm trying to piece together the technical spec for these module cells (in absence of any known datasheet)

Do these specs listed below here match up with what you have found?

Nominal Voltage: 3.7V although I have also see 3.65 and 3.6 listed (Jaguar now state in the owners manual that it is a 388V (nominal) pack therefore with 108S pack that gives nominal 3.6V per cell)
Max charge current: 60A
Nominal capacity: 60000mah
Net weight: 850g
Internal resistance: 1m(ohm)
Discharge current (continuous) 120(2C)
Plus discharge current: 300(5C)
Cutoff charge voltage: 4.2V
Cut-off discharge voltage: 2.75V


That would all be true. But i would not go to the edges of SOC. I use cell from 3.0V to 4.08V and that is it. Everything else i can confirm.


* The configuration of the BMS is one master and six slave modules, and communication between them is CAN-based.
* Each slave module has two sections (seen in the pictures above). Each is based on TI 76PL455ATQ (cell monitoring) and NXP processor for communication. So there are 12 CAN nodes, each monitoring 9S cells (3 blocks x 3S)








https://www.jaguarforums.com/forum/general-tech-help-7/canbus-hacking-ipace-242124/

https://www.i-paceforum.com/threads/parts-numbers-descriptions-and-names.6839/

https://docs.google.com/spreadsheets/d/1wNMtpPqMAejNeOZGsPCcgau8HODROzceFcUSfk2lVz8/edit?gid=445693275#gid=445693275

Now, with the latest sample data I collected this morning (ext. temp is -14c) I am 95% sure that the battery temperature can be read on the CANbus on ECU ID 7E4[7EC] (BECM), and the PID for the battery temperature are 0x492B, 0x492C, 0x492D, 0x492E, 0x492F, 0x4930. These are the 6 values for the 6 plates in the battery. The EV battery coolant outlet temperature sensor is on 7E4[7EC]:0x491B, and the EV battery coolant inlet temperature sensor is on 7E4[7EC]:0x491C. The formula for those temperature is (value-40), and the result is in deg celcius. The external ambient temperature can be read on the HVAC module 733[73B]:0x9924, and the formula is (value*0.5 -40). If you have a way to read the High Speed CANbus (pin 6/14 on the diagnostic connector), ie. TorquePro, you can read it yourself.









On the BECM module, PID 0x4910, 0x4911, 0x4914 seems to be the real battery % (divide the value by 100). At 100% of battery , this value is around 9600 (9600/100 = 96%), and with a linear regression, I can extrapolate that the value would be around 3% with the dash says 0% of battery. SO I can see a real 3% when the dash says 0%, and a real 96% at 100%.


PID 0x4913 on BECM seems to be the max regen:
- 2 bytes
- when battery is fully charged => 0
- when battery SOC =93%, 950
- when battery SOC =82, 4600
- when battery SOC =60, 10600
- when battery SOC =8, 15000

So if you take that value and divide by 100, this gives 150kw at 8% of SOC, 106kw at 60%, etc...







PID 0x498f on BCCM seems to be the Voltage of the charger plugged in the car
Unplug: 0
plug on 240v:
- 76 7a
- 77 32,
- 79 18,
- 76 8d,
Plug on 120v:
- 38 2a

If you take that (value / 128), you have something close to 240 or close to 110.






In the BECM module, PID 0x4901, 0x4909, 0x490e ,0x490f are all within the same value range (36000-44000). If we divide them by 100, that could be a battery voltage.

Since the battery pack is organized with 9 cells in serie, and 4 groups of 9 cells in parallel, I am expecting to have 4 values for these 4 groups in the range of 36000-44000. That's what we have.

Now I am looking for the Amp (something around 58Ah per cell, or 232Ah per row









PID 0x490A on BECM is interesting. it is a one 1 Byte. On my sample data, the min is 58 and max is 146. If I apply the formula (value/2 - 40), just like the external temperature sensor , this PID gives a value close to the external temperature when the car stand still in the driveway, without charging, this value goes up a bit when the car is charging on the 240v charger, but significantly higher (+33c) when I did a fast charge yesterday, even if the external temp was -5c. Could be an internal temp sensor on an electronic module.




This is the status so far for the BECM

PID Description Formula Unit
4886
4887
48c2
4900
4901 Voltage for battery row#1 (9 modules) (256A+B)/100 Volt
4902
4903 Max voltage of the pouch cells (256A+B)/1000 Volt
4904 Min voltage of the pouch cells (256A+B)/1000 Volt
4905 Maybe a temperature of a componant ?? (A*0.5)-40
4906 Maybe a temperature of a componant ?? (A*0.5)-41
4907 Maybe a temperature of a componant ?? (A*0.5)-42
4908 Maybe a temperature of a componant ?? (A*0.5)-43
4909 Voltage for battery row#2 (9 modules) (256A+B)/100 Volt
490a Maybe a temperature of a componant ?? (A*0.5)-43
490b
490c Battery current in and out ((256A+B)-0x8000)/24 or 25 Amp
490d
490e Voltage for battery row#3 (9 modules) (256A+B)/100 Volt
490f Voltage for battery row#4 (9 modules) (256A+B)/100 Volt
4910 Average Battery SOC (256A+B)/100 %
4911 Min Battery SOC (256A+B)/100 %
4912 ??? Battery current ??? (256A+B)/160 Amp
4913 Max Regen (256A+B)/100 Kw
4914 Max Battery SOC (256A+B)/100 %
4915
4916
4917
4918
4919
491a
491b EV battery coolant outlet temperature A-40 DegC
491c EV battery coolant inlet temperature A-40 DegC
491d
491e set of bit / flag
491f
4920 maybe some voltage 256A+B Volt
4921 maybe some voltage 256A+B Volt
4923 set of bit / flag
492b Battery CSC #1 temperature A-40 DegC
492c Battery CSC #2 temperature A-40 DegC
492d Battery CSC #3 temperature A-40 DegC
492e Battery CSC #4 temperature A-40 DegC
492f Battery CSC #5 temperature A-40 DegC
4930 Battery CSC #6 temperature A-40 DegC
4931
4933
4934
4935
4936
4937
4938
4939
4941
4944 set of bit / flag
4945 set of bit / flag
494e set of bit / flag
4970
4971
497a maybe some voltage 256A+B Volt
497b maybe some voltage 256A+B Volt
497c maybe some voltage 256A+B Volt
497e
497f
4980
4981 maybe some voltage 256A+B Volt
498c
d015
d018
d019
d020
d021
d05b
d100
d10e set of bit / flag
d14d
d703
dd00 Elaspe time since factory built (16777216*A+65536*B+256*C+D)/10 Second
dd01 Odometer 65536*A+256*B+C KM
dd02
dd04
dd05
dd06
dd08
dd09






*/

/* TODO: Actually use a proper keepalive message */
CAN_frame_t ipace_keep_alive = {.FIR = {.B =
                                            {
                                                .DLC = 8,
                                                .FF = CAN_frame_std,
                                            }},
                                .MsgID = 0x063,
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame_t ipace_7e4 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x7e4,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void print_units(char* header, int value, char* units) {
  Serial.print(header);
  Serial.print(value);
  Serial.print(units);
}

void update_values_battery() { /* This function puts fake values onto the parameters sent towards the inverter */

  datalayer.battery.status.real_soc = HVBattAvgSOC * 100;  //Add two decimals

  datalayer.battery.status.soh_pptt = 9900;  //TODO: Map

  datalayer.battery.status.voltage_dV = 3700;  //TODO: Map

  datalayer.battery.status.current_dA = 0;  //TODO: Map

  datalayer.battery.info.total_capacity_Wh = HVBattEnergyUsableMax * 100;  // kWh+1 to Wh

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = HVBattCellVoltageMaxMv;

  datalayer.battery.status.cell_min_voltage_mV = HVBattCellVoltageMinMv;

  datalayer.battery.status.active_power_W = 0;  //TODO: Map

  datalayer.battery.status.temperature_min_dC = HVBattCellTempColdest * 10;  // C to dC

  datalayer.battery.status.temperature_max_dC = HVBattCellTempHottest * 10;  // C to dC

  datalayer.battery.status.max_discharge_power_W = 5000;  //TODO: Map

  datalayer.battery.status.max_charge_power_W = 5000;  //TODO: Map

  for (int i = 0; i < 107; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = 3500 + i;
  }

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_VIA_USB
  Serial.println("FAKE Values going to inverter");
  print_units("SOH%: ", (datalayer.battery.status.soh_pptt * 0.01), "% ");
  print_units(", SOC%: ", (datalayer.battery.status.reported_soc * 0.01), "% ");
  print_units(", Voltage: ", (datalayer.battery.status.voltage_dV * 0.1), "V ");
  print_units(", Max discharge power: ", datalayer.battery.status.max_discharge_power_W, "W ");
  print_units(", Max charge power: ", datalayer.battery.status.max_charge_power_W, "W ");
  print_units(", Max temp: ", (datalayer.battery.status.temperature_max_dC * 0.1), "°C ");
  print_units(", Min temp: ", (datalayer.battery.status.temperature_min_dC * 0.1), "°C ");
  print_units(", Max cell voltage: ", datalayer.battery.status.cell_max_voltage_mV, "mV ");
  print_units(", Min cell voltage: ", datalayer.battery.status.cell_min_voltage_mV, "mV ");
  Serial.println("");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {

  // Do not log noisy startup messages - there are many !
  if (rx_frame.MsgID == 0 && rx_frame.FIR.B.DLC == 8 && rx_frame.data.u8[0] == 0 && rx_frame.data.u8[1] == 0 &&
      rx_frame.data.u8[2] == 0 && rx_frame.data.u8[3] == 0 && rx_frame.data.u8[4] == 0 && rx_frame.data.u8[5] == 0 &&
      rx_frame.data.u8[6] == 0x80 && rx_frame.data.u8[7] == 0) {
    return;
  }

  switch (rx_frame.MsgID) {  // These messages are periodically transmitted by the battery
    case 0x080:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x100:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x102:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x104:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x10A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x198:
      break;
    case 0x1C4:
      //HVBattCurrentTR
      //HVBattPwrExtGPCounter
      //HVBattPwrExtGPCS
      //HVBattVoltageBusTF //
      //HVBattVoltageBusTR
      break;
    case 0x220:
      break;
    case 0x222:
      HVBattAvgSOC = rx_frame.data.u8[4];
      HVBattAverageTemperature = (rx_frame.data.u8[5] / 2) - 40;
      HVBattFastChgCounter = rx_frame.data.u8[7];
      //HVBattLogEvent
      HVBattTempColdCellID = rx_frame.data.u8[0];
      HVBatTempHotCellID = rx_frame.data.u8[1];
      HVBattVoltMaxCellID = rx_frame.data.u8[2];
      HVBattVoltMinCellID = rx_frame.data.u8[3];
      break;
    case 0x248:
      break;
    case 0x308:
      break;
    case 0x424:
      HVBattCellVoltageMaxMv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      HVBattCellVoltageMinMv = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      //HVBattHeatPowerGenChg // kW
      //HVBattHeatPowerGenDcg // kW
      //HVBattWarmupRateChg // degC/minute
      //HVBattWarmupRateDcg // degC/minute
      break;
    case 0x448:
      HVBattCellTempAverage = (rx_frame.data.u8[0] / 2) - 40;
      HVBattCellTempColdest = (rx_frame.data.u8[1] / 2) - 40;
      HVBattCellTempHottest = (rx_frame.data.u8[2] / 2) - 40;
      //HVBattCIntPumpDiagStat // Pump OK / NOK
      //HVBattCIntPumpDiagStat_UB // True/False
      //HVBattCoolantLevel // Coolant level OK / NOK
      //HVBattHeaterCtrlStat // Off / On
      HVBattInletCoolantTemp = (rx_frame.data.u8[5] / 2) - 40;
      //HVBattInlentCoolantTemp_UB // True/False
      //HVBattMILRequested // True/False
      //HVBattThermalOvercheck // OK / NOK
      break;
    case 0x449:
      break;
    case 0x464:
      HVBattEnergyAvailable =
          ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) / 2;  // 0x0198 = 408 / 2 = 204 = 20.4kWh
      HVBattEnergyUsableMax =
          ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) / 2;  // 0x06EA = 1770 / 2 = 885 = 88.5kWh
      HVBattTotalCapacityWhenNew = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);  //0x0395 = 917 = 91.7kWh
      break;
    case 0x522:
      break;
    default:
      break;
  }

  // Discard non-interesting can messages so they do not get logged via serial
  if (rx_frame.MsgID < 0x500) {
    return;
  }

  // All CAN messages recieved will be logged via serial
  Serial.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  Serial.print("  ");
  Serial.print(rx_frame.MsgID, HEX);
  Serial.print("  ");
  Serial.print(rx_frame.FIR.B.DLC);
  Serial.print("  ");
  for (int i = 0; i < rx_frame.FIR.B.DLC; ++i) {
    Serial.print(rx_frame.data.u8[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  /*

  Startup messages from battery ...

  */
}

int state = 0;

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //ESP32Can.CANWriteFrame(&ipace_keep_alive);
  }

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    CAN_frame_t msg;
    int err;

    switch (state) {
      case 0:

        // car response: 7ec 07 59 02 8f c0 64 88 28
        // response:     7EC 07 59 02 8F F0 01 00 28
        // response:     7EC 03 59 02 8F 00 00 00 00
        //               7EC  8  3 7F 19 11 0 0 0 0
        msg = {.FIR = {.B =
                           {
                               .DLC = 8,
                               .FF = CAN_frame_std,
                           }},
               .MsgID = 0x7e4,
               .data = {0x03, 0x19, 0x02, 0x8f, 0x00, 0x00, 0x00, 0x00}};
        err = ESP32Can.CANWriteFrame(&msg);
        if (err == 0)
          state++;

        break;
      case 1:
        // car response: 7EC 11 fa 59 04 c0 64 88 28
        // response:

        msg = {.FIR = {.B =
                           {
                               .DLC = 8,
                               .FF = CAN_frame_std,
                           }},
               .MsgID = 0x7e4,
               .data = {0x06, 0x19, 0x04, 0xc0, 0x64, 0x88, 0xff, 0x00}};
        err = ESP32Can.CANWriteFrame(&msg);
        if (err == 0)
          state++;
        break;
      case 2:
        /* reset */
        state = 0;
        break;
      default:
        break;
    }

    // TODO -1 is an error !!

    Serial.print("sending 7e4 err:");
    Serial.println(err);

    Serial.print("sending 7e4_2 err:");
    Serial.println(err);

    /*

on car this is ... 10x messages of 0x522 01 12 0 3f 0 0 0 0

9x total
77605  522  8  22 12 0 0 0 0 0 0
77914  522  8  22 1 0 0 0 0 0 0
78014  522  8  22 12 0 0 0 0 0 0
78324  522  8  22 1 0 0 0 0 0 0
78424  522  8  22 12 0 0 0 0 0 0
78734  522  8  22 1 0 0 0 0 0 0
78834  522  8  22 12 0 0 0 0 0 0
79144  522  8  22 1 0 0 0 0 0 0
79247  522  8  22 12 0 0 0 0 0 0
79554  522  8  22 1 0 0 0 0 0 0


*/
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Jaguar iPace 90kWh battery selected");
#endif

  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_design_voltage_dV = 4546;
  datalayer.battery.info.min_design_voltage_dV = 3230;
}

#endif
