#include "../include.h"
#ifdef KIA_SOUL_27_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "KIA-SOUL-27-BATTERY.h"

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

CAN_frame KIA_SOUL_200 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x200,
                          .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x100:
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  if (!startedUp) {
    return;  // Don't send any CAN messages towards battery until it has started up
  }

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&KIA_SOUL_200, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Kia Soul 27kWh battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
