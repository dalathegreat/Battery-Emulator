#include "IMIEV-CZERO-ION-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

//Code still very WIP
//TODO: Add CMU warning incase we detect a direct connection to the CMUs. It wont be safe to use the battery in this mode
//Map the missing values
//Check scaling of all values
//Generate messages towards BMU to keep it happy?

/* Do not change code below unless you are sure what you are doing */
#define BMU_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to inverter
#define BMU_MIN_SOC 0   //BMS never goes below this value. We use this info to rescale SOC% sent to inverter
static uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 
static uint8_t errorCode = 0; //stores if we have an error code active from battery control logic
static uint8_t BMU_Detected = 0;
static uint8_t CMU_Detected = 0;

static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was sent
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages

static int pid_index = 0;
static int cmu_id = 0;
static int voltage_index = 0;
static int temp_index = 0;
static int BMU_SOC = 0;
static double temp1 = 0;
static double temp2 = 0;
static double temp3 = 0;
static double voltage1 = 0;
static double voltage2 = 0;
static double BMU_Current = 0;
static double BMU_PackVoltage = 0;
static double BMU_Power = 0;
static uint16_t cell_voltages[89]; //array with all the cellvoltages //Todo, what is max array size? 80/88 cells?
static uint16_t cell_temperatures[89]; //array with all the celltemperatures //Todo, what is max array size? 80/88cells?


void update_values_imiev_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE; //Startout in active mode

  SOC = BMU_SOC; //Todo, scaling?

  battery_voltage = (BMU_PackVoltage*10); //Todo, scaling?

  battery_current = (BMU_Current*10); //Todo, scaling?

  capacity_Wh = BATTERY_WH_MAX; //Hardcoded to header value

  remaining_capacity_Wh = (SOC/100)*capacity_Wh;

  max_target_charge_power; //TODO, map

  max_target_discharge_power; //TODO, map

  stat_batt_power = BMU_Power; //TODO, Scaling?

  temperature_min; //TODO, map

  temperature_max; //TODO, map

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if(!CANstillAlive)
  {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  }
  else
  {
    CANstillAlive--;
  }
	
  #ifdef DEBUG_VIA_USB
    Serial.print("BMU SOC: ");
    Serial.println(BMU_SOC);
    Serial.print("BMU Current: ");
    Serial.println(BMU_Current);
    Serial.print("BMU Battery Voltage: ");
    Serial.println(BMU_PackVoltage);
  #endif
}

void receive_can_imiev_battery(CAN_frame_t rx_frame)
{
CANstillAlive = 12; //Todo, move this inside a known message ID to prevent CAN inverter from keeping battery alive detection going
  switch (rx_frame.MsgID)
  {
  case 0x374: //BMU message, 10ms - SOC
    BMU_SOC = ((rx_frame.data.u8[1] - 10) / 2);
    break;
  case 0x373: //BMU message, 100ms - Pack Voltage and current
    BMU_Current =  ((((((rx_frame.data.u8[2] * 256.0) + rx_frame.data.u8[3])) - 32768)) * 0.01);
    BMU_PackVoltage = ((rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]) * 0.1);
    BMU_Power = (BMU_Current * BMU_PackVoltage);
    break;
  case 0x6e1: //BMU message, 25ms - Battery temperatures and voltages
  case 0x6e2:
  case 0x6e3:
  case 0x6e4:
    BMU_Detected = 1;
    //Pid index 0-3
    pid_index = (rx_frame.MsgID) - 1761;
    //cmu index 1-12: ignore high order nibble which appears to sometimes contain other status bits
    cmu_id = (rx_frame.data.u8[0] & 0x0f);
    //
    temp1 = rx_frame.data.u8[1] - 50.0;
    temp2 = rx_frame.data.u8[2] - 50.0;
    temp3 = rx_frame.data.u8[3] - 50.0;

    voltage1 = (((rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]) * 0.005) + 2.1);
    voltage2 = (((rx_frame.data.u8[6] * 256.0 + rx_frame.data.u8[7]) * 0.005) + 2.1);

    voltage_index = ((cmu_id - 1) * 8 + (2 * pid_index));
    temp_index = ((cmu_id - 1) * 6 + (2 * pid_index));
    if (cmu_id >= 7)
    {
      voltage_index -= 4;
      temp_index -= 3;
    }
    cell_voltages[voltage_index] = voltage1;
    cell_voltages[voltage_index + 1] = voltage2;

    if (pid_index == 0)
    {
      cell_temperatures[temp_index] = temp2;
      cell_temperatures[temp_index + 1] = temp3;
    }
    else
    {
      cell_temperatures[temp_index] = temp1;
      if (cmu_id != 6 && cmu_id != 12)
      {
        cell_temperatures[temp_index + 1] = temp2;
      }
    }
    break;
  default:
    break;
  }
}

void send_can_imiev_battery()
{
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;
  }
}
