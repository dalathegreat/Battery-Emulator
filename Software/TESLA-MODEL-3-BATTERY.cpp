#include "TESLA-MODEL-3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
static unsigned long previousMillis30 = 0; // will store last time a 30ms CAN Message was send
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval30 = 30; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages
static uint8_t stillAliveCAN = 6; //counter for checking if CAN is still alive

CAN_frame_t TESLA_221_1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x40, 0x41, 0x05, 0x15, 0x00, 0x50, 0x71, 0x7f}};
CAN_frame_t TESLA_221_2 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x60, 0x55, 0x55, 0x15, 0x54, 0x51, 0xd1, 0xb8}};
CAN_frame_t TESLA_332_1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x332,.data = {0x61, 0x05, 0x55, 0x55, 0x00, 0x00, 0xe0, 0x13}};
CAN_frame_t TESLA_332_2 = {.FIR = {.B = {.DLC = 6,.FF = CAN_frame_std,}},.MsgID = 0x332,.data = {0x3C, 0x0C, 0x85, 0x80, 0x89, 0x86}};
uint8_t alternate221 = 0;
uint8_t alternate332 = 0;

uint16_t volts = 0;   // V
uint16_t amps = 0;    // A
uint16_t raw_amps = 0;// A
int16_t max_temp = 6; // C*
int16_t min_temp = 5; // C*
uint16_t energy_buffer = 0;
uint16_t energy_to_charge_complete = 0;
uint16_t expected_energy_remaining = 0;
uint8_t full_charge_complete = 0;
uint16_t ideal_energy_remaining = 0;
uint16_t nominal_energy_remaining = 0;
uint16_t nominal_full_pack_energy = 0;
uint16_t battery_charge_time_remaining = 0; // Minutes
uint16_t regenerative_limit = 0;
uint16_t discharge_limit = 0;
uint16_t max_heat_park = 0;
uint16_t hvac_max_power = 0;
float calculated_soc_float = 0;
uint16_t calculated_soc = 0;
uint16_t min_voltage = 0;
uint16_t max_discharge_current = 0;
uint16_t max_charge_current = 0;
uint16_t max_voltage = 0;
uint8_t contactor = 0; //State of contactor
uint8_t hvil_status = 0;
uint8_t packContNegativeState = 0;
uint8_t packContPositiveState = 0;
uint8_t packContactorSetState = 0;
uint8_t packCtrsClosingAllowed = 0;
uint8_t pyroTestInProgress = 0;
const char* contactorText[] = {"UNKNOWN0","OPEN","CLOSING","BLOCKED","OPENING","CLOSED","UNKNOWN6","WELDED","POS_CL","NEG_CL","UNKNOWN10","UNKNOWN11","UNKNOWN12"};
const char* contactorState[] = {"SNA","OPEN","PRECHARGE","BLOCKED","PULLED_IN","OPENING","ECONOMIZED","WELDED"};


void update_values_tesla_model_3_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
	StateOfHealth = 9900; //Hardcoded to 99%SOH
	
	SOC = calculated_soc;

	battery_voltage;

  battery_current;

	capacity_Wh = (nominal_full_pack_energy * 100); //Scale up 75.2kWh -> 75200Wh
  if(capacity_Wh > 60000)
  {
    capacity_Wh = 60000;
  }

	remaining_capacity_Wh = (expected_energy_remaining * 100); //Scale up 60.3kWh -> 60300Wh

	max_target_discharge_power;

	max_target_charge_power;
	
	stat_batt_power;

	temperature_min = convert2unsignedint16(min_temp);

	temperature_max = convert2unsignedint16(max_temp);

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

  if(printValues)
  {
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


    Serial.print("Volt: ");
    Serial.print(volts);
    Serial.print(", Discharge limit: ");
    Serial.print(discharge_limit);
    Serial.print(", Charge limit: ");
    Serial.print(regenerative_limit);
    Serial.print(", Max temp: ");
    Serial.print(max_temp);
    Serial.print(", Min temp: ");
    Serial.print(max_temp);
    Serial.print(", Nominal full energy: ");
    Serial.print(nominal_full_pack_energy);
    Serial.print(", Nominal energy remain: ");
    Serial.print(nominal_energy_remaining);
    Serial.print(", Expected energy remain: ");
    Serial.print(expected_energy_remaining);
    Serial.print(", Ideal energy remaining: ");
    Serial.print(ideal_energy_remaining);
    Serial.print(", Energy to charge complete: ");
    Serial.print(energy_to_charge_complete);
    Serial.print(", Energy buffer: ");
    Serial.print(energy_buffer);
    Serial.print(", Fully charged?: ");
    Serial.println(full_charge_complete);
    Serial.print("SOC: ");
    Serial.println(calculated_soc);

    Serial.print("Min voltage: ");
    Serial.print(min_voltage);
    Serial.print(", Max voltage: ");
    Serial.print(max_voltage);
    Serial.print(", Max charge current: ");
    Serial.print(max_charge_current);
    Serial.print(", Max discharge current: ");
    Serial.println(max_discharge_current);

  }
}

