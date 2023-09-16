#include "TESLA-MODEL-3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis30 = 0; // will store last time a 30ms CAN Message was send
static const int interval30 = 30; // interval (ms) at which send CAN Messages
static uint8_t stillAliveCAN = 6; //counter for checking if CAN is still alive

CAN_frame_t TESLA_221_1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96}};
CAN_frame_t TESLA_221_2 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA}};

static uint32_t temporaryvariable = 0;
static uint32_t total_discharge = 0;
static uint32_t total_charge = 0;
static uint16_t volts = 0;   // V
static int16_t amps = 0;    // A
static uint16_t raw_amps = 0; // A
static int16_t max_temp = 6; // C*
static int16_t min_temp = 5; // C*
static uint16_t energy_buffer = 0;
static uint16_t energy_to_charge_complete = 0;
static uint16_t expected_energy_remaining = 0;
static uint8_t full_charge_complete = 0;
static uint16_t ideal_energy_remaining = 0;
static uint16_t nominal_energy_remaining = 0;
static uint16_t nominal_full_pack_energy = 0;
static uint16_t battery_charge_time_remaining = 0; // Minutes
static uint16_t regenerative_limit = 0;
static uint16_t discharge_limit = 0;
static uint16_t max_heat_park = 0;
static uint16_t hvac_max_power = 0;
static uint16_t min_voltage = 0;
static uint16_t max_discharge_current = 0;
static uint16_t max_charge_current = 0;
static uint16_t max_voltage = 0;
static uint16_t high_voltage = 0;
static uint16_t low_voltage = 0;
static uint16_t output_current = 0;
static uint16_t soc_min = 0;
static uint16_t soc_max = 0;
static uint16_t soc_vi = 0;
static uint16_t soc_ave = 0;
static uint16_t cell_max_v = 0;
static uint16_t cell_min_v = 0;
static uint16_t	cell_deviation_mV = 0; //contains the deviation between highest and lowest cell in mV
static uint8_t max_vno = 0;
static uint8_t min_vno = 0;
static uint8_t contactor = 0; //State of contactor
static uint8_t hvil_status = 0;
static uint8_t packContNegativeState = 0;
static uint8_t packContPositiveState = 0;
static uint8_t packContactorSetState = 0;
static uint8_t packCtrsClosingAllowed = 0;
static uint8_t pyroTestInProgress = 0;
static const char* contactorText[] = {"UNKNOWN(0)","OPEN","CLOSING","BLOCKED","OPENING","CLOSED","UNKNOWN(6)","WELDED","POS_CL","NEG_CL","UNKNOWN(10)","UNKNOWN(11)","UNKNOWN(12)"};
static const char* contactorState[] = {"SNA","OPEN","PRECHARGE","BLOCKED","PULLED_IN","OPENING","ECONOMIZED","WELDED","UNKNOWN(8)","UNKNOWN(9)","UNKNOWN(10)","UNKNOWN(11)"};
static const char* hvilStatusState[] = {"NOT OK","STATUS_OK","CURRENT_SOURCE_FAULT","INTERNAL_OPEN_FAULT","VEHICLE_OPEN_FAULT","PENTHOUSE_LID_OPEN_FAULT","UNKNOWN_LOCATION_OPEN_FAULT","VEHICLE_NODE_FAULT","NO_12V_SUPPLY","VEHICLE_OR_PENTHOUSE_LID_OPENFAULT","UNKNOWN(10)","UNKNOWN(11)","UNKNOWN(12)","UNKNOWN(13)","UNKNOWN(14)","UNKNOWN(15)"};


#define MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to inverter
#define MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to inverter
#define MAX_CELL_VOLTAGE 4250 //Battery is put into emergency stop if one cell goes over this value (These values might need tweaking based on chemistry)
#define MIN_CELL_VOLTAGE 2950 //Battery is put into emergency stop if one cell goes below this value (These values might need tweaking based on chemistry)
#define MAX_CELL_DEVIATION 500 //LED turns yellow on the board if mv delta exceeds this value

