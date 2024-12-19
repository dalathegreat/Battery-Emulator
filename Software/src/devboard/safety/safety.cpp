#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

static uint16_t cell_deviation_mV = 0;
static uint8_t charge_limit_failures = 0;
static uint8_t discharge_limit_failures = 0;
static bool battery_full_event_fired = false;
static bool battery_empty_event_fired = false;

#define MAX_SOH_DEVIATION_PPTT 2500

//battery pause status begin
bool emulator_pause_request_ON = false;
bool emulator_pause_CAN_send_ON = false;
bool allowed_to_send_CAN = true;

battery_pause_status emulator_pause_status = NORMAL;
//battery pause status end

void update_machineryprotection() {
  // Start checking that the battery is within reason. Incase we see any funny business, raise an event!

  // Pause function is on
  if (emulator_pause_request_ON) {
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_power_W = 0;
  }

  // Battery is overheated!
  if (datalayer.battery.status.temperature_max_dC > BATTERY_MAXTEMPERATURE) {
    set_event(EVENT_BATTERY_OVERHEAT, datalayer.battery.status.temperature_max_dC);
  } else {
    clear_event(EVENT_BATTERY_OVERHEAT);
  }

  // Battery is frozen!
  if (datalayer.battery.status.temperature_min_dC < BATTERY_MINTEMPERATURE) {
    set_event(EVENT_BATTERY_FROZEN, datalayer.battery.status.temperature_min_dC);
  } else {
    clear_event(EVENT_BATTERY_FROZEN);
  }

  // Battery voltage is over designed max voltage!
  if (datalayer.battery.status.voltage_dV > datalayer.battery.info.max_design_voltage_dV) {
    set_event(EVENT_BATTERY_OVERVOLTAGE, datalayer.battery.status.voltage_dV);
  } else {
    clear_event(EVENT_BATTERY_OVERVOLTAGE);
  }

  // Battery voltage is under designed min voltage!
  if (datalayer.battery.status.voltage_dV < datalayer.battery.info.min_design_voltage_dV) {
    set_event(EVENT_BATTERY_UNDERVOLTAGE, datalayer.battery.status.voltage_dV);
  } else {
    clear_event(EVENT_BATTERY_UNDERVOLTAGE);
  }

  // Cell overvoltage, critical latching error without automatic reset. Requires user action.
  if (datalayer.battery.status.cell_max_voltage_mV >= datalayer.battery.info.max_cell_voltage_mV) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  // Cell undervoltage, critical latching error without automatic reset. Requires user action.
  if (datalayer.battery.status.cell_min_voltage_mV <= datalayer.battery.info.min_cell_voltage_mV) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

  // Battery is fully charged. Dont allow any more power into it
  // Normally the BMS will send 0W allowed, but this acts as an additional layer of safety
  if (datalayer.battery.status.reported_soc == 10000)  //Scaled SOC% value is 100.00%
  {
    if (!battery_full_event_fired) {
      set_event(EVENT_BATTERY_FULL, 0);
      battery_full_event_fired = true;
    }
    datalayer.battery.status.max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL);
    battery_full_event_fired = false;
  }

  // Battery is empty. Do not allow further discharge.
  // Normally the BMS will send 0W allowed, but this acts as an additional layer of safety
  if (datalayer.battery.status.reported_soc == 0) {  //Scaled SOC% value is 0.00%
    if (!battery_empty_event_fired) {
      set_event(EVENT_BATTERY_EMPTY, 0);
      battery_empty_event_fired = true;
    }
    datalayer.battery.status.max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY);
    battery_empty_event_fired = false;
  }

  // Battery is extremely degraded, not fit for secondlifestorage!
  if (datalayer.battery.status.soh_pptt < 2500) {
    set_event(EVENT_SOH_LOW, datalayer.battery.status.soh_pptt);
  } else {
    clear_event(EVENT_SOH_LOW);
  }

#ifdef NISSAN_LEAF_BATTERY
  // Check if SOC% is plausible
  if (datalayer.battery.status.voltage_dV >
      (datalayer.battery.info.max_design_voltage_dV -
       100)) {  // When pack voltage is close to max, and SOC% is still low, raise event
    if (datalayer.battery.status.real_soc < 6500) {  // 65.00%
      set_event(EVENT_SOC_PLAUSIBILITY_ERROR, datalayer.battery.status.real_soc);
    } else {
      clear_event(EVENT_SOC_PLAUSIBILITY_ERROR);
    }
  }
