#include "../include.h"
#ifdef BATRIUM_BMS
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BATRIUM-BMS.h"

/* Do not change code below unless you are sure what you are doing 
Extended canbus identifiers
Default baud rate : 500kB
Little endian for multibyte fields
Version 1.0 – 6/07/2018 – intial draft
Fetched Feb12-2025 from https://wiki.batrium.com/en/faq/native-canbus-protocols 
*/

static uint16_t Hardware_Version = 0;
static uint16_t Firmware_Version = 0;
static uint32_t Device_Serial_NO = 0;
static uint16_t min_cell_voltage = 3700;
static uint16_t max_cell_voltage = 3700;
static uint16_t avg_cell_voltage = 0;
static uint8_t min_voltage_cell_no = 0;
static uint8_t max_voltage_cell_no = 0;
static int16_t min_cell_temperature = 0;
static int16_t max_cell_temperature = 0;
static int16_t avg_cell_temperature = 0;
static uint8_t min_temp_cell_no = 0;
static uint8_t max_temp_cell_no = 0;
static uint8_t number_in_bypass = 0;
static uint8_t number_in_initial_bypass = 0;
static uint8_t number_in_final_bypass = 0;
static int16_t shunt_voltage = MIN_PACK_VOLTAGE_DV + 1;
static int16_t shunt_amperes = 0;
static int16_t shunt_power = 0;
static int16_t state_of_charge = 0;
static uint16_t state_of_health = 9900;
static uint16_t remaining_ah = 0;
static uint16_t nominal_capacity_ah = 0;
static uint16_t charge_target_voltage = 0;
static uint16_t charge_target_amp = 0;
static uint16_t discharge_target_voltage = 0;
static uint16_t discharge_target_amp = 0;
static uint8_t critical_control_flags = 0;
static uint8_t charge_control_flags = 0;
static uint8_t discharge_control_flags = 0;
static uint8_t heat_control_flags = 0;
static uint8_t cool_control_flags = 0;
static uint8_t cell_balancing_flags = 0;

