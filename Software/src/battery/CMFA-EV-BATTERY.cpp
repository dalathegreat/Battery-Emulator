#include "../include.h"
#ifdef CMFA_EV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "CMFA-EV-BATTERY.h"

/* TODO:
Before this integration can be considered stable, we need to:
- Find the following data points in the transmitted CAN data:
   - Pack voltage (Current implementation might be wrong)
   - Cellvoltage min
   - Cellvoltage max
    - Alternatively all these values can be taken from OBD2 PID polling
/*


/* Do not change code below unless you are sure what you are doing */
CAN_frame CMFA_1EA = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x1EA, .data = {0x00}};
CAN_frame CMFA_125 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x125,
                      .data = {0x7D, 0x7D, 0x7D, 0x07, 0x82, 0x6A, 0x8A}};
CAN_frame CMFA_134 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x134,
                      .data = {0x90, 0x8A, 0x7E, 0x3E, 0xB2, 0x4C, 0x80, 0x00}};
CAN_frame CMFA_135 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x135, .data = {0xD5, 0x85, 0x38, 0x80, 0x01}};
CAN_frame CMFA_3D3 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3D3,
                      .data = {0x47, 0x30, 0x00, 0x02, 0x5D, 0x80, 0x5D, 0xE7}};
CAN_frame CMFA_59B = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x59B, .data = {0x00, 0x02, 0x00}};

CAN_frame CMFA_ACK = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x79B,
                      .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame CMFA_POLLING_FRAME = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x79B,
                                .data = {0x03, 0x22, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00}};

static uint32_t average_voltage_of_cells = 270000;
static uint16_t highest_cell_voltage_mv = 3700;
static uint16_t leadAcidBatteryVoltage = 12000;
static uint16_t highest_cell_voltage_number = 0;
static uint16_t lowest_cell_voltage_number = 0;
static uint64_t cumulative_energy_when_discharging = 0;  // Wh
static uint16_t pack_voltage_polled = 2700;
static uint32_t poll_pid = PID_POLL_PACKVOLTAGE;
static uint16_t pid_reply = 0;
static uint8_t counter_10ms = 0;
static uint8_t content_125[16] = {0x07, 0x0C, 0x01, 0x06, 0x0B, 0x00, 0x05, 0x0A,
                                  0x0F, 0x04, 0x09, 0x0E, 0x03, 0x08, 0x0D, 0x02};
static uint8_t content_135[16] = {0x85, 0xD5, 0x25, 0x75, 0xC5, 0x15, 0x65, 0xB5,
                                  0x05, 0x55, 0xA5, 0xF5, 0x45, 0x95, 0xE5, 0x35};
static unsigned long previousMillis200ms = 0;
static unsigned long previousMillis100ms = 0;
static unsigned long previousMillis10ms = 0;

