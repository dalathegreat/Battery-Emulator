#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

static uint16_t cell_deviation_mV = 0;

void update_machineryprotection() {
  // Start checking that the battery is within reason. Incase we see any funny business, raise an event!

  // Battery is overheated!
  if (datalayer.battery.status.temperature_max_dC > 500) {
    set_event(EVENT_BATTERY_OVERHEAT, datalayer.battery.status.temperature_max_dC);
  } else {
    clear_event(EVENT_BATTERY_OVERHEAT);
  }

  // Battery is frozen!
  if (datalayer.battery.status.temperature_min_dC < -250) {
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

  // Battery is fully charged. Dont allow any more power into it
  // Normally the BMS will send 0W allowed, but this acts as an additional layer of safety
  if (datalayer.battery.status.reported_soc == 10000)  //Scaled SOC% value is 100.00%
  {
    set_event(EVENT_BATTERY_FULL, 0);
    datalayer.battery.status.max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL);
  }

  // Battery is empty. Do not allow further discharge.
  // Normally the BMS will send 0W allowed, but this acts as an additional layer of safety
  if (datalayer.battery.status.reported_soc == 0) {  //Scaled SOC% value is 0.00%
    set_event(EVENT_BATTERY_EMPTY, 0);
    datalayer.battery.status.max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY);
  }

  // Battery is extremely degraded, not fit for secondlifestorage!
  if (datalayer.battery.status.soh_pptt < 2500) {
    set_event(EVENT_LOW_SOH, datalayer.battery.status.soh_pptt);
  } else {
    clear_event(EVENT_LOW_SOH);
  }

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

  // Check diff between highest and lowest cell
  cell_deviation_mV = (datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV);
  if (cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, (cell_deviation_mV / 20));
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  // Inverter is charging with more power than battery wants!
  if (datalayer.battery.status.active_power_W > 0) {  // Charging
    if (datalayer.battery.status.active_power_W > (datalayer.battery.status.max_charge_power_W + 2000)) {
      set_event(EVENT_CHARGE_LIMIT_EXCEEDED, 0);  // Alert when 2kW over requested max
    } else {
      clear_event(EVENT_CHARGE_LIMIT_EXCEEDED);
    }
  }

  // Inverter is pulling too much power from battery!
  if (datalayer.battery.status.active_power_W < 0) {  // Discharging
    if (-datalayer.battery.status.active_power_W > (datalayer.battery.status.max_discharge_power_W + 2000)) {
      set_event(EVENT_DISCHARGE_LIMIT_EXCEEDED, 0);  // Alert when 2kW over requested max
    } else {
      clear_event(EVENT_DISCHARGE_LIMIT_EXCEEDED);
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
    set_event(EVENT_CAN_RX_WARNING, 0);
  } else {
    clear_event(EVENT_CAN_RX_WARNING);
  }
}