#endif  //NISSAN_LEAF_BATTERY

  // Check diff between highest and lowest cell
  cell_deviation_mV =
      std::abs(datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV);
  if (cell_deviation_mV > datalayer.battery.info.max_cell_voltage_deviation_mV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, (cell_deviation_mV / 20));
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  // Inverter is charging with more power than battery wants!
  if (datalayer.battery.status.active_power_W > 0) {  // Charging
    if (datalayer.battery.status.active_power_W > (datalayer.battery.status.max_charge_power_W + 2000)) {
      if (charge_limit_failures > MAX_CHARGE_DISCHARGE_LIMIT_FAILURES) {
        set_event(EVENT_CHARGE_LIMIT_EXCEEDED, 0);  // Alert when 2kW over requested max
      } else {
        charge_limit_failures++;
      }
    } else {
      clear_event(EVENT_CHARGE_LIMIT_EXCEEDED);
      charge_limit_failures = 0;
    }
  }

  // Inverter is pulling too much power from battery!
  if (datalayer.battery.status.active_power_W < 0) {  // Discharging
    if (-datalayer.battery.status.active_power_W > (datalayer.battery.status.max_discharge_power_W + 2000)) {
      if (discharge_limit_failures > MAX_CHARGE_DISCHARGE_LIMIT_FAILURES) {
        set_event(EVENT_DISCHARGE_LIMIT_EXCEEDED, 0);  // Alert when 2kW over requested max
      } else {
        discharge_limit_failures++;
      }
    } else {
      clear_event(EVENT_DISCHARGE_LIMIT_EXCEEDED);
      discharge_limit_failures = 0;
    }
  }

  // Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error
  if (!datalayer.battery.status.CAN_battery_still_alive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    datalayer.battery.status.CAN_battery_still_alive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  // Too many malformed CAN messages recieved!
  if (datalayer.battery.status.CAN_error_counter > MAX_CAN_FAILURES) {
    set_event(EVENT_CAN_RX_WARNING, 1);
  } else {
    clear_event(EVENT_CAN_RX_WARNING);
  }

#ifdef CAN_INVERTER_SELECTED
  // Check if the inverter is still sending CAN messages. If we go 60s without messages we raise an error
  if (!datalayer.system.status.CAN_inverter_still_alive) {
    set_event(EVENT_CAN_INVERTER_MISSING, 0);
  } else {
    datalayer.system.status.CAN_inverter_still_alive--;
    clear_event(EVENT_CAN_INVERTER_MISSING);
  }
#endif  //CAN_INVERTER_SELECTED

#ifdef DOUBLE_BATTERY  // Additional Double-Battery safeties are checked here
  // Check if the Battery 2 BMS is still sending CAN messages. If we go 60s without messages we raise an error

  // Pause function is on
  if (emulator_pause_request_ON) {
    datalayer.battery2.status.max_discharge_power_W = 0;
    datalayer.battery2.status.max_charge_power_W = 0;
  }

  if (!datalayer.battery2.status.CAN_battery_still_alive) {
    set_event(EVENT_CAN2_RX_FAILURE, 0);
  } else {
    datalayer.battery2.status.CAN_battery_still_alive--;
    clear_event(EVENT_CAN2_RX_FAILURE);
  }

  // Too many malformed CAN messages recieved!
  if (datalayer.battery2.status.CAN_error_counter > MAX_CAN_FAILURES) {
    set_event(EVENT_CAN_RX_WARNING, 2);
  } else {
    clear_event(EVENT_CAN_RX_WARNING);
  }

  // Cell overvoltage, critical latching error without automatic reset. Requires user action.
  if (datalayer.battery2.status.cell_max_voltage_mV >= datalayer.battery2.info.max_cell_voltage_mV) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  // Cell undervoltage, critical latching error without automatic reset. Requires user action.
  if (datalayer.battery2.status.cell_min_voltage_mV <= datalayer.battery2.info.min_cell_voltage_mV) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

  // Check diff between highest and lowest cell
  cell_deviation_mV = (datalayer.battery2.status.cell_max_voltage_mV - datalayer.battery2.status.cell_min_voltage_mV);
  if (cell_deviation_mV > datalayer.battery2.info.max_cell_voltage_deviation_mV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, (cell_deviation_mV / 20));
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  // Check if SOH% between the packs is too large
  if ((datalayer.battery.status.soh_pptt != 9900) && (datalayer.battery2.status.soh_pptt != 9900)) {
    // Both values available, check diff
    uint16_t soh_diff_pptt;
    if (datalayer.battery.status.soh_pptt > datalayer.battery2.status.soh_pptt) {
      soh_diff_pptt = datalayer.battery.status.soh_pptt - datalayer.battery2.status.soh_pptt;
    } else {
      soh_diff_pptt = datalayer.battery2.status.soh_pptt - datalayer.battery.status.soh_pptt;
    }

    if (soh_diff_pptt > MAX_SOH_DEVIATION_PPTT) {
      set_event(EVENT_SOH_DIFFERENCE, MAX_SOH_DEVIATION_PPTT);
    } else {
      clear_event(EVENT_SOH_DIFFERENCE);
    }
  }

#endif  // DOUBLE_BATTERY

  //Safeties verified, Zero charge/discharge ampere values incase any safety wrote the W to 0
  if (datalayer.battery.status.max_discharge_power_W == 0) {
    datalayer.battery.status.max_discharge_current_dA = 0;
  }
  if (datalayer.battery.status.max_charge_power_W == 0) {
    datalayer.battery.status.max_charge_current_dA = 0;
  }
}

