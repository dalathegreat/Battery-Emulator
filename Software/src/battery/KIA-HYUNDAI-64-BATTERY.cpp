#include "../include.h"
#ifdef KIA_HYUNDAI_64_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "KIA-HYUNDAI-64-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10 = 0;   // will store last time a 10s CAN Message was send

static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 1000;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint16_t allowedDischargePower = 0;
static uint16_t allowedChargePower = 0;
static uint16_t batteryVoltage = 0;
static uint16_t inverterVoltageFrameHigh = 0;
static uint16_t inverterVoltage = 0;
static uint16_t cellvoltages_mv[98];
static int16_t leadAcidBatteryVoltage = 120;
static int16_t batteryAmps = 0;
static int16_t temperatureMax = 0;
static int16_t temperatureMin = 0;
static int16_t poll_data_pid = 0;
static bool holdPidCounter = false;
static uint8_t CellVmaxNo = 0;
static uint8_t CellVminNo = 0;
static uint8_t batteryManagementMode = 0;
static uint8_t BMS_ign = 0;
static uint8_t batteryRelay = 0;
static uint8_t waterleakageSensor = 164;
static uint8_t counter_200 = 0;
static int8_t temperature_water_inlet = 0;
static int8_t heatertemp = 0;
static int8_t powerRelayTemperature = 0;
static bool startedUp = false;

#ifdef DOUBLE_BATTERY
static uint8_t counter_200_2 = 0;
static uint16_t battery2_soc_calculated = 0;
static uint16_t battery2_SOC_BMS = 0;
static uint16_t battery2_SOC_Display = 0;
static uint16_t battery2_batterySOH = 1000;
static uint16_t battery2_CellVoltMax_mV = 3700;
static uint16_t battery2_CellVoltMin_mV = 3700;
static uint16_t battery2_allowedDischargePower = 0;
static uint16_t battery2_allowedChargePower = 0;
static uint16_t battery2_batteryVoltage = 0;
static uint16_t battery2_inverterVoltageFrameHigh = 0;
static uint16_t battery2_inverterVoltage = 0;
static uint16_t battery2_cellvoltages_mv[98];
static int16_t battery2_leadAcidBatteryVoltage = 120;
static int16_t battery2_batteryAmps = 0;
static int16_t battery2_temperatureMax = 0;
static int16_t battery2_temperatureMin = 0;
static int16_t battery2_poll_data_pid = 0;
static bool battery2_holdPidCounter = false;
static uint8_t battery2_CellVmaxNo = 0;
static uint8_t battery2_CellVminNo = 0;
static uint8_t battery2_batteryManagementMode = 0;
static uint8_t battery2_BMS_ign = 0;
static uint8_t battery2_batteryRelay = 0;
static uint8_t battery2_waterleakageSensor = 164;
static uint8_t battery2_counter_200 = 0;
static int8_t battery2_temperature_water_inlet = 0;
static int8_t battery2_heatertemp = 0;
static int8_t battery2_powerRelayTemperature = 0;
static bool battery2_startedUp = false;
CAN_frame KIA_HYUNDAI_200_2 = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x200,
                               .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};  //2nd battery
#endif                                                                                     //DOUBLE_BATTERY

CAN_frame KIA_HYUNDAI_200 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x200,
                             .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};
CAN_frame KIA_HYUNDAI_523 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x523,
                             .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}};
CAN_frame KIA_HYUNDAI_524 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x524,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
//553 Needed frame 200ms
CAN_frame KIA64_553 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x553,
                       .data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
//57F Needed frame 100ms
CAN_frame KIA64_57F = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x57F,
                       .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