static uint8_t heartbeat = 0;   //Alternates between 0x55 and 0xAA every 5th frame
static uint8_t heartbeat2 = 0;  //Alternates between 0x55 and 0xAA every 5th frame
static uint32_t SOC = 0;
static uint16_t SOH = 99;
static int16_t current = 0;
static uint16_t pack_voltage = 2700;
static int16_t highest_cell_temperature = 0;
static int16_t lowest_cell_temperature = 0;
static uint32_t discharge_power_w = 0;
static uint32_t charge_power_w = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt = (SOH * 100);

  datalayer.battery.status.real_soc = (SOC * 0.25);

  datalayer.battery.status.current_dA = current * 10;

  datalayer.battery.status.voltage_dV = average_voltage_of_cells / 100;

  datalayer.battery.info.total_capacity_Wh = 27000;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = discharge_power_w;

  datalayer.battery.status.max_charge_power_W = charge_power_w;

  datalayer.battery.status.temperature_min_dC = (lowest_cell_temperature * 10);

  datalayer.battery.status.temperature_max_dC = (highest_cell_temperature * 10);

  datalayer.battery.status.cell_min_voltage_mV = highest_cell_voltage_mv;  //Placeholder until we can find minimum

  datalayer.battery.status.cell_max_voltage_mV = highest_cell_voltage_mv;

  if (leadAcidBatteryVoltage < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x127:           //10ms , Same structure as old Zoe 0x155 message!
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      current = (((((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2]) * 0.25) - 500);
      SOC = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x3D6:  //100ms, Same structure as old Zoe 0x424 message!
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      charge_power_w = rx_frame.data.u8[2] * 500;
      discharge_power_w = rx_frame.data.u8[3] * 500;
      lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
      SOH = rx_frame.data.u8[5];
      heartbeat = rx_frame.data.u8[6];
      highest_cell_temperature = (rx_frame.data.u8[7] - 40);
      break;
    case 0x3D7:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      pack_voltage = ((rx_frame.data.u8[6] << 4 | (rx_frame.data.u8[5] & 0x0F)));
      break;
    case 0x3D8:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //counter_3D8 = rx_frame.data.u8[3]; //?
      //CRC_3D8 = rx_frame.data.u8[4]; //?
      break;
    case 0x43C:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      heartbeat2 = rx_frame.data.u8[2];  //Alternates between 0x55 and 0xAA every 5th frame
      break;
    case 0x431:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //byte0 9C always
      //byte1 40 always
      break;
    case 0x5A9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7BB:                           // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can_frame(&CMFA_ACK, can_config.battery);
      }

      pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];

      switch (pid_reply) {
        case PID_POLL_PACKVOLTAGE:
          pack_voltage_polled =
              (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);  //Hmm, not same LSB as others
          break;
        case PID_POLL_AVERAGE_VOLTAGE_OF_CELLS:
          average_voltage_of_cells =
              (uint32_t)((rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
          break;
        case PID_POLL_HIGHEST_CELL_VOLTAGE:
          highest_cell_voltage_mv = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE:
          highest_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE:
          lowest_cell_voltage_number = rx_frame.data.u8[4];
          break;
        case PID_POLL_12V_BATTERY:
          leadAcidBatteryVoltage = (uint16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING:
          cumulative_energy_when_discharging = (uint64_t)((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                                          (rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]));
          break;
        default:
          break;
      }

      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10ms >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10ms));
    }
    previousMillis10ms = currentMillis;
    transmit_can_frame(&CMFA_1EA, can_config.battery);
    transmit_can_frame(&CMFA_135, can_config.battery);
    transmit_can_frame(&CMFA_134, can_config.battery);
    transmit_can_frame(&CMFA_125, can_config.battery);

    CMFA_135.data.u8[1] = content_135[counter_10ms];
    CMFA_125.data.u8[3] = content_125[counter_10ms];
    counter_10ms = (counter_10ms + 1) % 16;  // counter_10ms cycles between 0-1-2-3..15-0-1...
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&CMFA_59B, can_config.battery);
    transmit_can_frame(&CMFA_3D3, can_config.battery);
  }
  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    switch (poll_pid) {
      case PID_POLL_PACKVOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_PACKVOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_PACKVOLTAGE;
        poll_pid = PID_POLL_AVERAGE_VOLTAGE_OF_CELLS;
        break;
      case PID_POLL_AVERAGE_VOLTAGE_OF_CELLS:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_AVERAGE_VOLTAGE_OF_CELLS >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_AVERAGE_VOLTAGE_OF_CELLS;
        poll_pid = PID_POLL_HIGHEST_CELL_VOLTAGE;
        break;
      case PID_POLL_HIGHEST_CELL_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_HIGHEST_CELL_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_HIGHEST_CELL_VOLTAGE;
        poll_pid = PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE;
        break;
      case PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE;
        poll_pid = PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE;
        break;
      case PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE;
        poll_pid = PID_POLL_12V_BATTERY;
        break;
      case PID_POLL_12V_BATTERY:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_12V_BATTERY >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_12V_BATTERY;
        poll_pid = PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING;
        break;
      case PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING:
        CMFA_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING >> 8);
        CMFA_POLLING_FRAME.data.u8[3] = (uint8_t)PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING;
        poll_pid = PID_POLL_PACKVOLTAGE;
        break;
      default:
        poll_pid = PID_POLL_PACKVOLTAGE;
        break;
    }

    transmit_can_frame(&CMFA_POLLING_FRAME, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "CMFA platform, 27 kWh battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 72;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif  //CMFA_EV_BATTERY
