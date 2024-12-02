#include "../include.h"
#ifdef BOLT_AMPERA_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BOLT-AMPERA-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20ms = 0;   // will store last time a 20ms CAN Message was send
static unsigned long previousMillis200ms = 0;  // will store last time a 200ms CAN Message was send

CAN_frame BOLT_778 = {.FD = false,  // Unsure of what this message is, added only as example
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x778,
                      .data = {0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame BOLT_POLL_7E4 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x02, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static uint16_t battery_cell_voltages[96];  //array with all the cellvoltages
static uint16_t battery_capacity_my17_18 = 0;
static uint16_t battery_capacity_my19plus = 0;
static uint16_t battery_SOC_display = 0;
static uint16_t battery_SOC_raw_highprec = 0;
static uint16_t battery_max_temperature = 0;
static uint16_t battery_min_temperature = 0;
static uint16_t battery_min_cell_voltage = 0;
static uint16_t battery_max_cell_voltage = 0;
static uint16_t battery_internal_resistance = 0;
static uint16_t battery_min_voltage = 0;
static uint16_t battery_max_voltage = 0;
static uint16_t battery_voltage = 3700;
static uint16_t battery_vehicle_isolation = 0;
static uint16_t battery_isolation_kohm = 0;
static uint16_t battery_HV_locked = 0;
static uint16_t battery_crash_event = 0;
static uint16_t battery_HVIL = 0;
static uint16_t battery_HVIL_status = 0;
static int16_t battery_current = 0;

static uint8_t poll_index = 0;
static uint16_t currentpoll = POLL_CAPACITY_EST_GEN1;
static uint16_t reply_poll = 0;

const uint16_t poll_commands[115] = {POLL_CAPACITY_EST_GEN1,
                                     POLL_CAPACITY_EST_GEN2,
                                     POLL_SOC_DISPLAY,
                                     POLL_SOC_RAW_HIGHPREC,
                                     POLL_MAX_TEMPERATURE,
                                     POLL_MIN_TEMPERATURE,
                                     POLL_MIN_CELL_V,
                                     POLL_MAX_CELL_V,
                                     POLL_INTERNAL_RES,
                                     POLL_MIN_BATT_V,
                                     POLL_MAX_BATT_V,
                                     POLL_VOLTAGE,
                                     POLL_VEHICLE_ISOLATION,
                                     POLL_ISOLATION_TEST_KOHM,
                                     POLL_HV_LOCKED_OUT,
                                     POLL_CRASH_EVENT,
                                     POLL_HVIL,
                                     POLL_HVIL_STATUS,
                                     POLL_CURRENT,
                                     POLL_CELL_01,
                                     POLL_CELL_02,
                                     POLL_CELL_03,
                                     POLL_CELL_04,
                                     POLL_CELL_05,
                                     POLL_CELL_06,
                                     POLL_CELL_07,
                                     POLL_CELL_08,
                                     POLL_CELL_09,
                                     POLL_CELL_10,
                                     POLL_CELL_11,
                                     POLL_CELL_12,
                                     POLL_CELL_13,
                                     POLL_CELL_14,
                                     POLL_CELL_15,
                                     POLL_CELL_16,
                                     POLL_CELL_17,
                                     POLL_CELL_18,
                                     POLL_CELL_19,
                                     POLL_CELL_20,
                                     POLL_CELL_21,
                                     POLL_CELL_22,
                                     POLL_CELL_23,
                                     POLL_CELL_24,
                                     POLL_CELL_25,
                                     POLL_CELL_26,
                                     POLL_CELL_27,
                                     POLL_CELL_28,
                                     POLL_CELL_29,
                                     POLL_CELL_30,
                                     POLL_CELL_31,
                                     POLL_CELL_32,
                                     POLL_CELL_33,
                                     POLL_CELL_34,
                                     POLL_CELL_35,
                                     POLL_CELL_36,
                                     POLL_CELL_37,
                                     POLL_CELL_38,
                                     POLL_CELL_39,
                                     POLL_CELL_40,
                                     POLL_CELL_41,
                                     POLL_CELL_42,
                                     POLL_CELL_43,
                                     POLL_CELL_44,
                                     POLL_CELL_45,
                                     POLL_CELL_46,
                                     POLL_CELL_47,
                                     POLL_CELL_48,
                                     POLL_CELL_49,
                                     POLL_CELL_50,
                                     POLL_CELL_51,
                                     POLL_CELL_52,
                                     POLL_CELL_53,
                                     POLL_CELL_54,
                                     POLL_CELL_55,
                                     POLL_CELL_56,
                                     POLL_CELL_57,
                                     POLL_CELL_58,
                                     POLL_CELL_59,
                                     POLL_CELL_60,
                                     POLL_CELL_61,
                                     POLL_CELL_62,
                                     POLL_CELL_63,
                                     POLL_CELL_64,
                                     POLL_CELL_65,
                                     POLL_CELL_66,
                                     POLL_CELL_67,
                                     POLL_CELL_68,
                                     POLL_CELL_69,
                                     POLL_CELL_70,
                                     POLL_CELL_71,
                                     POLL_CELL_72,
                                     POLL_CELL_73,
                                     POLL_CELL_74,
                                     POLL_CELL_75,
                                     POLL_CELL_76,
                                     POLL_CELL_77,
                                     POLL_CELL_78,
                                     POLL_CELL_79,
                                     POLL_CELL_80,
                                     POLL_CELL_81,
                                     POLL_CELL_82,
                                     POLL_CELL_83,
                                     POLL_CELL_84,
                                     POLL_CELL_85,
                                     POLL_CELL_86,
                                     POLL_CELL_87,
                                     POLL_CELL_88,
                                     POLL_CELL_89,
                                     POLL_CELL_90,
                                     POLL_CELL_91,
                                     POLL_CELL_92,
                                     POLL_CELL_93,
                                     POLL_CELL_94,
                                     POLL_CELL_95,
                                     POLL_CELL_96};

void update_values_battery() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = (battery_SOC_display * 100 / 255);

  datalayer.battery.status.voltage_dV = battery_voltage * 0.52;

  datalayer.battery.status.current_dA = battery_current / -6.675;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, battery_cell_voltages, 96 * sizeof(uint16_t));
}

