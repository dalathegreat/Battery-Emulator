#include "../include.h"
#ifdef MEB_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MEB-BATTERY.h"

/*
TODO list
- Get contactors closing
- What CAN messages needs to be sent towards the battery to keep it alive
- Check value mappings on the PID polls
- Check value mappings on the constantly broadcasted messages
- Check all TODO:s in the code
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send

#define MAX_CELL_VOLTAGE 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950  //Battery is put into emergency stop if one cell goes below this value

#define PID_SOC 0x028C
#define PID_VOLTAGE 0x1E3B
#define PID_CURRENT 0x1E3D
#define PID_MAX_TEMP 0x1E0E
#define PID_MIN_TEMP 0x1E0F
#define PID_MAX_CHARGE_VOLTAGE 0x5171
#define PID_MIN_DISCHARGE_VOLTAGE 0x5170
#define PID_ALLOWED_CHARGE_POWER 0x1E1B
#define PID_ALLOWED_DISCHARGE_POWER 0x1E1C
static uint32_t poll_pid = 0;
static uint32_t pid_reply = 0;

static uint16_t battery_soc = 0;
static uint16_t battery_voltage = 0;
static int16_t battery_current = 0;
static int16_t battery_max_temp = 0;
static int16_t battery_min_temp = 0;
static uint16_t battery_max_charge_voltage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static uint16_t battery_allowed_charge_power = 0;
static uint16_t battery_allowed_discharge_power = 0;

static uint8_t BMS_20_CRC = 0;
static uint8_t BMS_20_BZ = 0;
static bool BMS_fault_status_contactor = false;
static bool BMS_exp_limits_active = 0;
static uint8_t BMS_is_mode = 0;
static bool BMS_HVIL_status = 0;
static bool BMS_fault_HVbatt_shutdown = 0;
static bool BMS_fault_HVbatt_shutdown_req = 0;
static bool BMS_fault_performance = 0;
static uint16_t BMS_current = 0;
static bool BMS_fault_emergency_shutdown_crash = 0;
static uint32_t BMS_voltage_intermediate = 0;
static uint32_t BMS_voltage = 0;

CAN_frame MEB_POLLING_FRAME = {.FD = true,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x1C40007B,  // SOC 02 8C
                               .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_ACK_FRAME = {.FD = true,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1C40007B,  // Ack
                           .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = battery_soc * 10;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = battery_voltage * 10;

  datalayer.battery.status.current_dA = battery_current;  //TODO: scaling?

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W = battery_allowed_charge_power * 10;  //TODO: scaling?

  datalayer.battery.status.max_discharge_power_W = battery_allowed_discharge_power * 10;  //TODO: scaling?

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.temperature_min_dC = battery_min_temp / 10;

  datalayer.battery.status.temperature_max_dC = battery_max_temp / 10;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;

#ifdef DEBUG_VIA_USB
  Serial.println();  //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print(BMS_voltage);
  Serial.print(" HVIL: ");
  Serial.print(BMS_HVIL_status);
  Serial.print(" Current: ");
  Serial.print(BMS_current);
#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0xCF:  //BMS_20 , TODO: confirm location for all these
      BMS_20_CRC = rx_frame.data.u8[0];
      BMS_20_BZ = (rx_frame.data.u8[1] & 0x0F);
      BMS_fault_status_contactor = (rx_frame.data.u8[1] & 0x20) >> 5;
      //BMS_exp_limits_active
      BMS_is_mode = (rx_frame.data.u8[2] & 0x07);
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x08) >> 3;
      //BMS_fault_HVbatt_shutdown
      //BMS_fault_HVbatt_shutdown_req
      //BMS_fault_performance
      BMS_current = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[3];
      //BMS_fault_emergency_shutdown_crash
      BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5])) * 25;
      BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4)) * 25;
      break;
    case 0x1C42007B:                      // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can(&MEB_ACK_FRAME, can_config.battery);
      }
      if (rx_frame.DLC == 8) {
        pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      } else {  //12 or 24bit message has reply in other location
        pid_reply = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
      }

      switch (pid_reply) {
        case PID_SOC:
          battery_soc = rx_frame.data.u8[4] * 4;  // 135*4 = 54.0%
        case PID_VOLTAGE:
          battery_voltage = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5] / 4;
          break;
        case PID_CURRENT:                                                      //(00 02) 49 F0 45
          battery_current = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5];  //TODO: right bits?
          break;
        case PID_MAX_TEMP:
          battery_max_temp = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        case PID_MIN_TEMP:
          battery_min_temp = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        case PID_MAX_CHARGE_VOLTAGE:
          battery_max_charge_voltage = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          battery_min_discharge_voltage = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_CHARGE_POWER:
          battery_allowed_charge_power = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          battery_allowed_discharge_power = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 500ms message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    switch (poll_pid) {
      case PID_SOC:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_SOC >> 8);  // High byte
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_SOC;         // Low byte
        poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_VOLTAGE;
        poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CURRENT >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CURRENT;
        poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_TEMP;
        poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_TEMP;
        poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_CHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_CHARGE_VOLTAGE;
        poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_DISCHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_DISCHARGE_VOLTAGE;
        poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_CHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_CHARGE_POWER;
        poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_DISCHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_DISCHARGE_POWER;
        poll_pid = PID_SOC;
        break;
      default:
        poll_pid = PID_SOC;
        break;
    }

    transmit_can(&MEB_POLLING_FRAME, can_config.battery);
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
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Volkswagen Group MEB platform battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;  // 404.0V TODO
  datalayer.battery.info.min_design_voltage_dV = 3100;  // 310.0V TODO
}

#endif
