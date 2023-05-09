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

CAN_frame_t TESLA_221 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x40, 0x41, 0x05, 0x15, 0x00, 0x50, 0x71, 0x7f}};

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
	
	SOC;

	battery_voltage;

  battery_current;

	capacity_Wh;

	remaining_capacity_Wh;

	max_target_discharge_power;

	max_target_charge_power;
	
	stat_batt_power;

	temperature_min = min_temp;

	temperature_max = max_temp;

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
        energy_buffer = (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x80) >> 7)) * 0.1;
        energy_to_charge_complete = (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 0.1;
        expected_energy_remaining = (((rx_frame.data.u8[4] & 0x01) << 10) | rx_frame.data.u8[3] << 2 | ((rx_frame.data.u8[2] & 0xC0) >> 6)) * 0.1;
        full_charge_complete = rx_frame.data.u8[7] & 0x80;
        //ideal_energy_remaining = extract_k_bits(msg.data, 33, 11, True) * 0.1
        //nominal_energy_remaining = extract_k_bits(msg.data, 11, 11, True) * 0.1
        //nominal_full_pack_energy = extract_k_bits(msg.data, 0, 11, True) * 0.1

        //soc = round(expected_energy_remaining / nominal_full_pack_energy * 100)

				break;
      case 0x20A:
        //Contactor state
        contactor = rx_frame.data.u8[1] & 0x0F;
        hvil_status = rx_frame.data.u8[5] & 0x0F;
        packContNegativeState = rx_frame.data.u8[0] & 0x07;
        packContPositiveState = (rx_frame.data.u8[0] & 0x38) >> 3;
        packContactorSetState = rx_frame.data.u8[1] & 0x0F;
        packCtrsClosingAllowed = (rx_frame.data.u8[4] & 0x38) >> 3; 
        pyroTestInProgress = (rx_frame.data.u8[4] & 0x20) >> 5;
        break;
      case 0x252:
        //Limits
        regenerative_limit = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01; //TODO, check if message is properly joined 
        discharge_limit = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.013; //TODO, check if message is properly joined
        max_heat_park = (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[4]) * 0.01; //TODO, check if message is properly joined
        hvac_max_power = (((rx_frame.data.u8[7] << 6) | ((rx_frame.data.u8[6] & 0xFC) >> 2))) * 0.02; //TODO, check if message is properly joined

        break;
      case 0x132:
        //battery amps/volts 
        volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01; //TODO, check if message is properly joined
        amps = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]); //TODO, check if message is properly joined
        if (amps > 32768)
        {
          amps = - (65535 - amps);
        }
        amps = amps * 0.1;
        raw_amps = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05; //TODO, check if message is properly joined
        battery_charge_time_remaining = (((rx_frame.data.u8[7] & 0x0F) << 8) | rx_frame.data.u8[6]) * 0.1; //TODO, check if message is properly joined
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
        }
        if(mux == 0)//Temperature sensors
        {
          temp = rx_frame.data.u8[2];
          max_temp = (temp * 0.5) - 40; //in celcius

          temp = rx_frame.data.u8[3];
          min_temp = (volts * 0.5) - 40; //in celcius
        }
        break;
      case 0x401:
        //cell voltages 
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

    if(packCtrsClosingAllowed)
    {
      //Command contactor to close
      ESP32Can.CANWriteFrame(&TESLA_221);
    }
	}
	//Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

		//ESP32Can.CANWriteFrame(&message10); 
	}
}