//battery pause status begin
void setBatteryPause(bool pause_battery, bool pause_CAN, bool equipment_stop, bool store_settings) {

  // First handle equipment stop / resume
  if (equipment_stop && !datalayer.system.settings.equipment_stop_active) {
    datalayer.system.settings.equipment_stop_active = true;
    if (store_settings) {
      store_settings_equipment_stop();
    }

    set_event(EVENT_EQUIPMENT_STOP, 1);
  } else if (!equipment_stop && datalayer.system.settings.equipment_stop_active) {
    datalayer.system.settings.equipment_stop_active = false;
    if (store_settings) {
      store_settings_equipment_stop();
    }
    clear_event(EVENT_EQUIPMENT_STOP);
  }

  emulator_pause_CAN_send_ON = pause_CAN;

  if (pause_battery) {

    set_event(EVENT_PAUSE_BEGIN, 1);
    emulator_pause_request_ON = true;
    emulator_pause_status = PAUSING;
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_power_W = 0;
#ifdef DOUBLE_BATTERY
    datalayer.battery2.status.max_discharge_power_W = 0;
    datalayer.battery2.status.max_charge_power_W = 0;
#endif

  } else {
    clear_event(EVENT_PAUSE_BEGIN);
    set_event(EVENT_PAUSE_END, 0);
    emulator_pause_request_ON = false;
    emulator_pause_CAN_send_ON = false;
    emulator_pause_status = RESUMING;
    clear_event(EVENT_PAUSE_END);
  }

  //immediate check if we can send CAN messages
  emulator_pause_state_send_CAN_battery();
}

/// @brief handle emulator pause status
/// @return true if CAN messages should be sent to battery, false if not
void emulator_pause_state_send_CAN_battery() {
  bool previous_allowed_to_send_CAN = allowed_to_send_CAN;

  if (emulator_pause_status == NORMAL) {
    allowed_to_send_CAN = true;
  }

  // in some inverters this values are not accurate, so we need to check if we are consider 1.8 amps as the limit
  if (emulator_pause_request_ON && emulator_pause_status == PAUSING && datalayer.battery.status.current_dA < 18 &&
      datalayer.battery.status.current_dA > -18) {
    emulator_pause_status = PAUSED;
  }

  if (!emulator_pause_request_ON && emulator_pause_status == RESUMING) {
    emulator_pause_status = NORMAL;
    allowed_to_send_CAN = true;
  }

  allowed_to_send_CAN = (!emulator_pause_CAN_send_ON || emulator_pause_status == NORMAL);

  if (previous_allowed_to_send_CAN && !allowed_to_send_CAN) {
    //completely force stop the CAN communication
    ESP32Can.CANStop();
  } else if (!previous_allowed_to_send_CAN && allowed_to_send_CAN) {
    //resume CAN communication
    ESP32Can.CANInit();
  }
}

std::string get_emulator_pause_status() {
  switch (emulator_pause_status) {
    case NORMAL:
      return "RUNNING";
    case PAUSING:
      return "PAUSING";
    case PAUSED:
      return "PAUSED";
    case RESUMING:
      return "RESUMING";
    default:
      return "UNKNOWN";
  }
}
//battery pause status