//Needed frame 100ms
CAN_frame KIA64_2A1 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x2A1,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA64_7E4_id1 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 01
CAN_frame KIA64_7E4_id2 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 02
CAN_frame KIA64_7E4_id3 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 03
CAN_frame KIA64_7E4_id4 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 04
CAN_frame KIA64_7E4_id5 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 05
CAN_frame KIA64_7E4_id6 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x03, 0x22, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 06
CAN_frame KIA64_7E4_ack = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x7E4,
    .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Ack frame, correct PID is returned

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W = allowedChargePower * 10;

  datalayer.battery.status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer.battery.status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Update webserver datalayer
  datalayer_extended.KiaHyundai64.total_cell_count = datalayer.battery.info.number_of_cells;
  datalayer_extended.KiaHyundai64.battery_12V = leadAcidBatteryVoltage;
  datalayer_extended.KiaHyundai64.waterleakageSensor = waterleakageSensor;
  datalayer_extended.KiaHyundai64.temperature_water_inlet = temperature_water_inlet;
  datalayer_extended.KiaHyundai64.powerRelayTemperature = powerRelayTemperature * 2;
  datalayer_extended.KiaHyundai64.batteryManagementMode = batteryManagementMode;
  datalayer_extended.KiaHyundai64.BMS_ign = BMS_ign;
  datalayer_extended.KiaHyundai64.batteryRelay = batteryRelay;

  //Perform logging if configured to do so
#ifdef DEBUG_LOG
  logging.println();  //sepatator
  logging.println("Values from battery: ");
  logging.print("SOC BMS: ");
  logging.print((uint16_t)SOC_BMS / 10.0, 1);
  logging.print("%  |  SOC Display: ");
  logging.print((uint16_t)SOC_Display / 10.0, 1);
  logging.print("%  |  SOH ");
  logging.print((uint16_t)batterySOH / 10.0, 1);
  logging.println("%");
  logging.print((int16_t)batteryAmps / 10.0, 1);
  logging.print(" Amps  |  ");
  logging.print((uint16_t)batteryVoltage / 10.0, 1);
  logging.print(" Volts  |  ");
  logging.print((int16_t)datalayer.battery.status.active_power_W);
  logging.println(" Watts");
  logging.print("Allowed Charge ");
  logging.print((uint16_t)allowedChargePower * 10);
  logging.print(" W  |  Allowed Discharge ");
  logging.print((uint16_t)allowedDischargePower * 10);
  logging.println(" W");
  logging.print("MaxCellVolt ");
  logging.print(CellVoltMax_mV);
  logging.print(" mV  No  ");
  logging.print(CellVmaxNo);
  logging.print("  |  MinCellVolt ");
  logging.print(CellVoltMin_mV);
  logging.print(" mV  No  ");
  logging.println(CellVminNo);
  logging.print("TempHi ");
  logging.print((int16_t)temperatureMax);
  logging.print("°C  TempLo ");
  logging.print((int16_t)temperatureMin);
  logging.print("°C  WaterInlet ");
  logging.print((int8_t)temperature_water_inlet);
  logging.print("°C  PowerRelay ");
  logging.print((int8_t)powerRelayTemperature * 2);
  logging.println("°C");
  logging.print("Aux12volt: ");
  logging.print((int16_t)leadAcidBatteryVoltage / 10.0, 1);
  logging.println("V  |  ");
  logging.print("BmsManagementMode ");
  logging.print((uint8_t)batteryManagementMode, BIN);
  if (bitRead((uint8_t)BMS_ign, 2) == 1) {
    logging.print("  |  BmsIgnition ON");
  } else {
    logging.print("  |  BmsIgnition OFF");
  }

  if (bitRead((uint8_t)batteryRelay, 0) == 1) {
    logging.print("  |  PowerRelay ON");
  } else {
    logging.print("  |  PowerRelay OFF");
  }
  logging.print("  |  Inverter ");
  logging.print(inverterVoltage);
  logging.println(" Volts");
#endif
}