void receive_can_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x2C7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:  //TODO: Confirm if this is the reply from BMS when polling
      //Frame 2 & 3 contains reply
      reply_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

      switch (reply_poll) {
        case POLL_CAPACITY_EST_GEN1:
          battery_capacity_my17_18 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CAPACITY_EST_GEN2:
          battery_capacity_my19plus = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_DISPLAY:
          battery_SOC_display = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_RAW_HIGHPREC:
          battery_SOC_raw_highprec = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_TEMPERATURE:
          battery_max_temperature = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_TEMPERATURE:
          battery_min_temperature = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_CELL_V:
          battery_min_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_CELL_V:
          battery_max_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_INTERNAL_RES:
          battery_internal_resistance = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_BATT_V:
          battery_min_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_BATT_V:
          battery_max_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_VOLTAGE:
          battery_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_VEHICLE_ISOLATION:
          battery_vehicle_isolation = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_ISOLATION_TEST_KOHM:
          battery_isolation_kohm = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_HV_LOCKED_OUT:
          battery_HV_locked = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CRASH_EVENT:
          battery_crash_event = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_HVIL:
          battery_HVIL = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_HVIL_STATUS:
          battery_HVIL_status = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT:
          battery_current = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_01:
          battery_cell_voltages[0] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_02:
          battery_cell_voltages[1] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_03:
          battery_cell_voltages[2] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_04:
          battery_cell_voltages[3] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_05:
          battery_cell_voltages[4] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_06:
          battery_cell_voltages[5] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_07:
          battery_cell_voltages[6] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_08:
          battery_cell_voltages[7] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_09:
          battery_cell_voltages[8] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_10:
          battery_cell_voltages[9] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_11:
          battery_cell_voltages[10] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_12:
          battery_cell_voltages[11] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_13:
          battery_cell_voltages[12] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_14:
          battery_cell_voltages[13] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_15:
          battery_cell_voltages[14] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_16:
          battery_cell_voltages[15] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_17:
          battery_cell_voltages[16] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_18:
          battery_cell_voltages[17] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_19:
          battery_cell_voltages[18] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_20:
          battery_cell_voltages[19] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_21:
          battery_cell_voltages[20] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_22:
          battery_cell_voltages[21] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_23:
          battery_cell_voltages[22] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_24:
          battery_cell_voltages[23] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_25:
          battery_cell_voltages[24] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_26:
          battery_cell_voltages[25] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_27:
          battery_cell_voltages[26] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_28:
          battery_cell_voltages[27] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_29:
          battery_cell_voltages[28] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_30:
          battery_cell_voltages[29] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_31:
          battery_cell_voltages[30] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_32:
          battery_cell_voltages[31] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_33:
          battery_cell_voltages[32] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_34:
          battery_cell_voltages[33] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_35:
          battery_cell_voltages[34] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_36:
          battery_cell_voltages[35] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_37:
          battery_cell_voltages[36] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_38:
          battery_cell_voltages[37] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_39:
          battery_cell_voltages[38] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_40:
          battery_cell_voltages[39] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_41:
          battery_cell_voltages[40] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_42:
          battery_cell_voltages[41] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_43:
          battery_cell_voltages[42] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_44:
          battery_cell_voltages[43] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_45:
          battery_cell_voltages[44] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_46:
          battery_cell_voltages[45] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_47:
          battery_cell_voltages[46] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_48:
          battery_cell_voltages[47] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_49:
          battery_cell_voltages[48] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_50:
          battery_cell_voltages[49] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_51:
          battery_cell_voltages[50] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_52:
          battery_cell_voltages[51] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_53:
          battery_cell_voltages[52] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_54:
          battery_cell_voltages[53] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_55:
          battery_cell_voltages[54] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_56:
          battery_cell_voltages[55] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_57:
          battery_cell_voltages[56] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_58:
          battery_cell_voltages[57] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_59:
          battery_cell_voltages[58] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_60:
          battery_cell_voltages[59] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_61:
          battery_cell_voltages[60] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_62:
          battery_cell_voltages[61] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_63:
          battery_cell_voltages[62] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_64:
          battery_cell_voltages[63] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_65:
          battery_cell_voltages[64] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_66:
          battery_cell_voltages[65] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_67:
          battery_cell_voltages[66] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_68:
          battery_cell_voltages[67] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_69:
          battery_cell_voltages[68] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_70:
          battery_cell_voltages[69] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_71:
          battery_cell_voltages[70] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_72:
          battery_cell_voltages[71] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_73:
          battery_cell_voltages[72] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_74:
          battery_cell_voltages[73] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_75:
          battery_cell_voltages[74] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_76:
          battery_cell_voltages[75] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_77:
          battery_cell_voltages[76] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_78:
          battery_cell_voltages[77] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_79:
          battery_cell_voltages[78] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_80:
          battery_cell_voltages[79] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_81:
          battery_cell_voltages[80] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_82:
          battery_cell_voltages[81] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_83:
          battery_cell_voltages[82] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_84:
          battery_cell_voltages[83] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_85:
          battery_cell_voltages[84] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_86:
          battery_cell_voltages[85] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_87:
          battery_cell_voltages[86] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_88:
          battery_cell_voltages[87] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_89:
          battery_cell_voltages[88] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_90:
          battery_cell_voltages[89] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_91:
          battery_cell_voltages[90] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_92:
          battery_cell_voltages[91] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_93:
          battery_cell_voltages[92] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_94:
          battery_cell_voltages[93] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_95:
          battery_cell_voltages[94] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_96:
          battery_cell_voltages[95] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        default:
          break;
      }
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 20ms message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis20ms >= INTERVAL_20_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis20ms));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis20ms = currentMillis;
    transmit_can(&BOLT_778, can_config.battery);
  }

  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    // Update current poll from the array
    currentpoll = poll_commands[poll_index];
    poll_index = (poll_index + 1) % 115;

    BOLT_POLL_7E4.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
    BOLT_POLL_7E4.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

    transmit_can(&BOLT_POLL_7E4, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Chevrolet Bolt EV/Opel Ampera-e", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
