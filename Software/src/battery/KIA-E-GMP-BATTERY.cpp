#include "../include.h"
#ifdef KIA_E_GMP_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "KIA-E-GMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send

#define MAX_CELL_VOLTAGE 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950  //Battery is put into emergency stop if one cell goes below this value

static uint16_t inverterVoltageFrameHigh = 0;
static uint16_t inverterVoltage = 0;
static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 1000;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint16_t batteryVoltage = 6700;
static int16_t leadAcidBatteryVoltage = 120;
static int16_t batteryAmps = 0;
static int16_t powerWatt = 0;
static int16_t temperatureMax = 0;
static int16_t temperatureMin = 0;
static int16_t allowedDischargePower = 0;
static int16_t allowedChargePower = 0;
static int16_t poll_data_pid = 0;
static uint8_t CellVmaxNo = 0;
static uint8_t CellVminNo = 0;
static uint8_t batteryManagementMode = 0;
static uint8_t BMS_ign = 0;
static uint8_t batteryRelay = 0;
static uint8_t waterleakageSensor = 164;
static uint8_t startedUp = false;
static uint8_t counter_200 = 0;
static uint8_t KIA_7E4_COUNTER = 0x01;
static int8_t temperature_water_inlet = 0;
static int8_t powerRelayTemperature = 0;
static int8_t heatertemp = 0;

CAN_frame EGMP_7E4 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x7E4,
                      .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 01
CAN_frame EGMP_7E4_ack = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x7E4,
    .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Ack frame, correct PID is returned

