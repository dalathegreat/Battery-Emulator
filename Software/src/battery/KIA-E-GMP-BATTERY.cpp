#include "../include.h"
#ifdef KIA_E_GMP_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "KIA-E-GMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send
static uint8_t CANstillAlive = 12;             //counter for checking if CAN is still alive

#define MAX_CELL_VOLTAGE 4250   //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950   //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION 150  //LED turns yellow on the board if mv delta exceeds this value

static uint16_t inverterVoltageFrameHigh = 0;
static uint16_t inverterVoltage = 0;
static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 1000;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint16_t cell_deviation_mV = 0;
static uint16_t batteryVoltage = 0;
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

CANFDMessage EGMP_7E4;
CANFDMessage EGMP_7E4_ack;

void set_cell_voltages(CANFDMessage frame, int start, int length, int startCell) {
  for (size_t i = 0; i < length; i++) {
    datalayer.battery.status.cell_voltages_mV[startCell + i] = (frame.data[start + i] * 20);
  }
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = batteryAmps;  //value is *10 (150 = 15.0)

  datalayer.battery.status.remaining_capacity_Wh =
      static_cast<uint32_t>((static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //datalayer.battery.status.max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We estimate this value
  if (datalayer.battery.status.reported_soc == 10000) {  // When scaled SOC is 100%, set allowed charge power to 0
    datalayer.battery.status.max_charge_power_W = 0;
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }
  //datalayer.battery.status.max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  if (datalayer.battery.status.reported_soc < 100) {  // When scaled SOC is <1%, set allowed charge power to 0
    datalayer.battery.status.max_discharge_power_W = 0;
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  powerWatt = ((batteryVoltage * batteryAmps) / 100);

  datalayer.battery.status.active_power_W = powerWatt;  //Power in watts, Negative = charging batt

  datalayer.battery.status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CANFD_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CANFD_RX_FAILURE);
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Check if cell voltages are within allowed range
  cell_deviation_mV = (datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV);

  if (CellVoltMax_mV >= MAX_CELL_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (CellVoltMin_mV <= MIN_CELL_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION) {
    set_event(EVENT_CELL_DEVIATION_HIGH, 0);
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  if (datalayer.battery.status.bms_status ==
      FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
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

void receive_canfd_battery(CANFDMessage frame) {
  CANstillAlive = 12;
  switch (frame.id) {
    case 0x7EC:
      // printFrame(frame);
      switch (frame.data[0]) {
        case 0x10:  //"PID Header"
          // Serial.println ("Send ack");
          poll_data_pid = frame.data[4];
          // if (frame.data[4] == poll_data_pid) {
          canfd.tryToSend(EGMP_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
          // }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            allowedChargePower = ((frame.data[3] << 8) + frame.data[4]);
            allowedDischargePower = ((frame.data[5] << 8) + frame.data[6]);
            SOC_BMS = frame.data[2] * 5;  //100% = 200 ( 200 * 5 = 1000 )

          } else if (poll_data_pid == 2) {
            // set cell voltages data, start bite, data length from start, start cell
            set_cell_voltages(frame, 2, 6, 0);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(frame, 2, 6, 32);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(frame, 2, 6, 64);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(frame, 2, 6, 96);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(frame, 2, 6, 128);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(frame, 2, 6, 160);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {
            batteryVoltage = (frame.data[3] << 8) + frame.data[4];
            batteryAmps = (frame.data[1] << 8) + frame.data[2];
            temperatureMax = frame.data[5];
            temperatureMin = frame.data[6];
            // temp1 = frame.data[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(frame, 1, 7, 6);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(frame, 1, 7, 38);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(frame, 1, 7, 70);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(frame, 1, 7, 102);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(frame, 1, 7, 134);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(frame, 1, 7, 166);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = frame.data[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = frame.data[6];
            CellVoltMax_mV = (frame.data[7] * 20);  //(volts *50) *20 =mV
            // temp2 = frame.data[1];
            // temp3 = frame.data[2];
            // temp4 = frame.data[3];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(frame, 1, 7, 13);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(frame, 1, 7, 45);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(frame, 1, 7, 77);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(frame, 1, 7, 109);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(frame, 1, 7, 141);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(frame, 1, 7, 173);
          } else if (poll_data_pid == 5) {
            // ac = frame.data[3];
            // Vdiff = frame.data[4];

            // airbag = frame.data[6];
            heatertemp = frame.data[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = frame.data[1];
            CellVoltMin_mV = (frame.data[2] * 20);  //(volts *50) *20 =mV
            CellVminNo = frame.data[3];
            // fanMod = frame.data[4];
            // fanSpeed = frame.data[5];
            leadAcidBatteryVoltage = frame.data[6];  //12v Battery Volts
            //cumulative_charge_current[0] = frame.data[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(frame, 1, 7, 20);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(frame, 1, 7, 52);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(frame, 1, 7, 84);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(frame, 1, 7, 116);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(frame, 1, 7, 148);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(frame, 1, 7, 180);
          } else if (poll_data_pid == 5) {
            batterySOH = ((frame.data[2] << 8) + frame.data[3]);
            // maxDetCell = frame.data[4];
            // minDet = (frame.data[5] << 8) + frame.data[6];
            // minDetCell = frame.data[7];
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_charge_current[1] = frame.data[1];
            //cumulative_charge_current[2] = frame.data[2];
            //cumulative_charge_current[3] = frame.data[3];
            //cumulative_discharge_current[0] = frame.data[4];
            //cumulative_discharge_current[1] = frame.data[5];
            //cumulative_discharge_current[2] = frame.data[6];
            //cumulative_discharge_current[3] = frame.data[7];
            //set_cumulative_charge_current();
            //set_cumulative_discharge_current();
          } else if (poll_data_pid == 2) {
            set_cell_voltages(frame, 1, 5, 27);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(frame, 1, 5, 59);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(frame, 1, 5, 91);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(frame, 1, 5, 123);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(frame, 1, 5, 155);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(frame, 1, 5, 187);
            //set_cell_count();
          } else if (poll_data_pid == 5) {
            // datalayer.battery.info.number_of_cells = 98;
            SOC_Display = frame.data[1] * 5;
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_charged[0] = frame.data[1];
            // cumulative_energy_charged[1] = frame.data[2];
            //cumulative_energy_charged[2] = frame.data[3];
            //cumulative_energy_charged[3] = frame.data[4];
            //cumulative_energy_discharged[0] = frame.data[5];
            //cumulative_energy_discharged[1] = frame.data[6];
            //cumulative_energy_discharged[2] = frame.data[7];
            // set_cumulative_energy_charged();
          }
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_discharged[3] = frame.data[1];

            //opTimeBytes[0] = frame.data[2];
            //opTimeBytes[1] = frame.data[3];
            //opTimeBytes[2] = frame.data[4];
            //opTimeBytes[3] = frame.data[5];

            BMS_ign = frame.data[6];
            inverterVoltageFrameHigh = frame.data[7];  // BMS Capacitoir

            // set_cumulative_energy_discharged();
            // set_opTime();
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + frame.data[1];  // BMS Capacitoir
          }
          break;
      }
      break;
    default:
      break;
  }
}

void receive_can_battery(CAN_frame_t frame) {}  // Not used on CAN-FD battery, just included to compile

void send_can_battery() {

  unsigned long currentMillis = millis();
  //Send 500ms CANFD message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {

    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis500ms >= INTERVAL_500_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis500ms));
    }
    previousMillis500ms = currentMillis;
    EGMP_7E4.data[3] = KIA_7E4_COUNTER;
    canfd.tryToSend(EGMP_7E4);

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

  EGMP_7E4.id = 0x7E4;
  EGMP_7E4.ext = false;
  EGMP_7E4.len = 8;
  uint8_t dataEGMP_7E4[8] = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};  //Poll PID 03 22 01 01
  memcpy(EGMP_7E4.data, dataEGMP_7E4, sizeof(dataEGMP_7E4));

  EGMP_7E4_ack.id = 0x7E4;
  EGMP_7E4_ack.ext = false;
  EGMP_7E4_ack.len = 8;
  uint8_t dataEGMP_7E4_ack[8] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //Ack frame, correct PID is returned
  memcpy(EGMP_7E4_ack.data, dataEGMP_7E4_ack, sizeof(dataEGMP_7E4_ack));
}

#endif