void handle_can_tesla_model_3_battery()
{
  CAN_frame_t rx_frame;
  unsigned long currentMillis = millis();
  static int mux = 0;
  static int temp = 0;

  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
	if (rx_frame.FIR.B.FF == CAN_frame_std)
	{
    stillAliveCAN = 6; //We got CAN-messages flowing in!
		//printf("New standard frame");
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
        full_charge_complete =      (rx_frame.data.u8[7] & 0x80);

        if(nominal_full_pack_energy > 0)
        { //Avoid division by 0
          calculated_soc_float = ((float)expected_energy_remaining / nominal_full_pack_energy) * 10000;
          calculated_soc = calculated_soc_float;
        }
        else
        {
          calculated_soc = 0;
        }
				break;
      case 0x20A:
        //Contactor state
        packContNegativeState =   (rx_frame.data.u8[0] & 0x07);
        packContPositiveState =   (rx_frame.data.u8[0] & 0x38) >> 3;
        contactor =               (rx_frame.data.u8[1] & 0x0F);
        packContactorSetState =   (rx_frame.data.u8[1] & 0x0F);
        packCtrsClosingAllowed =  (rx_frame.data.u8[4] & 0x38) >> 3; 
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
        volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01; //Example 37030mv * 0.01 = 370.3V
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
        break;
      case 0x332:
        //min/max hist values
        mux = rx_frame.data.u8[0];
        mux = mux & 0x03;

        if(mux == 1) //Cell voltages
        {
          //todo handle cell voltages
          //not required by the Gen24, but nice stats located here!
        }
        if(mux == 0)//Temperature sensors
        {
          temp = rx_frame.data.u8[2];
          max_temp = (temp * 0.5) - 40; //in celcius, Example 24

          temp = rx_frame.data.u8[3];
          min_temp = (volts * 0.5) - 40; //in celcius , Example 24
        }
        break;
      case 0x2d2:
        //Min / max limits
        min_voltage =           ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01 * 2; //Example 24148mv * 0.01 = 241.48 V
        max_voltage =           ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.01 * 2; //Example 40282mv * 0.01 = 402.82 V
        max_charge_current =    (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1; //Example 1301? * 0.1 = 130.1?
        max_discharge_current = (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128; //Example 430? * 0.128 = 55.4?
        break;
			default:
				break;
			}
		}
		else
		{
		  //printf("New extended frame");
		}
	}
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

		//ESP32Can.CANWriteFrame(&message100);

	}
  //Send 30ms message
	if (currentMillis - previousMillis30 >= interval30)
	{ 
		previousMillis10 = currentMillis;

    if(packCtrsClosingAllowed > 0) //Command contactor to close
    {
      alternate221++;
      if (alternate221 > 1)
      {
        alternate221 = 0;
      }
      if(alternate221 == 0)
      {
        ESP32Can.CANWriteFrame(&TESLA_221_1);
      }
      else //alternate221 == 1
      {
        ESP32Can.CANWriteFrame(&TESLA_221_2);
      }
    }
	}
	//Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

		//ESP32Can.CANWriteFrame(&message10); 
	}
}
