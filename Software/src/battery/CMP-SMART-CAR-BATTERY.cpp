#include "CMP-SMART-CAR-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For More Battery Info page
#include "../devboard/utils/events.h"

/*TODO:
The following messages should be sent towards the battery to emulate the vehicle still being attached
-0x208 VCU 10ms
-0x211 VCU 100ms (CONTACTOR CONTROL MESSAGE)
-0x217 INV 10ms
-0x231 VCU-HVAC 100ms
-0x241 VCU 10ms
-0x262 VCU 10ms
-0x351 Airbag 60ms
-0x421 VCU 50ms
-0x422 VCU 100ms
-0x432 BCM 50ms
-0x4A2 OBC plug 100ms
-0x552 VCU mileage 1000ms
*/

/* Do not change code below unless you are sure what you are doing */
void CmpSmartCarBattery::update_values() {

  datalayer.battery.status.real_soc = battery_soc * 10;

  datalayer.battery.status.soh_pptt = SOH_estimated * 100;

  datalayer.battery.status.voltage_dV = battery_voltage;

  if (battery_current > -1500) {
    datalayer.battery.status.current_dA = battery_current;
  }

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.info.total_capacity_Wh = total_energy_when_full_Wh;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W = regen_charge_cont_power * 100;

  datalayer.battery.status.max_discharge_power_W = discharge_cont_available_power * 100;

  datalayer.battery.status.temperature_min_dC = battery_temperature_minimum * 10;

  datalayer.battery.status.temperature_max_dC = battery_temperature_maximum * 10;

  datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;

  datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;

  if (number_of_cells > 0) {
    datalayer.battery.info.number_of_cells = number_of_cells;
  }

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cell_voltages_mV, number_of_cells * sizeof(uint16_t));

  datalayer.battery.status.total_discharged_battery_Wh = lifetime_kWh_discharged;

  datalayer.battery.status.total_charged_battery_Wh = lifetime_kWh_charged;

  if (thermal_runaway == 0x01) {
    set_event(EVENT_THERMAL_RUNAWAY, 0);
  }

  if (alert_overcharge) {
    set_event(EVENT_CHARGE_LIMIT_EXCEEDED, 0);
  }
}

bool checksum_OK(CAN_frame& rx_frame, uint8_t magic_byte) {
  // Sum all data nibbles from bytes 0-6 (excluding last byte)
  uint8_t sum = 0;

  for (int i = 0; i < 7; i++) {
    sum += (rx_frame.data.u8[i] >> 4);
    sum += (rx_frame.data.u8[i] & 0x0F);
  }

  // Get counter from last byte low nibble
  uint8_t counter = (rx_frame.data.u8[7] & 0x0F);

  // Get checksum from last byte high nibble
  uint8_t checksum = (rx_frame.data.u8[7] >> 4);

  // Calculate expected checksum: (magic_byte - sum - counter) mod 16
  int16_t expected = (magic_byte - (sum & 0x0F) - counter) % 16;
  if (expected < 0) {
    expected += 16;  // Ensure positive modulo result
  }

  return (checksum == expected);
}

uint8_t calculate_checksum(CAN_frame& rx_frame, uint8_t magic_byte) {
  // Sum all data nibbles from bytes 0-6 (excluding last byte)
  uint8_t sum = 0;

  for (int i = 0; i < 7; i++) {
    sum += (rx_frame.data.u8[i] >> 4);
    sum += (rx_frame.data.u8[i] & 0x0F);
  }

  // Get counter from last byte low nibble
  uint8_t counter = (rx_frame.data.u8[7] & 0x0F);

  // Calculate expected checksum: (magic_byte - sum - counter) mod 16
  int16_t calculated_checksum = (magic_byte - (sum & 0x0F) - counter) % 16;
  if (calculated_checksum < 0) {
    calculated_checksum += 16;  // Ensure positive modulo result
  }

  return calculated_checksum;
}

void CmpSmartCarBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x205:  //10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (!checksum_OK(rx_frame, 0x18)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message checksum incorrect, abort reading data from it
      }
      battery_current = ((rx_frame.data.u8[0] << 7) | (rx_frame.data.u8[1] >> 1)) - 1500;  //0 in all discharge logs?
      battery_soc = ((rx_frame.data.u8[2] & 0x1F) << 5) | (rx_frame.data.u8[3] >> 3);
      battery_voltage =
          ((((rx_frame.data.u8[3] & 0x07) << 10) | (rx_frame.data.u8[4] << 2) | (rx_frame.data.u8[5] >> 6)));
      battery_state = ((rx_frame.data.u8[1] & 0x01) | (rx_frame.data.u8[2] >> 5));
      battery_fault = ((rx_frame.data.u8[5] & 0x01) | (rx_frame.data.u8[6] >> 5));  // 0 no failt, 1-5 level
      /*In case of Level 5 Fault, Battery will go to safe state. 
        In case of Level 4 Fault, continue the ignition cycle and disable the next cycle unless the issues is addressed
        In case of Level 1 Fault, Battery will derate its Power output*/
      battery_negative_contactor_state = ((rx_frame.data.u8[5] & 0x06) >> 1);
      battery_precharge_contactor_state = ((rx_frame.data.u8[5] & 0x18) >> 3);
      battery_positive_contactor_state = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery_connect_status = (rx_frame.data.u8[6] & 0x03);
      battery_charging_status = ((rx_frame.data.u8[6] & 0x1C) >> 5);
      //counter_205 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x235:  //10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      discharge_available_10s_power = ((rx_frame.data.u8[0] << 4) | (rx_frame.data.u8[1] >> 4));  //*0.1kW
      discharge_available_10s_current =
          (((rx_frame.data.u8[1] & 0x0F) << 9) | (rx_frame.data.u8[2] << 1) | (rx_frame.data.u8[3] >> 7));  //*0.1A
      break;
    case 0x275:  //20ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (!checksum_OK(rx_frame, 0x01)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message checksum incorrect, abort reading data from it
      }
      discharge_cont_available_power = ((rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] >> 5));      //*0.1kW
      discharge_cont_available_current = (((rx_frame.data.u8[1] & 0x1F) << 8) | rx_frame.data.u8[2]);  //*0.1A
      discharge_available_30s_current = ((rx_frame.data.u8[3] << 5) | (rx_frame.data.u8[4] >> 3));     //*0.1A
      discharge_available_30s_power = (((rx_frame.data.u8[4] & 0x07) << 8) | rx_frame.data.u8[5]);     //*0.1kW
      //counter_275 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x285:  //20ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (!checksum_OK(rx_frame, 0x00)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message checksum incorrect, abort reading data from it
      }
      regen_charge_cont_power = ((rx_frame.data.u8[0] << 4) | (rx_frame.data.u8[1] >> 4));      //*0.1kW
      regen_charge_30s_power = (((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2]);     //*0.1kW
      regen_charge_30s_current = ((rx_frame.data.u8[3] << 5) | (rx_frame.data.u8[4] >> 3));     //*0.1A
      regen_charge_cont_current = (((rx_frame.data.u8[4] & 0x07) << 8) | rx_frame.data.u8[5]);  //*0.1A
      //counter_285 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x2A5:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_quickcharge_connect_status = rx_frame.data.u8[0] >> 6;
      regen_charge_10s_current = (((rx_frame.data.u8[0] & 0x3F) << 1) | (rx_frame.data.u8[1] >> 1));  //*0.1a
      regen_charge_10s_power = ((rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[3] >> 4));             //*0.1kW
      quick_charge_port_voltage = ((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4));          //*0.1kW
      qc_negative_contactor_status = rx_frame.data.u8[6] & 0x03;
      qc_positive_contactor_status = (rx_frame.data.u8[6] & 0x0C) >> 2;
      /*0b00 : Open
      0b01 : Close(after positive and negative contactor close)
      0b10 : Fault(weld or open)2
      0b11 : Reserved*/
      //counter_2A5 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x325:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (!checksum_OK(rx_frame, 0x05)) {
        datalayer.battery.status.CAN_error_counter++;
        break;  //Message checksum incorrect, abort reading data from it
      }
      battery_balancing_active = rx_frame.data.u8[0] & 0x01;
      eplug_status = (rx_frame.data.u8[0] & 0x06) >> 1;  //0 connected, 1 disconnected, 2 open status, 3 invalid
      ev_warning = (rx_frame.data.u8[0] & 0x18) >> 3;
      power_auth = (rx_frame.data.u8[0] & 0x20) >> 5;
      HVIL_status = (rx_frame.data.u8[0] & 0xC0) >> 6;
      hardware_fault_status = (rx_frame.data.u8[1] & 0x1C) >> 2;
      insulation_fault = (rx_frame.data.u8[1] & 0xE0) >> 5;
      temperature = (((rx_frame.data.u8[1] & 0x03) << 7) | (rx_frame.data.u8[2] >> 1));  //*0.1kW
      service_due = rx_frame.data.u8[3] & 0x03;
      plausibility_error = (rx_frame.data.u8[3] & 0x3C) >> 2;
      insulation_circuit_status = (rx_frame.data.u8[3] & 0xC0) >> 6;
      l3_fault = rx_frame.data.u8[4] & 0x3F;
      master_warning = (rx_frame.data.u8[4] & 0xC0) >> 6;
      insulation_resistance_kOhm = ((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6]));
      //counter_325 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x334:  // Cellvoltages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] >> 3);  // Mux goes from 0-25

      if (mux < 25) {  // Only process valid cell data frames (0-24) (100S, but protocol does support 108S)
        uint8_t base_index = mux * 4;

        cell_voltages_mV[base_index + 0] = ((rx_frame.data.u8[1] << 4) | (rx_frame.data.u8[2] >> 4)) * 4;
        cell_voltages_mV[base_index + 1] = ((rx_frame.data.u8[3] & 0xFF) << 4) | (rx_frame.data.u8[4] >> 4);
        cell_voltages_mV[base_index + 2] = (((rx_frame.data.u8[4] & 0x0F) << 8) | (rx_frame.data.u8[5])) * 4;
        cell_voltages_mV[base_index + 3] = ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[7];
      }
      break;
    case 0x335:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //checksum_335 //Init value 0xB
      DC_bus_voltage = ((rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3));
      charge_max_voltage =
          (((rx_frame.data.u8[1] & 0x07) << 10) | (rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] >> 6));
      charge_cont_curr_max = (((rx_frame.data.u8[3] & 0x3F) << 7) | (rx_frame.data.u8[4] >> 1));
      charge_cont_curr_req =
          (((rx_frame.data.u8[4] & 0x01) << 12) | (rx_frame.data.u8[5] << 4) | (rx_frame.data.u8[6] >> 4));
      //counter_335 = (rx_frame.data.u8[7] & 0x0F);
      break;
    case 0x385:  //1000ms Wakeup state message, not needed for integration
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F4:  //Event triggered
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] >> 5);
      if (mux == 0) {  //On some packs, there are more muxes (0-2-3-4-5-6-7)
        temperature_sensors[0] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[1] = (rx_frame.data.u8[2] - 40);
        temperature_sensors[2] = (rx_frame.data.u8[3] - 40);
        temperature_sensors[3] = (rx_frame.data.u8[4] - 40);
        temperature_sensors[4] = (rx_frame.data.u8[5] - 40);
        temperature_sensors[5] = (rx_frame.data.u8[6] - 40);
        temperature_sensors[6] = (rx_frame.data.u8[7] - 40);
      } else if (mux == 0x2) {
        temperature_sensors[7] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[8] = (rx_frame.data.u8[2] - 40);
        temperature_sensors[9] = (rx_frame.data.u8[3] - 40);
        temperature_sensors[10] = (rx_frame.data.u8[4] - 40);
        temperature_sensors[11] = (rx_frame.data.u8[5] - 40);
        temperature_sensors[12] = (rx_frame.data.u8[6] - 40);
        temperature_sensors[13] = (rx_frame.data.u8[7] - 40);
      } else if (mux == 0x4) {
        temperature_sensors[14] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[15] = (rx_frame.data.u8[2] - 40);
      }
      break;
    case 0x434:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      hours_spent_overvoltage = ((rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3)) / 10;
      hours_spent_overtemperature =
          (((rx_frame.data.u8[1] & 0x07) << 10) | (rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] >> 6)) / 10;
      hours_spent_undertemperature = (((rx_frame.data.u8[3] & 0x3F) << 7) | (rx_frame.data.u8[4] >> 1)) / 10;
      total_coloumb_counting_Ah =
          (((rx_frame.data.u8[4] & 0x01) << 11) | (rx_frame.data.u8[5] << 3) | (rx_frame.data.u8[6] >> 5)) * 125;
      total_coulomb_counting_kWh = (((rx_frame.data.u8[6] & 0x1F) << 7) | (rx_frame.data.u8[7] >> 1)) * 35;
      break;
    case 0x435:  //500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_temperature_maximum = rx_frame.data.u8[0] - 40;
      min_cell_voltage = ((rx_frame.data.u8[2] << 6) | (rx_frame.data.u8[3] >> 2));
      min_cell_voltage_number = rx_frame.data.u8[4];
      max_cell_voltage = ((rx_frame.data.u8[5] << 6) | (rx_frame.data.u8[6] >> 2));
      max_cell_voltage_number = rx_frame.data.u8[7];
      break;
    case 0x455:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      nominal_voltage = ((rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3));
      charge_continue_power_limit =
          (((rx_frame.data.u8[1] & 0x07) << 9) | (rx_frame.data.u8[2] << 1) | (rx_frame.data.u8[3] >> 7));
      charge_energy_amount_requested = (((rx_frame.data.u8[3] & 0x7F) << 5) | (rx_frame.data.u8[4] >> 3));
      bulk_SOC_DC_limit = rx_frame.data.u8[6] >> 1;
      break;
    case 0x485:  //Non-periodic
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //heating_contactor_operating_count = ((rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7]))
      break;
    case 0x494:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //total_coloumb_counting_Ah
      lifetime_kWh_charged = (((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2]) * 35;
      lifetime_kWh_discharged = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4)) * 35;
      hours_spent_exceeding_charge_power = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]);
      hours_spent_exceeding_discharge_power = (((rx_frame.data.u8[4] & 0x7F) << 5) | (rx_frame.data.u8[7] >> 3));
      break;
    case 0x535:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_actual = ((rx_frame.data.u8[0] << 2) | (rx_frame.data.u8[1] >> 6));
      alert_low_battery_energy = rx_frame.data.u8[1] & 0x01;
      battery_temperature_average = rx_frame.data.u8[4] - 40;
      battery_minimum_voltage_reached_warning = rx_frame.data.u8[7] & 0x01;
      break;
    case 0x543:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      remaining_energy_Wh = ((rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] >> 5)) * 32;
      total_energy_when_full_Wh = ((rx_frame.data.u8[2] << 3) | (rx_frame.data.u8[3] >> 5)) * 32;
      SOH_internal_resistance = (((rx_frame.data.u8[3] & 0x1F) << 2) | (rx_frame.data.u8[4] >> 6));
      SOH_estimated = (rx_frame.data.u8[5] >> 1);
      break;
    case 0x583:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_temperature_probe_number = rx_frame.data.u8[0];
      min_temperature_probe_number = rx_frame.data.u8[1];
      battery_temperature_minimum = rx_frame.data.u8[2] - 40;
      alert_cell_undervoltage = rx_frame.data.u8[3] & 0x01;
      alert_cell_overvoltage = (rx_frame.data.u8[3] & 0x02) >> 1;
      alert_high_SOC = (rx_frame.data.u8[3] & 0x04) >> 2;
      alert_low_SOC = (rx_frame.data.u8[3] & 0x08) >> 3;
      alert_overvoltage = (rx_frame.data.u8[3] & 0x10) >> 4;
      alert_high_temperature = (rx_frame.data.u8[3] & 0x20) >> 5;
      alert_temperature_delta = (rx_frame.data.u8[3] & 0x40) >> 6;
      alert_battery = (rx_frame.data.u8[3] & 0x80) >> 7;
      alert_SOC_jump = (rx_frame.data.u8[4] & 0x80) >> 7;
      alert_cell_poor_consistency = (rx_frame.data.u8[4] & 0x40) >> 6;
      alert_overcharge = (rx_frame.data.u8[4] & 0x20) >> 5;
      alert_contactor_opening = (rx_frame.data.u8[4] & 0x10) >> 4;
      if (rx_frame.data.u8[5] < 200) {
        number_of_temperature_sensors = rx_frame.data.u8[5];
      }
      if (rx_frame.data.u8[6] < 200) {
        number_of_cells = rx_frame.data.u8[6];
      }
      break;
    case 0x595:  //1000ms - Time to charge to 20/40/680/100 , not needed for integration
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5B5:  //100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      heater_relay_status = rx_frame.data.u8[0] & 0x03;
      preheating_status = (rx_frame.data.u8[0] & 0x0C) >> 3;
      cooling_enabled = (rx_frame.data.u8[0] & 0x10) >> 4;
      coolant_temperature_warning = (rx_frame.data.u8[0] & 0x60) >> 5;
      coolant_alarm = (rx_frame.data.u8[0] & 0x80) >> 7;
      coolant_temperature = rx_frame.data.u8[1] - 40;
      thermal_control = (rx_frame.data.u8[2] & 0x70) >> 4;
      thermal_runaway = (rx_frame.data.u8[6] & 0xC0) >> 6;
      thermal_runaway_module_ID = (rx_frame.data.u8[6] & 0x1C) >> 2;
      break;
    case 0x615:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x625:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x665:  //1000ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      main_contactor_cycle_count = ((rx_frame.data.u8[0] << 2) | (rx_frame.data.u8[1] >> 6)) * 200;
      QC_contactor_cycle_count = (((rx_frame.data.u8[1] & 0x3F) << 4) | (rx_frame.data.u8[2] >> 4)) * 200;
      break;
    case 0x575:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x694:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x795:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7B5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void CmpSmartCarBattery::transmit_can(unsigned long currentMillis) {

  // Send periodic CAN Messages simulating the car still being attached
  // Send 10ms messages
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    counter_10ms = (counter_10ms + 1) % 16;  // counter_100ms repeats after 16 messages. 0-1..15-0

    CMP_262.data.u8[0] =
        0x40;  //0b0000 powertrain inactive, 0b0100 powertrain activation, 0b1000 powertrain active (Goes from E0, 00, 40 to 80 in logs)

    CMP_208.data.u8[1] = 0x20;  //Goes from 00->20->3E in logs while starting to drive
    CMP_208.data.u8[7] = counter_10ms;
    CMP_208.data.u8[7] = (calculate_checksum(CMP_208, 0x05) << 4) | counter_10ms;

    transmit_can_frame(&CMP_208);
    //transmit_can_frame(&CMP_217);
    //transmit_can_frame(&CMP_241);
    transmit_can_frame(&CMP_262);
  }

  // Send 50ms messages
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    CMP_432.data.u8[0] = 0x00;  //TODO, this message is important. What should content be?
    CMP_421.data.u8[3] = 0x00;  //TODO, this contains wakeup request from VCU. What should content be?

    transmit_can_frame(&CMP_432);  //Main wakeup
    transmit_can_frame(&CMP_421);  //More wakeup
  }

  // Send 60ms messages
  if (currentMillis - previousMillis60 >= INTERVAL_60_MS) {
    previousMillis60 = currentMillis;
    //transmit_can_frame(&CMP_351);
  }

  // Send 100ms messages
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    counter_100ms = (counter_100ms + 1) % 16;  // counter_100ms repeats after 16 messages. 0-1..15-0

    //CMP_211.data.u8[4] = ??? //Contactor closing section

    //transmit_can_frame(&CMP_211);
    //transmit_can_frame(&CMP_231);
    //transmit_can_frame(&CMP_422);
    transmit_can_frame(&CMP_4A2);  //Should we send plugged in, or unplugged?
  }

  // Send 1s messages
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    vehicle_time_counter++;  //TODO: Real CAN log needed to see how this should be formatted

    CMP_552.data.u8[0] = ((vehicle_time_counter / 10) & 0xFF000000) >> 24;
    CMP_552.data.u8[1] = ((vehicle_time_counter / 10) & 0x00FF0000) >> 16;
    CMP_552.data.u8[2] = ((vehicle_time_counter / 10) & 0x0000FF00) >> 8;
    CMP_552.data.u8[3] = ((vehicle_time_counter / 10) & 0x000000FF);

    //TODO, rest of bits in this message needs to be initialized to sane values

    transmit_can_frame(&CMP_552);
  }
}

void CmpSmartCarBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.total_capacity_Wh = 41400;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_100S_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_100S_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