void update_number_of_cells() {
  //If we have cell values and number_of_cells not initialized yet
  if (cellvoltages_mv[0] > 0 && datalayer.battery.info.number_of_cells == 0) {
    // Check if we have 98S or 90S battery
    if (datalayer.battery.status.cell_voltages_mV[97] > 0) {
      datalayer.battery.info.number_of_cells = 98;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_98S_DV;
      datalayer.battery.info.total_capacity_Wh = 64000;
    } else {
      datalayer.battery.info.number_of_cells = 90;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_90S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;
      datalayer.battery.info.total_capacity_Wh = 40000;
    }
  }
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4DE:
      startedUp = true;
      break;
    case 0x542:  //BMS SOC
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      startedUp = true;
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      startedUp = true;
      batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (counter_200 > 3) {
        KIA_HYUNDAI_524.data.u8[0] = (uint8_t)(batteryVoltage / 10);
        KIA_HYUNDAI_524.data.u8[1] = (uint8_t)((batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms
      break;
    case 0x596:
      startedUp = true;
      leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      startedUp = true;
      break;
    case 0x5D5:
      startedUp = true;
      waterleakageSensor = rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      startedUp = true;

      //PID data is polled after last message sent from battery every other time:
      if (holdPidCounter == true) {
        holdPidCounter = false;
      } else {
        holdPidCounter = true;
        if (poll_data_pid >= 6) {  //polling one of six PIDs at 100ms*2, resolution = 1200ms
          poll_data_pid = 0;
        }
        poll_data_pid++;
        if (poll_data_pid == 1) {
          transmit_can_frame(&KIA64_7E4_id1, can_config.battery);
        } else if (poll_data_pid == 2) {
          transmit_can_frame(&KIA64_7E4_id2, can_config.battery);
        } else if (poll_data_pid == 3) {
          transmit_can_frame(&KIA64_7E4_id3, can_config.battery);
        } else if (poll_data_pid == 4) {
          transmit_can_frame(&KIA64_7E4_id4, can_config.battery);
        } else if (poll_data_pid == 5) {
          transmit_can_frame(&KIA64_7E4_id5, can_config.battery);
        } else if (poll_data_pid == 6) {
          transmit_can_frame(&KIA64_7E4_id6, can_config.battery);
        }
      }
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[4] == poll_data_pid) {
            transmit_can_frame(&KIA64_7E4_ack,
                               can_config.battery);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            batteryRelay = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[32] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[37] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[64] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[65] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[66] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[67] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[68] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[69] = (rx_frame.data.u8[7] * 20);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 2) {
            cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[38] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[39] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[43] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[70] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[71] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[72] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[73] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[74] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[75] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[76] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[45] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[46] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[49] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[77] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[78] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[79] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[80] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[81] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[82] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[83] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[52] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[53] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[84] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[85] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[86] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[87] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[88] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[89] = (rx_frame.data.u8[6] * 20);
            if (rx_frame.data.u8[7] > 4) {                       // Data only valid on 98S
              cellvoltages_mv[90] = (rx_frame.data.u8[7] * 20);  // Perform extra checks
            }
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 2) {
            cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[31] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[59] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[60] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[61] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[62] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[63] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 4) {  // Data only valid on 98S
            if (rx_frame.data.u8[1] > 4) {  // Perform extra checks
              cellvoltages_mv[91] = (rx_frame.data.u8[1] * 20);
            }
            if (rx_frame.data.u8[2] > 4) {  // Perform extra checks
              cellvoltages_mv[92] = (rx_frame.data.u8[2] * 20);
            }
            if (rx_frame.data.u8[3] > 4) {  // Perform extra checks
              cellvoltages_mv[93] = (rx_frame.data.u8[3] * 20);
            }
            if (rx_frame.data.u8[4] > 4) {  // Perform extra checks
              cellvoltages_mv[94] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              cellvoltages_mv[95] = (rx_frame.data.u8[5] * 20);
            }
          } else if (poll_data_pid == 5) {  // Data only valid on 98S
            if (rx_frame.data.u8[4] > 4) {  // Perform extra checks
              cellvoltages_mv[96] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              cellvoltages_mv[97] = (rx_frame.data.u8[5] * 20);
            }
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          //We have read all cells, check that content is valid:
          for (uint8_t i = 85; i < 97; ++i) {
            if (cellvoltages_mv[i] < 300) {  // Zero the value if it's below 300
              cellvoltages_mv[i] = 0;        // Some packs incorrectly report the last unpopulated cells as 20-60mV
            }
          }
          //Map all cell voltages to the global array
          memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mv, 98 * sizeof(uint16_t));
          //Update number of cells
          update_number_of_cells();
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];
          }
          break;
      }
      break;
    default:
      break;
  }
}