void update_values_battery() {

  if (state_of_charge < 0) {
    datalayer.battery.status.real_soc = 0;  //Emulator cannot handle negative values
  } else {
    datalayer.battery.status.real_soc = (state_of_charge);
  }

  datalayer.battery.status.soh_pptt = state_of_health;

  datalayer.battery.status.voltage_dV = shunt_voltage;

  datalayer.battery.status.current_dA = shunt_amperes;

  datalayer.battery.status.max_charge_power_W = (charge_target_amp * datalayer.battery.status.voltage_dV) / 100;

  datalayer.battery.status.max_discharge_power_W = (discharge_target_amp * datalayer.battery.status.voltage_dV) / 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;

  datalayer.battery.status.temperature_min_dC = min_cell_temperature;

  datalayer.battery.status.temperature_max_dC = max_cell_temperature;

  //Map to extended datalayer for visualization on the Webserver
  datalayer_extended.batriumBMS.Hardware_Version = Hardware_Version;
  datalayer_extended.batriumBMS.Firmware_Version = Firmware_Version;
  datalayer_extended.batriumBMS.Device_Serial_NO = Device_Serial_NO;
  datalayer_extended.batriumBMS.min_cell_voltage = min_cell_voltage;
  datalayer_extended.batriumBMS.max_cell_voltage = max_cell_voltage;
  datalayer_extended.batriumBMS.avg_cell_voltage = avg_cell_voltage;
  datalayer_extended.batriumBMS.min_voltage_cell_no = min_voltage_cell_no;
  datalayer_extended.batriumBMS.max_voltage_cell_no = max_voltage_cell_no;
  datalayer_extended.batriumBMS.min_cell_temperature = min_cell_temperature;
  datalayer_extended.batriumBMS.max_cell_temperature = max_cell_temperature;
  datalayer_extended.batriumBMS.avg_cell_temperature = avg_cell_temperature;
  datalayer_extended.batriumBMS.min_temp_cell_no = min_temp_cell_no;
  datalayer_extended.batriumBMS.max_temp_cell_no = max_temp_cell_no;
  datalayer_extended.batriumBMS.number_in_bypass = number_in_bypass;
  datalayer_extended.batriumBMS.number_in_initial_bypass = number_in_initial_bypass;
  datalayer_extended.batriumBMS.number_in_final_bypass = number_in_final_bypass;
  datalayer_extended.batriumBMS.shunt_voltage = shunt_voltage;
  datalayer_extended.batriumBMS.shunt_amperes = shunt_amperes;
  datalayer_extended.batriumBMS.shunt_power = shunt_power;
  datalayer_extended.batriumBMS.state_of_charge = state_of_charge;
  datalayer_extended.batriumBMS.state_of_health = state_of_health;
  datalayer_extended.batriumBMS.remaining_ah = remaining_ah;
  datalayer_extended.batriumBMS.nominal_capacity_ah = nominal_capacity_ah;
  datalayer_extended.batriumBMS.charge_target_voltage = charge_target_voltage;
  datalayer_extended.batriumBMS.charge_target_amp = charge_target_amp;
  datalayer_extended.batriumBMS.discharge_target_voltage = discharge_target_voltage;
  datalayer_extended.batriumBMS.discharge_target_amp = discharge_target_amp;
  datalayer_extended.batriumBMS.critical_control_flags = critical_control_flags;
  datalayer_extended.batriumBMS.charge_control_flags = charge_control_flags;
  datalayer_extended.batriumBMS.discharge_control_flags = discharge_control_flags;
  datalayer_extended.batriumBMS.heat_control_flags = heat_control_flags;
  datalayer_extended.batriumBMS.cool_control_flags = cool_control_flags;
  datalayer_extended.batriumBMS.cell_balancing_flags = cell_balancing_flags;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case (CAN_BASE_ADDRESS + 0x00):  //Device versioning
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      Hardware_Version = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      Firmware_Version = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      Device_Serial_NO =
          (rx_frame.data.u8[7] << 24 | rx_frame.data.u8[6] << 16 | rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      break;
    case (CAN_BASE_ADDRESS + 0x01):  //Cell voltage limits
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      min_cell_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      max_cell_voltage = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      avg_cell_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      min_voltage_cell_no = rx_frame.data.u8[6];
      max_voltage_cell_no = rx_frame.data.u8[7];
      break;
    case (CAN_BASE_ADDRESS + 0x02):  //Cell temperature limits
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      min_cell_temperature = ((rx_frame.data.u8[0]) - 40);
      max_cell_temperature = ((rx_frame.data.u8[1]) - 40);
      avg_cell_temperature = ((rx_frame.data.u8[2]) - 40);
      min_temp_cell_no = rx_frame.data.u8[3];
      max_temp_cell_no = rx_frame.data.u8[4];
      break;
    case (CAN_BASE_ADDRESS + 0x03):  //Cell bypass summary
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      number_in_bypass = rx_frame.data.u8[0];
      number_in_initial_bypass = rx_frame.data.u8[1];
      number_in_final_bypass = rx_frame.data.u8[2];
      break;
    case (CAN_BASE_ADDRESS + 0x04):  //Shunt power monitoring
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      shunt_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      shunt_amperes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      shunt_power = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      break;
    case (CAN_BASE_ADDRESS + 0x05):  //Shunt state monitoring
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      state_of_charge = (rx_frame.data.u8[1] << 8 |
                         rx_frame.data.u8[0]);  //Note, signed since can go to -10. TODO, make sure this works!
      state_of_health = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      remaining_ah = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      nominal_capacity_ah = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case (CAN_BASE_ADDRESS + 0x06):  //Remote control target limits
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      charge_target_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      charge_target_amp = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      discharge_target_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      discharge_target_amp = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case (CAN_BASE_ADDRESS + 0x07):  //Control flag logic state
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      critical_control_flags = rx_frame.data.u8[0];
      charge_control_flags = rx_frame.data.u8[1];
      discharge_control_flags = rx_frame.data.u8[2];
      heat_control_flags = rx_frame.data.u8[3];
      cool_control_flags = rx_frame.data.u8[4];
      cell_balancing_flags = rx_frame.data.u8[5];
      break;
    case (CAN_BASE_ADDRESS + 0x10):  //Telemetry configuration TX
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case (CAN_BASE_ADDRESS + 0x11):  //Telemetry configuration RX
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // No periodic transmitting towards this BMS needed
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Batrium BMS, DIY battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = NUMBER_OF_CELLS;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}

#endif  // BATRIUM_BMS