void set_cell_voltages(CAN_frame rx_frame, int start, int length, int startCell) {
  for (size_t i = 0; i < length; i++) {
    if ((rx_frame.data.u8[start + i] * 20) > 1000) {
      datalayer.battery.status.cell_voltages_mV[startCell + i] = (rx_frame.data.u8[start + i] * 20);
    }
  }
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0)

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //datalayer.battery.status.max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We hardcode this value for now
  datalayer.battery.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;

  //datalayer.battery.status.max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  //The allowed discharge power is not available. We hardcode this value for now
  datalayer.battery.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;

  powerWatt = ((batteryVoltage * batteryAmps) / 100);

  datalayer.battery.status.active_power_W = powerWatt;  //Power in watts, Negative = charging batt

  datalayer.battery.status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!datalayer.battery.status.CAN_battery_still_alive) {
    set_event(EVENT_CANFD_RX_FAILURE, 0);
  } else {
    datalayer.battery.status.CAN_battery_still_alive--;
    clear_event(EVENT_CANFD_RX_FAILURE);
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Check if cell voltages are within allowed range
  if (CellVoltMax_mV >= MAX_CELL_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (CellVoltMin_mV <= MIN_CELL_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

  /* Safeties verified. Perform USB serial printout if configured to do so */

#ifdef DEBUG_VIA_USB
  Serial.println();  //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print((uint16_t)SOC_BMS / 10.0, 1);
  Serial.print("%  |  SOC Display: ");
  Serial.print((uint16_t)SOC_Display / 10.0, 1);
  Serial.print("%  |  SOH ");
  Serial.print((uint16_t)batterySOH / 10.0, 1);
  Serial.println("%");
  Serial.print((int16_t)batteryAmps / 10.0, 1);
  Serial.print(" Amps  |  ");
  Serial.print((uint16_t)batteryVoltage / 10.0, 1);
  Serial.print(" Volts  |  ");
  Serial.print((int16_t)datalayer.battery.status.active_power_W);
  Serial.println(" Watts");
  Serial.print("Allowed Charge ");
  Serial.print((uint16_t)allowedChargePower * 10);
  Serial.print(" W  |  Allowed Discharge ");
  Serial.print((uint16_t)allowedDischargePower * 10);
  Serial.println(" W");
  Serial.print("MaxCellVolt ");
  Serial.print(CellVoltMax_mV);
  Serial.print(" mV  No  ");
  Serial.print(CellVmaxNo);
  Serial.print("  |  MinCellVolt ");
  Serial.print(CellVoltMin_mV);
  Serial.print(" mV  No  ");
  Serial.println(CellVminNo);
  Serial.print("TempHi ");
  Serial.print((int16_t)temperatureMax);
  Serial.print("°C  TempLo ");
  Serial.print((int16_t)temperatureMin);
  Serial.print("°C  WaterInlet ");
  Serial.print((int8_t)temperature_water_inlet);
  Serial.print("°C  PowerRelay ");
  Serial.print((int8_t)powerRelayTemperature * 2);
  Serial.println("°C");
  Serial.print("Aux12volt: ");
  Serial.print((int16_t)leadAcidBatteryVoltage / 10.0, 1);
  Serial.println("V  |  ");
  Serial.print("BmsManagementMode ");
  Serial.print((uint8_t)batteryManagementMode, BIN);
  if (bitRead((uint8_t)BMS_ign, 2) == 1) {
    Serial.print("  |  BmsIgnition ON");
  } else {
    Serial.print("  |  BmsIgnition OFF");
  }

  if (bitRead((uint8_t)batteryRelay, 0) == 1) {
    Serial.print("  |  PowerRelay ON");
  } else {
    Serial.print("  |  PowerRelay OFF");
  }
  Serial.print("  |  Inverter ");
  Serial.print(inverterVoltage);
  Serial.println(" Volts");
#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x055:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x21A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x235:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x25A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x275:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2FA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x325:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x335:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x365:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3BA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:
      // print_canfd_frame(frame);
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          // Serial.println ("Send ack");
          poll_data_pid = rx_frame.data.u8[4];
          // if (rx_frame.data.u8[4] == poll_data_pid) {
          transmit_can(&EGMP_7E4_ack, can_config.battery);  //Send ack to BMS if the same frame is sent as polled
          // }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
            allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
            SOC_BMS = rx_frame.data.u8[2] * 5;  //100% = 200 ( 200 * 5 = 1000 )

          } else if (poll_data_pid == 2) {
            // set cell voltages data, start bite, data length from start, start cell
            set_cell_voltages(rx_frame, 2, 6, 0);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 2, 6, 32);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 2, 6, 64);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 2, 6, 96);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 2, 6, 128);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 2, 6, 160);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {
            batteryVoltage = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
            batteryAmps = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2];
            temperatureMax = rx_frame.data.u8[5];
            temperatureMin = rx_frame.data.u8[6];
            // temp1 = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 6);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 38);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 70);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 102);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 134);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 166);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
            // temp2 = rx_frame.data.u8[1];
            // temp3 = rx_frame.data.u8[2];
            // temp4 = rx_frame.data.u8[3];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 13);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 45);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 77);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 109);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 141);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 173);
          } else if (poll_data_pid == 5) {
            // ac = rx_frame.data.u8[3];
            // Vdiff = rx_frame.data.u8[4];

            // airbag = rx_frame.data.u8[6];
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
            CellVminNo = rx_frame.data.u8[3];
            // fanMod = rx_frame.data.u8[4];
            // fanSpeed = rx_frame.data.u8[5];
            leadAcidBatteryVoltage = rx_frame.data.u8[6];  //12v Battery Volts
            //cumulative_charge_current[0] = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 20);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 52);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 84);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 116);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 148);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 180);
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
            // maxDetCell = rx_frame.data.u8[4];
            // minDet = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6];
            // minDetCell = rx_frame.data.u8[7];
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_charge_current[1] = rx_frame.data.u8[1];
            //cumulative_charge_current[2] = rx_frame.data.u8[2];
            //cumulative_charge_current[3] = rx_frame.data.u8[3];
            //cumulative_discharge_current[0] = rx_frame.data.u8[4];
            //cumulative_discharge_current[1] = rx_frame.data.u8[5];
            //cumulative_discharge_current[2] = rx_frame.data.u8[6];
            //cumulative_discharge_current[3] = rx_frame.data.u8[7];
            //set_cumulative_charge_current();
            //set_cumulative_discharge_current();
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 5, 27);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 5, 59);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 5, 91);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 5, 123);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 5, 155);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 5, 187);
            //set_cell_count();
          } else if (poll_data_pid == 5) {
            // datalayer.battery.info.number_of_cells = 98;
            SOC_Display = rx_frame.data.u8[1] * 5;
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_charged[0] = rx_frame.data.u8[1];
            // cumulative_energy_charged[1] = rx_frame.data.u8[2];
            //cumulative_energy_charged[2] = rx_frame.data.u8[3];
            //cumulative_energy_charged[3] = rx_frame.data.u8[4];
            //cumulative_energy_discharged[0] = rx_frame.data.u8[5];
            //cumulative_energy_discharged[1] = rx_frame.data.u8[6];
            //cumulative_energy_discharged[2] = rx_frame.data.u8[7];
            // set_cumulative_energy_charged();
          }
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_discharged[3] = rx_frame.data.u8[1];

            //opTimeBytes[0] = rx_frame.data.u8[2];
            //opTimeBytes[1] = rx_frame.data.u8[3];
            //opTimeBytes[2] = rx_frame.data.u8[4];
            //opTimeBytes[3] = rx_frame.data.u8[5];

            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];  // BMS Capacitoir

            // set_cumulative_energy_discharged();
            // set_opTime();
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];  // BMS Capacitoir
          }
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {

  unsigned long currentMillis = millis();
  //Send 500ms CANFD message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {

    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis500ms >= INTERVAL_500_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis500ms));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis500ms = currentMillis;
    //  Section added to close contractor
    if (datalayer.battery.status.bms_status == ACTIVE) {
      datalayer.system.status.battery_allows_contactor_closing = true;
    } else {  //datalayer.battery.status.bms_status == FAULT or inverter requested opening contactors
      datalayer.system.status.battery_allows_contactor_closing = false;
    }
    //  Section end
    EGMP_7E4.data.u8[3] = KIA_7E4_COUNTER;
    transmit_can(&EGMP_7E4, can_config.battery);

    KIA_7E4_COUNTER++;
    if (KIA_7E4_COUNTER > 0x0D) {  // gets up to 0x010C before repeating
      KIA_7E4_COUNTER = 0x01;
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Hyundai E-GMP (Electric Global Modular Platform) battery selected");
#endif

  datalayer.battery.info.number_of_cells = 192;  // TODO: will vary depending on battery

  datalayer.battery.info.max_design_voltage_dV =
      8064;  // TODO: define when battery is known, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV =
      4320;  // TODO: define when battery is known. discharging further is disabled
}

#endif