#ifdef DOUBLE_BATTERY
void update_values_battery2() {  // Handle the values coming in from battery #2
  /* Start with mapping all values */
  datalayer.battery2.status.real_soc = (battery2_SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery2.status.soh_pptt = (battery2_batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery2.status.voltage_dV = battery2_batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery2.status.current_dA = -battery2_batteryAmps;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery2.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery2.status.real_soc) / 10000) * datalayer.battery2.info.total_capacity_Wh);

  datalayer.battery2.status.max_charge_power_W = battery2_allowedChargePower * 10;

  datalayer.battery2.status.max_discharge_power_W = battery2_allowedDischargePower * 10;

  datalayer.battery2.status.temperature_min_dC =
      (int8_t)battery2_temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery2.status.temperature_max_dC =
      (int8_t)battery2_temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery2.status.cell_max_voltage_mV = battery2_CellVoltMax_mV;

  datalayer.battery2.status.cell_min_voltage_mV = battery2_CellVoltMin_mV;

  if (battery2_waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (battery2_leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Update webserver datalayer
  datalayer_extended.KiaHyundai64.battery2_total_cell_count = datalayer.battery2.info.number_of_cells;
  datalayer_extended.KiaHyundai64.battery2_battery_12V = battery2_leadAcidBatteryVoltage;
  datalayer_extended.KiaHyundai64.battery2_waterleakageSensor = battery2_waterleakageSensor;
  datalayer_extended.KiaHyundai64.battery2_temperature_water_inlet = battery2_temperature_water_inlet;
  datalayer_extended.KiaHyundai64.battery2_powerRelayTemperature = battery2_powerRelayTemperature * 2;
  datalayer_extended.KiaHyundai64.battery2_batteryManagementMode = battery2_batteryManagementMode;
  datalayer_extended.KiaHyundai64.battery2_BMS_ign = battery2_BMS_ign;
  datalayer_extended.KiaHyundai64.battery2_batteryRelay = battery2_batteryRelay;

  //Perform logging if configured to do so
#ifdef DEBUG_LOG
  logging.println();  //sepatator
  logging.println("Values from battery: ");
  logging.print("SOC BMS: ");
  logging.print((uint16_t)battery2_SOC_BMS / 10.0, 1);
  logging.print("%  |  SOC Display: ");
  logging.print((uint16_t)battery2_SOC_Display / 10.0, 1);
  logging.print("%  |  SOH ");
  logging.print((uint16_t)battery2_batterySOH / 10.0, 1);
  logging.println("%");
  logging.print((int16_t)battery2_batteryAmps / 10.0, 1);
  logging.print(" Amps  |  ");
  logging.print((uint16_t)battery2_batteryVoltage / 10.0, 1);
  logging.print(" Volts  |  ");
  logging.print((int16_t)datalayer.battery2.status.active_power_W);
  logging.println(" Watts");
  logging.print("Allowed Charge ");
  logging.print((uint16_t)battery2_allowedChargePower * 10);
  logging.print(" W  |  Allowed Discharge ");
  logging.print((uint16_t)battery2_allowedDischargePower * 10);
  logging.println(" W");
  logging.print("MaxCellVolt ");
  logging.print(battery2_CellVoltMax_mV);
  logging.print(" mV  No  ");
  logging.print(battery2_CellVmaxNo);
  logging.print("  |  MinCellVolt ");
  logging.print(battery2_CellVoltMin_mV);
  logging.print(" mV  No  ");
  logging.println(battery2_CellVminNo);
  logging.print("TempHi ");
  logging.print((int16_t)battery2_temperatureMax);
  logging.print("°C  TempLo ");
  logging.print((int16_t)battery2_temperatureMin);
  logging.print("°C  WaterInlet ");
  logging.print((int8_t)battery2_temperature_water_inlet);
  logging.print("°C  PowerRelay ");
  logging.print((int8_t)battery2_powerRelayTemperature * 2);
  logging.println("°C");
  logging.print("Aux12volt: ");
  logging.print((int16_t)battery2_leadAcidBatteryVoltage / 10.0, 1);
  logging.println("V  |  ");
  logging.print("BmsManagementMode ");
  logging.print((uint8_t)battery2_batteryManagementMode, BIN);
  if (bitRead((uint8_t)battery2_BMS_ign, 2) == 1) {
    logging.print("  |  BmsIgnition ON");
  } else {
    logging.print("  |  BmsIgnition OFF");
  }

  if (bitRead((uint8_t)battery2_batteryRelay, 0) == 1) {
    logging.print("  |  PowerRelay ON");
  } else {
    logging.print("  |  PowerRelay OFF");
  }
  logging.print("  |  Inverter ");
  logging.print(battery2_inverterVoltage);
  logging.println(" Volts");
#endif
}

void update_number_of_cells_battery2() {
  //If we have cell values and number_of_cells not initialized yet
  if (battery2_cellvoltages_mv[0] > 0 && datalayer.battery2.info.number_of_cells == 0) {
    // Check if we have 98S or 90S battery
    if (datalayer.battery2.status.cell_voltages_mV[97] > 0) {
      datalayer.battery2.info.number_of_cells = 98;
      datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;
      datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_98S_DV;
      datalayer.battery2.info.total_capacity_Wh = 64000;
    } else {
      datalayer.battery2.info.number_of_cells = 90;
      datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_90S_DV;
      datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;
      datalayer.battery2.info.total_capacity_Wh = 40000;
    }
  }
}

void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4DE:
      battery2_startedUp = true;
      break;
    case 0x542:  //BMS SOC
      battery2_startedUp = true;
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      battery2_startedUp = true;
      battery2_allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery2_allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      battery2_SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      battery2_startedUp = true;
      battery2_batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      battery2_batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (battery2_counter_200 > 3) {
        //KIA_HYUNDAI_524.data.u8[0] = (uint8_t)(battery2_batteryVoltage / 10);
        //KIA_HYUNDAI_524.data.u8[1] = (uint8_t)((battery2_batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms (Not required since battery1 writes this)
      break;
    case 0x596:
      battery2_startedUp = true;
      battery2_leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      battery2_temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      battery2_temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      battery2_startedUp = true;
      break;
    case 0x5D5:
      battery2_startedUp = true;
      battery2_waterleakageSensor =
          rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      battery2_powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      battery2_startedUp = true;

      //PID data is polled after last message sent from battery every other time:
      if (battery2_holdPidCounter == true) {
        battery2_holdPidCounter = false;
      } else {
        battery2_holdPidCounter = true;
        if (battery2_poll_data_pid >= 6) {  //polling one of six PIDs at 100ms*2, resolution = 1200ms
          battery2_poll_data_pid = 0;
        }
        battery2_poll_data_pid++;
        if (battery2_poll_data_pid == 1) {
          transmit_can_frame(&KIA64_7E4_id1, can_config.battery_double);
        } else if (battery2_poll_data_pid == 2) {
          transmit_can_frame(&KIA64_7E4_id2, can_config.battery_double);
        } else if (battery2_poll_data_pid == 3) {
          transmit_can_frame(&KIA64_7E4_id3, can_config.battery_double);
        } else if (battery2_poll_data_pid == 4) {
          transmit_can_frame(&KIA64_7E4_id4, can_config.battery_double);
        } else if (battery2_poll_data_pid == 5) {
          transmit_can_frame(&KIA64_7E4_id5, can_config.battery_double);
        } else if (battery2_poll_data_pid == 6) {
          transmit_can_frame(&KIA64_7E4_id6, can_config.battery_double);
        }
      }
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[4] == battery2_poll_data_pid) {
            transmit_can_frame(&KIA64_7E4_ack,
                               can_config.battery_double);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (battery2_poll_data_pid == 1) {
            battery2_batteryRelay = rx_frame.data.u8[7];
          } else if (battery2_poll_data_pid == 2) {
            battery2_cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 3) {
            battery2_cellvoltages_mv[32] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[33] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[34] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[35] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[36] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[37] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 4) {
            battery2_cellvoltages_mv[64] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[65] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[66] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[67] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[68] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[69] = (rx_frame.data.u8[7] * 20);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (battery2_poll_data_pid == 2) {
            battery2_cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 3) {
            battery2_cellvoltages_mv[38] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[39] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[40] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[41] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[42] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[43] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[44] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 4) {
            battery2_cellvoltages_mv[70] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[71] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[72] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[73] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[74] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[75] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[76] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 6) {
            battery2_batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (battery2_poll_data_pid == 1) {
            battery2_temperature_water_inlet = rx_frame.data.u8[6];
            battery2_CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (battery2_poll_data_pid == 2) {
            battery2_cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 3) {
            battery2_cellvoltages_mv[45] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[46] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[47] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[48] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[49] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[50] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[51] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 4) {
            battery2_cellvoltages_mv[77] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[78] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[79] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[80] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[81] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[82] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[83] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 5) {
            battery2_heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (battery2_poll_data_pid == 1) {
            battery2_CellVmaxNo = rx_frame.data.u8[1];
            battery2_CellVminNo = rx_frame.data.u8[3];
            battery2_CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (battery2_poll_data_pid == 2) {
            battery2_cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 3) {
            battery2_cellvoltages_mv[52] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[53] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[54] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (battery2_poll_data_pid == 4) {
            battery2_cellvoltages_mv[84] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[85] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[86] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[87] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[88] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[89] = (rx_frame.data.u8[6] * 20);
            if (rx_frame.data.u8[7] > 4) {                                // Data only valid on 98S
              battery2_cellvoltages_mv[90] = (rx_frame.data.u8[7] * 20);  // Perform extra checks
            }
          } else if (battery2_poll_data_pid == 5) {
            battery2_batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (battery2_poll_data_pid == 2) {
            battery2_cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[31] = (rx_frame.data.u8[5] * 20);
          } else if (battery2_poll_data_pid == 3) {
            battery2_cellvoltages_mv[59] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[60] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[61] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[62] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[63] = (rx_frame.data.u8[5] * 20);
          } else if (battery2_poll_data_pid == 4) {  // Data only valid on 98S
            if (rx_frame.data.u8[1] > 4) {           // Perform extra checks
              battery2_cellvoltages_mv[91] = (rx_frame.data.u8[1] * 20);
            }
            if (rx_frame.data.u8[2] > 4) {  // Perform extra checks
              battery2_cellvoltages_mv[92] = (rx_frame.data.u8[2] * 20);
            }
            if (rx_frame.data.u8[3] > 4) {  // Perform extra checks
              battery2_cellvoltages_mv[93] = (rx_frame.data.u8[3] * 20);
            }
            if (rx_frame.data.u8[4] > 4) {  // Perform extra checks
              battery2_cellvoltages_mv[94] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              battery2_cellvoltages_mv[95] = (rx_frame.data.u8[5] * 20);
            }
          } else if (battery2_poll_data_pid == 5) {  // Data only valid on 98S
            if (rx_frame.data.u8[4] > 4) {           // Perform extra checks
              battery2_cellvoltages_mv[96] = (rx_frame.data.u8[4] * 20);
            }
            if (rx_frame.data.u8[5] > 4) {  // Perform extra checks
              battery2_cellvoltages_mv[97] = (rx_frame.data.u8[5] * 20);
            }
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          //We have read all cells, check that content is valid:
          for (uint8_t i = 85; i < 97; ++i) {
            if (battery2_cellvoltages_mv[i] < 300) {  // Zero the value if it's below 300
              battery2_cellvoltages_mv[i] = 0;  // Some packs incorrectly report the last unpopulated cells as 20-60mV
            }
          }
          //Map all cell voltages to the global array
          memcpy(datalayer.battery2.status.cell_voltages_mV, battery2_cellvoltages_mv, 98 * sizeof(uint16_t));
          //Update number of cells
          update_number_of_cells_battery2();
          break;
        case 0x27:  //Seventh datarow in PID group
          if (battery2_poll_data_pid == 1) {
            battery2_BMS_ign = rx_frame.data.u8[6];
            battery2_inverterVoltageFrameHigh = rx_frame.data.u8[7];
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (battery2_poll_data_pid == 1) {
            battery2_inverterVoltage = (battery2_inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];
          }
          break;
      }
      break;
    default:
      break;
  }
}
#endif  //DOUBLE_BATTERY

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  if (!startedUp) {
    return;  // Don't send any CAN messages towards battery until it has started up
  }

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&KIA64_553, can_config.battery);
    transmit_can_frame(&KIA64_57F, can_config.battery);
    transmit_can_frame(&KIA64_2A1, can_config.battery);
#ifdef DOUBLE_BATTERY
    if (battery2_startedUp && datalayer.system.status.battery2_allows_contactor_closing) {
      transmit_can_frame(&KIA64_553, can_config.battery_double);
      transmit_can_frame(&KIA64_57F, can_config.battery_double);
      transmit_can_frame(&KIA64_2A1, can_config.battery_double);
    }
#endif  // DOUBLE_BATTERY
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10 = currentMillis;

    switch (counter_200) {
      case 0:
        KIA_HYUNDAI_200.data.u8[5] = 0x17;
        ++counter_200;
        break;
      case 1:
        KIA_HYUNDAI_200.data.u8[5] = 0x57;
        ++counter_200;
        break;
      case 2:
        KIA_HYUNDAI_200.data.u8[5] = 0x97;
        ++counter_200;
        break;
      case 3:
        KIA_HYUNDAI_200.data.u8[5] = 0xD7;
        ++counter_200;
        break;
      case 4:
        KIA_HYUNDAI_200.data.u8[3] = 0x10;
        KIA_HYUNDAI_200.data.u8[5] = 0xFF;
        ++counter_200;
        break;
      case 5:
        KIA_HYUNDAI_200.data.u8[5] = 0x3B;
        ++counter_200;
        break;
      case 6:
        KIA_HYUNDAI_200.data.u8[5] = 0x7B;
        ++counter_200;
        break;
      case 7:
        KIA_HYUNDAI_200.data.u8[5] = 0xBB;
        ++counter_200;
        break;
      case 8:
        KIA_HYUNDAI_200.data.u8[5] = 0xFB;
        counter_200 = 5;
        break;
    }

    transmit_can_frame(&KIA_HYUNDAI_200, can_config.battery);

    transmit_can_frame(&KIA_HYUNDAI_523, can_config.battery);

    transmit_can_frame(&KIA_HYUNDAI_524, can_config.battery);

#ifdef DOUBLE_BATTERY

    if (battery2_startedUp && datalayer.system.status.battery2_allows_contactor_closing) {
      switch (counter_200_2) {
        case 0:
          KIA_HYUNDAI_200_2.data.u8[5] = 0x17;
          ++counter_200_2;
          break;
        case 1:
          KIA_HYUNDAI_200_2.data.u8[5] = 0x57;
          ++counter_200_2;
          break;
        case 2:
          KIA_HYUNDAI_200_2.data.u8[5] = 0x97;
          ++counter_200_2;
          break;
        case 3:
          KIA_HYUNDAI_200_2.data.u8[5] = 0xD7;
          ++counter_200_2;
          break;
        case 4:
          KIA_HYUNDAI_200_2.data.u8[3] = 0x10;
          KIA_HYUNDAI_200_2.data.u8[5] = 0xFF;
          ++counter_200_2;
          break;
        case 5:
          KIA_HYUNDAI_200_2.data.u8[5] = 0x3B;
          ++counter_200_2;
          break;
        case 6:
          KIA_HYUNDAI_200_2.data.u8[5] = 0x7B;
          ++counter_200_2;
          break;
        case 7:
          KIA_HYUNDAI_200_2.data.u8[5] = 0xBB;
          ++counter_200_2;
          break;
        case 8:
          KIA_HYUNDAI_200_2.data.u8[5] = 0xFB;
          counter_200_2 = 5;
          break;
      }

      transmit_can_frame(&KIA_HYUNDAI_200_2, can_config.battery_double);

      transmit_can_frame(&KIA_HYUNDAI_523, can_config.battery_double);

      transmit_can_frame(&KIA_HYUNDAI_524, can_config.battery_double);
    }
#endif  // DOUBLE_BATTERY
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Kia/Hyundai 64/40kWh battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;  //Start with 98S value. Precised later
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;  //Start with 90S value. Precised later
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  datalayer.battery2.info.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  datalayer.battery2.info.max_cell_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  datalayer.battery2.info.min_cell_voltage_mV = datalayer.battery.info.min_cell_voltage_mV;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
#endif  //DOUBLE_BATTERY
}

#endif
