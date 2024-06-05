#include "../include.h"
#ifdef VOLVO_SPA_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "VOLVO-SPA-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

#define MAX_CELL_VOLTAGE 4210  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2700  //Battery is put into emergency stop if one cell goes below this value

static float BATT_U = 0;                 //0x3A
static float MAX_U = 0;                  //0x3A
static float MIN_U = 0;                  //0x3A
static float BATT_I = 0;                 //0x3A
static int32_t CHARGE_ENERGY = 0;        //0x1A1
static uint8_t BATT_ERR_INDICATION = 0;  //0x413
static float BATT_T_MAX = 0;             //0x413
static float BATT_T_MIN = 0;             //0x413
static float BATT_T_AVG = 0;             //0x413
static uint16_t SOC_BMS = 0;             //0X37D
static uint16_t SOC_CALC = 0;
static uint16_t CELL_U_MAX = 0;             //0x37D
static uint16_t CELL_U_MIN = 0;             //0x37D
static uint8_t CELL_ID_U_MAX = 0;           //0x37D
static uint16_t HvBattPwrLimDchaSoft = 0;   //0x369
static uint8_t batteryModuleNumber = 0x10;  // First battery module
static uint8_t battery_request_idx = 0;
static uint8_t rxConsecutiveFrames = 0;
static uint16_t min_max_voltage[2];  //contains cell min[0] and max[1] values in mV
static uint8_t cellcounter = 0;
static uint32_t remaining_capacity = 0;
static uint16_t cell_voltages[108];  //array with all the cellvoltages

