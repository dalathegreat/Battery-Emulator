#include "FOXESS-EP-CAN.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void FoxessEpCanInverter::transmit_cell_voltage_frame(uint32_t frame_id, uint16_t first_cell_index) {
  FOXESS_CELLVOLTAGES.ID = frame_id;

  const uint16_t foxess_ep_reported_cell_count = datalayer.battery.info.number_of_cells;

  const uint16_t foxess_ep_cell_count =
      foxess_ep_reported_cell_count > MAX_AMOUNT_CELLS ? MAX_AMOUNT_CELLS : foxess_ep_reported_cell_count;

  // Use the final valid cell as a sensible filler for positions
  // beyond the physical battery cell count.
  uint16_t foxess_ep_fallback_cell_voltage_mV = 3300U;

  if (foxess_ep_cell_count > 0U) {
    const uint16_t final_cell_voltage_mV = datalayer.battery.status.cell_voltages_mV[foxess_ep_cell_count - 1U];

    if (final_cell_voltage_mV > 0U) {
      foxess_ep_fallback_cell_voltage_mV = final_cell_voltage_mV;
    }
  }

  for (uint8_t cell_slot = 0U; cell_slot < 4U; ++cell_slot) {
    const uint16_t virtual_cell_index = first_cell_index + cell_slot;

    uint16_t source_cell_index = 0U;

    if (foxess_ep_cell_count > 0U) {
      source_cell_index =
          static_cast<uint16_t>((static_cast<uint32_t>(virtual_cell_index) * foxess_ep_cell_count) / 144U);

      if (source_cell_index >= foxess_ep_cell_count) {
        source_cell_index = foxess_ep_cell_count - 1U;
      }
    }

    uint16_t cell_voltage_mV = foxess_ep_fallback_cell_voltage_mV;

    if (source_cell_index < foxess_ep_cell_count) {
      const uint16_t live_cell_voltage_mV = datalayer.battery.status.cell_voltages_mV[source_cell_index];

      if (live_cell_voltage_mV > 0U) {
        cell_voltage_mV = live_cell_voltage_mV;
      }
    }

    const uint8_t payload_index = static_cast<uint8_t>(cell_slot * 2U);

    FOXESS_CELLVOLTAGES.data.u8[payload_index] = static_cast<uint8_t>(cell_voltage_mV);

    FOXESS_CELLVOLTAGES.data.u8[payload_index + 1U] = static_cast<uint8_t>(cell_voltage_mV >> 8);
  }

  transmit_can_frame(&FOXESS_CELLVOLTAGES);
}

void FoxessEpCanInverter::transmit_temperature_frame(uint32_t frame_id, uint8_t first_virtual_sensor_index) {
  FOXESS_CELLTEMPERATURES.ID = frame_id;

  int16_t minimum_temperature_dC = datalayer.battery.status.temperature_min_dC;

  int16_t maximum_temperature_dC = datalayer.battery.status.temperature_max_dC;

  // Preserve correct minimum/maximum ordering if an integration
  // temporarily supplies the values in reverse.
  if (minimum_temperature_dC > maximum_temperature_dC) {
    const int16_t temporary_temperature_dC = minimum_temperature_dC;

    minimum_temperature_dC = maximum_temperature_dC;

    maximum_temperature_dC = temporary_temperature_dC;
  }

  const int16_t generic_temperature_dC[2] = {
      minimum_temperature_dC,
      maximum_temperature_dC,
  };

  for (uint8_t byte_index = 0U; byte_index < 8U; ++byte_index) {
    const uint8_t virtual_sensor_index = first_virtual_sensor_index + byte_index;

    const int16_t temperature_dC = generic_temperature_dC[virtual_sensor_index % 2U];

    int16_t encoded_temperature = (temperature_dC / 10) + 50;

    if (encoded_temperature < 0) {
      encoded_temperature = 0;
    } else if (encoded_temperature > 255) {
      encoded_temperature = 255;
    }

    FOXESS_CELLTEMPERATURES.data.u8[byte_index] = static_cast<uint8_t>(encoded_temperature);
  }

  transmit_can_frame(&FOXESS_CELLTEMPERATURES);
}

/*
 * Standalone FoxESS EP-Series CAN protocol.
 * Built from genuine EP12 captures, Fox manager-firmware analysis,
 * and prior FoxESS CAN research:
 * https://github.com/FozzieUK/FoxESS-Canbus-Protocol
 */

