#include "IMIEV-CZERO-ION-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

//Code still work in progress, TODO:
//Figure out if CAN messages need to be sent to keep the system happy?

void ImievCZeroIonBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.real_soc = (uint16_t)(BMU_SOC * 100);  //increase BMU_SOC range from 0-100 -> 100.00

  datalayer.battery.status.voltage_dV = (uint16_t)(BMU_PackVoltage * 10);  // Multiply by 10 and cast to uint16_t

  datalayer.battery.status.current_dA = (BMU_Current * 10);  //Todo, scaling?

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //We do not know the max charge/discharge power is sent by the battery. We hardcode value for now.
  datalayer.battery.status.max_charge_power_W = 10000;  // 10kW   //TODO: Fix when CAN is decoded

  datalayer.battery.status.max_discharge_power_W = 10000;  // 10kW   //TODO: Fix when CAN is decoded

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

  static int m = sizeof(cell_temperatures) / sizeof(cell_temperatures[0]);
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
  datalayer.battery.info.number_of_cells = 88;
  if (max_volt_cel > 2.2) {  // Only update cellvoltage when we have a value
    datalayer.battery.status.cell_max_voltage_mV = (uint16_t)(max_volt_cel * 1000);
  }

  if (min_volt_cel > 2.2) {  // Only update cellvoltage when we have a value
    datalayer.battery.status.cell_min_voltage_mV = (uint16_t)(min_volt_cel * 1000);
  }

  if (min_temp_cel > -49) {  // Only update temperature when we have a value
    datalayer.battery.status.temperature_min_dC = (int16_t)(min_temp_cel * 10);
  }

  if (max_temp_cel > -49) {  // Only update temperature when we have a value
    datalayer.battery.status.temperature_max_dC = (int16_t)(max_temp_cel * 10);
  }

  if (!BMU_Detected) {
#ifdef DEBUG_LOG
    logging.println("BMU not detected, check wiring!");
#endif
  }

#ifdef DEBUG_LOG
  logging.println("Battery Values");
  logging.print("BMU SOC: ");
  logging.print(BMU_SOC);
  logging.print(" BMU Current: ");
  logging.print(BMU_Current);
  logging.print(" BMU Battery Voltage: ");
  logging.print(BMU_PackVoltage);
  logging.print(" BMU_Power: ");
  logging.print(BMU_Power);
  logging.print(" Cell max voltage: ");
  logging.print(max_volt_cel);
  logging.print(" Cell min voltage: ");
  logging.print(min_volt_cel);
  logging.print(" Cell max temp: ");
  logging.print(max_temp_cel);
  logging.print(" Cell min temp: ");
  logging.println(min_temp_cel);
#endif
}

void ImievCZeroIonBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x374:  //BMU message, 10ms - SOC
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temp_value = ((rx_frame.data.u8[1] - 10) / 2);

      if (temp_value == 205) {
        // Use the value we sampled last time
      } else {  // We have a valid value
        if (temp_value < 0) {
          BMU_SOC = 0;
        } else if (temp_value > 100) {
          BMU_SOC = 100;
        } else {  // Between 0-100
          BMU_SOC = temp_value;
        }
      }

      break;
    case 0x373:  //BMU message, 100ms - Pack Voltage and current
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMU_Current = ((((((rx_frame.data.u8[2] * 256.0) + rx_frame.data.u8[3])) - 32768)) * 0.01);
      BMU_PackVoltage = ((rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]) * 0.1);
      BMU_Power = (BMU_Current * BMU_PackVoltage);
      break;
    case 0x6e1:  //BMU message, 25ms - Battery temperatures and voltages
    case 0x6e2:
    case 0x6e3:
    case 0x6e4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMU_Detected = 1;
      //Pid index 0-3
      pid_index = (rx_frame.ID) - 1761;
      //cmu index 1-12: ignore high order nibble which appears to sometimes contain other status bits
      cmu_id = (rx_frame.data.u8[0] & 0x0f);
      //
      if (rx_frame.data.u8[1] != 0) {  // Only update temperatures if value is available
        temp1 = rx_frame.data.u8[1] - 50.0;
      }
      if (rx_frame.data.u8[2] != 0) {
        temp2 = rx_frame.data.u8[1] - 50.0;
      }
      if (rx_frame.data.u8[3] != 0) {
        temp3 = rx_frame.data.u8[1] - 50.0;
      }

      voltage1 = (((rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]) * 0.005) + 2.1);
      voltage2 = (((rx_frame.data.u8[6] * 256.0 + rx_frame.data.u8[7]) * 0.005) + 2.1);

      voltage_index = ((cmu_id - 1) * 8 + (2 * pid_index));
      temp_index = ((cmu_id - 1) * 6 + (2 * pid_index));
      if (cmu_id >= 7) {
        voltage_index -= 4;
        temp_index -= 3;
      }

      if (voltage1 > 2.2) {  // Only update cellvoltages incase we have a value
        cell_voltages[voltage_index] = voltage1;
      }
      if (voltage2 > 2.2) {
        cell_voltages[voltage_index + 1] = voltage2;
      }

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

void ImievCZeroIonBattery::transmit_can(unsigned long currentMillis) {

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    // Send CAN goes here...
  }
}

void ImievCZeroIonBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