CAN_frame_t VOLVO_536 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x536,
                         .data = {0x00, 0x40, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame
CAN_frame_t VOLVO_372 = {
    .FIR = {.B =
                {
                    .DLC = 8,
                    .FF = CAN_frame_std,
                }},
    .MsgID = 0x372,
    .data = {0x00, 0xA6, 0x07, 0x14, 0x04, 0x00, 0x80, 0x00}};  //Ambient Temp -->>VERIFY this data content!!!<<--
CAN_frame_t VOLVO_CELL_U_Req = {.FIR = {.B =
                                            {
                                                .DLC = 8,
                                                .FF = CAN_frame_std,
                                            }},
                                .MsgID = 0x735,
                                .data = {0x03, 0x22, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Cell voltage request frame
CAN_frame_t VOLVO_FlowControl = {.FIR = {.B =
                                             {
                                                 .DLC = 8,
                                                 .FF = CAN_frame_std,
                                             }},
                                 .MsgID = 0x735,
                                 .data = {0x30, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Flowcontrol
CAN_frame_t VOLVO_SOH_Req = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x735,
                             .data = {0x03, 0x22, 0x49, 0x6D, 0x00, 0x00, 0x00, 0x00}};  //Battery SOH request frame

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter
  uint8_t cnt = 0;

  remaining_capacity = (78200 - CHARGE_ENERGY);

  //datalayer.battery.status.real_soc = SOC_BMS;			// Use BMS reported SOC, havent figured out how to get the BMS to calibrate empty/full yet
  SOC_CALC = remaining_capacity / 78;  // Use calculated SOC based on remaining_capacity

  datalayer.battery.status.real_soc = SOC_CALC * 10;

  if (BATT_U > MAX_U)  // Protect if overcharged
  {
    datalayer.battery.status.real_soc = 10000;
  } else if (BATT_U < MIN_U)  //Protect if undercharged
  {
    datalayer.battery.status.real_soc = 0;
  }

  datalayer.battery.status.voltage_dV = BATT_U * 10;
  datalayer.battery.status.current_dA = BATT_I * 10;
  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity;

  //datalayer.battery.status.max_discharge_power_W = HvBattPwrLimDchaSoft * 1000;	// Use power limit reported from BMS, not trusted ATM
  datalayer.battery.status.max_discharge_power_W = 30000;
  datalayer.battery.status.max_charge_power_W = 30000;
  datalayer.battery.status.active_power_W = (BATT_U)*BATT_I;
  datalayer.battery.status.temperature_min_dC = BATT_T_MIN;
  datalayer.battery.status.temperature_max_dC = BATT_T_MAX;

  datalayer.battery.status.cell_max_voltage_mV = CELL_U_MAX * 10;  // Use min/max reported from BMS
  datalayer.battery.status.cell_min_voltage_mV = CELL_U_MIN * 10;

  //Map all cell voltages to the global array
  for (int i = 0; i < 108; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = cell_voltages[i];
  }

#ifdef DEBUG_VIA_USB
  Serial.print("BMS reported SOC%: ");
  Serial.println(SOC_BMS);
  Serial.print("Calculated SOC%: ");
  Serial.println(SOC_CALC);
  Serial.print("Rescaled SOC%: ");
  Serial.println(datalayer.battery.status.reported_soc / 100);
  Serial.print("Battery current: ");
  Serial.println(BATT_I);
  Serial.print("Battery voltage: ");
  Serial.println(BATT_U);
  Serial.print("Battery maximum voltage limit: ");
  Serial.println(MAX_U);
  Serial.print("Battery minimum voltage limit: ");
  Serial.println(MIN_U);
  Serial.print("Remaining Energy: ");
  Serial.println(remaining_capacity);
  Serial.print("Discharge limit: ");
  Serial.println(HvBattPwrLimDchaSoft);
  Serial.print("Battery Error Indication: ");
  Serial.println(BATT_ERR_INDICATION);
  Serial.print("Maximum battery temperature: ");
  Serial.println(BATT_T_MAX / 10);
  Serial.print("Minimum battery temperature: ");
  Serial.println(BATT_T_MIN / 10);
  Serial.print("Average battery temperature: ");
  Serial.println(BATT_T_AVG / 10);
  Serial.print("BMS Highest cell voltage: ");
  Serial.println(CELL_U_MAX * 10);
  Serial.print("BMS Lowest cell voltage: ");
  Serial.println(CELL_U_MIN * 10);
  Serial.print("BMS Highest cell nr: ");
  Serial.println(CELL_ID_U_MAX);
  Serial.print("Highest cell voltage: ");
  Serial.println(min_max_voltage[1]);
  Serial.print("Lowest cell voltage: ");
  Serial.println(min_max_voltage[0]);
  Serial.print("Cell voltage,");
  while (cnt < 108) {
    Serial.print(cell_voltages[cnt++]);
    Serial.print(",");
  }
  Serial.println(";");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0x3A:
      if ((rx_frame.data.u8[6] & 0x80) == 0x80)
        BATT_I = (0 - ((((rx_frame.data.u8[6] & 0x7F) * 256.0 + rx_frame.data.u8[7]) * 0.1) - 1638));
      else {
        BATT_I = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("BATT_I not valid");
#endif
      }

      if ((rx_frame.data.u8[2] & 0x08) == 0x08)
        MAX_U = (((rx_frame.data.u8[2] & 0x07) * 256.0 + rx_frame.data.u8[3]) * 0.25);
      else {
        //MAX_U = 0;
        //Serial.println("MAX_U not valid");	// Value toggles between true/false from BMS
      }

      if ((rx_frame.data.u8[4] & 0x08) == 0x08)
        MIN_U = (((rx_frame.data.u8[4] & 0x07) * 256.0 + rx_frame.data.u8[5]) * 0.25);
      else {
        //MIN_U = 0;
        //Serial.println("MIN_U not valid");	// Value toggles between true/false from BMS
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08)
        BATT_U = (((rx_frame.data.u8[0] & 0x07) * 256.0 + rx_frame.data.u8[1]) * 0.25);
      else {
        BATT_U = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("BATT_U not valid");
#endif
      }
      break;
    case 0x1A1:
      if ((rx_frame.data.u8[4] & 0x10) == 0x10)
        CHARGE_ENERGY = ((((rx_frame.data.u8[4] & 0x0F) * 256.0 + rx_frame.data.u8[5]) * 50) - 500);
      else {
        CHARGE_ENERGY = 0;
        set_event(EVENT_KWH_PLAUSIBILITY_ERROR, CHARGE_ENERGY);
      }
      break;
    case 0x413:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80)
        BATT_ERR_INDICATION = ((rx_frame.data.u8[0] & 0x40) >> 6);
      else {
        BATT_ERR_INDICATION = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("BATT_ERR_INDICATION not valid");
#endif
      }
      if ((rx_frame.data.u8[0] & 0x20) == 0x20) {
        BATT_T_MAX = ((rx_frame.data.u8[2] & 0x1F) * 256.0 + rx_frame.data.u8[3]);
        BATT_T_MIN = ((rx_frame.data.u8[4] & 0x1F) * 256.0 + rx_frame.data.u8[5]);
        BATT_T_AVG = ((rx_frame.data.u8[0] & 0x1F) * 256.0 + rx_frame.data.u8[1]);
      } else {
        BATT_T_MAX = 0;
        BATT_T_MIN = 0;
        BATT_T_AVG = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("BATT_T not valid");
#endif
      }
      break;
    case 0x369:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80) {
        HvBattPwrLimDchaSoft = (((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[6]) >> 2);
      } else {
        HvBattPwrLimDchaSoft = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("HvBattPwrLimDchaSoft not valid");
#endif
      }
      break;
    case 0x37D:
      if ((rx_frame.data.u8[0] & 0x40) == 0x40) {
        SOC_BMS = ((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[7]);
      } else {
        SOC_BMS = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("SOC_BMS not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x04) == 0x04)
        CELL_U_MAX = ((rx_frame.data.u8[2] & 0x01) * 256 + rx_frame.data.u8[3]);
      else {
        CELL_U_MAX = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("CELL_U_MAX not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x02) == 0x02)
        CELL_U_MIN = ((rx_frame.data.u8[0] & 0x01) * 256.0 + rx_frame.data.u8[1]);
      else {
        CELL_U_MIN = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("CELL_U_MIN not valid");
#endif
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08)
        CELL_ID_U_MAX = ((rx_frame.data.u8[4] & 0x01) * 256.0 + rx_frame.data.u8[5]);
      else {
        CELL_ID_U_MAX = 0;
#ifdef DEBUG_VIA_USB
        Serial.println("CELL_ID_U_MAX not valid");
#endif
      }
      break;
    case 0x635:  // Diag request response
      if ((rx_frame.data.u8[0] == 0x07) && (rx_frame.data.u8[1] == 0x62) && (rx_frame.data.u8[2] == 0x49) &&
          (rx_frame.data.u8[3] == 0x6D))  // SOH response frame
      {
        datalayer.battery.status.soh_pptt = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      } else if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[1] == 0x0B) && (rx_frame.data.u8[2] == 0x62) &&
                 (rx_frame.data.u8[3] == 0x4B))  // First response frame of cell voltages
      {
        cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
        cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        ESP32Can.CANWriteFrame(&VOLVO_FlowControl);  // Send flow control
        rxConsecutiveFrames = 1;
      } else if ((rx_frame.data.u8[0] == 0x21) && (rxConsecutiveFrames == 1)) {
        cell_voltages[battery_request_idx++] = cell_voltages[battery_request_idx] | rx_frame.data.u8[1];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];

        if (batteryModuleNumber <= 0x2A)  // Run until last pack is read
        {
          VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
          ESP32Can.CANWriteFrame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for next module
        } else {
          min_max_voltage[0] = 9999;
          min_max_voltage[1] = 0;
          for (cellcounter = 0; cellcounter < 108; cellcounter++) {
            if (min_max_voltage[0] > cell_voltages[cellcounter])
              min_max_voltage[0] = cell_voltages[cellcounter];
            if (min_max_voltage[1] < cell_voltages[cellcounter])
              min_max_voltage[1] = cell_voltages[cellcounter];
          }

          if (min_max_voltage[1] >= MAX_CELL_VOLTAGE) {
            set_event(EVENT_CELL_OVER_VOLTAGE, 0);
          }
          if (min_max_voltage[0] <= MIN_CELL_VOLTAGE) {
            set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
          }
          ESP32Can.CANWriteFrame(&VOLVO_SOH_Req);  //Send SOH read request
        }
        rxConsecutiveFrames = 0;
      }
      break;
    default:
      break;
  }
}

void readCellVoltages() {
  battery_request_idx = 0;
  batteryModuleNumber = 0x10;
  rxConsecutiveFrames = 0;
  VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
  ESP32Can.CANWriteFrame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for first module
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

    ESP32Can.CANWriteFrame(&VOLVO_536);  //Send 0x536 Network managing frame to keep BMS alive
    ESP32Can.CANWriteFrame(&VOLVO_372);  //Send 0x372 ECMAmbientTempCalculated

    if (datalayer.battery.status.bms_status == ACTIVE) {
      datalayer.system.status.battery_allows_contactor_closing = true;
    } else {  //datalayer.battery.status.bms_status == FAULT or inverter requested opening contactors
      datalayer.system.status.battery_allows_contactor_closing = false;
    }
  }
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;
    if (datalayer.battery.status.bms_status == ACTIVE) {
      readCellVoltages();
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Volvo SPA XC40 Recharge / Polestar2 78kWh battery selected");
#endif

  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_design_voltage_dV =
      4540;  // 454.0V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 2938;  // 293.8V under this, discharging further is disabled
}
#endif