void update_values_tesla_model_3_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
	StateOfHealth = 9900; //Hardcoded to 99%SOH

  //Calculate the SOC% value to send to inverter
  soc_vi = MIN_SOC + (MAX_SOC - MIN_SOC) * (soc_vi - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE); 
  if (soc_vi < 0)
  { //We are in the real SOC% range of 0-20%, always set SOC sent to Inverter as 0%
      soc_vi = 0;
  }
  if (soc_vi > 1000)
  { //We are in the real SOC% range of 80-100%, always set SOC sent to Inverter as 100%
      soc_vi = 1000;
  }
  SOC = (soc_vi * 10); //increase SOC range from 0-100.0 -> 100.00

	battery_voltage = (volts*10); //One more decimal needed (370 -> 3700)

  battery_current = amps; //TODO, this needs verifying if scaling is right

	capacity_Wh = (nominal_full_pack_energy * 100); //Scale up 75.2kWh -> 75200Wh
  if(capacity_Wh > 60000)
  {
    capacity_Wh = 60000;
  }

	remaining_capacity_Wh = (expected_energy_remaining * 100); //Scale up 60.3kWh -> 60300Wh

  //Calculate the allowed discharge power, cap it if it gets too large
  temporaryvariable = (max_discharge_current * volts);
  if(temporaryvariable > 60000){
    max_target_discharge_power = 60000;
  }
  else{
    max_target_discharge_power = temporaryvariable;
  }

  //Calculate the allowed charge power, cap it if it gets too large
	temporaryvariable = (max_charge_current * volts);
	if(temporaryvariable > 60000){
    max_target_charge_power = 60000;
  }
  else{
    max_target_charge_power = temporaryvariable;
  }

	stat_batt_power = (volts * amps); //TODO, check if scaling is OK

  min_temp = (min_temp * 10);
	temperature_min = convert2unsignedint16(min_temp);

  max_temp = (max_temp * 10);
	temperature_max = convert2unsignedint16(max_temp);

  bms_status = ACTIVE; //Startout in active mode before checking if we have any faults

	/* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
	if(!stillAliveCAN)
	{
	bms_status = FAULT;
	Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
	}
	else
	{
	stillAliveCAN--;
	}

  if(cell_max_v >= MAX_CELL_VOLTAGE){ 
    bms_status = FAULT;
    Serial.println("CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
  }
  if(cell_min_v <= MIN_CELL_VOLTAGE){ 
    bms_status = FAULT;
    Serial.println("CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
  }

  cell_deviation_mV = (cell_max_v - cell_min_v);

  if(cell_deviation_mV > MAX_CELL_DEVIATION){
    LEDcolor = YELLOW;
    Serial.println("HIGH CELL DEVIATION!!! Inspect battery!");
  }

  #ifdef DEBUG_VIA_USB
    if (packCtrsClosingAllowed == 0)
    {
      Serial.println("Check high voltage connectors and interlock circuit! Closing contactor not allowed! Values: ");
    }
    if (pyroTestInProgress == 1)
    {
      Serial.println("Please wait for Pyro Connection check to finish, HV cables successfully seated!");
    }

    Serial.print("Contactor: ");
    Serial.print(contactorText[contactor]); //Display what state the contactor is in
    Serial.print(" , HVIL: ");
    Serial.print(hvilStatusState[hvil_status]);
    Serial.print(" , NegativeState: ");
    Serial.print(contactorState[packContNegativeState]);
    Serial.print(" , PositiveState: ");
    Serial.print(contactorState[packContPositiveState]);
    Serial.print(", setState: ");
    Serial.print(contactorState[packContactorSetState]);
    Serial.print(", close allowed: ");
    Serial.print(packCtrsClosingAllowed);
    Serial.print(", Pyrotest: ");
    Serial.println(pyroTestInProgress);

    Serial.println("Battery values: ");
    Serial.print(" Vi SOC: ");
    Serial.print(soc_vi);
    Serial.print(" SOC max: ");
    Serial.print(soc_max);
    Serial.print(" SOC min: ");
    Serial.print(soc_min);
    Serial.print(" SOC avg: ");
    Serial.print(soc_ave);
    Serial.print(", Battery voltage: ");
    Serial.print(volts);
    Serial.print(", Battery amps: ");
    Serial.print(amps);
    Serial.print(", Discharge limit battery (kW): ");
    Serial.print(discharge_limit);
    Serial.print(", Charge limit battery (kW): ");
    Serial.print(regenerative_limit);
    Serial.print(", Fully charged?: ");
    Serial.print(full_charge_complete);
    Serial.print(", Min discharge voltage allowed: ");
    Serial.print(min_voltage);
    Serial.print(", Max charge voltage allowed: ");
    Serial.print(max_voltage);
    Serial.print(", Max charge current (A): ");
    Serial.print(max_charge_current);
    Serial.print(", Max discharge current (A): ");
    Serial.println(max_discharge_current);

    Serial.print("Cellstats, Max mV: ");
    Serial.print(cell_max_v);
    Serial.print(", on cell no#: ");
    Serial.print(max_vno);
    Serial.print(", Min mV: ");
    Serial.print(cell_min_v);
    Serial.print(", on cell no#: ");
    Serial.println(min_vno);

    Serial.print("HighVoltage Output Pins: ");
    Serial.print(high_voltage);
    Serial.print(", V, Low Voltage:");
    Serial.print(low_voltage);
    Serial.print(", V, Current Output:");
    Serial.println(output_current);

    Serial.println("Values heading towards inverter: ");
    Serial.print(" SOC% (XX.XX%): ");
    Serial.print(SOC);
    Serial.print(" Max discharge power (W): ");
    Serial.print(max_target_discharge_power);
    Serial.print(" Max charge power (W): ");
    Serial.print(max_target_charge_power);
    Serial.print(" Max temperature (C): ");
    Serial.print(temperature_max);
    Serial.print(" Min temperature (C): ");
    Serial.println(temperature_min);
  #endif
}

void receive_can_tesla_model_3_battery(CAN_frame_t rx_frame)
{
  static int mux = 0;
  static int temp = 0;
  
  stillAliveCAN = 12; //We are getting CAN messages, set the CAN detect counter

  switch (rx_frame.MsgID)
    {
    case 0x352:
      //SOC
      nominal_full_pack_energy =  (((rx_frame.data.u8[1] & 0x0F) << 8) | (rx_frame.data.u8[0])); //Example 752 (75.2kWh)
      nominal_energy_remaining =  (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0xF8) >> 3)) * 0.1; //Example 1247 * 0.1 = 124.7kWh
      expected_energy_remaining = (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) | ((rx_frame.data.u8[2] & 0xC0) >> 6)); //Example 622 (62.2kWh)
      ideal_energy_remaining =    (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0xFE) >> 1)) * 0.1; //Example 311 * 0.1 = 31.1kWh
      energy_to_charge_complete = (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 0.1; //Example 147 * 0.1 = 14.7kWh
      energy_buffer =             (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x80) >> 7)) * 0.1; //Example 1 * 0.1 = 0
      full_charge_complete =      ((rx_frame.data.u8[7] & 0x80) >> 7);
      break;
    case 0x20A:
      //Contactor state
      packContNegativeState =   (rx_frame.data.u8[0] & 0x07);
      packContPositiveState =   (rx_frame.data.u8[0] & 0x38) >> 3;
      contactor =               (rx_frame.data.u8[1] & 0x0F);
      packContactorSetState =   (rx_frame.data.u8[1] & 0x0F);
      packCtrsClosingAllowed =  (rx_frame.data.u8[4] & 0x08) >> 3;
      pyroTestInProgress =      (rx_frame.data.u8[4] & 0x20) >> 5;
      hvil_status =             (rx_frame.data.u8[5] & 0x0F);
      break;
    case 0x252:
      //Limits
      regenerative_limit =  ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01; //Example 4715 * 0.01 = 47.15kW
      discharge_limit =     ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.013; //Example 2009 * 0.013 = 26.117???
      max_heat_park =       (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[4]) * 0.01; //Example 500 * 0.01 = 5kW
      hvac_max_power =      (((rx_frame.data.u8[7] << 6) | ((rx_frame.data.u8[6] & 0xFC) >> 2))) * 0.02; //Example 1000 * 0.02 = 20kW?
      break;
    case 0x132:
      //battery amps/volts 
      volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01; //Example 37030mv * 0.01 = 370V
      amps =  ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]); //Example 65492 (-4.3A) OR 225 (22.5A)
      if (amps > 32768)
      {
        amps = - (65535 - amps);
      }
      amps = amps * 0.1;
      raw_amps = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05; //Example 10425 * -0.05 = ?
      battery_charge_time_remaining = (((rx_frame.data.u8[7] & 0x0F) << 8) | rx_frame.data.u8[6]) * 0.1; //Example 228 * 0.1 = 22.8min
      if(battery_charge_time_remaining == 4095)
      {
        battery_charge_time_remaining = 0;
      }

      break;
    case 0x3D2:
      // total charge/discharge kwh
      total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) | (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.001;
      total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.001;
      break;
    case 0x332:
      //min/max hist values
      mux = (rx_frame.data.u8[0] & 0x03);

      if(mux == 1) //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        cell_max_v = temp*2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        cell_min_v = temp*2;
        max_vno = 1 + (rx_frame.data.u8[4] & 0x7F); //This cell has highest voltage
        min_vno = 1 + (rx_frame.data.u8[5] & 0x7F); //This cell has lowest voltage
      }
      if(mux == 0)//Temperature sensors
      {
        temp = rx_frame.data.u8[2];
        max_temp = (temp * 0.5) - 40; //in celcius, Example 24

        temp = rx_frame.data.u8[3];
        min_temp = (temp * 0.5) - 40; //in celcius , Example 24
      }
      break;
    case 0x2d2:
      //Min / max limits
      min_voltage =           ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01 * 2; //Example 24148mv * 0.01 = 241.48 V
      max_voltage =           ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.01 * 2; //Example 40282mv * 0.01 = 402.82 V
      max_charge_current =    (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1; //Example 1301? * 0.1 = 130.1?
      max_discharge_current = (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128; //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:
      low_voltage =     (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]) * 0.0390625;
      high_voltage =    (((rx_frame.data.u8[2] << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2))) * 0.146484;
      output_current =  (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]) / 100;
      break;
    case 0x292:
      soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      soc_vi =  (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));
      soc_max = (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));
      soc_ave = ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));
    break;
    default:
      break;
  }
}
void send_can_tesla_model_3_battery()
{
  unsigned long currentMillis = millis();
  //Send 30ms message
	if (currentMillis - previousMillis30 >= interval30)
	{ 
		previousMillis30 = currentMillis;
    if(bms_status != FAULT){
      ESP32Can.CANWriteFrame(&TESLA_221_1);
      ESP32Can.CANWriteFrame(&TESLA_221_2);
    }
	}
}
