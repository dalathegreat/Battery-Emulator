#include "../include.h"
#ifdef STELLANTIS_ECMP_BATTERY
#include <algorithm>  // For std::min and std::max
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For More Battery Info page
#include "../devboard/utils/events.h"
#include "ECMP-BATTERY.h"

/* TODO:
This integration is still ongoing. Here is what still needs to be done in order to use this battery type
- Figure out contactor closing
   - Which CAN messages need to be sent towards the battery?
- Handle 54/70kWh cellcounting properly
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent

//Actual content messages
CAN_frame ECMP_382 = {
    .FD = false,  //BSI_Info (VCU) PSA specific
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x382,
    .data = {0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //09 20 on AC charge. 0A 20 on DC charge

static uint16_t battery_voltage = 370;
static uint16_t battery_soc = 0;
static int16_t battery_current = 0;
static uint8_t battery_MainConnectorState = 0;
static bool battery_RelayOpenRequest = false;
static uint16_t battery_AllowedMaxChargeCurrent = 0;
static uint16_t battery_AllowedMaxDischargeCurrent = 0;
static uint16_t battery_insulationResistanceKOhm = 0;
static int16_t battery_highestTemperature = 0;
static int16_t battery_lowestTemperature = 0;
static uint16_t cellvoltages[108];

void update_values_battery() {

  datalayer.battery.status.real_soc = battery_soc * 10;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = battery_voltage * 10;

  datalayer.battery.status.current_dA = battery_current * 10;

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.max_charge_power_W = battery_AllowedMaxChargeCurrent * battery_voltage;

  datalayer.battery.status.max_discharge_power_W = battery_AllowedMaxDischargeCurrent * battery_voltage;

  datalayer.battery.status.temperature_min_dC = battery_lowestTemperature * 10;

  datalayer.battery.status.temperature_max_dC = battery_highestTemperature * 10;

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < datalayer.battery.info.number_of_cells; ++i) {
    uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
    if (voltage_mV != 0) {  // Skip unread values (0)
      min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
      max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
    }
  }
  // If all array values are 0, reset min/max to 3700
  if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
    min_cell_mv_value = 3700;
    max_cell_mv_value = 3700;
  }

  datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
  datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;

  // Update extended datalayer (More Battery Info page)
  datalayer_extended.stellantisECMP.MainConnectorState = battery_MainConnectorState;
  datalayer_extended.stellantisECMP.InsulationResistance = battery_insulationResistanceKOhm;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x125:  //Common
      battery_soc = (rx_frame.data.u8[0] << 2) |
                    (rx_frame.data.u8[1] >> 6);  // Byte1, bit 7 length 10 (0x3FE when abnormal) (0-1000 ppt)
      battery_MainConnectorState = ((rx_frame.data.u8[2] & 0x30) >>
                                    4);  //Byte2 , bit 4, length 2 ((00 contactors open, 01 precharged, 11 invalid))
      battery_voltage =
          (rx_frame.data.u8[3] << 1) | (rx_frame.data.u8[4] >> 7);  //Byte 4, bit 7, length 9 (0x1FE if invalid)
      battery_current = (((rx_frame.data.u8[4] & 0x7F) << 5) | (rx_frame.data.u8[5] >> 3)) -
                        600;  // Byte5, Bit 3 length 12 (0xFFE when abnormal) (-600 to 600 , offset -600)
      //battery_RelayOpenRequest = // Byte 5, bit 6, length 1 (0 no request, 1 battery requests contactor opening)
      //Stellantis doc seems wrong, could Byte5 be misspelled as Byte2? Bit will otherwise collide with battery_current
      break;
    case 0x127:  //DFM specific
      battery_AllowedMaxChargeCurrent =
          (rx_frame.data.u8[0] << 2) |
          ((rx_frame.data.u8[1] & 0xC0) >> 6);  //Byte 1, bit 7, length 10 (0-600A) [0x3FF if invalid]
      battery_AllowedMaxDischargeCurrent =
          ((rx_frame.data.u8[2] & 0x3F) << 4) |
          (rx_frame.data.u8[3] >> 4);  //Byte 2, bit 5, length 10 (0-600A) [0x3FF if invalid]
      break;
    case 0x129:  //PSA specific
      break;
    case 0x31B:
      break;
    case 0x358:  //Common
      battery_highestTemperature = rx_frame.data.u8[6] - 40;
      battery_lowestTemperature = rx_frame.data.u8[7] - 40;
      break;
    case 0x359:
      break;
    case 0x361:
      break;
    case 0x362:
      break;
    case 0x454:
      break;
    case 0x494:
      break;
    case 0x594:
      break;
    case 0x6D0:  //Common
      battery_insulationResistanceKOhm =
          (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];  //Byte 2, bit 7, length 16 (0-60000 kOhm)
      break;
    case 0x6D1:
      break;
    case 0x6D2:
      break;
    case 0x6D3:
      cellvoltages[0] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[1] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[2] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[3] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6D4:
      cellvoltages[4] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[5] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[6] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[7] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E0:
      break;
    case 0x6E1:
      cellvoltages[8] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[9] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[10] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[11] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E2:
      cellvoltages[12] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[13] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[14] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[15] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E3:
      break;
    case 0x6E4:
      break;
    case 0x6E5:
      break;
    case 0x6E6:
      break;
    case 0x6E7:
      cellvoltages[16] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[17] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[18] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[19] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E8:
      cellvoltages[20] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[21] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[22] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[23] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E9:
      cellvoltages[24] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[25] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[26] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[27] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EB:
      cellvoltages[28] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[29] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[30] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[31] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EC:
      //Not available on e-C4
      break;
    case 0x6ED:
      cellvoltages[32] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[33] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[34] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[35] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EE:
      cellvoltages[36] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[37] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[38] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[39] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EF:
      cellvoltages[40] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[41] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[42] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[43] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F0:
      cellvoltages[44] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[45] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[46] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[47] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F1:
      cellvoltages[48] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[49] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[50] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[51] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F2:
      cellvoltages[52] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[53] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[54] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[55] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F3:
      cellvoltages[56] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[57] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[58] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[59] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F4:
      cellvoltages[60] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[61] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[62] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[63] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F5:
      cellvoltages[64] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[65] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[66] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[67] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F6:
      cellvoltages[68] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[69] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[70] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[71] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F7:
      cellvoltages[72] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[73] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[74] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[75] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F8:
      cellvoltages[76] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[77] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[78] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[79] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F9:
      cellvoltages[80] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[81] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[82] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[83] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FA:
      cellvoltages[84] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[85] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[86] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[87] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FB:
      cellvoltages[88] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[89] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[90] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[91] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FC:
      cellvoltages[92] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[93] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[94] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[95] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FD:
      cellvoltages[96] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[97] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[98] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[99] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FE:
      cellvoltages[100] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[101] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[102] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[103] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FF:
      cellvoltages[104] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[105] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[106] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[107] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, 108 * sizeof(uint16_t));
      break;
    case 0x794:
      break;
    default:
      break;
  }
}

void transmit_can_battery(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&ECMP_382, can_config.battery);  //PSA Specific!
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Stellantis ECMP battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_design_voltage_dV = 4546;  // 454.6V, charging over this is not possible
  datalayer.battery.info.min_design_voltage_dV = 3210;  // 321.0V, under this, discharging further is disabled
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
