#include "../include.h"
#ifdef IMIEV_CZERO_ION_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "IMIEV-CZERO-ION-BATTERY.h"

//Code still work in progress, TODO:
//Figure out if CAN messages need to be sent to keep the system happy?

/* Do not change code below unless you are sure what you are doing */
static uint8_t CANstillAlive = 12;  //counter for checking if CAN is still alive
static uint8_t errorCode = 0;       //stores if we have an error code active from battery control logic
static uint8_t BMU_Detected = 0;
static uint8_t CMU_Detected = 0;

static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent

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
static double cell_voltages[88];      //array with all the cellvoltages
static double cell_temperatures[88];  //array with all the celltemperatures
static double max_volt_cel = 3.70;
static double min_volt_cel = 3.70;
static double max_temp_cel = 20.00;
static double min_temp_cel = 19.00;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.real_soc = (uint16_t)(BMU_SOC * 100);  //increase BMU_SOC range from 0-100 -> 100.00

  datalayer.battery.status.voltage_dV = (uint16_t)(BMU_PackVoltage * 10);  // Multiply by 10 and cast to uint16_t

  datalayer.battery.status.current_dA = (BMU_Current * 10);  //Todo, scaling?

  datalayer.battery.info.total_capacity_Wh = BATTERY_WH_MAX;  //Hardcoded to header value

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //We do not know if the max charge power is sent by the battery. So we estimate the value based on SOC%
  if (datalayer.battery.status.reported_soc == 10000) {  //100.00%
    datalayer.battery.status.max_charge_power_W = 0;     //When battery is 100% full, set allowed charge W to 0
  } else {
    datalayer.battery.status.max_charge_power_W = 10000;  //Otherwise we can push 10kW into the pack!
  }

  if (datalayer.battery.status.reported_soc < 200) {  //2.00%
    datalayer.battery.status.max_discharge_power_W =
        0;  //When battery is empty (below 2%), set allowed discharge W to 0
  } else {
    datalayer.battery.status.max_discharge_power_W = 10000;  //Otherwise we can discharge 10kW from the pack!
  }

  datalayer.battery.status.active_power_W = BMU_Power;  //TODO: Scaling?

  static int n = sizeof(cell_voltages) / sizeof(cell_voltages[0]);
  max_volt_cel = cell_voltages[0];  // Initialize max with the first element of the array
  for (int i = 1; i < n; i++) {
    if (cell_voltages[i] > max_volt_cel) {
      max_volt_cel = cell_voltages[i];  // Update max if we find a larger element
    }
  }
  min_volt_cel = cell_voltages[0];  // Initialize min with the first element of the array
  for (int i = 1; i < n; i++) {
    if (cell_voltages[i] < min_volt_cel) {
      min_volt_cel = cell_voltages[i];  // Update min if we find a smaller element
    }
  }

  static int m = sizeof(cell_voltages) / sizeof(cell_temperatures[0]);
  max_temp_cel = cell_temperatures[0];  // Initialize max with the first element of the array
  for (int i = 1; i < m; i++) {
    if (cell_temperatures[i] > max_temp_cel) {
      max_temp_cel = cell_temperatures[i];  // Update max if we find a larger element
    }
  }
  min_temp_cel = cell_temperatures[0];  // Initialize min with the first element of the array
  for (int i = 1; i < m; i++) {
    if (cell_temperatures[i] < min_temp_cel) {
      if (min_temp_cel != -50.00) {           //-50.00 means this sensor not connected
        min_temp_cel = cell_temperatures[i];  // Update max if we find a smaller element
      }
    }
  }

  //Map all cell voltages to the global array
  for (int i = 0; i < 88; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = (uint16_t)(cell_voltages[i] * 1000);
  }

  datalayer.battery.status.cell_max_voltage_mV = (uint16_t)(max_volt_cel * 1000);

  datalayer.battery.status.cell_min_voltage_mV = (uint16_t)(min_volt_cel * 1000);

  datalayer.battery.status.temperature_min_dC = (int16_t)(min_temp_cel * 10);

  datalayer.battery.status.temperature_min_dC = (int16_t)(max_temp_cel * 10);

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  if (!BMU_Detected) {
#ifdef DEBUG_VIA_USB
    Serial.println("BMU not detected, check wiring!");
#endif
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
  Serial.print(datalayer.battery.status.reported_soc);
  Serial.print(" Voltage (0-400.0): ");
  Serial.print(datalayer.battery.status.voltage_dV);
  Serial.print(" Capacity WH full (0-60000): ");
  Serial.print(datalayer.battery.info.total_capacity_Wh);
  Serial.print(" Capacity WH remain (0-60000): ");
  Serial.print(datalayer.battery.status.remaining_capacity_Wh);
  Serial.print(" Max charge power W (0-10000): ");
  Serial.print(datalayer.battery.status.max_charge_power_W);
  Serial.print(" Max discharge power W (0-10000): ");
  Serial.print(datalayer.battery.status.max_discharge_power_W);
  Serial.print(" Temp max ");
  Serial.print(datalayer.battery.status.temperature_max_dC);
  Serial.print(" Temp min ");
  Serial.print(datalayer.battery.status.temperature_min_dC);
  Serial.print(" Cell mV max ");
  Serial.print(datalayer.battery.status.cell_max_voltage_mV);
  Serial.print(" Cell mV min ");
  Serial.print(datalayer.battery.status.cell_min_voltage_mV);

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  CANstillAlive =
      12;  //TODO: move this inside a known message ID to prevent CAN inverter from keeping battery alive detection going
  switch (rx_frame.MsgID) {
    case 0x374:  //BMU message, 10ms - SOC
      temp_value = ((rx_frame.data.u8[1] - 10) / 2);
      if (temp_value >= 0 && temp_value <= 101) {
        BMU_SOC = temp_value;
      }
      break;
    case 0x373:  //BMU message, 100ms - Pack Voltage and current
      BMU_Current = ((((((rx_frame.data.u8[2] * 256.0) + rx_frame.data.u8[3])) - 32768)) * 0.01);
      BMU_PackVoltage = ((rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]) * 0.1);
      BMU_Power = (BMU_Current * BMU_PackVoltage);
      break;
    case 0x6e1:  //BMU message, 25ms - Battery temperatures and voltages
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
      if (cmu_id >= 7) {
        voltage_index -= 4;
        temp_index -= 3;
      }
      cell_voltages[voltage_index] = voltage1;
      cell_voltages[voltage_index + 1] = voltage2;

      if (pid_index == 0) {
        cell_temperatures[temp_index] = temp2;
        cell_temperatures[temp_index + 1] = temp3;
      } else {
        cell_temperatures[temp_index] = temp1;
        if (cmu_id != 6 && cmu_id != 12) {
          cell_temperatures[temp_index + 1] = temp2;
        }
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis100 >= INTERVAL_100_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis100));
    }
    previousMillis100 = currentMillis;

    // Send CAN goes here...
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Mitsubishi i-MiEV / Citroen C-Zero / Peugeot Ion battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV =
      3600;  // 360.0V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 3160;  // 316.0V under this, discharging further is disabled
}

#endif