void FoxessEpCanInverter::
    update_values() {  //This function maps all the CAN values fetched from battery. It also checks some safeties.

  //Calculate the required values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  // Common generic FoxESS EP readiness state.
  const bool foxess_ep_system_fault =
      datalayer.system.status.system_status == FAULT || datalayer.battery.status.real_bms_status == BMS_FAULT;

  const bool foxess_ep_battery_comm_live = datalayer.battery.status.CAN_battery_still_alive > 0U;

  const bool foxess_ep_battery_ready = foxess_ep_battery_comm_live && datalayer.system.status.system_status == ACTIVE &&
                                       datalayer.system.status.battery_allows_contactor_closing &&
                                       !foxess_ep_system_fault;

  const bool foxess_ep_power_path_active = foxess_ep_battery_ready && datalayer.system.status.contactors_engaged == 1U;

  constexpr int32_t FOXESS_EP_ACTIVITY_DEADBAND_dA = 10;

  const int32_t foxess_ep_activity_current_dA = static_cast<int32_t>(datalayer.battery.status.reported_current_dA);

  const bool foxess_ep_charging_active =
      foxess_ep_power_path_active && foxess_ep_activity_current_dA >= FOXESS_EP_ACTIVITY_DEADBAND_dA;

  const bool foxess_ep_discharging_active =
      foxess_ep_power_path_active && foxess_ep_activity_current_dA <= -FOXESS_EP_ACTIVITY_DEADBAND_dA;

  // Integrate battery power into separate charged and discharged
  // energy counters. voltage_dV is volts x10 and current_dA is
  // amps x10, so voltage_dV x current_dA is watts x100.
  unsigned long foxess_now_millis = millis();

  if (!foxess_energy_counter_initialised || !foxess_ep_power_path_active) {
    foxess_previous_energy_millis = foxess_now_millis;
    foxess_energy_counter_initialised = true;
  } else {
    unsigned long elapsed_ms = foxess_now_millis - foxess_previous_energy_millis;
    foxess_previous_energy_millis = foxess_now_millis;

    int32_t signed_current_dA = (int32_t)datalayer.battery.status.reported_current_dA;

    int32_t absolute_current_dA = signed_current_dA;

    if (absolute_current_dA < 0) {
      absolute_current_dA = -absolute_current_dA;
    }

    uint64_t energy_increment =
        (uint64_t)datalayer.battery.status.voltage_dV * (uint64_t)absolute_current_dA * (uint64_t)elapsed_ms;

    // dV x dA x milliseconds:
    // divide by 100 to obtain watts,
    // then by 3,600,000 to obtain Wh.
    static const uint64_t FOXESS_WH_DIVISOR = 360000000ULL;
    static const uint64_t FOXESS_DAH_DIVISOR_dAms = 3600000ULL;

    if (signed_current_dA > 0) {
      foxess_charged_energy_remainder += energy_increment;

      uint64_t completed_Wh = foxess_charged_energy_remainder / FOXESS_WH_DIVISOR;

      if (completed_Wh > 0ULL) {
        foxess_installation_charged_energy_Wh += completed_Wh;

        foxess_throughput_energy_Wh += completed_Wh;

        foxess_charged_energy_remainder %= FOXESS_WH_DIVISOR;
      }

      foxess_charged_capacity_remainder_dAms +=
          static_cast<uint64_t>(signed_current_dA) * static_cast<uint64_t>(elapsed_ms);

      const uint64_t completed_charged_dAh = foxess_charged_capacity_remainder_dAms / FOXESS_DAH_DIVISOR_dAms;

      if (completed_charged_dAh > 0ULL) {
        foxess_charged_capacity_dAh += completed_charged_dAh;

        foxess_charged_capacity_remainder_dAms %= FOXESS_DAH_DIVISOR_dAms;
      }
    } else if (signed_current_dA < 0) {
      foxess_discharged_energy_remainder += energy_increment;

      uint64_t completed_Wh = foxess_discharged_energy_remainder / FOXESS_WH_DIVISOR;

      if (completed_Wh > 0ULL) {
        foxess_installation_discharged_energy_Wh += completed_Wh;

        foxess_throughput_energy_Wh += completed_Wh;

        foxess_discharged_energy_remainder %= FOXESS_WH_DIVISOR;
      }

      foxess_discharged_capacity_remainder_dAms +=
          static_cast<uint64_t>(absolute_current_dA) * static_cast<uint64_t>(elapsed_ms);

      const uint64_t completed_discharged_dAh = foxess_discharged_capacity_remainder_dAms / FOXESS_DAH_DIVISOR_dAms;

      if (completed_discharged_dAh > 0ULL) {
        foxess_discharged_capacity_dAh += completed_discharged_dAh;

        foxess_discharged_capacity_remainder_dAms %= FOXESS_DAH_DIVISOR_dAms;
      }
    }
  }
  //Put the values into the CAN messages
  //BMS_Limits
  uint16_t foxess_ep_max_charge_current_dA = 0U;
  uint16_t foxess_ep_max_discharge_current_dA = 0U;

  if (foxess_ep_battery_ready) {
    foxess_ep_max_charge_current_dA = datalayer.battery.status.max_charge_current_dA;

    foxess_ep_max_discharge_current_dA = datalayer.battery.status.max_discharge_current_dA;
  }
  FOXESS_1872.data.u8[0] = (uint8_t)datalayer.battery.info.max_design_voltage_dV;
  FOXESS_1872.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[2] = (uint8_t)datalayer.battery.info.min_design_voltage_dV;
  FOXESS_1872.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[4] = static_cast<uint8_t>(foxess_ep_max_charge_current_dA);
  FOXESS_1872.data.u8[5] = static_cast<uint8_t>(foxess_ep_max_charge_current_dA >> 8);
  FOXESS_1872.data.u8[6] = static_cast<uint8_t>(foxess_ep_max_discharge_current_dA);
  FOXESS_1872.data.u8[7] = static_cast<uint8_t>(foxess_ep_max_discharge_current_dA >> 8);

  // BMS_PackData - 0x1873

  // Bytes 0-1: live battery voltage in 0.1 V.
  FOXESS_1873.data.u8[0] = (uint8_t)datalayer.battery.status.voltage_dV;
  FOXESS_1873.data.u8[1] = (uint8_t)(datalayer.battery.status.voltage_dV >> 8);

  // Bytes 2-3: signed current in 0.1 A.
  // Battery-Emulator uses positive current while charging.
  // Genuine FoxESS EP uses negative current while charging.
  int32_t foxess_ep_current_dA = 0;

  if (foxess_ep_power_path_active) {
    foxess_ep_current_dA = -static_cast<int32_t>(datalayer.battery.status.reported_current_dA);
  }

  if (foxess_ep_current_dA > 32767) {
    foxess_ep_current_dA = 32767;
  } else if (foxess_ep_current_dA < -32768) {
    foxess_ep_current_dA = -32768;
  }

  int16_t foxess_ep_current_dA_signed = (int16_t)foxess_ep_current_dA;

  FOXESS_1873.data.u8[2] = (uint8_t)foxess_ep_current_dA_signed;
  FOXESS_1873.data.u8[3] = (uint8_t)(foxess_ep_current_dA_signed >> 8);

  // Bytes 4-5: SOC in whole percent.
  uint16_t foxess_ep_pack_soc_percent = datalayer.battery.status.reported_soc / 100U;

  if (foxess_ep_pack_soc_percent > 100U) {
    foxess_ep_pack_soc_percent = 100U;
  }

  FOXESS_1873.data.u8[4] = static_cast<uint8_t>(foxess_ep_pack_soc_percent);
  FOXESS_1873.data.u8[5] = 0x00;

  // Bytes 6-7: nominal battery energy in 10 Wh units.
  // Fox Cloud applies SOH to the advertised nominal energy.
  // Battery-Emulator's reported_total_capacity_Wh is the present
  // effective capacity, so convert it back to the equivalent
  // pre-SOH nominal value. Fox can then apply SOH once and recover
  // the accurate effective capacity.
  const uint32_t foxess_ep_effective_energy_Wh = datalayer.battery.info.reported_total_capacity_Wh;

  uint32_t foxess_ep_nominal_energy_Wh = datalayer.battery.info.total_capacity_Wh;

  uint32_t foxess_ep_soh_pptt_for_energy = datalayer.battery.status.soh_pptt;

  if (foxess_ep_soh_pptt_for_energy > 10000U) {
    foxess_ep_soh_pptt_for_energy = 10000U;
  }

  if (foxess_ep_effective_energy_Wh > 0U) {
    if (foxess_ep_soh_pptt_for_energy > 0U) {
      foxess_ep_nominal_energy_Wh = static_cast<uint32_t>(
          (static_cast<uint64_t>(foxess_ep_effective_energy_Wh) * 10000ULL + (foxess_ep_soh_pptt_for_energy / 2U)) /
          foxess_ep_soh_pptt_for_energy);
    } else {
      foxess_ep_nominal_energy_Wh = foxess_ep_effective_energy_Wh;
    }
  }

  uint32_t foxess_ep_nominal_energy_10Wh = foxess_ep_nominal_energy_Wh / 10U;

  if (foxess_ep_nominal_energy_10Wh > 65535U) {
    foxess_ep_nominal_energy_10Wh = 65535U;
  }
  FOXESS_1873.data.u8[6] = static_cast<uint8_t>(foxess_ep_nominal_energy_10Wh);

  FOXESS_1873.data.u8[7] = static_cast<uint8_t>(foxess_ep_nominal_energy_10Wh >> 8);

  // BMS_CellData - 0x1874

  // Bytes 0-1: maximum battery temperature in 0.1 C.
  FOXESS_1874.data.u8[0] = (uint8_t)datalayer.battery.status.temperature_max_dC;
  FOXESS_1874.data.u8[1] = (uint8_t)(datalayer.battery.status.temperature_max_dC >> 8);

  // Bytes 2-3: minimum battery temperature in 0.1 C.
  FOXESS_1874.data.u8[2] = (uint8_t)datalayer.battery.status.temperature_min_dC;
  FOXESS_1874.data.u8[3] = (uint8_t)(datalayer.battery.status.temperature_min_dC >> 8);

  // Bytes 4-5 and 6-7: 1-based positions of the
  // highest-voltage and lowest-voltage cells.
  uint8_t foxess_ep_max_cell_position = 0;
  uint8_t foxess_ep_min_cell_position = 0;

  uint16_t foxess_ep_max_cell_voltage_mV = 0;
  uint16_t foxess_ep_min_cell_voltage_mV = UINT16_MAX;

  const uint16_t foxess_ep_reported_cell_count_for_extrema = datalayer.battery.info.number_of_cells;

  const uint16_t foxess_ep_cell_count_for_extrema = foxess_ep_reported_cell_count_for_extrema > MAX_AMOUNT_CELLS
                                                        ? MAX_AMOUNT_CELLS
                                                        : foxess_ep_reported_cell_count_for_extrema;

  for (uint16_t cell_index = 0U; cell_index < foxess_ep_cell_count_for_extrema; ++cell_index) {

    const uint16_t cell_voltage_mV = datalayer.battery.status.cell_voltages_mV[cell_index];

    if (cell_voltage_mV == 0) {
      continue;
    }

    if (foxess_ep_max_cell_position == 0 || cell_voltage_mV > foxess_ep_max_cell_voltage_mV) {
      foxess_ep_max_cell_voltage_mV = cell_voltage_mV;
      foxess_ep_max_cell_position = static_cast<uint8_t>(cell_index + 1);
    }

    if (foxess_ep_min_cell_position == 0 || cell_voltage_mV < foxess_ep_min_cell_voltage_mV) {
      foxess_ep_min_cell_voltage_mV = cell_voltage_mV;
      foxess_ep_min_cell_position = static_cast<uint8_t>(cell_index + 1);
    }
  }

  // Convert the real battery cell positions to the corresponding
  // positions in the 144-cell FoxESS virtual layout.
  uint16_t foxess_ep_virtual_max_cell_position = 0U;
  uint16_t foxess_ep_virtual_min_cell_position = 0U;

  if (foxess_ep_cell_count_for_extrema > 0U) {
    if (foxess_ep_max_cell_position > 0U) {
      foxess_ep_virtual_max_cell_position = static_cast<uint16_t>(
          ((static_cast<uint32_t>(foxess_ep_max_cell_position - 1U) * 144U + foxess_ep_cell_count_for_extrema - 1U) /
           foxess_ep_cell_count_for_extrema) +
          1U);
    }

    if (foxess_ep_min_cell_position > 0U) {
      foxess_ep_virtual_min_cell_position = static_cast<uint16_t>(
          ((static_cast<uint32_t>(foxess_ep_min_cell_position - 1U) * 144U + foxess_ep_cell_count_for_extrema - 1U) /
           foxess_ep_cell_count_for_extrema) +
          1U);
    }
  }

  if (foxess_ep_virtual_max_cell_position > 144U) {
    foxess_ep_virtual_max_cell_position = 144U;
  }

  if (foxess_ep_virtual_min_cell_position > 144U) {
    foxess_ep_virtual_min_cell_position = 144U;
  }

  FOXESS_1874.data.u8[4] = static_cast<uint8_t>(foxess_ep_virtual_max_cell_position);
  FOXESS_1874.data.u8[5] = 0x00;

  FOXESS_1874.data.u8[6] = static_cast<uint8_t>(foxess_ep_virtual_min_cell_position);
  FOXESS_1874.data.u8[7] = 0x00;

  //BMS_Status
  FOXESS_1875.data.u8[0] = (uint8_t)temperature_average;
  FOXESS_1875.data.u8[1] = (temperature_average >> 8);
  FOXESS_1875.data.u8[2] = (uint8_t)STATUS_OPERATIONAL_PACKS;
  FOXESS_1875.data.u8[3] = (uint8_t)configured_number_of_modules;
  // Contactor Status: 0 = off, 1 = on.
  FOXESS_1875.data.u8[4] = datalayer.system.status.contactors_engaged ? 0x01 : 0x00;
  FOXESS_1875.data.u8[5] = (uint8_t)0;  //0 Confirmed Unused in Battery Details page

  const uint64_t foxess_cycle_charged_energy_Wh = foxess_installation_charged_energy_Wh;

  const uint64_t foxess_cycle_discharged_energy_Wh = foxess_installation_discharged_energy_Wh;

  const uint64_t foxess_cycle_throughput_Wh = foxess_cycle_charged_energy_Wh + foxess_cycle_discharged_energy_Wh;

  const uint64_t foxess_cycle_denominator_Wh = static_cast<uint64_t>(datalayer.battery.info.total_capacity_Wh) * 2ULL;

  uint64_t foxess_equivalent_cycles = 0ULL;

  if (foxess_ep_battery_ready && foxess_cycle_denominator_Wh > 0ULL) {
    foxess_equivalent_cycles = foxess_cycle_throughput_Wh / foxess_cycle_denominator_Wh;
  }

  if (foxess_equivalent_cycles > UINT16_MAX) {
    foxess_equivalent_cycles = UINT16_MAX;
  }

  FOXESS_1875.data.u8[6] = static_cast<uint8_t>(foxess_equivalent_cycles);

  FOXESS_1875.data.u8[7] = static_cast<uint8_t>(foxess_equivalent_cycles >> 8);

  //BMS_PackTemps
  // 0x1876 b0 bit 0 appears to be 1 when at maxsoc and BMS says charge is not allowed -
  // when at 0 indicates charge is possible - additional note there is something more to it than this,
  // it's not as straight forward - needs more testing to find what sets/unsets bit0 of byte0
  if (!foxess_ep_battery_ready || datalayer.battery.status.max_charge_current_dA == 0U ||
      datalayer.battery.status.reported_soc >= 10000U) {
    FOXESS_1876.data.u8[0] = 0x01;
  } else {  //continue using battery
    FOXESS_1876.data.u8[0] = 0x00;
  }

  FOXESS_1876.data.u8[1] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[2] = (uint8_t)datalayer.battery.status.cell_max_voltage_mV;
  FOXESS_1876.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  FOXESS_1876.data.u8[4] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[5] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[6] = (uint8_t)datalayer.battery.status.cell_min_voltage_mV;
  FOXESS_1876.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //BMS_ErrorsBrand
  //0x1877 b0 appears to be an error code, 0x02 when pack is in error.
  if (foxess_ep_system_fault) {
    FOXESS_1877.data.u8[0] = (uint8_t)0x02;
  } else {
    FOXESS_1877.data.u8[0] = (uint8_t)0;
  }
  FOXESS_1877.data.u8[1] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[2] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[3] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[5] = (uint8_t)0;  //Unused
  if (current_pack_info == MAIN) {
    FOXESS_1877.data.u8[4] = (uint8_t)configured_battery_type;
    FOXESS_1877.data.u8[6] = (uint8_t)FIRMWARE_VERSION_MAIN_BMS;
    FOXESS_1877.data.u8[7] = (uint8_t)0x01;
  } else {  // 1-8
    FOXESS_1877.data.u8[4] = (uint8_t)configured_battery_subtype;
    FOXESS_1877.data.u8[6] = (uint8_t)FIRMWARE_VERSION_SUBSTACKS;
    FOXESS_1877.data.u8[7] = (uint8_t)(current_pack_info << 4);
  }

  // BMS_ExtendedData - 0x187B
  //
  // Final firmware/capture-backed mapping:
  // byte 0     = SOH in whole percent
  // byte 1     = operating state
  //              0x04 initialising
  //              0x02 charging
  //              0x01 discharging
  //              0x00 idle
  // bytes 2-3 = reserved, zero
  // bytes 4-5 = design capacity in 0.1 Ah
  // bytes 6-7 = BMS-reported full/effective capacity in 0.1 Ah

  uint16_t foxess_soh_percent = datalayer.battery.status.soh_pptt / 100U;

  if (foxess_soh_percent > 100U) {
    foxess_soh_percent = 100U;
  }

  FOXESS_187B.data.u8[0] = static_cast<uint8_t>(foxess_soh_percent);

  // Use the generic design-voltage range to estimate a stable
  // nominal pack voltage. Battery-Emulator does not currently
  // provide a dedicated generic nominal-voltage field.
  const uint32_t foxess_min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;

  const uint32_t foxess_max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;

  uint32_t foxess_nominal_voltage_dV = 0U;

  if (foxess_min_design_voltage_dV > 0U && foxess_max_design_voltage_dV >= foxess_min_design_voltage_dV) {
    foxess_nominal_voltage_dV = (foxess_min_design_voltage_dV + foxess_max_design_voltage_dV + 1U) / 2U;
  }

  // Rated/base energy prefers the generic total-capacity field.
  const uint32_t foxess_rated_energy_Wh = datalayer.battery.info.total_capacity_Wh > 0U
                                              ? datalayer.battery.info.total_capacity_Wh
                                              : datalayer.battery.info.reported_total_capacity_Wh;

  // Full/inverter-visible energy prefers the reported capacity.
  const uint32_t foxess_full_energy_Wh = datalayer.battery.info.reported_total_capacity_Wh > 0U
                                             ? datalayer.battery.info.reported_total_capacity_Wh
                                             : foxess_rated_energy_Wh;

  const bool foxess_full_capacity_valid =
      foxess_nominal_voltage_dV > 0U && foxess_rated_energy_Wh > 0U && foxess_full_energy_Wh > 0U;

  uint32_t foxess_rated_capacity_dAh = 0U;
  uint32_t foxess_full_capacity_dAh = 0U;

  if (foxess_full_capacity_valid) {
    foxess_rated_capacity_dAh = static_cast<uint32_t>(
        (static_cast<uint64_t>(foxess_rated_energy_Wh) * 100ULL + (foxess_nominal_voltage_dV / 2U)) /
        foxess_nominal_voltage_dV);

    foxess_full_capacity_dAh = static_cast<uint32_t>(
        (static_cast<uint64_t>(foxess_full_energy_Wh) * 100ULL + (foxess_nominal_voltage_dV / 2U)) /
        foxess_nominal_voltage_dV);
  }

  if (foxess_rated_capacity_dAh > 65535U) {
    foxess_rated_capacity_dAh = 65535U;
  }

  if (foxess_full_capacity_dAh > 65535U) {
    foxess_full_capacity_dAh = 65535U;
  }

  // Byte 1 operating state.
  // Use a 1.0 A threshold to prevent current noise reporting
  // false charging/discharging states.
  if (!foxess_ep_battery_ready) {
    FOXESS_187B.data.u8[1] = 0x04;
  } else if (foxess_ep_charging_active) {
    FOXESS_187B.data.u8[1] = 0x02;
  } else if (foxess_ep_discharging_active) {
    FOXESS_187B.data.u8[1] = 0x01;
  } else {
    FOXESS_187B.data.u8[1] = 0x00;
  }

  FOXESS_187B.data.u8[2] = 0x00;
  FOXESS_187B.data.u8[3] = 0x00;

  // Design capacity, little-endian.
  FOXESS_187B.data.u8[4] = static_cast<uint8_t>(foxess_rated_capacity_dAh);
  FOXESS_187B.data.u8[5] = static_cast<uint8_t>(foxess_rated_capacity_dAh >> 8);

  // Reported full/effective capacity, little-endian.
  FOXESS_187B.data.u8[6] = static_cast<uint8_t>(foxess_full_capacity_dAh);
  FOXESS_187B.data.u8[7] = static_cast<uint8_t>(foxess_full_capacity_dAh >> 8);

  // EP capacity summary - 0x1900
  // Bytes 0-1: rated battery capacity in 0.1 Ah.
  FOXESS_1900.data.u8[0] = (uint8_t)foxess_rated_capacity_dAh;
  FOXESS_1900.data.u8[1] = (uint8_t)(foxess_rated_capacity_dAh >> 8);

  // Bytes 2-3: nominal battery energy in 10 Wh units.
  FOXESS_1900.data.u8[2] = (uint8_t)foxess_ep_nominal_energy_10Wh;
  FOXESS_1900.data.u8[3] = (uint8_t)(foxess_ep_nominal_energy_10Wh >> 8);

  // Capture-confirmed fixed EP bytes.
  FOXESS_1900.data.u8[4] = 0x00;
  FOXESS_1900.data.u8[5] = 0x00;
  FOXESS_1900.data.u8[6] = 0x3C;
  FOXESS_1900.data.u8[7] = 0x46;

  // EP battery capability model - 0x1902
  //
  // Bytes 0-1 : battery model coefficient (permille).
  // Healthy EP12 batteries report approximately 942 (94.2%).
  const uint16_t foxess_ep_model_coefficient = 942;

  FOXESS_1902.data.u8[0] = static_cast<uint8_t>(foxess_ep_model_coefficient);

  FOXESS_1902.data.u8[1] = static_cast<uint8_t>(foxess_ep_model_coefficient >> 8);
  // Bytes 2-3: maximum permitted discharge power in watts.
  // The Fox field is uint16, so clamp larger battery values.
  uint32_t foxess_ep_max_discharge_power_1902_W = 0U;

  if (foxess_ep_battery_ready && datalayer.battery.status.max_discharge_current_dA > 0U) {
    foxess_ep_max_discharge_power_1902_W = datalayer.battery.status.max_discharge_power_W;
  }

  if (foxess_ep_max_discharge_power_1902_W > 65535U) {
    foxess_ep_max_discharge_power_1902_W = 65535U;
  }

  FOXESS_1902.data.u8[2] = static_cast<uint8_t>(foxess_ep_max_discharge_power_1902_W);

  FOXESS_1902.data.u8[3] = static_cast<uint8_t>(foxess_ep_max_discharge_power_1902_W >> 8);

  // Bytes 4-5 : equivalent DC resistance (0.1 mÎ©)
  // Battery-Emulator currently has no live pack-resistance source,
  // so use the genuine EP12 settled value as the fallback.
  const uint16_t foxess_ep_equivalent_resistance_0p1mOhm = 892;

  FOXESS_1902.data.u8[4] = static_cast<uint8_t>(foxess_ep_equivalent_resistance_0p1mOhm);

  FOXESS_1902.data.u8[5] = static_cast<uint8_t>(foxess_ep_equivalent_resistance_0p1mOhm >> 8);

  // Bytes 6-7 : filtered battery/model temperature (whole Â°C)

  int32_t foxess_ep_filtered_temperature_1902_C = (static_cast<int32_t>(datalayer.battery.status.temperature_max_dC) +
                                                   static_cast<int32_t>(datalayer.battery.status.temperature_min_dC)) /
                                                  20;

  if (foxess_ep_filtered_temperature_1902_C < 0) {
    foxess_ep_filtered_temperature_1902_C = 0;
  } else if (foxess_ep_filtered_temperature_1902_C > 65535) {
    foxess_ep_filtered_temperature_1902_C = 65535;
  }

  const uint16_t foxess_ep_filtered_temperature_1902_raw = static_cast<uint16_t>(foxess_ep_filtered_temperature_1902_C);

  FOXESS_1902.data.u8[6] = static_cast<uint8_t>(foxess_ep_filtered_temperature_1902_raw);

  FOXESS_1902.data.u8[7] = static_cast<uint8_t>(foxess_ep_filtered_temperature_1902_raw >> 8);

  // 0x1903 - system nominal energy

  // Bytes 0-5: reserved/zero in all captured EP12 operation.
  FOXESS_1903.data.u8[0] = 0x00;
  FOXESS_1903.data.u8[1] = 0x00;
  FOXESS_1903.data.u8[2] = 0x00;
  FOXESS_1903.data.u8[3] = 0x00;
  FOXESS_1903.data.u8[4] = 0x00;
  FOXESS_1903.data.u8[5] = 0x00;

  // Bytes 6-7: total installed nominal energy in 200 Wh units.
  // Genuine EP12 behaviour uses integer truncation.
  uint32_t foxess_ep_nominal_energy_200Wh = foxess_ep_nominal_energy_Wh / 200U;

  if (foxess_ep_nominal_energy_200Wh > 65535U) {
    foxess_ep_nominal_energy_200Wh = 65535U;
  }

  FOXESS_1903.data.u8[6] = static_cast<uint8_t>(foxess_ep_nominal_energy_200Wh);

  FOXESS_1903.data.u8[7] = static_cast<uint8_t>(foxess_ep_nominal_energy_200Wh >> 8);

  // 0x1904 - extreme measurement locations

  // Bytes 0-1: first cell-voltage extreme position.
  // The 0x1874 scan above already provides a 1-based position.
  FOXESS_1904.data.u8[0] = static_cast<uint8_t>(foxess_ep_virtual_max_cell_position);

  FOXESS_1904.data.u8[1] = 0x00;

  FOXESS_1904.data.u8[2] = static_cast<uint8_t>(foxess_ep_virtual_min_cell_position);

  FOXESS_1904.data.u8[3] = 0x00;

  // Bytes 4-5: capture-backed temperature-extreme location codes.
  // Exact packed sensor-address format remains unconfirmed.
  FOXESS_1904.data.u8[4] = 0x3A;

  FOXESS_1904.data.u8[5] = (configured_number_of_modules == 2) ? 0x12 : 0x0A;

  // Bytes 6-7: reserved.
  FOXESS_1904.data.u8[6] = 0x00;
  FOXESS_1904.data.u8[7] = 0x00;

  // 0x1905 - EP SOC/capacity model

  // Bytes 0-1: EP12 nominal cell-voltage reference,
  // 10 mV/count = 3.20 V.
  const uint16_t foxess_ep_nominal_cell_voltage_1905_10mV = 320U;

  FOXESS_1905.data.u8[0] = static_cast<uint8_t>(foxess_ep_nominal_cell_voltage_1905_10mV);

  FOXESS_1905.data.u8[1] = static_cast<uint8_t>(foxess_ep_nominal_cell_voltage_1905_10mV >> 8);

  // Byte 2: primary whole-percent SOC.
  // Genuine EP12 sends zero until capacity data is valid.
  uint16_t foxess_ep_primary_soc_1905 = datalayer.battery.status.reported_soc / 100U;

  if (foxess_ep_primary_soc_1905 > 100U) {
    foxess_ep_primary_soc_1905 = 100U;
  }

  const bool foxess_ep_capacity_model_ready = foxess_ep_battery_ready && foxess_full_capacity_valid;

  if (!foxess_ep_capacity_model_ready) {
    foxess_ep_primary_soc_1905 = 0U;
  }

  FOXESS_1905.data.u8[2] = static_cast<uint8_t>(foxess_ep_primary_soc_1905);

  // Byte 3: SOC/capacity-model state.
  FOXESS_1905.data.u8[3] = foxess_ep_capacity_model_ready ? 0x08 : 0x04;

  // Byte 7: reserved.
  FOXESS_1905.data.u8[7] = 0x00;

  // 0x1906 - per-unit battery model parameters

  const uint16_t foxess_ep_model_parameter_a_1906 = 81U;
  const uint16_t foxess_ep_model_parameter_b_1906 = 970U;

  // Bytes 0-1: model parameter A.
  FOXESS_1906.data.u8[0] = static_cast<uint8_t>(foxess_ep_model_parameter_a_1906);

  FOXESS_1906.data.u8[1] = static_cast<uint8_t>(foxess_ep_model_parameter_a_1906 >> 8);

  // Bytes 2-3: zero.
  FOXESS_1906.data.u8[2] = 0x00;
  FOXESS_1906.data.u8[3] = 0x00;

  // Bytes 4-5: model parameter B.
  FOXESS_1906.data.u8[4] = static_cast<uint8_t>(foxess_ep_model_parameter_b_1906);

  FOXESS_1906.data.u8[5] = static_cast<uint8_t>(foxess_ep_model_parameter_b_1906 >> 8);

  // Bytes 6-7: zero.
  FOXESS_1906.data.u8[6] = 0x00;
  FOXESS_1906.data.u8[7] = 0x00;

  // Keep 0x1905 byte 4 consistent with 0x1906 parameter B.
  FOXESS_1905.data.u8[4] = static_cast<uint8_t>(foxess_ep_model_parameter_b_1906 / 10U);

  // 0x1907 - high-resolution battery state estimates

  uint32_t foxess_ep_fine_soc_permille_1907 = 0U;

  if (foxess_ep_capacity_model_ready) {
    foxess_ep_fine_soc_permille_1907 = datalayer.battery.status.reported_soc / 10U;

    if (foxess_ep_fine_soc_permille_1907 > 1000U) {
      foxess_ep_fine_soc_permille_1907 = 1000U;
    }
  }

  // No separate generic OCV/SOE value is currently exposed.
  // Mirror fine SOC rather than transmitting zero during normal operation.
  const uint32_t foxess_ep_voltage_model_state_permille_1907 = foxess_ep_fine_soc_permille_1907;

  // Bytes 0-3: voltage-sensitive/model battery state.
  FOXESS_1907.data.u8[0] = static_cast<uint8_t>(foxess_ep_voltage_model_state_permille_1907);

  FOXESS_1907.data.u8[1] = static_cast<uint8_t>(foxess_ep_voltage_model_state_permille_1907 >> 8);

  FOXESS_1907.data.u8[2] = static_cast<uint8_t>(foxess_ep_voltage_model_state_permille_1907 >> 16);

  FOXESS_1907.data.u8[3] = static_cast<uint8_t>(foxess_ep_voltage_model_state_permille_1907 >> 24);

  // Bytes 4-7: fine internal SOC/SOE in 0.1%.
  FOXESS_1907.data.u8[4] = static_cast<uint8_t>(foxess_ep_fine_soc_permille_1907);

  FOXESS_1907.data.u8[5] = static_cast<uint8_t>(foxess_ep_fine_soc_permille_1907 >> 8);

  FOXESS_1907.data.u8[6] = static_cast<uint8_t>(foxess_ep_fine_soc_permille_1907 >> 16);

  FOXESS_1907.data.u8[7] = static_cast<uint8_t>(foxess_ep_fine_soc_permille_1907 >> 24);

  // Keep 0x1905 byte 5 consistent with the fine 0x1907 value.
  FOXESS_1905.data.u8[5] = static_cast<uint8_t>(foxess_ep_fine_soc_permille_1907 / 10U);

  // 0x1908 - capacity-adjusted battery-state model

  uint32_t foxess_ep_adjusted_soc_permille_1908 = foxess_ep_fine_soc_permille_1907;

  if (foxess_ep_capacity_model_ready && foxess_rated_capacity_dAh > 0U) {
    foxess_ep_adjusted_soc_permille_1908 =
        static_cast<uint32_t>((static_cast<uint64_t>(foxess_ep_fine_soc_permille_1907) * foxess_full_capacity_dAh) /
                              foxess_rated_capacity_dAh);
  }

  if (foxess_ep_adjusted_soc_permille_1908 > 1000U) {
    foxess_ep_adjusted_soc_permille_1908 = 1000U;
  }

  // Bytes 0-3: model state.
  // 14 = initialising, 16 = operational.
  const uint32_t foxess_ep_energy_model_state_1908 = foxess_ep_capacity_model_ready ? 16U : 14U;

  FOXESS_1908.data.u8[0] = static_cast<uint8_t>(foxess_ep_energy_model_state_1908);

  FOXESS_1908.data.u8[1] = static_cast<uint8_t>(foxess_ep_energy_model_state_1908 >> 8);

  FOXESS_1908.data.u8[2] = static_cast<uint8_t>(foxess_ep_energy_model_state_1908 >> 16);

  FOXESS_1908.data.u8[3] = static_cast<uint8_t>(foxess_ep_energy_model_state_1908 >> 24);

  // Bytes 4-7: fine capacity-adjusted SOC/SOE in 0.1%.
  FOXESS_1908.data.u8[4] = static_cast<uint8_t>(foxess_ep_adjusted_soc_permille_1908);

  FOXESS_1908.data.u8[5] = static_cast<uint8_t>(foxess_ep_adjusted_soc_permille_1908 >> 8);

  FOXESS_1908.data.u8[6] = static_cast<uint8_t>(foxess_ep_adjusted_soc_permille_1908 >> 16);

  FOXESS_1908.data.u8[7] = static_cast<uint8_t>(foxess_ep_adjusted_soc_permille_1908 >> 24);

  // Keep 0x1905 byte 6 consistent with the fine 0x1908 value.
  FOXESS_1905.data.u8[6] = static_cast<uint8_t>(foxess_ep_adjusted_soc_permille_1908 / 10U);

  // BMS_PackStats - 0x1878
  //
  // H1 Manager firmware decoding:
  // byte 0 is not used by the manager;
  // byte 1 is decoded separately;
  // bytes 2-3 form a separate field;
  // bytes 4-7 form one little-endian 32-bit field.
  // EP12 captures show byte 1 tracking the individual unit SOC.
  // Bytes 4-7: cumulative absolute energy throughput in Wh.

  uint16_t foxess_ep_unit_soc = datalayer.battery.status.reported_soc / 100U;

  if (foxess_ep_unit_soc > 100U) {
    foxess_ep_unit_soc = 100U;
  }

  FOXESS_1878.data.u8[0] = 0x00;
  FOXESS_1878.data.u8[1] = static_cast<uint8_t>(foxess_ep_unit_soc);

  FOXESS_1878.data.u8[2] = 0x00;
  FOXESS_1878.data.u8[3] = 0x00;

  const uint64_t foxess_ep_absolute_throughput_Wh =
      foxess_installation_charged_energy_Wh + foxess_installation_discharged_energy_Wh;

  const uint32_t foxess_ep_absolute_throughput_Wh_wire = foxess_ep_absolute_throughput_Wh > UINT32_MAX
                                                             ? UINT32_MAX
                                                             : static_cast<uint32_t>(foxess_ep_absolute_throughput_Wh);

  FOXESS_1878.data.u8[4] = static_cast<uint8_t>(foxess_ep_absolute_throughput_Wh_wire);

  FOXESS_1878.data.u8[5] = static_cast<uint8_t>(foxess_ep_absolute_throughput_Wh_wire >> 8);

  FOXESS_1878.data.u8[6] = static_cast<uint8_t>(foxess_ep_absolute_throughput_Wh_wire >> 16);

  FOXESS_1878.data.u8[7] = static_cast<uint8_t>(foxess_ep_absolute_throughput_Wh_wire >> 24);

  // EP 0x1879 - cumulative directional battery capacity.
  //
  // Bytes 0-3: charged capacity, 0.1 Ah/count.
  // Bytes 4-7: discharged capacity, 0.1 Ah/count.

  const uint32_t foxess_ep_charged_capacity_dAh_wire =
      foxess_charged_capacity_dAh > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(foxess_charged_capacity_dAh);

  const uint32_t foxess_ep_discharged_capacity_dAh_wire =
      foxess_discharged_capacity_dAh > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(foxess_discharged_capacity_dAh);

  FOXESS_1879.data.u8[0] = static_cast<uint8_t>(foxess_ep_charged_capacity_dAh_wire);

  FOXESS_1879.data.u8[1] = static_cast<uint8_t>(foxess_ep_charged_capacity_dAh_wire >> 8);

  FOXESS_1879.data.u8[2] = static_cast<uint8_t>(foxess_ep_charged_capacity_dAh_wire >> 16);

  FOXESS_1879.data.u8[3] = static_cast<uint8_t>(foxess_ep_charged_capacity_dAh_wire >> 24);

  FOXESS_1879.data.u8[4] = static_cast<uint8_t>(foxess_ep_discharged_capacity_dAh_wire);

  FOXESS_1879.data.u8[5] = static_cast<uint8_t>(foxess_ep_discharged_capacity_dAh_wire >> 8);

  FOXESS_1879.data.u8[6] = static_cast<uint8_t>(foxess_ep_discharged_capacity_dAh_wire >> 16);

  FOXESS_1879.data.u8[7] = static_cast<uint8_t>(foxess_ep_discharged_capacity_dAh_wire >> 24);
  // Charged energy - 0x187A
  // Bytes 0-3: accumulated charged energy in 0.1 kWh units.
  const uint64_t foxess_ep_charged_energy_100Wh_raw = foxess_installation_charged_energy_Wh / 100ULL;

  const uint32_t foxess_ep_charged_energy_100Wh = foxess_ep_charged_energy_100Wh_raw > UINT32_MAX
                                                      ? UINT32_MAX
                                                      : static_cast<uint32_t>(foxess_ep_charged_energy_100Wh_raw);

  // Bytes 4-7: accumulated discharged energy in 0.1 kWh units.
  const uint64_t foxess_ep_discharged_energy_100Wh_raw = foxess_installation_discharged_energy_Wh / 100ULL;

  const uint32_t foxess_ep_discharged_energy_100Wh = foxess_ep_discharged_energy_100Wh_raw > UINT32_MAX
                                                         ? UINT32_MAX
                                                         : static_cast<uint32_t>(foxess_ep_discharged_energy_100Wh_raw);

  FOXESS_187A.data.u8[0] = static_cast<uint8_t>(foxess_ep_charged_energy_100Wh);
  FOXESS_187A.data.u8[1] = static_cast<uint8_t>(foxess_ep_charged_energy_100Wh >> 8);
  FOXESS_187A.data.u8[2] = static_cast<uint8_t>(foxess_ep_charged_energy_100Wh >> 16);
  FOXESS_187A.data.u8[3] = static_cast<uint8_t>(foxess_ep_charged_energy_100Wh >> 24);

  FOXESS_187A.data.u8[4] = static_cast<uint8_t>(foxess_ep_discharged_energy_100Wh);
  FOXESS_187A.data.u8[5] = static_cast<uint8_t>(foxess_ep_discharged_energy_100Wh >> 8);
  FOXESS_187A.data.u8[6] = static_cast<uint8_t>(foxess_ep_discharged_energy_100Wh >> 16);
  FOXESS_187A.data.u8[7] = static_cast<uint8_t>(foxess_ep_discharged_energy_100Wh >> 24);

  current_pack_info = (current_pack_info + 1);
  if (current_pack_info > configured_number_of_modules) {
    current_pack_info = 0;
  }

  // Individual EP temperature encoding.
  // Whole degrees Celsius with a +50 offset.
  int16_t foxess_ep_max_temperature_encoded = (datalayer.battery.status.temperature_max_dC / 10) + 50;

  int16_t foxess_ep_min_temperature_encoded = (datalayer.battery.status.temperature_min_dC / 10) + 50;

  if (foxess_ep_max_temperature_encoded < 0) {
    foxess_ep_max_temperature_encoded = 0;
  } else if (foxess_ep_max_temperature_encoded > 255) {
    foxess_ep_max_temperature_encoded = 255;
  }

  if (foxess_ep_min_temperature_encoded < 0) {
    foxess_ep_min_temperature_encoded = 0;
  } else if (foxess_ep_min_temperature_encoded > 255) {
    foxess_ep_min_temperature_encoded = 255;
  }

  temperature_max_per_pack = static_cast<uint8_t>(foxess_ep_max_temperature_encoded);

  temperature_min_per_pack = static_cast<uint8_t>(foxess_ep_min_temperature_encoded);

  // Individual EP unit data - 0x0C05
  //
  // Bytes 0-1: signed current in 0.1 A.
  // Battery-Emulator uses positive current while charging;
  // genuine FoxESS EP uses negative current while charging.
  int32_t foxess_ep_unit_current_dA = 0;

  if (foxess_ep_power_path_active) {
    foxess_ep_unit_current_dA = -static_cast<int32_t>(datalayer.battery.status.reported_current_dA);
  }

  if (foxess_ep_unit_current_dA > INT16_MAX) {
    foxess_ep_unit_current_dA = INT16_MAX;
  } else if (foxess_ep_unit_current_dA < INT16_MIN) {
    foxess_ep_unit_current_dA = INT16_MIN;
  }

  const int16_t foxess_ep_unit_current_signed = static_cast<int16_t>(foxess_ep_unit_current_dA);

  FOXESS_0C05.data.u8[0] = static_cast<uint8_t>(foxess_ep_unit_current_signed);
  FOXESS_0C05.data.u8[1] = static_cast<uint8_t>(foxess_ep_unit_current_signed >> 8);

  // Bytes 2-3: maximum and minimum temperatures in whole
  // degrees Celsius, encoded with a +50 offset.
  FOXESS_0C05.data.u8[2] = temperature_max_per_pack;
  FOXESS_0C05.data.u8[3] = temperature_min_per_pack;

  // Byte 4: SOC in whole percent.

  FOXESS_0C05.data.u8[4] = static_cast<uint8_t>(foxess_ep_unit_soc);

  // Bytes 5-7: maximum and minimum cell voltages,
  // packed as two unsigned 12-bit millivolt values.
  uint16_t foxess_ep_unit_max_cell_mV = datalayer.battery.status.cell_max_voltage_mV;
  uint16_t foxess_ep_unit_min_cell_mV = datalayer.battery.status.cell_min_voltage_mV;

  if (foxess_ep_unit_max_cell_mV > 0x0FFFU) {
    foxess_ep_unit_max_cell_mV = 0x0FFFU;
  }

  if (foxess_ep_unit_min_cell_mV > 0x0FFFU) {
    foxess_ep_unit_min_cell_mV = 0x0FFFU;
  }

  FOXESS_0C05.data.u8[5] = static_cast<uint8_t>(foxess_ep_unit_max_cell_mV);

  FOXESS_0C05.data.u8[6] =
      static_cast<uint8_t>(((foxess_ep_unit_max_cell_mV >> 8) & 0x0FU) | ((foxess_ep_unit_min_cell_mV & 0x000FU) << 4));

  FOXESS_0C05.data.u8[7] = static_cast<uint8_t>(foxess_ep_unit_min_cell_mV >> 4);
}

