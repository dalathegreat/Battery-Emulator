#include "IMIEV-CZERO-ION-BATTERY.h"
#include "../devboard/can/ESP32CAN.h"
#include "../lib/ThomasBarth-ESP32-CAN-Driver/CAN_config.h"

//Code still work in progress, TODO:
//Figure out if CAN messages need to be sent to keep the system happy?

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
static uint8_t BMU_SOC = 0;
static int temp_value = 0;
static double temp1 = 0;
static double temp2 = 0;
static double temp3 = 0;
static double voltage1 = 0;
static double voltage2 = 0;
static double BMU_Current = 0;
static double BMU_PackVoltage = 0;
static double BMU_Power = 0;
static double cell_voltages[89]; //array with all the cellvoltages //Todo, what is max array size? 80/88 cells?
static double cell_temperatures[89]; //array with all the celltemperatures //Todo, what is max array size? 80/88cells?
static double max_volt_cel = 3.70;
static double min_volt_cel = 3.70;
static double max_temp_cel = 20.00;
static double min_temp_cel = 19.00;


void update_values_imiev_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE; //Startout in active mode

  SOC = (uint16_t)(BMU_SOC * 100); //increase BMU_SOC range from 0-100 -> 100.00

  battery_voltage = (uint16_t)(BMU_PackVoltage * 10); // Multiply by 10 and cast to uint16_t

  battery_current = (BMU_Current*10); //Todo, scaling?

  capacity_Wh = BATTERY_WH_MAX; //Hardcoded to header value

  remaining_capacity_Wh = (uint16_t)((SOC/10000)*capacity_Wh);

  //We do not know if the max charge power is sent by the battery. So we estimate the value based on SOC%
	if(SOC == 10000){ //100.00%
    max_target_charge_power = 0; //When battery is 100% full, set allowed charge W to 0
  }
  else{
    max_target_charge_power = 10000; //Otherwise we can push 10kW into the pack!
  }

  if(SOC < 200){ //2.00%
    max_target_discharge_power = 0; //When battery is empty (below 2%), set allowed discharge W to 0
  }
  else{
    max_target_discharge_power = 10000; //Otherwise we can discharge 10kW from the pack!
  }

  stat_batt_power = BMU_Power; //TODO, Scaling?
  
  static int n = sizeof(cell_voltages) / sizeof(cell_voltages[0]);
  max_volt_cel = cell_voltages[0]; // Initialize max with the first element of the array
    for (int i = 1; i < n; i++) {
      if (cell_voltages[i] > max_volt_cel) {
          max_volt_cel = cell_voltages[i];  // Update max if we find a larger element
      }
  }
  min_volt_cel = cell_voltages[0]; // Initialize min with the first element of the array
    for (int i = 1; i < n; i++) {
      if (cell_voltages[i] < min_volt_cel) {
          min_volt_cel = cell_voltages[i];  // Update min if we find a smaller element
      }
  }

  static int m = sizeof(cell_voltages) / sizeof(cell_temperatures[0]);
  max_temp_cel = cell_temperatures[0]; // Initialize max with the first element of the array
    for (int i = 1; i < m; i++) {
      if (cell_temperatures[i] > max_temp_cel) {
          max_temp_cel = cell_temperatures[i];  // Update max if we find a larger element
      }
  }
  min_temp_cel = cell_temperatures[0]; // Initialize min with the first element of the array
    for (int i = 1; i < m; i++) {
      if (cell_temperatures[i] < min_temp_cel) {
          if(min_temp_cel != -50.00){ //-50.00 means this sensor not connected
            min_temp_cel = cell_temperatures[i];  // Update max if we find a smaller element
          }
      }
  }
  
  cell_max_voltage = (uint16_t)(max_volt_cel*1000);

  cell_min_voltage = (uint16_t)(min_volt_cel*1000);

  temperature_min = (uint16_t)(min_temp_cel*1000);

  temperature_max = (uint16_t)(max_temp_cel*1000);

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

  if(!BMU_Detected){
    Serial.println("BMU not detected, check wiring!");
  }
	
  #ifdef DEBUG_VIA_USB

    Serial.println("Battery Values");
    Serial.print("BMU SOC: ");
    Serial.print(BMU_SOC);
    Serial.print(" BMU Current: ");
    Serial.print(BMU_Current);
    Serial.print(" BMU Battery Voltage: ");
    Serial.print(BMU_PackVoltage);
    Serial.print(" BMU_Power: ");
    Serial.print(BMU_Power);
    Serial.print(" Cell max voltage: ");
    Serial.print(max_volt_cel);
    Serial.print(" Cell min voltage: ");
    Serial.print(min_volt_cel);
	  Serial.print(" Cell max temp: ");
    Serial.print(max_temp_cel);
    Serial.print(" Cell min temp: ");
    Serial.println(min_temp_cel);
	
	Serial.println("Values sent to inverter");
	Serial.print("SOC% (0-100.00): ");
	Serial.print(SOC);
	Serial.print(" Voltage (0-400.0): ");
	Serial.print(battery_voltage);
	Serial.print(" Capacity WH full (0-60000): ");
	Serial.print(capacity_Wh);
	Serial.print(" Capacity WH remain (0-60000): ");
	Serial.print(remaining_capacity_Wh);
	Serial.print(" Max charge power W (0-10000): ");
	Serial.print(max_target_charge_power);
	Serial.print(" Max discharge power W (0-10000): ");
	Serial.print(max_target_discharge_power);
	Serial.print(" Temp max ");
	Serial.print(temperature_max);
	Serial.print(" Temp min ");
	Serial.print(temperature_min);
	Serial.print(" Cell mV max ");
	Serial.print(cell_max_voltage);
	Serial.print(" Cell mV min ");
	Serial.print(cell_min_voltage);
	
  #endif
}

void receive_can_imiev_battery(CAN_frame_t rx_frame)
{
CANstillAlive = 12; //Todo, move this inside a known message ID to prevent CAN inverter from keeping battery alive detection going
  switch (rx_frame.MsgID)
  {
  case 0x374: //BMU message, 10ms - SOC
    temp_value = ((rx_frame.data.u8[1] - 10) / 2);
    if(temp_value >= 0 && temp_value <= 101){
      BMU_SOC = temp_value;
    }
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