void FoxessEpCanInverter::transmit_can(unsigned long currentMillis) {
  if (send_bms_info) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisBMSinfo >= delay_between_batches_ms) {
      previousMillisBMSinfo = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_bms_index) {
        case 0:
          transmit_can_frame(&FOXESS_1872);
          transmit_can_frame(&FOXESS_1873);
          transmit_can_frame(&FOXESS_1874);
          transmit_can_frame(&FOXESS_1875);
          break;
        case 1:
          transmit_can_frame(&FOXESS_1876);
          transmit_can_frame(&FOXESS_1877);
          transmit_can_frame(&FOXESS_1878);
          transmit_can_frame(&FOXESS_1879);
          break;

        case 2:
          transmit_can_frame(&FOXESS_187A);
          transmit_can_frame(&FOXESS_187B);
          transmit_can_frame(&FOXESS_187F);
          transmit_can_frame(&FOXESS_1900);
          break;

        case 3:
          transmit_can_frame(&FOXESS_1901);
          transmit_can_frame(&FOXESS_1902);
          transmit_can_frame(&FOXESS_1903);
          transmit_can_frame(&FOXESS_1904);
          break;

        case 4:
          transmit_can_frame(&FOXESS_1905);
          transmit_can_frame(&FOXESS_1906);
          transmit_can_frame(&FOXESS_1907);
          transmit_can_frame(&FOXESS_1908);
          transmit_can_frame(&FOXESS_1909);

          send_bms_info = false;
          break;
      }

      // Increment message index and wrap around if needed
      can_message_bms_index++;

      if (send_bms_info == false) {
        can_message_bms_index = 0;
      }
    }
  }
  if (send_individual_pack_status) {
    if (currentMillis - previousMillisIndividualPacks >= delay_between_batches_ms) {
      previousMillisIndividualPacks = currentMillis;

      // Standalone EP profile currently represents one complete EP unit.
      transmit_can_frame(&FOXESS_0C05);

      send_individual_pack_status = false;
    }
  }

  if (send_serial_numbers) {
    if (currentMillis - previousMillisSerialNumber >= delay_between_batches_ms) {
      previousMillisSerialNumber = currentMillis;

      // Standalone EP profile exposes one complete virtual EP unit.
      FOXESS_1881.data.u8[0] = 0U;
      FOXESS_1882.data.u8[0] = 0U;
      FOXESS_1883.data.u8[0] = 0U;

      transmit_can_frame(&FOXESS_1881);
      transmit_can_frame(&FOXESS_1882);
      transmit_can_frame(&FOXESS_1883);

      send_serial_numbers = false;
    }
  }

  if (send_cellvoltages) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisCellvoltage >= delay_between_batches_ms) {
      previousMillisCellvoltage = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_cellvolt_index) {
        case 0:
          transmit_cell_voltage_frame(0x0C1D, 0U);   // Cells 1-4
          transmit_cell_voltage_frame(0x0C21, 4U);   // Cells 5-8
          transmit_cell_voltage_frame(0x0C25, 8U);   // Cells 9-12
          transmit_cell_voltage_frame(0x0C29, 12U);  // Cells 13-16
          transmit_cell_voltage_frame(0x0C2D, 16U);  // Cells 17-20
          transmit_cell_voltage_frame(0x0C31, 20U);  // Cells 21-24
          break;
        case 1:
          transmit_cell_voltage_frame(0x0C35, 24U);  // Cells 25-28
          transmit_cell_voltage_frame(0x0C39, 28U);  // Cells 29-32
          transmit_cell_voltage_frame(0x0C3D, 32U);  // Cells 33-36
          transmit_cell_voltage_frame(0x0C41, 36U);  // Cells 37-40
          transmit_cell_voltage_frame(0x0C45, 40U);  // Cells 41-44
          break;
        case 2:
          transmit_cell_voltage_frame(0x0C49, 44U);  // Cells 45-48
          transmit_cell_voltage_frame(0x0C4D, 48U);  // Cells 49-52
          transmit_cell_voltage_frame(0x0C51, 52U);  // Cells 53-56
          transmit_cell_voltage_frame(0x0C55, 56U);  // Cells 57-60
          transmit_cell_voltage_frame(0x0C59, 60U);  // Cells 61-64
          break;
        case 3:
          transmit_cell_voltage_frame(0x0C5D, 64U);  // Cells 65-68
          transmit_cell_voltage_frame(0x0C61, 68U);  // Cells 69-72
          transmit_cell_voltage_frame(0x0C65, 72U);  // Cells 73-76
          transmit_cell_voltage_frame(0x0C69, 76U);  // Cells 77-80
          transmit_cell_voltage_frame(0x0C6D, 80U);  // Cells 81-84
          break;
        case 4:
          transmit_cell_voltage_frame(0x0C71, 84U);   // Cells 85-88
          transmit_cell_voltage_frame(0x0C75, 88U);   // Cells 89-92
          transmit_cell_voltage_frame(0x0C79, 92U);   // Cells 93-96
          transmit_cell_voltage_frame(0x0C7D, 96U);   // Cells 97-100
          transmit_cell_voltage_frame(0x0C81, 100U);  // Cells 101-104
          break;
        case 5:
          transmit_cell_voltage_frame(0x0C85, 104U);  // Cells 105-108
          transmit_cell_voltage_frame(0x0C89, 108U);  // Cells 109-112
          transmit_cell_voltage_frame(0x0C8D, 112U);  // Cells 113-116
          transmit_cell_voltage_frame(0x0C91, 116U);  // Cells 117-120
          transmit_cell_voltage_frame(0x0C95, 120U);  // Cells 121-124
          break;
        case 6:
          transmit_cell_voltage_frame(0x0C99, 124U);  // Cells 125-128
          transmit_cell_voltage_frame(0x0C9D, 128U);  // Cells 129-132
          transmit_cell_voltage_frame(0x0CA1, 132U);  // Cells 133-136
          transmit_cell_voltage_frame(0x0CA5, 136U);  // Cells 137-140
          transmit_cell_voltage_frame(0x0CA9, 140U);  // Cells 141-144

          send_cellvoltages = false;
          break;

        default:
          send_cellvoltages = false;
          break;
      }

      can_message_cellvolt_index++;

      if (send_cellvoltages == false) {
        can_message_cellvolt_index = 0;
      }
    }
  }

  // Detailed temperatures for the single virtual EP unit.
  if (send_celltemperatures) {
    if (currentMillis - previousMillisCelltemperature >= delay_between_batches_ms) {
      previousMillisCelltemperature = currentMillis;

      transmit_temperature_frame(0x0D21, 0U);
      transmit_temperature_frame(0x0D22, 8U);

      send_celltemperatures = false;
    }
  }
}

void FoxessEpCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {

  if (rx_frame.ID == 0x1871) {
    datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
    if (rx_frame.data.u8[0] == 0x03) {  //0x1871 [0x03, 0x06, 0x17, 0x05, 0x09, 0x09, 0x28, 0x22]
      //This message is sent by the inverter every '6' seconds (0.5s after the pack serial numbers)
      //and contains a timestamp in bytes 2-7 i.e. <YY>,<MM>,<DD>,<HH>,<mm>,<ss>
    } else if (rx_frame.data.u8[0] == 0x01) {
      if (rx_frame.data.u8[4] == 0x00) {
        // Inverter wants to know bms info (every 1s)
        send_bms_info = true;
      } else if (rx_frame.data.u8[4] == 0x01) {  // b4 0x01 , 0x1871 [0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
        //Inverter wants to know all individual cellvoltages (occurs 6 seconds after valid BMS reply)
        send_individual_pack_status = true;
      } else if (rx_frame.data.u8[4] == 0x02) {
        // EP request for detailed individual cell voltages.
        send_cellvoltages = true;
      } else if (rx_frame.data.u8[4] == 0x04) {
        // EP request for individual temperature values.
        send_celltemperatures = true;
      }

    } else if (rx_frame.data.u8[0] == 0x02) {  //0x1871 [0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
                                               // Ack message
    } else if (rx_frame.data.u8[0] == 0x05) {  //0x1871 [0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
      // Inverter wants to know the serial numbers of the packs (occurs 6 seconds after valid BMS reply)
      send_serial_numbers = true;
    }
  }
}

bool FoxessEpCanInverter::setup(void) {
  configured_number_of_modules = DEFAULT_NUMBER_OF_MODULES;
  configured_battery_type = DEFAULT_BATTERY_TYPE;
  configured_battery_subtype = DEFAULT_BATTERY_SUBTYPE;

  return true;
}
