#include "TESLA-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For Advanced Battery Insights webpage
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/* Credits: */
/* Some of the original CAN frame parsing code below comes from Per Carlen's bms_comms_tesla_model3.py (https://gitlab.com/pelle8/batt2gen24/) */
/* Most of the additional CAN frame parsing/information/display comes from Josiah Higgs (https://github.com/josiahhiggs/) */

inline const char* getContactorText(int index) {
  switch (index) {
    case 0:
      return "UNKNOWN(0)";
    case 1:
      return "OPEN";
    case 2:
      return "CLOSING";
    case 3:
      return "BLOCKED";
    case 4:
      return "OPENING";
    case 5:
      return "CLOSED";
    case 6:
      return "UNKNOWN(6)";
    case 7:
      return "WELDED";
    case 8:
      return "POS_CL";
    case 9:
      return "NEG_CL";
    case 10:
      return "UNKNOWN(10)";
    case 11:
      return "UNKNOWN(11)";
    case 12:
      return "UNKNOWN(12)";
    default:
      return "UNKNOWN";
  }
}

inline const char* getContactorState(int index) {
  switch (index) {
    case 0:
      return "SNA";
    case 1:
      return "OPEN";
    case 2:
      return "PRECHARGE";
    case 3:
      return "BLOCKED";
    case 4:
      return "PULLED_IN";
    case 5:
      return "OPENING";
    case 6:
      return "ECONOMIZED";
    case 7:
      return "WELDED";
    case 8:
      return "UNKNOWN(8)";
    case 9:
      return "UNKNOWN(9)";
    case 10:
      return "UNKNOWN(10)";
    case 11:
      return "UNKNOWN(11)";
    default:
      return "UNKNOWN";
  }
}

inline const char* getHvilStatusState(int index) {
  switch (index) {
    case 0:
      return "NOT OK";
    case 1:
      return "STATUS_OK";
    case 2:
      return "CURRENT_SOURCE_FAULT";
    case 3:
      return "INTERNAL_OPEN_FAULT";
    case 4:
      return "VEHICLE_OPEN_FAULT";
    case 5:
      return "PENTHOUSE_LID_OPEN_FAULT";
    case 6:
      return "UNKNOWN_LOCATION_OPEN_FAULT";
    case 7:
      return "VEHICLE_NODE_FAULT";
    case 8:
      return "NO_12V_SUPPLY";
    case 9:
      return "VEHICLE_OR_PENTHOUSE_LID_OPENFAULT";
    case 10:
      return "UNKNOWN(10)";
    case 11:
      return "UNKNOWN(11)";
    case 12:
      return "UNKNOWN(12)";
    case 13:
      return "UNKNOWN(13)";
    case 14:
      return "UNKNOWN(14)";
    case 15:
      return "UNKNOWN(15)";
    default:
      return "UNKNOWN";
  }
}

inline const char* getBMSContactorState(int index) {
  switch (index) {
    case 0:
      return "SNA";
    case 1:
      return "OPEN";
    case 2:
      return "OPENING";
    case 3:
      return "CLOSING";
    case 4:
      return "CLOSED";
    case 5:
      return "WELDED";
    case 6:
      return "BLOCKED";
    default:
      return "UNKNOWN";
  }
}

inline const char* getNoYes(bool value) {
  return value ? "Yes" : "No";
}

// Clamp DLC to 0–8 bytes for classic CAN
inline int getDataLen(uint8_t dlc) {
  return std::min<int>(dlc, 8);
}

// Fast bit‐field writer: writes 'bitLen' bits of 'value' starting at 'startBit'
inline void setBitField(uint8_t* data, int bytes, int startBit, int bitLen, uint64_t value) {
  int bit = startBit;
  for (int i = 0; i < bitLen && bit < bytes * 8; ++i, ++bit) {
    uint8_t* p = data + (bit >> 3);
    uint8_t m = uint8_t(1u << (bit & 7));
    *p = (*p & ~m) | (uint8_t((value >> i) & 1) << (bit & 7));
  }
}

/// Increment a counter field and recompute an 8‑bit checksum as part of a mux
void generateMuxFrameCounterChecksum(CAN_frame& f,
                                     uint8_t frameCounter,  // counter value
                                     int ctrStartBit,       // bit index of counter LSB
                                     int ctrBitLength,      // width of counter in bits
                                     int csumStartBit,      // bit index of checksum LSB
                                     int csumBitLength      // width of checksum in bits
) {
  int bytes = getDataLen(f.DLC);
  auto data = f.data.u8;

  // Pack payload into a 64‑bit word
  uint64_t w = 0;
  for (uint8_t i = 0; i < bytes; ++i) {
    w |= uint64_t(data[i]) << (8 * i);
  }

  // Increment the counter
  {
    uint64_t mask = (uint64_t(1) << ctrBitLength) - 1;
    uint64_t ctr = frameCounter & mask;  // External counter 0-15
    w = (w & ~(mask << ctrStartBit)) | (ctr << ctrStartBit);
  }

  // Unpack back into the frame bytes
  for (uint8_t i = 0; i < bytes; ++i) {
    data[i] = uint8_t((w >> (8 * i)) & 0xFF);
  }

  // Build a small buffer and zero out the checksum bits
  uint8_t buf[8];
  for (uint8_t i = 0; i < bytes; ++i) {
    buf[i] = data[i];
  }
  for (int bit = csumStartBit; bit < csumStartBit + csumBitLength; ++bit) {
    int b = bit >> 3;
    if (b >= bytes)
      break;
    buf[b] &= uint8_t(~(1u << (bit & 7)));
  }

  // Compute checksum offset from most significant hex digit of CAN ID
  uint8_t checksum_offset = uint8_t((f.ID >> 8) & 0xF);  // high nibble of top byte

  // Sum the low byte of ID + buf[]
  uint8_t sum = uint8_t(f.ID & 0xFF);
  for (int i = 0; i < bytes; ++i) {
    sum = uint8_t(sum + buf[i]);
  }
  uint8_t checksum = uint8_t(sum + checksum_offset);

  // Write the checksum back into the frame
  setBitField(data, bytes, csumStartBit, csumBitLength, checksum);
}

// Increment a counter field and recompute an 8‑bit checksum
void generateFrameCounterChecksum(CAN_frame& f,
                                  int ctrStartBit,   // bit index of counter LSB
                                  int ctrBitLength,  // width of counter in bits
                                  int csumStartBit,  // bit index of checksum LSB
                                  int csumBitLength  // width of checksum in bits
) {
  int bytes = getDataLen(f.DLC);
  auto data = f.data.u8;

  // Pack payload into a 64‑bit word
  uint64_t w = 0;
  for (int i = 0; i < bytes; ++i) {
    w |= uint64_t(data[i]) << (8 * i);
  }

  // Increment the counter by +1 modulo its width
  {
    uint64_t mask = (uint64_t(1) << ctrBitLength) - 1;
    uint64_t ctr = ((w >> ctrStartBit) & mask) + 1;
    ctr &= mask;
    w = (w & ~(mask << ctrStartBit)) | (ctr << ctrStartBit);
  }

  // Unpack back into the frame bytes
  for (int i = 0; i < bytes; ++i) {
    data[i] = uint8_t((w >> (8 * i)) & 0xFF);
  }

  // Build a small buffer and zero out the checksum bits
  uint8_t buf[8];
  for (int i = 0; i < bytes; ++i) {
    buf[i] = data[i];
  }
  for (int bit = csumStartBit; bit < csumStartBit + csumBitLength; ++bit) {
    int b = bit >> 3;
    if (b >= bytes)
      break;
    buf[b] &= uint8_t(~(1u << (bit & 7)));
  }

  // Compute checksum offset from most significant hex digit of CAN ID
  uint8_t checksum_offset = uint8_t((f.ID >> 8) & 0xF);  // high nibble of top byte

  // Sum the low byte of ID + buf[]
  uint8_t sum = uint8_t(f.ID & 0xFF);
  for (int i = 0; i < bytes; ++i) {
    sum = uint8_t(sum + buf[i]);
  }
  uint8_t checksum = uint8_t(sum + checksum_offset);

  // Write the checksum back into the frame
  setBitField(data, bytes, csumStartBit, csumBitLength, checksum);
}

// Function to extract raw bits/values from a given CAN frame signal
inline uint64_t extract_signal_value(const uint8_t* data, uint32_t start_bit, uint32_t bit_length) {
  //
  // Usage: uint8_t bms_state = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 31, 4));
  //
  // Calculate the starting byte and bit offset
  uint32_t byte_index = start_bit / 8;
  uint32_t bit_offset = start_bit % 8;

  // Read up to 8 bytes starting from byte_index (need enough to cover bit_length + bit_offset)
  uint64_t raw = 0;
  for (int i = 0; i < 8 && (byte_index + i) < 64; ++i) {
    raw |= (uint64_t)data[byte_index + i] << (8 * i);
  }

  // Shift and mask
  raw >>= bit_offset;
  if (bit_length == 64)
    return raw;
  return raw & ((1ULL << bit_length) - 1);
}

// Function to write a value to a given CAN frame signal
void write_signal_value(CAN_frame* frame, uint16_t start_bit, uint8_t bit_length, int64_t value, bool is_signed) {
  if (bit_length == 0 || bit_length > 64 || frame == nullptr)
    return;

  uint64_t uvalue;

  if (is_signed) {
    int64_t min_val = -(1LL << (bit_length - 1));
    int64_t max_val = (1LL << (bit_length - 1)) - 1;

    // Clamp to valid range
    if (value < min_val)
      value = min_val;
    if (value > max_val)
      value = max_val;

    // Two's complement encoding
    uvalue = static_cast<uint64_t>(value) & ((1ULL << bit_length) - 1);
  } else {
    uvalue = static_cast<uint64_t>(value) & ((1ULL << bit_length) - 1);
  }

  // Write value into frame->data.u8 using little-endian bit layout
  for (uint8_t i = 0; i < bit_length; ++i) {
    uint8_t bit_val = (uvalue >> i) & 1;
    uint16_t bit_pos = start_bit + i;

    uint8_t byte_index = bit_pos / 8;
    uint8_t bit_index = bit_pos % 8;

    if (byte_index >= frame->DLC)
      continue;  // Prevent overrun

    if (bit_val)
      frame->data.u8[byte_index] |= (1 << bit_index);
    else
      frame->data.u8[byte_index] &= ~(1 << bit_index);
  }
}

void generateTESLA_229(CAN_frame& f) {
  static const uint8_t checksumLookup[16] = {0x46, 0x44, 0x52, 0x6D, 0x43, 0x41, 0xDD, 0xF9,
                                             0x4C, 0xA5, 0xF6, 0x8C, 0x49, 0x2F, 0x31, 0x3B};

  // Safety, only run if this is the right ID
  if (f.ID != 0x229)
    return;

  const int ctrStartBit = 8;
  const int ctrBitLength = 4;
  const int csumStartBit = 0;
  const int csumBitLength = 8;

  int bytes = getDataLen(f.DLC);
  auto data = f.data.u8;

  // Pack the first few bytes into a word
  uint64_t w = 0;
  for (int i = 0; i < bytes; ++i) {
    w |= uint64_t(data[i]) << (8 * i);
  }

  // Extract current counter
  uint64_t mask = (uint64_t(1) << ctrBitLength) - 1;
  uint8_t ctr = (w >> ctrStartBit) & mask;

  // Increment counter mod 16
  ctr = (ctr + 1) & 0xF;

  // Write updated counter back
  w = (w & ~(mask << ctrStartBit)) | (uint64_t(ctr) << ctrStartBit);
  for (int i = 0; i < bytes; ++i) {
    data[i] = uint8_t((w >> (8 * i)) & 0xFF);
  }

  // Look up and insert checksum
  uint8_t checksum = checksumLookup[ctr];
  setBitField(data, bytes, csumStartBit, csumBitLength, checksum);
}

void generateTESLA_213(CAN_frame& f) {
  static uint8_t counter = 0;

  // Increment counter (wrap at 16)
  counter = (counter + 1) & 0xF;

  // Safety, only modify if ID is 0x213 and DLC is at least 2
  if (f.ID != 0x213 || f.DLC < 2)
    return;

  // Byte 0: counter in high nibble
  uint8_t value = counter << 4;

  // Byte 1: checksum = value + 0x15
  uint8_t checksum = (value + 0x15) & 0xFF;

  f.data.u8[0] = value;
  f.data.u8[1] = checksum;
}

// Function to check if a year is a leap year
bool isLeapYear(int year) {
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    return true;
  }
  return false;
}

// Function to convert year and day of year (i.e. Julian date) into human readable date
char* dayOfYearToDate(int year, int dayOfYear) {

  // Arrays to hold the number of days in each month for standard/leap years
  int daysInMonthStandard[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int daysInMonthLeap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Select the appropriate array for the given year
  int* daysInMonth = isLeapYear(year) ? daysInMonthLeap : daysInMonthStandard;
  int month = 0;

  // Find the month and the day within the month
  while (month < 12 && dayOfYear > daysInMonth[month]) {
    dayOfYear -= daysInMonth[month];
    month++;
  }

  // Ensure month is in valid range [0, 11]
  if (month >= 12) {
    month = 11;
    dayOfYear = daysInMonth[11];  // Set to last day of December
  }

  // Ensure day is in valid range [1, daysInMonth[month]]
  if (dayOfYear < 1) {
    dayOfYear = 1;
  } else if (dayOfYear > daysInMonth[month]) {
    dayOfYear = daysInMonth[month];
  }

  static char dateString[11];  // For "YYYY-MM-DD\0"

  // Clamp values to ensure they fit in the expected number of digits
  int safeYear = year % 10000;
  if (safeYear < 0)
    safeYear = 0;
  if (safeYear > 9999)
    safeYear = 9999;

  int safeMonth = month + 1;
  if (safeMonth < 1)
    safeMonth = 1;
  if (safeMonth > 12)
    safeMonth = 12;

  int safeDay = dayOfYear;
  if (safeDay < 1)
    safeDay = 1;
  if (safeDay > 31)
    safeDay = 31;

  // Format the date string in "YYYY-MM-DD" format
  snprintf(dateString, sizeof(dateString), "%04d-%02d-%02d", safeYear, safeMonth, safeDay);
  return dateString;
}

void TeslaBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery.status.soh_pptt = 9900;  //Tesla batteries do not send a SOH% value on bus. Hardcode to 99%

  datalayer.battery.status.real_soc = (battery_soc_ui * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = battery_volts;

  datalayer.battery.status.current_dA = battery_amps;  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  // Define the allowed discharge power
  datalayer.battery.status.max_discharge_power_W = (battery_max_discharge_current * (battery_volts / 10));
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery.status.max_discharge_power_W > datalayer.battery.status.override_discharge_power_W) {
    datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery_soc_ui > 990) {
    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery_soc_ui >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        RAMPDOWNPOWERALLOWED * (1 - (battery_soc_ui - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    } else {  //NCM/A
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  datalayer.battery.status.temperature_min_dC = battery_min_temp;

  datalayer.battery.status.temperature_max_dC = battery_max_temp;

  datalayer.battery.status.cell_max_voltage_mV = battery_cell_max_v;

  datalayer.battery.status.cell_min_voltage_mV = battery_cell_min_v;

  /* Value mapping is completed. Start to check all safeties */

  //INTERNAL_OPEN_FAULT - Someone disconnected a high voltage cable while battery was in use
  if (battery_hvil_status == 3) {
    set_event(EVENT_INTERNAL_OPEN_FAULT, 0);
  } else {
    clear_event(EVENT_INTERNAL_OPEN_FAULT);
  }
  //Voltage between 0.5-5.0V, pyrofuse most likely blown
  if (datalayer.battery.status.voltage_dV >= 5 && datalayer.battery.status.voltage_dV <= 50) {
    set_event(EVENT_BATTERY_FUSE, 0);
  } else {
    clear_event(EVENT_BATTERY_FUSE);
  }
  // Raise any Tesla BMS events in BE
  // Events: Informational
  if (BMS_a145_SW_SOC_Change) {                             // BMS has newly recalibrated pack SOC
    set_event_latched(EVENT_BATTERY_SOC_RECALIBRATION, 0);  // Latcched as BMS_a145 can be active for a while
  } else if (!BMS_a145_SW_SOC_Change) {
    clear_event(EVENT_BATTERY_SOC_RECALIBRATION);
  }
  // Events: Warning
  if (BMS_contactorState == 5) {  // BMS has detected welded contactor(s)
    set_event_latched(EVENT_CONTACTOR_WELDED, 0);
  } else if (BMS_contactorState != 5) {
    clear_event(EVENT_CONTACTOR_WELDED);
  }

  if (user_selected_tesla_GTW_chassisType > 1) {  //{{0, "Model S"}, {1, "Model X"}, {2, "Model 3"}, {3, "Model Y"}};
    // Autodetect algorithm for chemistry on 3/Y packs.
    // NCM/A batteries have 96s, LFP has 102-108s
    // Drawback with this check is that it takes 3-5 minutes before all cells have been counted!
    if (datalayer.battery.info.number_of_cells > 101) {
      datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
    }

    //Once cell chemistry is determined, set maximum and minimum total pack voltage safety limits
    if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
      datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
      datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
      datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
    } else {  // NCM/A chemistry
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
      datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
      datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
      datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
    }

    // During forced balancing request via webserver, we allow the battery to exceed normal safety parameters
    if (datalayer.battery.settings.user_requests_balancing) {
      datalayer.battery.status.real_soc = 9900;  //Force battery to show up as 99% when balancing
      datalayer.battery.info.max_design_voltage_dV = datalayer.battery.settings.balancing_max_pack_voltage_dV;
      datalayer.battery.info.max_cell_voltage_mV = datalayer.battery.settings.balancing_max_cell_voltage_mV;
      datalayer.battery.info.max_cell_voltage_deviation_mV =
          datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV;
      datalayer.battery.status.max_charge_power_W = datalayer.battery.settings.balancing_float_power_W;
    }
  }

  // Check if user requests some action
  if (datalayer.battery.settings.user_requests_tesla_isolation_clear) {
    stateMachineClearIsolationFault = 0;  //Start the isolation fault statemachine
    datalayer.battery.settings.user_requests_tesla_isolation_clear = false;
  }
  if (datalayer.battery.settings.user_requests_tesla_bms_reset) {
    if (battery_contactor == 1 && BMS_a180_SW_ECU_reset_blocked == false) {
      //Start the BMS ECU reset statemachine, only if contactors are OPEN and BMS ECU allows it
      stateMachineBMSReset = 0;
      datalayer.battery.settings.user_requests_tesla_bms_reset = false;
      logging.println("INFO: BMS reset requested");
    } else {
      logging.println("ERROR: BMS reset failed due to contactors not being open, or BMS ECU not allowing it");
      stateMachineBMSReset = 0xFF;
      datalayer.battery.settings.user_requests_tesla_bms_reset = false;
      set_event(EVENT_BMS_RESET_REQ_FAIL, 0);
      clear_event(EVENT_BMS_RESET_REQ_FAIL);
    }
  }
  if (datalayer.battery.settings.user_requests_tesla_soc_reset) {
    if ((datalayer.battery.status.real_soc < 1500 || datalayer.battery.status.real_soc > 9000) &&
        battery_contactor == 1) {
      //Start the SOC reset statemachine, only if SOC less than 15% or greater than 90%, and contactors open
      stateMachineSOCReset = 0;
      datalayer.battery.settings.user_requests_tesla_soc_reset = false;
      logging.println("INFO: SOC reset requested");
    } else {
      logging.println("ERROR: SOC reset failed, SOC not < 15 or > 90, or contactors not open");
      stateMachineSOCReset = 0xFF;
      datalayer.battery.settings.user_requests_tesla_soc_reset = false;
      set_event(EVENT_BATTERY_SOC_RESET_FAIL, 0);
      clear_event(EVENT_BATTERY_SOC_RESET_FAIL);
    }
  }

  //Update 0x333 UI_chargeTerminationPct (bit 16, width 10) value to SOC max value - expose via UI?
  //One firmware version this was seen at bit 17 width 11
  write_signal_value(&TESLA_333, 16, 10, static_cast<int64_t>(datalayer.battery.settings.max_percentage / 10), false);

  // Update webserver datalayer
  //datalayer_extended.tesla.BMS_hvilFault = BMS_a036_SW_HvpHvilFault;
  datalayer_extended.tesla.hvil_status = battery_hvil_status;
  //0x20A
  datalayer_extended.tesla.packContNegativeState = battery_packContNegativeState;
  datalayer_extended.tesla.packContPositiveState = battery_packContPositiveState;
  datalayer_extended.tesla.packContactorSetState = battery_packContactorSetState;
  datalayer_extended.tesla.packCtrsClosingBlocked = battery_packCtrsClosingBlocked;
  datalayer_extended.tesla.pyroTestInProgress = battery_pyroTestInProgress;
  datalayer_extended.tesla.battery_packCtrsOpenNowRequested = battery_packCtrsOpenNowRequested;
  datalayer_extended.tesla.battery_packCtrsOpenRequested = battery_packCtrsOpenRequested;
  datalayer_extended.tesla.battery_packCtrsRequestStatus = battery_packCtrsRequestStatus;
  datalayer_extended.tesla.battery_packCtrsResetRequestRequired = battery_packCtrsResetRequestRequired;
  datalayer_extended.tesla.battery_dcLinkAllowedToEnergize = battery_dcLinkAllowedToEnergize;
  //0x72A
  if (parsed_battery_serialNumber && battery_serialNumber[13] != 0) {
    memcpy(datalayer_extended.tesla.battery_serialNumber, battery_serialNumber, sizeof(battery_serialNumber));
    datalayer_extended.tesla.battery_manufactureDate = battery_manufactureDate;
    //We have valid data and comms with the battery, attempt to query part number
    if (!parsed_battery_partNumber && stateMachineBMSQuery == 0xFF) {
      stateMachineBMSQuery = 0;
    }
  }
  //Via UDS
  if (parsed_battery_partNumber && battery_partNumber[11] != 0) {
    memcpy(datalayer_extended.tesla.battery_partNumber, battery_partNumber, sizeof(battery_partNumber));
  }
  //0x3C4
  if (parsed_PCS_partNumber && PCS_partNumber[11] != 0) {
    memcpy(datalayer_extended.tesla.PCS_partNumber, PCS_partNumber, sizeof(PCS_partNumber));
  }
  //0x2B4
  datalayer_extended.tesla.battery_dcdcLvBusVolt = battery_dcdcLvBusVolt;
  datalayer_extended.tesla.battery_dcdcHvBusVolt = battery_dcdcHvBusVolt;
  datalayer_extended.tesla.battery_dcdcLvOutputCurrent = battery_dcdcLvOutputCurrent;
  //0x352
  datalayer_extended.tesla.BMS352_mux = BMS352_mux;
  datalayer_extended.tesla.battery_nominal_full_pack_energy = battery_nominal_full_pack_energy;
  datalayer_extended.tesla.battery_nominal_full_pack_energy_m0 = battery_nominal_full_pack_energy_m0;
  datalayer_extended.tesla.battery_nominal_energy_remaining = battery_nominal_energy_remaining;
  datalayer_extended.tesla.battery_nominal_energy_remaining_m0 = battery_nominal_energy_remaining_m0;
  datalayer_extended.tesla.battery_ideal_energy_remaining = battery_ideal_energy_remaining;
  datalayer_extended.tesla.battery_ideal_energy_remaining_m0 = battery_ideal_energy_remaining_m0;
  datalayer_extended.tesla.battery_energy_to_charge_complete = battery_energy_to_charge_complete;
  datalayer_extended.tesla.battery_energy_to_charge_complete_m1 = battery_energy_to_charge_complete_m1;
  datalayer_extended.tesla.battery_energy_buffer = battery_energy_buffer;
  datalayer_extended.tesla.battery_energy_buffer_m1 = battery_energy_buffer_m1;
  datalayer_extended.tesla.battery_expected_energy_remaining_m1 = battery_expected_energy_remaining_m1;
  datalayer_extended.tesla.battery_full_charge_complete = battery_full_charge_complete;
  datalayer_extended.tesla.battery_fully_charged = battery_fully_charged;
  //0x3D2
  datalayer.battery.status.total_discharged_battery_Wh = battery_total_discharge;
  datalayer.battery.status.total_charged_battery_Wh = battery_total_charge;
  //0x392
  datalayer_extended.tesla.battery_moduleType = battery_moduleType;
  datalayer_extended.tesla.battery_packMass = battery_packMass;
  datalayer_extended.tesla.battery_platformMaxBusVoltage = battery_platformMaxBusVoltage;
  //0x2D2
  datalayer_extended.tesla.BMS_min_voltage = BMS_min_voltage;
  datalayer_extended.tesla.BMS_max_voltage = BMS_max_voltage;
  datalayer_extended.tesla.battery_max_charge_current = battery_max_charge_current;
  datalayer_extended.tesla.battery_max_discharge_current = battery_max_discharge_current;
  //0x292
  datalayer_extended.tesla.battery_beginning_of_life = battery_beginning_of_life;
  datalayer_extended.tesla.battery_battTempPct = battery_battTempPct;
  datalayer_extended.tesla.battery_soc_ave = battery_soc_ave;
  datalayer_extended.tesla.battery_soc_max = battery_soc_max;
  datalayer_extended.tesla.battery_soc_min = battery_soc_min;
  datalayer_extended.tesla.battery_soc_ui = battery_soc_ui;
  //0x332
  datalayer_extended.tesla.battery_BrickVoltageMax = battery_BrickVoltageMax;
  datalayer_extended.tesla.battery_BrickVoltageMin = battery_BrickVoltageMin;
  datalayer_extended.tesla.battery_BrickTempMaxNum = battery_BrickTempMaxNum;
  datalayer_extended.tesla.battery_BrickTempMinNum = battery_BrickTempMinNum;
  datalayer_extended.tesla.battery_BrickModelTMax = battery_BrickModelTMax;
  datalayer_extended.tesla.battery_BrickModelTMin = battery_BrickModelTMin;
  //0x212
  datalayer_extended.tesla.BMS_isolationResistance = BMS_isolationResistance;
  datalayer_extended.tesla.BMS_contactorState = BMS_contactorState;
  datalayer_extended.tesla.BMS_state = BMS_state;
  datalayer_extended.tesla.BMS_hvState = BMS_hvState;
  datalayer_extended.tesla.BMS_uiChargeStatus = BMS_uiChargeStatus;
  datalayer_extended.tesla.BMS_diLimpRequest = BMS_diLimpRequest;
  datalayer_extended.tesla.BMS_chgPowerAvailable = BMS_chgPowerAvailable;
  datalayer_extended.tesla.BMS_pcsPwmEnabled = BMS_pcsPwmEnabled;
  //0x224
  datalayer_extended.tesla.PCS_dcdcPrechargeStatus = PCS_dcdcPrechargeStatus;
  datalayer_extended.tesla.PCS_dcdc12VSupportStatus = PCS_dcdc12VSupportStatus;
  datalayer_extended.tesla.PCS_dcdcHvBusDischargeStatus = PCS_dcdcHvBusDischargeStatus;
  datalayer_extended.tesla.PCS_dcdcMainState = PCS_dcdcMainState;
  datalayer_extended.tesla.PCS_dcdcSubState = PCS_dcdcSubState;
  datalayer_extended.tesla.PCS_dcdcFaulted = PCS_dcdcFaulted;
  datalayer_extended.tesla.PCS_dcdcOutputIsLimited = PCS_dcdcOutputIsLimited;
  datalayer_extended.tesla.PCS_dcdcMaxOutputCurrentAllowed = PCS_dcdcMaxOutputCurrentAllowed;
  datalayer_extended.tesla.PCS_dcdcPrechargeRtyCnt = PCS_dcdcPrechargeRtyCnt;
  datalayer_extended.tesla.PCS_dcdc12VSupportRtyCnt = PCS_dcdc12VSupportRtyCnt;
  datalayer_extended.tesla.PCS_dcdcDischargeRtyCnt = PCS_dcdcDischargeRtyCnt;
  datalayer_extended.tesla.PCS_dcdcPwmEnableLine = PCS_dcdcPwmEnableLine;
  datalayer_extended.tesla.PCS_dcdcSupportingFixedLvTarget = PCS_dcdcSupportingFixedLvTarget;
  datalayer_extended.tesla.PCS_dcdcPrechargeRestartCnt = PCS_dcdcPrechargeRestartCnt;
  datalayer_extended.tesla.PCS_dcdcInitialPrechargeSubState = PCS_dcdcInitialPrechargeSubState;
  //0x252
  datalayer_extended.tesla.BMS_maxRegenPower = BMS_maxRegenPower;
  datalayer_extended.tesla.BMS_maxDischargePower = BMS_maxDischargePower;
  datalayer_extended.tesla.BMS_maxStationaryHeatPower = BMS_maxStationaryHeatPower;
  datalayer_extended.tesla.BMS_hvacPowerBudget = BMS_hvacPowerBudget;
  datalayer_extended.tesla.BMS_notEnoughPowerForHeatPump = BMS_notEnoughPowerForHeatPump;
  datalayer_extended.tesla.BMS_powerLimitState = BMS_powerLimitState;
  datalayer_extended.tesla.BMS_inverterTQF = BMS_inverterTQF;
  //Via UDS
  //datalayer_extended.tesla.BMS_info_buildConfigId = BMS_info_buildConfigId;
  //0x300
  datalayer_extended.tesla.BMS_info_buildConfigId = BMS_info_buildConfigId;
  datalayer_extended.tesla.BMS_info_hardwareId = BMS_info_hardwareId;
  datalayer_extended.tesla.BMS_info_componentId = BMS_info_componentId;
  datalayer_extended.tesla.BMS_info_pcbaId = BMS_info_pcbaId;
  datalayer_extended.tesla.BMS_info_assemblyId = BMS_info_assemblyId;
  datalayer_extended.tesla.BMS_info_usageId = BMS_info_usageId;
  datalayer_extended.tesla.BMS_info_subUsageId = BMS_info_subUsageId;
  datalayer_extended.tesla.BMS_info_platformType = BMS_info_platformType;
  datalayer_extended.tesla.BMS_info_appCrc = BMS_info_appCrc;
  datalayer_extended.tesla.BMS_info_bootGitHash = BMS_info_bootGitHash;
  datalayer_extended.tesla.BMS_info_bootUdsProtoVersion = BMS_info_bootUdsProtoVersion;
  datalayer_extended.tesla.BMS_info_bootCrc = BMS_info_bootCrc;
  //0x312
  datalayer_extended.tesla.BMS_powerDissipation = BMS_powerDissipation;
  datalayer_extended.tesla.BMS_flowRequest = BMS_flowRequest;
  datalayer_extended.tesla.BMS_inletActiveCoolTargetT = BMS_inletActiveCoolTargetT;
  datalayer_extended.tesla.BMS_inletPassiveTargetT = BMS_inletPassiveTargetT;
  datalayer_extended.tesla.BMS_inletActiveHeatTargetT = BMS_inletActiveHeatTargetT;
  datalayer_extended.tesla.BMS_packTMin = BMS_packTMin;
  datalayer_extended.tesla.BMS_packTMax = BMS_packTMax;
  datalayer_extended.tesla.BMS_pcsNoFlowRequest = BMS_pcsNoFlowRequest;
  datalayer_extended.tesla.BMS_noFlowRequest = BMS_noFlowRequest;
  //0x3C4
  datalayer_extended.tesla.PCS_info_buildConfigId = PCS_info_buildConfigId;
  datalayer_extended.tesla.PCS_info_hardwareId = PCS_info_hardwareId;
  datalayer_extended.tesla.PCS_info_componentId = PCS_info_componentId;
  //0x2A4
  datalayer_extended.tesla.PCS_dcdcTemp = PCS_dcdcTemp;
  datalayer_extended.tesla.PCS_ambientTemp = PCS_ambientTemp;
  datalayer_extended.tesla.PCS_chgPhATemp = PCS_chgPhATemp;
  datalayer_extended.tesla.PCS_chgPhBTemp = PCS_chgPhBTemp;
  datalayer_extended.tesla.PCS_chgPhCTemp = PCS_chgPhCTemp;
  //0x2C4
  datalayer_extended.tesla.PCS_dcdcMaxLvOutputCurrent = PCS_dcdcMaxLvOutputCurrent;
  datalayer_extended.tesla.PCS_dcdcCurrentLimit = PCS_dcdcCurrentLimit;
  datalayer_extended.tesla.PCS_dcdcLvOutputCurrentTempLimit = PCS_dcdcLvOutputCurrentTempLimit;
  datalayer_extended.tesla.PCS_dcdcUnifiedCommand = PCS_dcdcUnifiedCommand;
  datalayer_extended.tesla.PCS_dcdcCLAControllerOutput = PCS_dcdcCLAControllerOutput;
  datalayer_extended.tesla.PCS_dcdcTankVoltage = PCS_dcdcTankVoltage;
  datalayer_extended.tesla.PCS_dcdcTankVoltageTarget = PCS_dcdcTankVoltageTarget;
  datalayer_extended.tesla.PCS_dcdcClaCurrentFreq = PCS_dcdcClaCurrentFreq;
  datalayer_extended.tesla.PCS_dcdcTCommMeasured = PCS_dcdcTCommMeasured;
  datalayer_extended.tesla.PCS_dcdcShortTimeUs = PCS_dcdcShortTimeUs;
  datalayer_extended.tesla.PCS_dcdcHalfPeriodUs = PCS_dcdcHalfPeriodUs;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxFrequency = PCS_dcdcIntervalMaxFrequency;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxHvBusVolt = PCS_dcdcIntervalMaxHvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxLvBusVolt = PCS_dcdcIntervalMaxLvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxLvOutputCurr = PCS_dcdcIntervalMaxLvOutputCurr;
  datalayer_extended.tesla.PCS_dcdcIntervalMinFrequency = PCS_dcdcIntervalMinFrequency;
  datalayer_extended.tesla.PCS_dcdcIntervalMinHvBusVolt = PCS_dcdcIntervalMinHvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMinLvBusVolt = PCS_dcdcIntervalMinLvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMinLvOutputCurr = PCS_dcdcIntervalMinLvOutputCurr;
  datalayer_extended.tesla.PCS_dcdc12vSupportLifetimekWh = PCS_dcdc12vSupportLifetimekWh;
  //0x310
  datalayer_extended.tesla.HVP_info_buildConfigId = HVP_info_buildConfigId;
  datalayer_extended.tesla.HVP_info_hardwareId = HVP_info_hardwareId;
  datalayer_extended.tesla.HVP_info_componentId = HVP_info_componentId;
  //0x7AA
  datalayer_extended.tesla.HVP_gpioPassivePyroDepl = HVP_gpioPassivePyroDepl;
  datalayer_extended.tesla.HVP_gpioPyroIsoEn = HVP_gpioPyroIsoEn;
  datalayer_extended.tesla.HVP_gpioCpFaultIn = HVP_gpioCpFaultIn;
  datalayer_extended.tesla.HVP_gpioPackContPowerEn = HVP_gpioPackContPowerEn;
  datalayer_extended.tesla.HVP_gpioHvCablesOk = HVP_gpioHvCablesOk;
  datalayer_extended.tesla.HVP_gpioHvpSelfEnable = HVP_gpioHvpSelfEnable;
  datalayer_extended.tesla.HVP_gpioLed = HVP_gpioLed;
  datalayer_extended.tesla.HVP_gpioCrashSignal = HVP_gpioCrashSignal;
  datalayer_extended.tesla.HVP_gpioShuntDataReady = HVP_gpioShuntDataReady;
  datalayer_extended.tesla.HVP_gpioFcContPosAux = HVP_gpioFcContPosAux;
  datalayer_extended.tesla.HVP_gpioFcContNegAux = HVP_gpioFcContNegAux;
  datalayer_extended.tesla.HVP_gpioBmsEout = HVP_gpioBmsEout;
  datalayer_extended.tesla.HVP_gpioCpFaultOut = HVP_gpioCpFaultOut;
  datalayer_extended.tesla.HVP_gpioPyroPor = HVP_gpioPyroPor;
  datalayer_extended.tesla.HVP_gpioShuntEn = HVP_gpioShuntEn;
  datalayer_extended.tesla.HVP_gpioHvpVerEn = HVP_gpioHvpVerEn;
  datalayer_extended.tesla.HVP_gpioPackCoontPosFlywheel = HVP_gpioPackCoontPosFlywheel;
  datalayer_extended.tesla.HVP_gpioCpLatchEnable = HVP_gpioCpLatchEnable;
  datalayer_extended.tesla.HVP_gpioPcsEnable = HVP_gpioPcsEnable;
  datalayer_extended.tesla.HVP_gpioPcsDcdcPwmEnable = HVP_gpioPcsDcdcPwmEnable;
  datalayer_extended.tesla.HVP_gpioPcsChargePwmEnable = HVP_gpioPcsChargePwmEnable;
  datalayer_extended.tesla.HVP_gpioFcContPowerEnable = HVP_gpioFcContPowerEnable;
  datalayer_extended.tesla.HVP_gpioHvilEnable = HVP_gpioHvilEnable;
  datalayer_extended.tesla.HVP_gpioSecDrdy = HVP_gpioSecDrdy;
  datalayer_extended.tesla.HVP_hvp1v5Ref = HVP_hvp1v5Ref;
  datalayer_extended.tesla.HVP_shuntCurrentDebug = HVP_shuntCurrentDebug;
  datalayer_extended.tesla.HVP_packCurrentMia = HVP_packCurrentMia;
  datalayer_extended.tesla.HVP_auxCurrentMia = HVP_auxCurrentMia;
  datalayer_extended.tesla.HVP_currentSenseMia = HVP_currentSenseMia;
  datalayer_extended.tesla.HVP_shuntRefVoltageMismatch = HVP_shuntRefVoltageMismatch;
  datalayer_extended.tesla.HVP_shuntThermistorMia = HVP_shuntThermistorMia;
  datalayer_extended.tesla.HVP_shuntHwMia = HVP_shuntHwMia;
  datalayer_extended.tesla.HVP_dcLinkVoltage = HVP_dcLinkVoltage;
  datalayer_extended.tesla.HVP_packVoltage = HVP_packVoltage;
  datalayer_extended.tesla.HVP_fcLinkVoltage = HVP_fcLinkVoltage;
  datalayer_extended.tesla.HVP_packContVoltage = HVP_packContVoltage;
  datalayer_extended.tesla.HVP_packNegativeV = HVP_packNegativeV;
  datalayer_extended.tesla.HVP_packPositiveV = HVP_packPositiveV;
  datalayer_extended.tesla.HVP_pyroAnalog = HVP_pyroAnalog;
  datalayer_extended.tesla.HVP_dcLinkNegativeV = HVP_dcLinkNegativeV;
  datalayer_extended.tesla.HVP_dcLinkPositiveV = HVP_dcLinkPositiveV;
  datalayer_extended.tesla.HVP_fcLinkNegativeV = HVP_fcLinkNegativeV;
  datalayer_extended.tesla.HVP_fcContCoilCurrent = HVP_fcContCoilCurrent;
  datalayer_extended.tesla.HVP_fcContVoltage = HVP_fcContVoltage;
  datalayer_extended.tesla.HVP_hvilInVoltage = HVP_hvilInVoltage;
  datalayer_extended.tesla.HVP_hvilOutVoltage = HVP_hvilOutVoltage;
  datalayer_extended.tesla.HVP_fcLinkPositiveV = HVP_fcLinkPositiveV;
  datalayer_extended.tesla.HVP_packContCoilCurrent = HVP_packContCoilCurrent;
  datalayer_extended.tesla.HVP_battery12V = HVP_battery12V;
  datalayer_extended.tesla.HVP_shuntRefVoltageDbg = HVP_shuntRefVoltageDbg;
  datalayer_extended.tesla.HVP_shuntAuxCurrentDbg = HVP_shuntAuxCurrentDbg;
  datalayer_extended.tesla.HVP_shuntBarTempDbg = HVP_shuntBarTempDbg;
  datalayer_extended.tesla.HVP_shuntAsicTempDbg = HVP_shuntAsicTempDbg;
  datalayer_extended.tesla.HVP_shuntAuxCurrentStatus = HVP_shuntAuxCurrentStatus;
  datalayer_extended.tesla.HVP_shuntBarTempStatus = HVP_shuntBarTempStatus;
  datalayer_extended.tesla.HVP_shuntAsicTempStatus = HVP_shuntAsicTempStatus;

  //Safety checks for CAN message sending
  if ((datalayer.system.status.inverter_allows_contactor_closing == true) &&
      (datalayer.battery.status.bms_status != FAULT) && (!datalayer.system.settings.equipment_stop_active)) {
    // Carry on: 0x221 DRIVE state & reset power down timer
    vehicleState = CAR_DRIVE;
    powerDownSeconds = 9;
  } else {
    // Faulted state, or inverter blocks contactor closing
    // Shut down: 0x221 ACCESSORY state for 3 seconds, followed by GOING_DOWN, then OFF
    if (powerDownSeconds <= 9 && powerDownSeconds > 6) {
      vehicleState = ACCESSORY;
      powerDownSeconds--;
    }
    if (powerDownSeconds <= 6 && powerDownSeconds > 3) {
      vehicleState = GOING_DOWN;
      powerDownSeconds--;
    }
    if (powerDownSeconds <= 3 && powerDownSeconds > 0) {
      vehicleState = CAR_OFF;
      powerDownSeconds--;
    }
  }

  printFaultCodesIfActive();
  logging.printf("Contactor State: ");
  logging.printf(getBMSContactorState(battery_contactor));  // Display what state the BMS thinks the contactors are in
  logging.printf(" HVIL: ");
  logging.printf(getHvilStatusState(battery_hvil_status));
  logging.printf(" NegState: ");
  logging.printf(getContactorState(battery_packContNegativeState));
  logging.printf(" PosState: ");
  logging.println(getContactorState(battery_packContPositiveState));
  logging.printf("Cont. setState: ");
  logging.printf(
      getContactorText(battery_packContactorSetState));  // Display what state the HVP has set the contactors to be in
  logging.printf(" Closing blocked: ");
  logging.printf(getNoYes(battery_packCtrsClosingBlocked));
  if (battery_packContactorSetState == 5) {
    logging.printf(" (already CLOSED)");
  }
  logging.printf(", Pyrotest: ");
  logging.println(getNoYes(battery_pyroTestInProgress));

  logging.printf("HV: %.2f V, 12V: %.2f V, 12V current: %.2f A.\n", (battery_dcdcHvBusVolt * 0.146484),
                 (battery_dcdcLvBusVolt * 0.0390625), (battery_dcdcLvOutputCurrent * 0.1));
}

void TeslaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  static uint16_t temp = 0;
  static bool mux0_read = false;
  static bool mux1_read = false;

  switch (rx_frame.ID) {
    case 0x352:  // 850 BMS_energyStatus newer BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = ((rx_frame.data.u8[0]) & 0x03);  //BMS_energyStatusIndex M : 0|2@1+ (1,0) [0|0] ""  X
      if (mux == 0) {
        battery_nominal_full_pack_energy_m0 =
            (((rx_frame.data.u8[3]) << 8) |
             rx_frame.data.u8[2]);  //16|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_nominal_energy_remaining_m0 =
            (((rx_frame.data.u8[5]) << 8) |
             rx_frame.data.u8[4]);  //32|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_ideal_energy_remaining_m0 =
            (((rx_frame.data.u8[7]) << 8) |
             rx_frame.data.u8[6]);  //48|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        mux0_read = true;           //Set flag to true
      }
      if (mux == 1) {
        battery_fully_charged = (rx_frame.data.u8[1] & 0x01);  //BMS_fullChargeComplete : 15|1@1+ (1,0) [0|1]//noYes
        battery_energy_buffer_m1 =
            ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);  //16|16@1+ (0.01,0) [0|0] "kWh"//to datalayer_extended
        battery_expected_energy_remaining_m1 =
            ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);  //32|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_energy_to_charge_complete_m1 =
            ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);  //48|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        mux1_read = true;                                        //Set flag to true
      }
      if (mux == 2) {
      }  // Additional information needed on this mux 2, example frame: 02 26 02 20 02 80 00 00 doesn't change
      if (mux0_read && mux1_read) {
        mux0_read = false;
        mux1_read = false;
        BMS352_mux = true;  //Set flag to true
        break;
      }
      if (mux0_read || mux1_read || BMS352_mux) {
        //Skip older BMS frame parsing
        break;
      }
      // Older BMS <2021 without mux
      battery_nominal_full_pack_energy =  //BMS_nominalFullPackEnergy : 0|11@1+ (0.1,0) [0|204.6] "kWh" //((_d[1] & (0x07U)) << 8) | (_d[0] & (0xFFU));
          (((rx_frame.data.u8[1] & 0x07) << 8) | (rx_frame.data.u8[0]));  //Example 752 (75.2kWh)
      battery_nominal_energy_remaining =  //BMS_nominalEnergyRemaining : 11|11@1+ (0.1,0) [0|204.6] "kWh" //((_d[2] & (0x3FU)) << 5) | ((_d[1] >> 3) & (0x1FU));
          (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0x1F) >> 3));  //Example 1247 * 0.1 = 124.7kWh
      battery_expected_energy_remaining =  //BMS_expectedEnergyRemaining : 22|11@1+ (0.1,0) [0|204.6] "kWh"// ((_d[4] & (0x01U)) << 10) | ((_d[3] & (0xFFU)) << 2) | ((_d[2] >> 6) & (0x03U));
          (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) |
           ((rx_frame.data.u8[2] & 0x03) >> 6));  //Example 622 (62.2kWh)
      battery_ideal_energy_remaining =  //BMS_idealEnergyRemaining : 33|11@1+ (0.1,0) [0|204.6] "kWh" //((_d[5] & (0x0FU)) << 7) | ((_d[4] >> 1) & (0x7FU));
          (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0x7F) >> 1));  //Example 311 * 0.1 = 31.1kWh
      battery_energy_to_charge_complete =  // BMS_energyToChargeComplete : 44|11@1+ (0.1,0) [0|204.6] "kWh"// ((_d[6] & (0x7FU)) << 4) | ((_d[5] >> 4) & (0x0FU));
          (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0x0F) << 4));  //Example 147 * 0.1 = 14.7kWh
      battery_energy_buffer =  //BMS_energyBuffer : 55|8@1+ (0.1,0) [0|25.4] "kWh"// ((_d[7] & (0x7FU)) << 1) | ((_d[6] >> 7) & (0x01U));
          (((rx_frame.data.u8[7] & 0xFE) >> 1) | ((rx_frame.data.u8[6] & 0x80) >> 7));  //Example 1 * 0.1 = 0
      battery_fully_charged =  //BMS_fullChargeComplete : 63|1@1+ (1,0) [0|1] ""//((_d[7] >> 7) & (0x01U));
          ((rx_frame.data.u8[7] >> 7) & 0x01);  //noYes
      break;
    case 0x20A:  //522 HVP_contactorState:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_packContNegativeState =
          (rx_frame.data.u8[0] & 0x07);  //(_d[0] & (0x07U)); 0|3@1+ (1,0) [0|7] //contactorState
      battery_packContPositiveState =
          (rx_frame.data.u8[0] & 0x38) >> 3;  //((_d[0] >> 3) & (0x07U)); 3|3@1+ (1,0) [0|7] //contactorState
      //battery_contactor = (rx_frame.data.u8[1] & 0x0F);  // 8|4@1+ (1,0) [0|9] //contactorText
      battery_packContactorSetState =
          (rx_frame.data.u8[1] & 0x0F);  //(_d[1] & (0x0FU)); 8|4@1+ (1,0) [0|9] //contactorState
      battery_packCtrsClosingBlocked =
          (rx_frame.data.u8[4] & 0x08) >> 3;  //((_d[4] >> 3) & (0x01U)); 35|1@1+ (1,0) [0|1] //noYes
      battery_pyroTestInProgress =
          (rx_frame.data.u8[4] & 0x20) >> 5;  //((_d[4] >> 5) & (0x01U));//37|1@1+ (1,0) [0|1] //noYes
      battery_hvil_status =
          (rx_frame.data.u8[5] & 0x0F);  //(_d[5] & (0x0FU));   //40|4@1+ (1,0) [0|9] //hvilStatusState
      battery_packCtrsOpenNowRequested = ((rx_frame.data.u8[4] >> 1) & (0x01U));  //33|1@1+ (1,0) [0|1] //noYes
      battery_packCtrsOpenRequested = ((rx_frame.data.u8[4] >> 2) & (0x01U));     //34|1@1+ (1,0) [0|1] //noYes
      battery_packCtrsRequestStatus = ((rx_frame.data.u8[3] >> 6) & (0x03U));     //30|2@1+ (1,0) [0|2] //HVP_contactor
      battery_packCtrsResetRequestRequired = (rx_frame.data.u8[4] & (0x01U));     //32|1@1+ (1,0) [0|1] //noYes
      battery_dcLinkAllowedToEnergize = ((rx_frame.data.u8[4] >> 4) & (0x01U));   //36|1@1+ (1,0) [0|1] //noYes
      battery_fcContNegativeAuxOpen = ((rx_frame.data.u8[0] >> 7) & (0x01U));     //7|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcContNegativeState = ((rx_frame.data.u8[1] >> 4) & (0x07U));       //12|3@1+ (1,0) [0|7] ""  Receiver
      battery_fcContPositiveAuxOpen = ((rx_frame.data.u8[0] >> 6) & (0x01U));     //6|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcContPositiveState = (rx_frame.data.u8[2] & (0x07U));              //16|3@1+ (1,0) [0|7] ""  Receiver
      battery_fcContactorSetState = ((rx_frame.data.u8[2] >> 3) & (0x0FU));       //19|4@1+ (1,0) [0|9] ""  Receiver
      battery_fcCtrsClosingAllowed = ((rx_frame.data.u8[3] >> 5) & (0x01U));      //29|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsOpenNowRequested = ((rx_frame.data.u8[3] >> 3) & (0x01U));    //27|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsOpenRequested = ((rx_frame.data.u8[3] >> 4) & (0x01U));       //28|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsRequestStatus = (rx_frame.data.u8[3] & (0x03U));              //24|2@1+ (1,0) [0|2] ""  Receiver
      battery_fcCtrsResetRequestRequired = ((rx_frame.data.u8[3] >> 2) & (0x01U));  //26|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcLinkAllowedToEnergize = ((rx_frame.data.u8[5] >> 4) & (0x03U));     //44|2@1+ (1,0) [0|2] ""  Receiver
      break;
    case 0x212:  //530 BMS_status: 8
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_hvacPowerRequest = (rx_frame.data.u8[0] & (0x01U));
      BMS_notEnoughPowerForDrive = ((rx_frame.data.u8[0] >> 1) & (0x01U));
      BMS_notEnoughPowerForSupport = ((rx_frame.data.u8[0] >> 2) & (0x01U));
      BMS_preconditionAllowed = ((rx_frame.data.u8[0] >> 3) & (0x01U));
      BMS_updateAllowed = ((rx_frame.data.u8[0] >> 4) & (0x01U));
      BMS_activeHeatingWorthwhile = ((rx_frame.data.u8[0] >> 5) & (0x01U));
      BMS_cpMiaOnHvs = ((rx_frame.data.u8[0] >> 6) & (0x01U));
      BMS_contactorState = (rx_frame.data.u8[1] &
                            (0x07U));  //0 "SNA" 1 "OPEN" 2 "OPENING" 3 "CLOSING" 4 "CLOSED" 5 "WELDED" 6 "BLOCKED" ;
      battery_contactor = BMS_contactorState;
      //BMS_state = // Original code from older DBCs
      //((rx_frame.data.u8[1] >> 3) &
      //(0x0FU));  //0 "STANDBY" 1 "DRIVE" 2 "SUPPORT" 3 "CHARGE" 4 "FEIM" 5 "CLEAR_FAULT" 6 "FAULT" 7 "WELD" 8 "TEST" 9 "SNA" ;
      BMS_state = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 31, 4));
      //0 "STANDBY" 1 "DRIVE" 2 "SUPPORT" 3 "CHARGE" 4 "FEIM" 5 "CLEAR_FAULT" 6 "FAULT" 7 "WELD" 8 "TEST" 9 "SNA" 10 "BMS_DIAG";
      BMS_hvState = (rx_frame.data.u8[2] & (0x07U));
      //0 "DOWN" 1 "COMING_UP" 2 "GOING_DOWN" 3 "UP_FOR_DRIVE" 4 "UP_FOR_CHARGE" 5 "UP_FOR_DC_CHARGE" 6 "UP" ;
      BMS_isolationResistance =
          ((rx_frame.data.u8[3] & (0x1FU)) << 5) |
          ((rx_frame.data.u8[2] >> 3) & (0x1FU));  //19|10@1+ (10,0) [0|0] "kOhm"/to datalayer_extended
      //BMS_chargeRequest = ((rx_frame.data.u8[3] >> 5) & (0x01U));
      BMS_chargeRequest = static_cast<bool>(extract_signal_value(rx_frame.data.u8, 29, 1));
      BMS_keepWarmRequest = ((rx_frame.data.u8[3] >> 6) & (0x01U));
      BMS_uiChargeStatus = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 32, 3));
      //BMS_uiChargeStatus =
      //(rx_frame.data.u8[4] &
      //(0x07U));
      // 0 "DISCONNECTED" 1 "NO_POWER" 2 "ABOUT_TO_CHARGE" 3 "CHARGING" 4 "CHARGE_COMPLETE" 5 "CHARGE_STOPPED" ;
      BMS_diLimpRequest = ((rx_frame.data.u8[4] >> 3) & (0x01U));
      BMS_okToShipByAir = ((rx_frame.data.u8[4] >> 4) & (0x01U));
      BMS_okToShipByLand = ((rx_frame.data.u8[4] >> 5) & (0x01U));
      BMS_chgPowerAvailable = ((rx_frame.data.u8[6] & (0x01U)) << 10) | ((rx_frame.data.u8[5] & (0xFFU)) << 2) |
                              ((rx_frame.data.u8[4] >> 6) & (0x03U));  //38|11@1+ (0.125,0) [0|0] "kW"
      BMS_chargeRetryCount = ((rx_frame.data.u8[6] >> 1) & (0x0FU));
      BMS_pcsPwmEnabled = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      BMS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      BMS_minPackTemperature = (rx_frame.data.u8[7] & (0xFFU));  //56|8@1+ (0.5,-40) [0|0] "DegC
      break;
    case 0x224:  //548 PCS_dcdcStatus:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      PCS_dcdcPrechargeStatus = (rx_frame.data.u8[0] & (0x03U));              //0 "IDLE" 1 "ACTIVE" 2 "FAULTED" ;
      PCS_dcdc12VSupportStatus = ((rx_frame.data.u8[0] >> 2) & (0x03U));      //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      PCS_dcdcHvBusDischargeStatus = ((rx_frame.data.u8[0] >> 4) & (0x03U));  //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      PCS_dcdcMainState =
          ((rx_frame.data.u8[1] & (0x03U)) << 2) |
          ((rx_frame.data.u8[0] >> 6) &
           (0x03U));  //0 "STANDBY" 1 "12V_SUPPORT_ACTIVE" 2 "PRECHARGE_STARTUP" 3 "PRECHARGE_ACTIVE" 4 "DIS_HVBUS_ACTIVE" 5 "SHUTDOWN" 6 "FAULTED" ;
      PCS_dcdcSubState =
          ((rx_frame.data.u8[1] >> 2) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      PCS_dcdcFaulted = ((rx_frame.data.u8[1] >> 7) & (0x01U));
      PCS_dcdcOutputIsLimited = ((rx_frame.data.u8[3] >> 4) & (0x01U));
      PCS_dcdcMaxOutputCurrentAllowed = ((rx_frame.data.u8[5] & (0x01U)) << 11) |
                                        ((rx_frame.data.u8[4] & (0xFFU)) << 3) |
                                        ((rx_frame.data.u8[3] >> 5) & (0x07U));  //29|12@1+ (0.1,0) [0|0] "A"
      PCS_dcdcPrechargeRtyCnt = ((rx_frame.data.u8[5] >> 1) & (0x07U));          //Retry Count
      PCS_dcdc12VSupportRtyCnt = ((rx_frame.data.u8[5] >> 4) & (0x0FU));         //Retry Count
      PCS_dcdcDischargeRtyCnt = (rx_frame.data.u8[6] & (0x0FU));                 //Retry Count
      PCS_dcdcPwmEnableLine = ((rx_frame.data.u8[6] >> 4) & (0x01U));
      PCS_dcdcSupportingFixedLvTarget = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      PCS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      PCS_dcdcPrechargeRestartCnt = (rx_frame.data.u8[7] & (0x07U));
      PCS_dcdcInitialPrechargeSubState =
          ((rx_frame.data.u8[7] >> 3) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      break;
    case 0x252:  //Limit //594 BMS_powerAvailable:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_maxRegenPower = ((rx_frame.data.u8[1] << 8) |
                           rx_frame.data.u8[0]);  //0|16@1+ (0.01,0) [0|655.35] "kW"  //Example 4715 * 0.01 = 47.15kW
      BMS_maxDischargePower =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8[2]);  //16|16@1+ (0.013,0) [0|655.35] "kW"  //Example 2009 * 0.013 = 26.117???
      BMS_maxStationaryHeatPower =
          (((rx_frame.data.u8[5] & 0x03) << 8) |
           rx_frame.data.u8[4]);  //32|10@1+ (0.01,0) [0|10.23] "kW"  //Example 500 * 0.01 = 5kW
      BMS_hvacPowerBudget =
          (((rx_frame.data.u8[7] << 6) |
            ((rx_frame.data.u8[6] & 0xFC) >> 2)));  //50|10@1+ (0.02,0) [0|20.46] "kW"  //Example 1000 * 0.02 = 20kW?
      BMS_notEnoughPowerForHeatPump =
          ((rx_frame.data.u8[5] >> 2) & (0x01U));  //BMS_notEnoughPowerForHeatPump : 42|1@1+ (1,0) [0|1] ""  Receiver
      BMS_powerLimitState =
          (rx_frame.data.u8[6] &
           (0x01U));  //BMS_powerLimitsState : 48|1@1+ (1,0) [0|1] 0 "NOT_CALCULATED_FOR_DRIVE" 1 "CALCULATED_FOR_DRIVE"
      BMS_inverterTQF = ((rx_frame.data.u8[7] >> 4) & (0x03U));
      break;
    case 0x132:  //battery amps/volts //HVBattAmpVolt
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                      0.1;  //0|16@1+ (0.01,0) [0|655.35] "V"  //Example 37030mv * 0.01 = 3703dV
      battery_amps =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8
               [2]);  //SmoothBattCurrent : 16|16@1- (-0.1,0) [-3276.7|3276.7] "A"//Example 65492 (-4.3A) OR 225 (22.5A)
      battery_raw_amps =
          ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05 +
          822;  //RawBattCurrent : 32|16@1- (-0.05,822) [-1138.35|2138.4] "A"  //Example 10425 * -0.05 = ?
      battery_charge_time_remaining =
          (((rx_frame.data.u8[7] & 0x0F) << 8) |
           rx_frame.data.u8[6]);  //ChargeHoursRemaining : 48|12@1+ (1,0) [0|4095] "Min"  //Example 228 * 0.1 = 22.8min
      if (battery_charge_time_remaining == 4095) {
        battery_charge_time_remaining = 0;
      }
      break;
    case 0x3D2:  //TotalChargeDischarge:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                 (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      //0|32@1+ (0.001,0) [0|4294970] "kWh"
      battery_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                              rx_frame.data.u8[4]);
      //32|32@1+ (0.001,0) [0|4294970] "kWh"
      break;
    case 0x332:  //min/max hist values //BattBrickMinMax:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & 0x03);  //BattBrickMultiplexer M : 0|2@1+ (1,0) [0|0] ""

      if (mux == 1)  //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        battery_cell_max_v = temp * 2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        battery_cell_min_v = temp * 2;
        cellvoltagesRead = true;
        //BattBrickVoltageMax m1 : 2|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[1] & (0x3FU)) << 6) | ((_d[0] >> 2) & (0x3FU));
        battery_BrickVoltageMax =
            ((rx_frame.data.u8[1] & (0x3F)) << 6) | ((rx_frame.data.u8[0] >> 2) & (0x3F));  //to datalayer_extended
        //BattBrickVoltageMin m1 : 16|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[3] & (0x0FU)) << 8) | (_d[2] & (0xFFU));
        battery_BrickVoltageMin =
            ((rx_frame.data.u8[3] & (0x0F)) << 8) | (rx_frame.data.u8[2] & (0xFF));  //to datalayer_extended
        //BattBrickVoltageMaxNum m1 : 32|7@1+ (1,1) [0|0] ""  Receiver
        battery_BrickVoltageMaxNum =
            1 + (rx_frame.data.u8[4] & 0x7F);  //(_d[4] & (0x7FU)); //This cell has highest voltage
        //BattBrickVoltageMinNum m1 : 40|7@1+ (1,1) [0|0] ""  Receiver
        battery_BrickVoltageMinNum =
            1 + (rx_frame.data.u8[5] & 0x7F);  //(_d[5] & (0x7FU)); //This cell has lowest voltage
      }
      if (mux == 0)  //Temperature sensors
      {              //BattBrickTempMax m0 : 16|8@1+ (0.5,-40) [0|0] "C" (_d[2] & (0xFFU));
        battery_max_temp = (rx_frame.data.u8[2] * 5) - 400;  //Temperature values have 40.0*C offset, 0.5*C /bit
        //BattBrickTempMin m0 : 24|8@1+ (0.5,-40) [0|0] "C" (_d[3] & (0xFFU));
        battery_min_temp =
            (rx_frame.data.u8[3] * 5) - 400;  //Multiply by 5 and remove offset to get C+1 (0x61*5=485-400=8.5*C)
        //BattBrickTempMaxNum m0 : 2|4@1+ (1,0) [0|0] "" ((_d[0] >> 2) & (0x0FU));
        battery_BrickTempMaxNum = ((rx_frame.data.u8[0] >> 2) & (0x0F));  //to datalayer_extended
        //BattBrickTempMinNum m0 : 8|4@1+ (1,0) [0|0] "" (_d[1] & (0x0FU));
        battery_BrickTempMinNum = (rx_frame.data.u8[1] & (0x0F));  //to datalayer_extended
        //BattBrickModelTMax m0 : 32|8@1+ (0.5,-40) [0|0] "C" (_d[4] & (0xFFU));
        battery_BrickModelTMax = (rx_frame.data.u8[4] & (0xFFU));  //to datalayer_extended
        //BattBrickModelTMin m0 : 40|8@1+ (0.5,-40) [0|0] "C" (_d[5] & (0xFFU));
        battery_BrickModelTMin = (rx_frame.data.u8[5] & (0xFFU));  //to datalayer_extended
      }
      break;
    case 0x312:  // 786 BMS_thermalStatus
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_powerDissipation =
          ((rx_frame.data.u8[1] & (0x03U)) << 8) | (rx_frame.data.u8[0] & (0xFFU));  //0|10@1+ (0.02,0) [0|0] "kW"
      BMS_flowRequest = ((rx_frame.data.u8[2] & (0x01U)) << 6) |
                        ((rx_frame.data.u8[1] >> 2) & (0x3FU));  //10|7@1+ (0.3,0) [0|0] "LPM"
      BMS_inletActiveCoolTargetT = ((rx_frame.data.u8[3] & (0x03U)) << 7) |
                                   ((rx_frame.data.u8[2] >> 1) & (0x7FU));  //17|9@1+ (0.25,-25) [0|0] "DegC"
      BMS_inletPassiveTargetT = ((rx_frame.data.u8[4] & (0x07U)) << 6) |
                                ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //26|9@1+ (0.25,-25) [0|0] "DegC"  X
      BMS_inletActiveHeatTargetT = ((rx_frame.data.u8[5] & (0x0FU)) << 5) |
                                   ((rx_frame.data.u8[4] >> 3) & (0x1FU));  //35|9@1+ (0.25,-25) [0|0] "DegC"
      BMS_packTMin = ((rx_frame.data.u8[6] & (0x1FU)) << 4) |
                     ((rx_frame.data.u8[5] >> 4) & (0x0FU));  //44|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS_packTMax = ((rx_frame.data.u8[7] & (0x3FU)) << 3) |
                     ((rx_frame.data.u8[6] >> 5) & (0x07U));          //53|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS_pcsNoFlowRequest = ((rx_frame.data.u8[7] >> 6) & (0x01U));  // 62|1@1+ (1,0) [0|0] ""
      BMS_noFlowRequest = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""
      break;
    case 0x2A4:  //676 PCS_thermalStatus
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      PCS_chgPhATemp = (rx_frame.data.u8[0] & 0xFF) | ((rx_frame.data.u8[1] & 0x07) << 8);  //0|11@1- (0.1,40) [0|0] "C"
      PCS_chgPhBTemp =
          ((rx_frame.data.u8[1] & 0xF8) >> 3) | ((rx_frame.data.u8[2] & 0x3F) << 5);  //11|11@1- (0.1,40) [0|0] "C"
      PCS_chgPhCTemp = ((rx_frame.data.u8[2] & 0xC0) >> 6) | (rx_frame.data.u8[3] << 2) |
                       ((rx_frame.data.u8[4] & 0x01) << 10);  //22|11@1- (0.1,40) [0|0] "C"
      PCS_dcdcTemp =
          ((rx_frame.data.u8[4] & 0xFE) >> 1) | ((rx_frame.data.u8[5] & 0x0F) << 7);       //33|11@1- (0.1,40) [0|0] "C"
      PCS_ambientTemp = ((rx_frame.data.u8[5] & 0xF0) >> 4) | (rx_frame.data.u8[6] << 4);  //44|11@1- (0.1,40) [0|0] "C"
      break;
    case 0x2C4:  // 708 PCS_logging: not all frames are listed, just ones relating to dcdc
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & (0x1FU));
      //PCS_logMessageSelect = (rx_frame.data.u8[0] & (0x1FU));  //0|5@1+ (1,0) [0|0] ""
      if (mux == 6) {
        PCS_dcdcMaxLvOutputCurrent = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                                     ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //m6 : 28|12@1+ (0.1,0) [0|0] "A"  X
        PCS_dcdcCurrentLimit = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  //m6 : 40|12@1+ (0.1,0) [0|0] "A"  X
        PCS_dcdcLvOutputCurrentTempLimit = ((rx_frame.data.u8[7] & (0xFFU)) << 4) |
                                           ((rx_frame.data.u8[6] >> 4) & (0x0FU));  //m6 : 52|12@1+ (0.1,0) [0|0] "A"  X
      }
      if (mux == 7) {
        PCS_dcdcUnifiedCommand = ((rx_frame.data.u8[1] & (0x7FU)) << 3) |
                                 ((rx_frame.data.u8[0] >> 5) & (0x07U));  //m7 : 5|10@1+ (0.001,0) [0|0] "1"  X
        PCS_dcdcCLAControllerOutput = ((rx_frame.data.u8[3] & (0x03U)) << 8) |
                                      (rx_frame.data.u8[2] & (0xFFU));  //m7 : 16|10@1+ (0.001,0) [0|0] "1"  X
        PCS_dcdcTankVoltage = ((rx_frame.data.u8[4] & (0x1FU)) << 6) |
                              ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //m7 : 26|11@1- (1,0) [0|0] "V"  X
        PCS_dcdcTankVoltageTarget = ((rx_frame.data.u8[5] & (0x7FU)) << 3) |
                                    ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m7 : 37|10@1+ (1,0) [0|0] "V"  X
        PCS_dcdcClaCurrentFreq = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                 (rx_frame.data.u8[6] & (0xFFU));  //P m7 : 48|12@1+ (0.0976563,0) [0|0] "kHz"  X
      }
      if (mux == 8) {
        PCS_dcdcTCommMeasured = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[1] & (0xFFU));  // m8 : 8|16@1- (0.00195313,0) [0|0] "us"  X
        PCS_dcdcShortTimeUs = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  // m8 : 24|16@1+ (0.000488281,0) [0|0] "us"  X
        PCS_dcdcHalfPeriodUs = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  // m8 : 40|16@1+ (0.000488281,0) [0|0] "us"  X
      }
      if (mux == 18) {
        PCS_dcdcIntervalMaxFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                       (rx_frame.data.u8[1] & (0xFFU));  // m18 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS_dcdcIntervalMaxHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                       (rx_frame.data.u8[3] & (0xFFU));  //m18 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMaxLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                       ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m18 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMaxLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                          (rx_frame.data.u8[6] & (0xFFU));  //m18 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 19) {
        PCS_dcdcIntervalMinFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                       (rx_frame.data.u8[1] & (0xFFU));  //m19 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS_dcdcIntervalMinHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                       (rx_frame.data.u8[3] & (0xFFU));  //m19 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMinLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                       ((rx_frame.data.u8[4] >> 5) & (0x07U));  //m19 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMinLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                          (rx_frame.data.u8[6] & (0xFFU));  // m19 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 22) {
        PCS_dcdc12vSupportLifetimekWh = ((rx_frame.data.u8[3] & (0xFFU)) << 16) |
                                        ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                        (rx_frame.data.u8[1] & (0xFFU));  // m22 : 8|24@1+ (0.01,0) [0|0] "kWh"  X
      }
      break;
    case 0x401:  // Cell stats  //BrickVoltages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0]);  //MultiplexSelector M : 0|8@1+ (1,0) [0|0] ""
                                    //StatusFlags : 8|8@1+ (1,0) [0|0] ""
                                    //Brick0 m0 : 16|16@1+ (0.0001,0) [0|0] "V"
                                    //Brick1 m0 : 32|16@1+ (0.0001,0) [0|0] "V"
                                    //Brick2 m0 : 48|16@1+ (0.0001,0) [0|0] "V"
      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

      if (rx_frame.data.u8[1] == 0x2A)  // status byte must be 0x2A to read cellvoltages
      {
        // Example, frame3=0x89,frame2=0x1D = 35101 / 10 = 3510mV
        volts = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 10;
        datalayer.battery.status.cell_voltages_mV[mux * 3] = volts;
        volts = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) / 10;
        datalayer.battery.status.cell_voltages_mV[1 + mux * 3] = volts;
        volts = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) / 10;
        datalayer.battery.status.cell_voltages_mV[2 + mux * 3] = volts;

        // Track the max value of mux. If we've seen two 0 values for mux, we've probably gathered all
        // cell voltages. Then, 2 + mux_max * 3 + 1 is the number of cell voltages.
        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = 2 + 3 * mux_max + 1;
            // Increase the counter arbitrarily another time to make the initial if-statement evaluate to false
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x2d2:  //BMSVAlimits:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_min_voltage = ((rx_frame.data.u8[1] << 8) |
                         rx_frame.data.u8[0]);  //0|16@1+ (0.01,0) [0|430] "V"  //Example 24148mv * 0.01 = 241.48 V
      BMS_max_voltage = ((rx_frame.data.u8[3] << 8) |
                         rx_frame.data.u8[2]);  //16|16@1+ (0.01,0) [0|430] "V"  //Example 40282mv * 0.01 = 402.82 V
      battery_max_charge_current = (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) *
                                   0.1;  //32|14@1+ (0.1,0) [0|1638.2] "A"  //Example 1301? * 0.1 = 130.1?
      battery_max_discharge_current = (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) *
                                      0.128;  //48|14@1+ (0.128,0) [0|2096.9] "A"  //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:  //PCS_dcdcRailStatus:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_dcdcLvBusVolt =
          (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);  //0|10@1+ (0.0390625,0) [0|39.9609] "V"
      battery_dcdcHvBusVolt = (((rx_frame.data.u8[2] & 0x3F) << 6) |
                               ((rx_frame.data.u8[1] & 0xFC) >> 2));  //10|12@1+ (0.146484,0) [0|599.854] "V"
      battery_dcdcLvOutputCurrent =
          (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]);  //24|12@1+ (0.1,0) [0|400] "A"
      break;
    case 0x292:  //BMS_socStatus
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_beginning_of_life =
          (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[5]) * 0.1;          //40|10@1+ (0.1,0) [0|102.3] "kWh"
      battery_soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);  //0|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_ui =
          (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));  //10|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_max =
          (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));  //20|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_ave =
          ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));  //30|10@1+ (0.1,0) [0|102.3] "%"
      battery_battTempPct =
          (((rx_frame.data.u8[7] & 0x03) << 6) | (rx_frame.data.u8[6] & 0x3F) >> 2);  //50|8@1+ (0.4,0) [0|100] "%"
      break;
    case 0x392:  //BMS_packConfig
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & (0xFF));
      if (mux == 1) {
        battery_packConfigMultiplexer = (rx_frame.data.u8[0] & (0xff));  //0|8@1+ (1,0) [0|1] ""
        battery_moduleType =
            (rx_frame.data.u8[1] &
             (0x07));  //8|3@1+ (1,0) [0|4] ""//0 "UNKNOWN" 1 "E3_NCT" 2 "E1_NCT" 3 "E3_CT" 4 "E1_CT" 5 "E1_CP" ;//to datalayer_extended
        battery_packMass = (rx_frame.data.u8[2]) + 300;  //16|8@1+ (1,300) [342|469] "kg"
        battery_platformMaxBusVoltage =
            (((rx_frame.data.u8[4] & 0x03) << 8) | (rx_frame.data.u8[3]));  //24|10@1+ (0.1,375) [0|0] "V"
      }
      if (mux == 0) {
        battery_reservedConfig =
            (rx_frame.data.u8[1] &
             (0x1F));  //8|5@1+ (1,0) [0|31] ""//0 "BMS_CONFIG_0" 1 "BMS_CONFIG_1" 10 "BMS_CONFIG_10" 11 "BMS_CONFIG_11" 12 "BMS_CONFIG_12" 13 "BMS_CONFIG_13" 14 "BMS_CONFIG_14" 15 "BMS_CONFIG_15" 16 "BMS_CONFIG_16" 17 "BMS_CONFIG_17" 18 "BMS_CONFIG_18" 19 "BMS_CONFIG_19" 2 "BMS_CONFIG_2" 20 "BMS_CONFIG_20" 21 "BMS_CONFIG_21" 22 "BMS_CONFIG_22" 23 "BMS_CONFIG_23" 24 "BMS_CONFIG_24" 25 "BMS_CONFIG_25" 26 "BMS_CONFIG_26" 27 "BMS_CONFIG_27" 28 "BMS_CONFIG_28" 29 "BMS_CONFIG_29" 3 "BMS_CONFIG_3" 30 "BMS_CONFIG_30" 31 "BMS_CONFIG_31" 4 "BMS_CONFIG_4" 5 "BMS_CONFIG_5" 6 "BMS_CONFIG_6" 7 "BMS_CONFIG_7" 8 "BMS_CONFIG_8" 9 "BMS_CONFIG_9" ;
      }
      break;
    case 0x7AA:  //1962 HVP_debugMessage:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & (0x0FU));
      //HVP_debugMessageMultiplexer = (rx_frame.data.u8[0] & (0x0FU));  //0|4@1+ (1,0) [0|6] ""
      if (mux == 0) {
        HVP_gpioPassivePyroDepl = ((rx_frame.data.u8[0] >> 4) & (0x01U));       //: 4|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPyroIsoEn = ((rx_frame.data.u8[0] >> 5) & (0x01U));             //: 5|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpFaultIn = ((rx_frame.data.u8[0] >> 6) & (0x01U));             //: 6|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPackContPowerEn = ((rx_frame.data.u8[0] >> 7) & (0x01U));       //: 7|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvCablesOk = (rx_frame.data.u8[1] & (0x01U));                   //: 8|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvpSelfEnable = ((rx_frame.data.u8[1] >> 1) & (0x01U));         //: 9|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioLed = ((rx_frame.data.u8[1] >> 2) & (0x01U));                   //: 10|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCrashSignal = ((rx_frame.data.u8[1] >> 3) & (0x01U));           //: 11|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioShuntDataReady = ((rx_frame.data.u8[1] >> 4) & (0x01U));        //: 12|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContPosAux = ((rx_frame.data.u8[1] >> 5) & (0x01U));          //: 13|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContNegAux = ((rx_frame.data.u8[1] >> 6) & (0x01U));          //: 14|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioBmsEout = ((rx_frame.data.u8[1] >> 7) & (0x01U));               //: 15|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpFaultOut = (rx_frame.data.u8[2] & (0x01U));                   //: 16|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPyroPor = ((rx_frame.data.u8[2] >> 1) & (0x01U));               //: 17|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioShuntEn = ((rx_frame.data.u8[2] >> 2) & (0x01U));               //: 18|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvpVerEn = ((rx_frame.data.u8[2] >> 3) & (0x01U));              //: 19|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPackCoontPosFlywheel = ((rx_frame.data.u8[2] >> 4) & (0x01U));  //: 20|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpLatchEnable = ((rx_frame.data.u8[2] >> 5) & (0x01U));         //: 21|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsEnable = ((rx_frame.data.u8[2] >> 6) & (0x01U));             //: 22|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsDcdcPwmEnable = ((rx_frame.data.u8[2] >> 7) & (0x01U));      //: 23|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsChargePwmEnable = (rx_frame.data.u8[3] & (0x01U));           //: 24|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContPowerEnable = ((rx_frame.data.u8[3] >> 1) & (0x01U));     //: 25|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvilEnable = ((rx_frame.data.u8[3] >> 2) & (0x01U));            //: 26|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioSecDrdy = ((rx_frame.data.u8[3] >> 3) & (0x01U));               //: 27|1@1+ (1,0) [0|1] ""  Receiver
        HVP_hvp1v5Ref = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                        ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|3] "V"  Receiver
        HVP_shuntCurrentDebug = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[5] & (0xFFU));     //: 40|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP_packCurrentMia = (rx_frame.data.u8[7] & (0x01U));        //: 56|1@1+ (1,0) [0|1] ""  Receiver
        HVP_auxCurrentMia = ((rx_frame.data.u8[7] >> 1) & (0x01U));  //: 57|1@1+ (1,0) [0|1] ""  Receiver
        HVP_currentSenseMia = ((rx_frame.data.u8[7] >> 2) & (0x03U));          //: 58|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntRefVoltageMismatch = ((rx_frame.data.u8[7] >> 3) & (0x01U));  //: 59|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntThermistorMia = ((rx_frame.data.u8[7] >> 4) & (0x01U));       //: 60|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntHwMia = ((rx_frame.data.u8[7] >> 5) & (0x01U));               //: 61|1@1+ (1,0) [0|1] ""  Receiver
      }
      if (mux == 1) {
        HVP_dcLinkVoltage = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP_packVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                          (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP_fcLinkVoltage = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
      }
      if (mux == 2) {
        HVP_packContVoltage = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                              ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_packNegativeV = ((rx_frame.data.u8[3] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[2] & (0xFFU));  //: 16|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_packPositiveV = ((rx_frame.data.u8[5] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[4] & (0xFFU));  //: 32|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_pyroAnalog = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                         (rx_frame.data.u8[6] & (0xFFU));  //: 48|12@1+ (0.1,0) [0|3] "V"  Receiver
      }
      if (mux == 3) {
        HVP_dcLinkNegativeV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_dcLinkPositiveV = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_fcLinkNegativeV = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-550|550] "V"  Receiver
      }
      if (mux == 4) {
        HVP_fcContCoilCurrent = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                                ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP_fcContVoltage = ((rx_frame.data.u8[3] & (0x0FU)) << 8) |
                            (rx_frame.data.u8[2] & (0xFFU));  //: 16|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_hvilInVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                            ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_hvilOutVoltage = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                             (rx_frame.data.u8[5] & (0xFFU));  //: 40|12@1+ (0.1,0) [0|30] "V"  Receiver
      }
      if (mux == 5) {
        HVP_fcLinkPositiveV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_packContCoilCurrent = ((rx_frame.data.u8[4] & (0x0FU)) << 8) |
                                  (rx_frame.data.u8[3] & (0xFFU));  //: 24|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP_battery12V = ((rx_frame.data.u8[5] & (0xFFU)) << 4) |
                         ((rx_frame.data.u8[4] >> 4) & (0x0FU));  //: 36|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_shuntRefVoltageDbg = ((rx_frame.data.u8[7] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[6] & (0xFFU));  //: 48|16@1- (0.001,0) [-32.768|32.767] "V"  Receiver
      }
      if (mux == 6) {
        HVP_shuntAuxCurrentDbg = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP_shuntBarTempDbg = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP_shuntAsicTempDbg = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP_shuntAuxCurrentStatus = (rx_frame.data.u8[7] & (0x03U));       //: 56|2@1+ (1,0) [0|3] ""  Receiver
        HVP_shuntBarTempStatus = ((rx_frame.data.u8[7] >> 2) & (0x03U));   //: 58|2@1+ (1,0) [0|3] ""  Receiver
        HVP_shuntAsicTempStatus = ((rx_frame.data.u8[7] >> 4) & (0x03U));  //: 60|2@1+ (1,0) [0|3] ""  Receiver
      }
      break;
    /* We ignore 0x3AA for now, as on later software/firmware this is a muxed frame so values aren't correct
    case 0x3aa:  //HVP_alertMatrix1
      battery_WatchdogReset = (rx_frame.data.u8[0] & 0x01);
      battery_PowerLossReset = ((rx_frame.data.u8[0] & 0x02) >> 1);
      battery_SwAssertion = ((rx_frame.data.u8[0] & 0x04) >> 2);
      battery_CrashEvent = ((rx_frame.data.u8[0] & 0x08) >> 3);
      battery_OverDchgCurrentFault = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery_OverChargeCurrentFault = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery_OverCurrentFault = ((rx_frame.data.u8[0] & 0x40) >> 6);
      battery_OverTemperatureFault = ((rx_frame.data.u8[0] & 0x80) >> 7);
      battery_OverVoltageFault = (rx_frame.data.u8[1] & 0x01);
      battery_UnderVoltageFault = ((rx_frame.data.u8[1] & 0x02) >> 1);
      battery_PrimaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x04) >> 2);
      battery_SecondaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x08) >> 3);
      battery_BmbMismatchFault = ((rx_frame.data.u8[1] & 0x10) >> 4);
      battery_BmsHviMiaFault = ((rx_frame.data.u8[1] & 0x20) >> 5);
      battery_CpMiaFault = ((rx_frame.data.u8[1] & 0x40) >> 6);
      battery_PcsMiaFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery_BmsFault = (rx_frame.data.u8[2] & 0x01);
      battery_PcsFault = ((rx_frame.data.u8[2] & 0x02) >> 1);
      battery_CpFault = ((rx_frame.data.u8[2] & 0x04) >> 2);
      battery_ShuntHwMiaFault = ((rx_frame.data.u8[2] & 0x08) >> 3);
      battery_PyroMiaFault = ((rx_frame.data.u8[2] & 0x10) >> 4);
      battery_hvsMiaFault = ((rx_frame.data.u8[2] & 0x20) >> 5);
      battery_hviMiaFault = ((rx_frame.data.u8[2] & 0x40) >> 6);
      battery_Supply12vFault = ((rx_frame.data.u8[2] & 0x80) >> 7);
      battery_VerSupplyFault = (rx_frame.data.u8[3] & 0x01);
      battery_HvilFault = ((rx_frame.data.u8[3] & 0x02) >> 1);
      battery_BmsHvsMiaFault = ((rx_frame.data.u8[3] & 0x04) >> 2);
      battery_PackVoltMismatchFault = ((rx_frame.data.u8[3] & 0x08) >> 3);
      battery_EnsMiaFault = ((rx_frame.data.u8[3] & 0x10) >> 4);
      battery_PackPosCtrArcFault = ((rx_frame.data.u8[3] & 0x20) >> 5);
      battery_packNegCtrArcFault = ((rx_frame.data.u8[3] & 0x40) >> 6);
      battery_ShuntHwAndBmsMiaFault = ((rx_frame.data.u8[3] & 0x80) >> 7);
      battery_fcContHwFault = (rx_frame.data.u8[4] & 0x01);
      battery_robinOverVoltageFault = ((rx_frame.data.u8[4] & 0x02) >> 1);
      battery_packContHwFault = ((rx_frame.data.u8[4] & 0x04) >> 2);
      battery_pyroFuseBlown = ((rx_frame.data.u8[4] & 0x08) >> 3);
      battery_pyroFuseFailedToBlow = ((rx_frame.data.u8[4] & 0x10) >> 4);
      battery_CpilFault = ((rx_frame.data.u8[4] & 0x20) >> 5);
      battery_PackContactorFellOpen = ((rx_frame.data.u8[4] & 0x40) >> 6);
      battery_FcContactorFellOpen = ((rx_frame.data.u8[4] & 0x80) >> 7);
      battery_packCtrCloseBlocked = (rx_frame.data.u8[5] & 0x01);
      battery_fcCtrCloseBlocked = ((rx_frame.data.u8[5] & 0x02) >> 1);
      battery_packContactorForceOpen = ((rx_frame.data.u8[5] & 0x04) >> 2);
      battery_fcContactorForceOpen = ((rx_frame.data.u8[5] & 0x08) >> 3);
      battery_dcLinkOverVoltage = ((rx_frame.data.u8[5] & 0x10) >> 4);
      battery_shuntOverTemperature = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery_passivePyroDeploy = ((rx_frame.data.u8[5] & 0x40) >> 6);
      battery_logUploadRequest = ((rx_frame.data.u8[5] & 0x80) >> 7);
      battery_packCtrCloseFailed = (rx_frame.data.u8[6] & 0x01);
      battery_fcCtrCloseFailed = ((rx_frame.data.u8[6] & 0x02) >> 1);
      battery_shuntThermistorMia = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;*/
    case 0x320:  //800 BMS_alertMatrix                                                //BMS_alertMatrix 800 BMS_alertMatrix: 8 VEH
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & (0x0F));
      if (mux == 0) {                                                               //mux0
        BMS_matrixIndex = (rx_frame.data.u8[0] & (0x0F));                           // 0|4@1+ (1,0) [0|0] ""  X
        BMS_a017_SW_Brick_OV = ((rx_frame.data.u8[2] >> 4) & (0x01));               //20|1@1+ (1,0) [0|0] ""  X
        BMS_a018_SW_Brick_UV = ((rx_frame.data.u8[2] >> 5) & (0x01));               //21|1@1+ (1,0) [0|0] ""  X
        BMS_a019_SW_Module_OT = ((rx_frame.data.u8[2] >> 6) & (0x01));              //22|1@1+ (1,0) [0|0] ""  X
        BMS_a021_SW_Dr_Limits_Regulation = (rx_frame.data.u8[3] & (0x01U));         //24|1@1+ (1,0) [0|0] ""  X
        BMS_a022_SW_Over_Current = ((rx_frame.data.u8[3] >> 1) & (0x01U));          //25|1@1+ (1,0) [0|0] ""  X
        BMS_a023_SW_Stack_OV = ((rx_frame.data.u8[3] >> 2) & (0x01U));              //26|1@1+ (1,0) [0|0] ""  X
        BMS_a024_SW_Islanded_Brick = ((rx_frame.data.u8[3] >> 3) & (0x01U));        //27|1@1+ (1,0) [0|0] ""  X
        BMS_a025_SW_PwrBalance_Anomaly = ((rx_frame.data.u8[3] >> 4) & (0x01U));    //28|1@1+ (1,0) [0|0] ""  X
        BMS_a026_SW_HFCurrent_Anomaly = ((rx_frame.data.u8[3] >> 5) & (0x01U));     //29|1@1+ (1,0) [0|0] ""  X
        BMS_a034_SW_Passive_Isolation = ((rx_frame.data.u8[4] >> 5) & (0x01U));     //37|1@1+ (1,0) [0|0] ""  X ?
        BMS_a035_SW_Isolation = ((rx_frame.data.u8[4] >> 6) & (0x01U));             //38|1@1+ (1,0) [0|0] ""  X
        BMS_a036_SW_HvpHvilFault = ((rx_frame.data.u8[4] >> 6) & (0x01U));          //39|1@1+ (1,0) [0|0] ""  X
        BMS_a037_SW_Flood_Port_Open = (rx_frame.data.u8[5] & (0x01U));              //40|1@1+ (1,0) [0|0] ""  X
        BMS_a039_SW_DC_Link_Over_Voltage = ((rx_frame.data.u8[5] >> 2) & (0x01U));  //42|1@1+ (1,0) [0|0] ""  X
        BMS_a041_SW_Power_On_Reset = ((rx_frame.data.u8[5] >> 4) & (0x01U));        //44|1@1+ (1,0) [0|0] ""  X
        BMS_a042_SW_MPU_Error = ((rx_frame.data.u8[5] >> 5) & (0x01U));             //45|1@1+ (1,0) [0|0] ""  X
        BMS_a043_SW_Watch_Dog_Reset = ((rx_frame.data.u8[5] >> 6) & (0x01U));       //46|1@1+ (1,0) [0|0] ""  X
        BMS_a044_SW_Assertion = ((rx_frame.data.u8[5] >> 7) & (0x01U));             //47|1@1+ (1,0) [0|0] ""  X
        BMS_a045_SW_Exception = (rx_frame.data.u8[6] & (0x01U));                    //48|1@1+ (1,0) [0|0] ""  X
        BMS_a046_SW_Task_Stack_Usage = ((rx_frame.data.u8[6] >> 1) & (0x01U));      //49|1@1+ (1,0) [0|0] ""  X
        BMS_a047_SW_Task_Stack_Overflow = ((rx_frame.data.u8[6] >> 2) & (0x01U));   //50|1@1+ (1,0) [0|0] ""  X
        BMS_a048_SW_Log_Upload_Request = ((rx_frame.data.u8[6] >> 3) & (0x01U));    //51|1@1+ (1,0) [0|0] ""  X
        BMS_a050_SW_Brick_Voltage_MIA = ((rx_frame.data.u8[6] >> 5) & (0x01U));     //53|1@1+ (1,0) [0|0] ""  X
        BMS_a051_SW_HVC_Vref_Bad = ((rx_frame.data.u8[6] >> 6) & (0x01U));          //54|1@1+ (1,0) [0|0] ""  X
        BMS_a052_SW_PCS_MIA = ((rx_frame.data.u8[6] >> 7) & (0x01U));               //55|1@1+ (1,0) [0|0] ""  X
        BMS_a053_SW_ThermalModel_Sanity = (rx_frame.data.u8[7] & (0x01U));          //56|1@1+ (1,0) [0|0] ""  X
        BMS_a054_SW_Ver_Supply_Fault = ((rx_frame.data.u8[7] >> 1) & (0x01U));      //57|1@1+ (1,0) [0|0] ""  X
        BMS_a059_SW_Pack_Voltage_Sensing = ((rx_frame.data.u8[7] >> 6) & (0x01U));  //62|1@1+ (1,0) [0|0] ""  X
        BMS_a060_SW_Leakage_Test_Failure = ((rx_frame.data.u8[7] >> 7) & (0x01U));  //63|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 1) {                                                               //mux1
        BMS_a061_robinBrickOverVoltage = ((rx_frame.data.u8[0] >> 4) & (0x01U));    //4|1@1+ (1,0) [0|0] ""  X
        BMS_a062_SW_BrickV_Imbalance = ((rx_frame.data.u8[0] >> 5) & (0x01U));      //5|1@1+ (1,0) [0|0] ""  X
        BMS_a063_SW_ChargePort_Fault = ((rx_frame.data.u8[0] >> 6) & (0x01U));      //6|1@1+ (1,0) [0|0] ""  X
        BMS_a064_SW_SOC_Imbalance = ((rx_frame.data.u8[0] >> 7) & (0x01U));         //7|1@1+ (1,0) [0|0] ""  X
        BMS_a069_SW_Low_Power = ((rx_frame.data.u8[1] >> 4) & (0x01U));             //12|1@1+ (1,0) [0|0] ""  X
        BMS_a071_SW_SM_TransCon_Not_Met = ((rx_frame.data.u8[1] >> 6) & (0x01U));   //14|1@1+ (1,0) [0|0] ""  X
        BMS_a075_SW_Chg_Disable_Failure = ((rx_frame.data.u8[2] >> 2) & (0x01U));   //18|1@1+ (1,0) [0|0] ""  X
        BMS_a076_SW_Dch_While_Charging = ((rx_frame.data.u8[2] >> 3) & (0x01U));    //19|1@1+ (1,0) [0|0] ""  X
        BMS_a077_SW_Charger_Regulation = ((rx_frame.data.u8[2] >> 4) & (0x01U));    //20|1@1+ (1,0) [0|0] ""  X
        BMS_a081_SW_Ctr_Close_Blocked = (rx_frame.data.u8[3] & (0x01U));            //24|1@1+ (1,0) [0|0] ""  X
        BMS_a082_SW_Ctr_Force_Open = ((rx_frame.data.u8[3] >> 1) & (0x01U));        //25|1@1+ (1,0) [0|0] ""  X
        BMS_a083_SW_Ctr_Close_Failure = ((rx_frame.data.u8[3] >> 2) & (0x01U));     //26|1@1+ (1,0) [0|0] ""  X
        BMS_a084_SW_Sleep_Wake_Aborted = ((rx_frame.data.u8[3] >> 3) & (0x01U));    //27|1@1+ (1,0) [0|0] ""  X
        BMS_a087_SW_Feim_Test_Blocked = ((rx_frame.data.u8[3] >> 6) & (0x01U));     //30|1@1+ (1,0) [0|0] ""  X
        BMS_a088_SW_VcFront_MIA_InDrive = ((rx_frame.data.u8[3] >> 7) & (0x01U));   //31|1@1+ (1,0) [0|0] ""  X
        BMS_a089_SW_VcFront_MIA = (rx_frame.data.u8[4] & (0x01U));                  //32|1@1+ (1,0) [0|0] ""  X
        BMS_a090_SW_Gateway_MIA = ((rx_frame.data.u8[4] >> 1) & (0x01U));           //33|1@1+ (1,0) [0|0] ""  X
        BMS_a091_SW_ChargePort_MIA = ((rx_frame.data.u8[4] >> 2) & (0x01U));        //34|1@1+ (1,0) [0|0] ""  X
        BMS_a092_SW_ChargePort_Mia_On_Hv = ((rx_frame.data.u8[4] >> 3) & (0x01U));  //35|1@1+ (1,0) [0|0] ""  X
        BMS_a094_SW_Drive_Inverter_MIA = ((rx_frame.data.u8[4] >> 5) & (0x01U));    //37|1@1+ (1,0) [0|0] ""  X
        BMS_a099_SW_BMB_Communication = ((rx_frame.data.u8[5] >> 2) & (0x01U));     //42|1@1+ (1,0) [0|0] ""  X
        BMS_a105_SW_One_Module_Tsense = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        BMS_a106_SW_All_Module_Tsense = ((rx_frame.data.u8[6] >> 1) & (0x01U));     //49|1@1+ (1,0) [0|0] ""  X
        BMS_a107_SW_Stack_Voltage_MIA = ((rx_frame.data.u8[6] >> 2) & (0x01U));     //50|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 2) {                                                               //mux2
        BMS_a121_SW_NVRAM_Config_Error = ((rx_frame.data.u8[0] >> 4) & (0x01U));    // 4|1@1+ (1,0) [0|0] ""  X
        BMS_a122_SW_BMS_Therm_Irrational = ((rx_frame.data.u8[0] >> 5) & (0x01U));  //5|1@1+ (1,0) [0|0] ""  X
        BMS_a123_SW_Internal_Isolation = ((rx_frame.data.u8[0] >> 6) & (0x01U));    //6|1@1+ (1,0) [0|0] ""  X
        BMS_a127_SW_shunt_SNA = ((rx_frame.data.u8[1] >> 2) & (0x01U));             //10|1@1+ (1,0) [0|0] ""  X
        BMS_a128_SW_shunt_MIA = ((rx_frame.data.u8[1] >> 3) & (0x01U));             //11|1@1+ (1,0) [0|0] ""  X
        BMS_a129_SW_VSH_Failure = ((rx_frame.data.u8[1] >> 4) & (0x01U));           //12|1@1+ (1,0) [0|0] ""  X
        BMS_a130_IO_CAN_Error = ((rx_frame.data.u8[1] >> 5) & (0x01U));             //13|1@1+ (1,0) [0|0] ""  X
        BMS_a131_Bleed_FET_Failure = ((rx_frame.data.u8[1] >> 6) & (0x01U));        //14|1@1+ (1,0) [0|0] ""  X
        BMS_a132_HW_BMB_OTP_Uncorrctbl = ((rx_frame.data.u8[1] >> 7) & (0x01U));    //15|1@1+ (1,0) [0|0] ""  X
        BMS_a134_SW_Delayed_Ctr_Off = ((rx_frame.data.u8[2] >> 1) & (0x01U));       //17|1@1+ (1,0) [0|0] ""  X
        BMS_a136_SW_Module_OT_Warning = ((rx_frame.data.u8[2] >> 3) & (0x01U));     //19|1@1+ (1,0) [0|0] ""  X
        BMS_a137_SW_Brick_UV_Warning = ((rx_frame.data.u8[2] >> 4) & (0x01U));      //20|1@1+ (1,0) [0|0] ""  X
        BMS_a138_SW_Brick_OV_Warning = ((rx_frame.data.u8[2] >> 5) & (0x01U));      //21|1@1+ (1,0) [0|0] ""  X
        BMS_a139_SW_DC_Link_V_Irrational = ((rx_frame.data.u8[2] >> 6) & (0x01U));  //22|1@1+ (1,0) [0|0] ""  X
        BMS_a141_SW_BMB_Status_Warning = (rx_frame.data.u8[3] & (0x01U));           //24|1@1+ (1,0) [0|0] ""  X
        BMS_a144_Hvp_Config_Mismatch = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|0] ""  X
        BMS_a145_SW_SOC_Change = ((rx_frame.data.u8[3] >> 4) & (0x01U));            //28|1@1+ (1,0) [0|0] ""  X
        BMS_a146_SW_Brick_Overdischarged = ((rx_frame.data.u8[3] >> 5) & (0x01U));  //29|1@1+ (1,0) [0|0] ""  X
        BMS_a149_SW_Missing_Config_Block = (rx_frame.data.u8[4] & (0x01U));         //32|1@1+ (1,0) [0|0] ""  X
        BMS_a151_SW_external_isolation = ((rx_frame.data.u8[4] >> 2) & (0x01U));    //34|1@1+ (1,0) [0|0] ""  X
        BMS_a156_SW_BMB_Vref_bad = ((rx_frame.data.u8[4] >> 7) & (0x01U));          //39|1@1+ (1,0) [0|0] ""  X
        BMS_a157_SW_HVP_HVS_Comms = (rx_frame.data.u8[5] & (0x01U));                //40|1@1+ (1,0) [0|0] ""  X
        BMS_a158_SW_HVP_HVI_Comms = ((rx_frame.data.u8[5] >> 1) & (0x01U));         //41|1@1+ (1,0) [0|0] ""  X
        BMS_a159_SW_HVP_ECU_Error = ((rx_frame.data.u8[5] >> 2) & (0x01U));         //42|1@1+ (1,0) [0|0] ""  X
        BMS_a161_SW_DI_Open_Request = ((rx_frame.data.u8[5] >> 4) & (0x01U));       //44|1@1+ (1,0) [0|0] ""  X
        BMS_a162_SW_No_Power_For_Support = ((rx_frame.data.u8[5] >> 5) & (0x01U));  //45|1@1+ (1,0) [0|0] ""  X
        BMS_a163_SW_Contactor_Mismatch = ((rx_frame.data.u8[5] >> 6) & (0x01U));    //46|1@1+ (1,0) [0|0] ""  X
        BMS_a164_SW_Uncontrolled_Regen = ((rx_frame.data.u8[5] >> 7) & (0x01U));    //47|1@1+ (1,0) [0|0] ""  X
        BMS_a165_SW_Pack_Partial_Weld = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        BMS_a166_SW_Pack_Full_Weld = ((rx_frame.data.u8[6] >> 1) & (0x01U));        //49|1@1+ (1,0) [0|0] ""  X
        BMS_a167_SW_FC_Partial_Weld = ((rx_frame.data.u8[6] >> 2) & (0x01U));       //50|1@1+ (1,0) [0|0] ""  X
        BMS_a168_SW_FC_Full_Weld = ((rx_frame.data.u8[6] >> 3) & (0x01U));          //51|1@1+ (1,0) [0|0] ""  X
        BMS_a169_SW_FC_Pack_Weld = ((rx_frame.data.u8[6] >> 4) & (0x01U));          //52|1@1+ (1,0) [0|0] ""  X
        BMS_a170_SW_Limp_Mode = ((rx_frame.data.u8[6] >> 5) & (0x01U));             //53|1@1+ (1,0) [0|0] ""  X
        BMS_a171_SW_Stack_Voltage_Sense = ((rx_frame.data.u8[6] >> 6) & (0x01U));   //54|1@1+ (1,0) [0|0] ""  X
        BMS_a174_SW_Charge_Failure = ((rx_frame.data.u8[7] >> 1) & (0x01U));        //57|1@1+ (1,0) [0|0] ""  X
        BMS_a176_SW_GracefulPowerOff = ((rx_frame.data.u8[7] >> 3) & (0x01U));      //59|1@1+ (1,0) [0|0] ""  X
        BMS_a179_SW_Hvp_12V_Fault = ((rx_frame.data.u8[7] >> 6) & (0x01U));         //62|1@1+ (1,0) [0|0] ""  X
        BMS_a180_SW_ECU_reset_blocked = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""  X
      }
      break;
    case 0x72A:  //BMS_serialNumber
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Pack serial number in ASCII: 00 54 47 33 32 31 32 30 (mux 0) .TG32120 + 01 32 30 30 33 41 48 58 (mux 1) .2003AHX = TG321202003AHX
      if (rx_frame.data.u8[0] == 0x00 && !parsed_battery_serialNumber) {  // Serial number 1-7
        battery_serialNumber[0] = rx_frame.data.u8[1];
        battery_serialNumber[1] = rx_frame.data.u8[2];
        battery_serialNumber[2] = rx_frame.data.u8[3];
        battery_serialNumber[3] = rx_frame.data.u8[4];
        battery_serialNumber[4] = rx_frame.data.u8[5];
        battery_serialNumber[5] = rx_frame.data.u8[6];
        battery_serialNumber[6] = rx_frame.data.u8[7];
      }
      if (rx_frame.data.u8[0] == 0x01 && !parsed_battery_serialNumber) {  // Serial number 8-14
        battery_serialNumber[7] = rx_frame.data.u8[1];
        battery_serialNumber[8] = rx_frame.data.u8[2];
        battery_serialNumber[9] = rx_frame.data.u8[3];
        battery_serialNumber[10] = rx_frame.data.u8[4];
        battery_serialNumber[11] = rx_frame.data.u8[5];
        battery_serialNumber[12] = rx_frame.data.u8[6];
        battery_serialNumber[13] = rx_frame.data.u8[7];
      }
      if (battery_serialNumber[6] != 0 && battery_serialNumber[12] != 0 &&
          !parsed_battery_serialNumber) {  // Serial number complete
        //Manufacture year
        char yearStr[5];  // Full year string (including the "20" prefix)
        snprintf(yearStr, sizeof(yearStr), "20%c%c", battery_serialNumber[3], battery_serialNumber[4]);
        int year = atoi(yearStr);
        //Manufacture day (Julian calendar)
        char dayStr[4];
        snprintf(dayStr, sizeof(dayStr), "%c%c%c", battery_serialNumber[5], battery_serialNumber[6],
                 battery_serialNumber[7]);
        int day = atoi(dayStr);
        battery_manufactureDate = dayOfYearToDate(year, day);
        parsed_battery_serialNumber = true;
      }
      break;
    case 0x300:  //BMS_info
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Display internal BMS info and other build/version data
      if (rx_frame.data.u8[0] == 0x0A) {  // Mux 10: BUILD_HWID_COMPONENTID
        if (BMS_info_buildConfigId == 0) {
          BMS_info_buildConfigId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 16, 16));
        }
        if (BMS_info_hardwareId == 0) {
          BMS_info_hardwareId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));
        }
        if (BMS_info_componentId == 0) {
          BMS_info_componentId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));
        }
      }
      /*
      if (rx_frame.data.u8[0] == 0x0B) { // Mux 11: PCBAID_ASSYID_USAGEID
        if (BMS_info_pcbaId == 0) {BMS_info_pcbaId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 16, 8));}
        if (BMS_info_assemblyId == 0) {BMS_info_assemblyId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 24, 8));}
        if (BMS_info_usageId == 0) {BMS_info_usageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));}
        if (BMS_info_subUsageId == 0) {BMS_info_subUsageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));}
      }
      if (rx_frame.data.u8[0] == 0x0D) { // Mux 13: APP_CRC
        if (BMS_info_platformType == 0) {BMS_info_platformType = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));}
        if (BMS_info_appCrc == 0) {BMS_info_appCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));}
      }
      if (rx_frame.data.u8[0] == 0x12) { // Mux 18: BOOTLOADER_GITHASH
        if (BMS_info_bootGitHash == 0) {BMS_info_bootGitHash = static_cast<uint64_t>(extract_signal_value(rx_frame.data.u8, 10, 54));}
      }
      if (rx_frame.data.u8[0] == 0x14) { // Mux 20: UDS_PROTOCOL_BOOTCRC
        if (BMS_info_bootUdsProtoVersion == 0) {BMS_info_bootUdsProtoVersion = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));}
        if (BMS_info_bootCrc == 0) {BMS_info_bootCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));}
      }
      */
      break;
    case 0x3C4:  //PCS_info
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Display internal PCS info and other build/version data
      if (rx_frame.data.u8[0] == 0x0A) {  // Mux 10: BUILD_HWID_COMPONENTID
        if (PCS_info_buildConfigId == 0) {
          PCS_info_buildConfigId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 16, 16));
        }
        if (PCS_info_hardwareId == 0) {
          PCS_info_hardwareId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));
        }
        if (PCS_info_componentId == 0) {
          PCS_info_componentId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));
        }
      }
      /*
      if (rx_frame.data.u8[0] == 0x0B) { // Mux 11: PCBAID_ASSYID_USAGEID
        if (PCS_info_pcbaId == 0) {PCS_info_pcbaId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 16, 8));}
        if (PCS_info_assemblyId == 0) {PCS_info_assemblyId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 24, 8));}
        if (PCS_info_usageId == 0) {PCS_info_usageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));}
        if (PCS_info_subUsageId == 0) {PCS_info_subUsageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));}
      }
      if (rx_frame.data.u8[0] == 0x0D) { // Mux 13: APP_CRC
        PCS_info_platformType = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));
        PCS_info_appCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));
      }
      if (rx_frame.data.u8[0] == 0x10) { // Mux 16: SUBCOMPONENT
        PCS_info_cpu2AppCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));
      }
      if (rx_frame.data.u8[0] == 0x12) { // Mux 18: BOOTLOADER_GITHASH
        PCS_info_bootGitHash = static_cast<uint64_t>(extract_signal_value(rx_frame.data.u8, 10, 54));
      }
      if (rx_frame.data.u8[0] == 0x14) { // Mux 20: UDS_PROTOCOL_BOOTCRC
        PCS_info_bootUdsProtoVersion = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));
        PCS_info_bootCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));
      }
      */
      //PCS Part Number in ASCII
      if (rx_frame.data.u8[0] == 0x19 && !parsed_PCS_partNumber) {  // Part number 1-7
        PCS_partNumber[0] = rx_frame.data.u8[1];
        PCS_partNumber[1] = rx_frame.data.u8[2];
        PCS_partNumber[2] = rx_frame.data.u8[3];
        PCS_partNumber[3] = rx_frame.data.u8[4];
        PCS_partNumber[4] = rx_frame.data.u8[5];
        PCS_partNumber[5] = rx_frame.data.u8[6];
        PCS_partNumber[6] = rx_frame.data.u8[7];
      }
      if (rx_frame.data.u8[0] == 0x1A && !parsed_PCS_partNumber) {  // Part number 8-14
        PCS_partNumber[7] = rx_frame.data.u8[1];
        PCS_partNumber[8] = rx_frame.data.u8[2];
        PCS_partNumber[9] = rx_frame.data.u8[3];
        PCS_partNumber[10] = rx_frame.data.u8[4];
        PCS_partNumber[11] = rx_frame.data.u8[5];
        PCS_partNumber[12] = rx_frame.data.u8[6];
      }
      if (PCS_partNumber[6] != 0 && PCS_partNumber[11] != 0) {
        parsed_PCS_partNumber = true;
      }
      break;
    case 0x310:  //HVP_info
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Display internal HVP info and other build/version data
      if (rx_frame.data.u8[0] == 0x0A) {  // Mux 10: BUILD_HWID_COMPONENTID
        if (HVP_info_buildConfigId == 0) {
          HVP_info_buildConfigId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 16, 16));
        }
        if (HVP_info_hardwareId == 0) {
          HVP_info_hardwareId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));
        }
        if (HVP_info_componentId == 0) {
          HVP_info_componentId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));
        }
      }
      /*
      if (rx_frame.data.u8[0] == 0x0B) { // Mux 11: PCBAID_ASSYID_USAGEID
        if (HVP_info_pcbaId == 0) {HVP_info_pcbaId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 16, 8));}
        if (HVP_info_assemblyId == 0) {HVP_info_assemblyId = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 24, 8));}
        if (HVP_info_usageId == 0) {HVP_info_usageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 32, 16));}
        if (HVP_info_subUsageId == 0) {HVP_info_subUsageId = static_cast<uint16_t>(extract_signal_value(rx_frame.data.u8, 48, 16));}
      }
      if (rx_frame.data.u8[0] == 0x0D) { // Mux 13: APP_CRC
        HVP_info_platformType = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));
        HVP_info_appCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));
      }
      if (rx_frame.data.u8[0] == 0x12) { // Mux 18: BOOTLOADER_GITHASH
        HVP_info_bootGitHash = static_cast<uint64_t>(extract_signal_value(rx_frame.data.u8, 10, 54));
      }
      if (rx_frame.data.u8[0] == 0x14) { // Mux 20: UDS_PROTOCOL_BOOTCRC
        HVP_info_bootUdsProtoVersion = static_cast<uint8_t>(extract_signal_value(rx_frame.data.u8, 8, 8));
        HVP_info_bootCrc = static_cast<uint32_t>(extract_signal_value(rx_frame.data.u8, 32, 32));
      }
      */
      break;
    case 0x612:  // CAN UDS responses for BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //BMS Query
      if (stateMachineBMSQuery != 0xFF && stateMachineBMSReset == 0xFF && stateMachineSOCReset == 0xFF) {
        if (memcmp(rx_frame.data.u8, "\x02\x50\x03\xAA\xAA\xAA\xAA\xAA", 8) == 0) {
          //Received initial response, proceed to actual query
          logging.println("CAN UDS: Received BMS query initial handshake reply");
          stateMachineBMSQuery = 1;
          break;
        }
        if (rx_frame.data.u8[0] == 0x10) {
          //Received first data frame
          battery_partNumber[0] = rx_frame.data.u8[5];
          battery_partNumber[1] = rx_frame.data.u8[6];
          battery_partNumber[2] = rx_frame.data.u8[7];
          logging.println("CAN UDS: Received BMS query data frame");
          stateMachineBMSQuery = 2;
          break;
        }
        if (rx_frame.data.u8[0] == 0x21) {
          //Second part of part number after flow control
          battery_partNumber[3] = rx_frame.data.u8[1];
          battery_partNumber[4] = rx_frame.data.u8[2];
          battery_partNumber[5] = rx_frame.data.u8[3];
          battery_partNumber[6] = rx_frame.data.u8[4];
          battery_partNumber[7] = rx_frame.data.u8[5];
          battery_partNumber[8] = rx_frame.data.u8[6];
          battery_partNumber[9] = rx_frame.data.u8[7];
          logging.println("CAN UDS: Received BMS query second data frame");
          break;
        }
        if (rx_frame.data.u8[0] == 0x22) {
          //Final part of part number
          battery_partNumber[10] = rx_frame.data.u8[1];
          battery_partNumber[11] = rx_frame.data.u8[2];
          logging.println("CAN UDS: Received BMS query final data frame");
          break;
        }
        if (memcmp(rx_frame.data.u8, "\x23\x00\x00\x00\xAA\xAA\xAA\xAA", 8) == 0) {
          //Received final frame
          logging.println("CAN UDS: Received BMS query termination frame");
          parsed_battery_partNumber = true;
          stateMachineBMSQuery = 0xFF;
          break;
        }
      }
      //BMS ECU responses
      if (memcmp(rx_frame.data.u8, "\x02\x67\x06\xAA\xAA\xAA\xAA\xAA", 8) == 0) {
        logging.println("CAN UDS: BMS ECU unlocked");
      }
      if (memcmp(rx_frame.data.u8, "\x03\x7F\x11\x78\xAA\xAA\xAA\xAA", 8) == 0) {
        logging.println("CAN UDS: BMS ECU reset request successful but ECU busy, response pending");
      }
      if (memcmp(rx_frame.data.u8, "\x02\x51\x01\xAA\xAA\xAA\xAA\xAA", 8) == 0) {
        logging.println("CAN UDS: BMS ECU reset positive response, 1 second downtime");
        set_event(EVENT_BMS_RESET_REQ_SUCCESS, 0);
        clear_event(EVENT_BMS_RESET_REQ_SUCCESS);
      }
      if (memcmp(rx_frame.data.u8, "\x05\x71\x01\x04\x07\x01\xAA\xAA", 8) == 0) {
        logging.println("CAN UDS: BMS SOC reset accepted, resetting BMS ECU");
        set_event(EVENT_BATTERY_SOC_RESET_SUCCESS, 0);
        clear_event(EVENT_BATTERY_SOC_RESET_SUCCESS);
        stateMachineBMSReset = 6;  // BMS ECU already unlocked etc. so we jump straight to reset
      }
      if (memcmp(rx_frame.data.u8, "\x05\x71\x01\x04\x07\x00\xAA\xAA", 8) == 0) {
        logging.println("CAN UDS: BMS SOC reset failed");
        set_event(EVENT_BATTERY_SOC_RESET_FAIL, 0);
        clear_event(EVENT_BATTERY_SOC_RESET_FAIL);
      }
      break;
    default:
      break;
  }
}

CAN_frame can_msg_1CF[] = {
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x60, 0x69}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x80, 0x89}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xA0, 0xA9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xC0, 0xC9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xE0, 0xE9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x00, 0x09}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x20, 0x29}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x40, 0x49}}};

CAN_frame can_msg_118[] = {
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x61, 0x80, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x62, 0x81, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x63, 0x82, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x64, 0x83, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x65, 0x84, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x66, 0x85, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x67, 0x86, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x68, 0x87, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x69, 0x88, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6A, 0x89, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6B, 0x8A, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6C, 0x8B, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6D, 0x8C, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6E, 0x8D, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6F, 0x8E, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x70, 0x8F, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}}};

void TeslaBattery::transmit_can(unsigned long currentMillis) {

  //Send 10ms messages
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    if (user_selected_tesla_digital_HVIL) {  //Special Digital HVIL mode for S/X 2024+ batteries
      if ((datalayer.system.status.inverter_allows_contactor_closing) &&
          (datalayer.battery.status.bms_status != FAULT)) {
        transmit_can_frame(&can_msg_1CF[index_1CF]);
        index_1CF = (index_1CF + 1) % 8;
        transmit_can_frame(&can_msg_118[index_118]);
        index_118 = (index_118 + 1) % 16;
      }
    } else {  //Normal handling of 118 message (Non digital HVIL version)
      //0x118 DI_systemStatus
      transmit_can_frame(&TESLA_118);
      index_1CF = 0;  //Stop broadcasting Digital HVIL 1CF and 118 to keep contactors open
      index_118 = 0;
    }

    //0x2E1 VCFRONT_status
    switch (muxNumber_TESLA_2E1) {
      case 0:
        transmit_can_frame(&TESLA_2E1_VEHICLE_AND_RAILS);
        break;
      case 1:
        transmit_can_frame(&TESLA_2E1_HOMELINK);
        break;
      case 2:
        transmit_can_frame(&TESLA_2E1_REFRIGERANT_SYSTEM);
        break;
      case 3:
        transmit_can_frame(&TESLA_2E1_LV_BATTERY_DEBUG);
        break;
      case 4:
        transmit_can_frame(&TESLA_2E1_MUX_5);
        break;
      case 5:
        transmit_can_frame(&TESLA_2E1_BODY_CONTROLS);
        break;
      default:
        break;
    }
    muxNumber_TESLA_2E1 = (muxNumber_TESLA_2E1 + 1) % 6;  //Cycle betweeen 0-1-2-3-4-5-0...
    //Generate next frames
    generateFrameCounterChecksum(TESLA_118, 8, 4, 0, 8);
  }

  //Send 50ms messages
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    //0x221 VCFRONT_LVPowerState
    if (vehicleState == CAR_DRIVE) {
      if (alternateMux) {
        generateMuxFrameCounterChecksum(TESLA_221_DRIVE_Mux0, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_DRIVE_Mux0);
      } else {
        generateMuxFrameCounterChecksum(TESLA_221_DRIVE_Mux1, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_DRIVE_Mux1);
      }
    } else if (vehicleState == ACCESSORY) {
      if (alternateMux) {
        generateMuxFrameCounterChecksum(TESLA_221_ACCESSORY_Mux0, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_ACCESSORY_Mux0);
      } else {
        generateMuxFrameCounterChecksum(TESLA_221_ACCESSORY_Mux1, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_ACCESSORY_Mux1);
      }
    } else if (vehicleState == GOING_DOWN) {
      if (alternateMux) {
        generateMuxFrameCounterChecksum(TESLA_221_GOING_DOWN_Mux0, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_GOING_DOWN_Mux0);
      } else {
        generateMuxFrameCounterChecksum(TESLA_221_GOING_DOWN_Mux1, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_GOING_DOWN_Mux1);
      }
    } else if (vehicleState == CAR_OFF) {
      if (alternateMux) {
        generateMuxFrameCounterChecksum(TESLA_221_OFF_Mux0, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_OFF_Mux0);
      } else {
        generateMuxFrameCounterChecksum(TESLA_221_OFF_Mux1, frameCounter_TESLA_221, 52, 4, 56, 8);
        transmit_can_frame(&TESLA_221_OFF_Mux1);
      }
    }

    alternateMux ^= 1;  // Flips between 0 and 1. Used to Flip between sending Mux0 and Mux1 on each pass
    //Generate next new frame
    frameCounter_TESLA_221 = (frameCounter_TESLA_221 + 1) % 16;

    //0x3C2 VCLEFT_switchStatus
    transmit_can_frame(alternateMux == 0 ? &TESLA_3C2_Mux0 : &TESLA_3C2_Mux1);

    //0x39D IBST_status
    transmit_can_frame(&TESLA_39D);

    if (battery_contactor == 4) {  // Contactors closed

      // Frames to be sent only when contactors closed

      //0x3A1 VCFRONT_vehicleStatus, critical otherwise VCFRONT_MIA triggered
      transmit_can_frame(&TESLA_3A1[frameCounter_TESLA_3A1]);
      frameCounter_TESLA_3A1 = (frameCounter_TESLA_3A1 + 1) % 16;
    }

    //Generate next frame
    generateFrameCounterChecksum(TESLA_39D, 8, 4, 0, 8);
  }

  //Send 100ms messages
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //0x102 VCLEFT_doorStatus, static
    transmit_can_frame(&TESLA_102);
    //0x103 VCRIGHT_doorStatus, static
    transmit_can_frame(&TESLA_103);
    //0x229 SCCM_rightStalk
    transmit_can_frame(&TESLA_229);
    //0x241 VCFRONT_coolant, static
    transmit_can_frame(&TESLA_241);
    //0x2D1 VCFRONT_okToUseHighPower, static
    transmit_can_frame(&TESLA_2D1);
    //0x2A8 CMPD_state
    transmit_can_frame(&TESLA_2A8);
    //0x2E8 EPBR_status
    transmit_can_frame(&TESLA_2E8);
    //0x7FF GTW_carConfig
    switch (muxNumber_TESLA_7FF) {
      case 0:
        transmit_can_frame(&TESLA_7FF_Mux1);
        break;
      case 1:
        transmit_can_frame(&TESLA_7FF_Mux2);
        break;
      case 2:
        transmit_can_frame(&TESLA_7FF_Mux3);
        break;
      case 3:
        transmit_can_frame(&TESLA_7FF_Mux4);
        break;
      case 4:
        transmit_can_frame(&TESLA_7FF_Mux5);
        break;
      default:
        break;
    }
    muxNumber_TESLA_7FF = (muxNumber_TESLA_7FF + 1) % 5;  //Cycle betweeen 0-1-2-3-4-0...
    //Generate next frames
    generateTESLA_229(TESLA_229);
    generateFrameCounterChecksum(TESLA_2A8, 52, 4, 56, 8);
    generateFrameCounterChecksum(TESLA_2E8, 52, 4, 56, 8);

    if (stateMachineClearIsolationFault != 0xFF) {
      //This implementation should be rewritten to actually reply to the UDS responses sent by the BMS
      //While this may work, it is not the correct way to implement this clearing logic
      switch (stateMachineClearIsolationFault) {
        case 0:
          TESLA_602.data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineClearIsolationFault = 1;
          break;
        case 1:
          TESLA_602.data = {0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          // BMS should reply 02 50 C0 FF FF FF FF FF
          stateMachineClearIsolationFault = 2;
          break;
        case 2:
          TESLA_602.data = {0x10, 0x12, 0x27, 0x06, 0x35, 0x34, 0x37, 0x36};
          transmit_can_frame(&TESLA_602);
          // BMS should reply 7E FF FF FF FF FF FF
          stateMachineClearIsolationFault = 3;
          break;
        case 3:
          TESLA_602.data = {0x21, 0x31, 0x30, 0x33, 0x32, 0x3D, 0x3C, 0x3F};
          transmit_can_frame(&TESLA_602);
          stateMachineClearIsolationFault = 4;
          break;
        case 4:
          TESLA_602.data = {0x22, 0x3E, 0x39, 0x38, 0x3B, 0x3A, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          //Should generate a CAN UDS log message indicating ECU unlocked
          stateMachineClearIsolationFault = 5;
          break;
        case 5:
          TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x0A, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineClearIsolationFault = 0xFF;
          break;
        default:
          //Something went wrong. Reset all and cancel
          stateMachineClearIsolationFault = 0xFF;
          break;
      }
    }
    if (stateMachineBMSReset != 0xFF) {
      //This implementation should be rewritten to actually reply to the UDS responses sent by the BMS
      //While this may work, it is not the correct way to implement this reset logic
      switch (stateMachineBMSReset) {
        case 0:
          TESLA_602.data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 1;
          break;
        case 1:
          TESLA_602.data = {0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 2;
          break;
        case 2:
          TESLA_602.data = {0x10, 0x12, 0x27, 0x06, 0x35, 0x34, 0x37, 0x36};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 3;
          break;
        case 3:
          TESLA_602.data = {0x21, 0x31, 0x30, 0x33, 0x32, 0x3D, 0x3C, 0x3F};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 4;
          break;
        case 4:
          TESLA_602.data = {0x22, 0x3E, 0x39, 0x38, 0x3B, 0x3A, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          //Should generate a CAN UDS log message indicating ECU unlocked
          stateMachineBMSReset = 5;
          break;
        case 5:
          TESLA_602.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 6;
          break;
        case 6:
          TESLA_602.data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineBMSReset = 7;
          break;
        case 7:
          TESLA_602.data = {0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          //Should generate a CAN UDS log message(s) indicating ECU has reset
          stateMachineBMSReset = 0xFF;
          break;
        default:
          //Something went wrong. Reset all and cancel
          stateMachineBMSReset = 0xFF;
          break;
      }
    }
    if (stateMachineSOCReset != 0xFF) {
      //This implementation should be rewritten to actually reply to the UDS responses sent by the BMS
      //While this may work, it is not the correct way to implement this
      switch (stateMachineSOCReset) {
        case 0:
          TESLA_602.data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineSOCReset = 1;
          break;
        case 1:
          TESLA_602.data = {0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineSOCReset = 2;
          break;
        case 2:
          TESLA_602.data = {0x10, 0x12, 0x27, 0x06, 0x35, 0x34, 0x37, 0x36};
          transmit_can_frame(&TESLA_602);
          stateMachineSOCReset = 3;
          break;
        case 3:
          TESLA_602.data = {0x21, 0x31, 0x30, 0x33, 0x32, 0x3D, 0x3C, 0x3F};
          transmit_can_frame(&TESLA_602);
          stateMachineSOCReset = 4;
          break;
        case 4:
          TESLA_602.data = {0x22, 0x3E, 0x39, 0x38, 0x3B, 0x3A, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          //Should generate a CAN UDS log message indicating ECU unlocked
          stateMachineSOCReset = 5;
          break;
        case 5:
          TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x07, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          stateMachineSOCReset = 0xFF;
          break;
        default:
          //Something went wrong. Reset all and cancel
          stateMachineSOCReset = 0xFF;
          break;
      }
    }
    if (stateMachineBMSQuery != 0xFF) {
      //This implementation should be rewritten to actually reply to the UDS responses sent by the BMS
      //While this may work, it is not the correct way to implement this query logic
      switch (stateMachineBMSQuery) {
        case 0:
          //Initial request
          logging.println("CAN UDS: Sending BMS query initial handshake");
          TESLA_602.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          break;
        case 1:
          //Send query
          logging.println("CAN UDS: Sending BMS query for pack part number");
          TESLA_602.data = {0x03, 0x22, 0xF0, 0x14, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          break;
        case 2:
          //Flow control
          logging.println("CAN UDS: Sending BMS query flow control");
          TESLA_602.data = {0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
          transmit_can_frame(&TESLA_602);
          break;
        case 3:
          break;
        case 4:
          break;
        default:
          //Something went wrong. Reset all and cancel
          stateMachineBMSQuery = 0xFF;
          break;
      }
    }
  }

  //Send 500ms messages
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    transmit_can_frame(&TESLA_213);
    transmit_can_frame(&TESLA_284);
    transmit_can_frame(&TESLA_293);
    transmit_can_frame(&TESLA_313);
    transmit_can_frame(&TESLA_333);
    if (TESLA_334_INITIAL_SENT == false) {
      transmit_can_frame(&TESLA_334_INITIAL);
      TESLA_334_INITIAL_SENT = true;
    } else {
      transmit_can_frame(&TESLA_334);
    }
    transmit_can_frame(&TESLA_3B3);
    transmit_can_frame(&TESLA_55A);

    //Generate next frames
    generateTESLA_213(TESLA_213);
    generateFrameCounterChecksum(TESLA_293, 52, 4, 56, 8);
    generateFrameCounterChecksum(TESLA_313, 52, 4, 56, 8);
    generateFrameCounterChecksum(TESLA_334, 52, 4, 56, 8);
  }

  //Send 1000ms messages
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    transmit_can_frame(&TESLA_082);
    transmit_can_frame(&TESLA_321);

    //Generate next frames
    generateFrameCounterChecksum(TESLA_321, 52, 4, 56, 8);
  }
}

void printDebugIfActive(uint8_t symbol, const char* message) {
  if (symbol == 1) {
    logging.println(message);
  }
}

void TeslaBattery::printFaultCodesIfActive() {
  if (battery_packCtrsClosingBlocked &&
      battery_packContactorSetState != 5) {  // Contactors blocked closing and not already closed
    logging.println("ERROR: Check high voltage connectors and interlock circuit, closing contactors not allowed!");
  }
  if (battery_pyroTestInProgress) {
    logging.println("ERROR: Please wait for pyro test to finish, HV cables successfully seated!");
  }
  if (datalayer.system.status.inverter_allows_contactor_closing == false) {
    logging.println(
        "ERROR: Solar inverter does not allow for contactor closing. Check communication connection to the inverter "
        "or "
        "disable the inverter protocol to proceed in Tesla battery testing mode.");
  }
  // Check each symbol and print debug information if its value is 1
  // 0X3AA: 938 HVP_alertMatrix1
  //printDebugIfActive(battery_WatchdogReset, "ERROR: The processor has experienced a reset due to watchdog reset"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PowerLossReset, "ERROR: The processor has experienced a reset due to power loss");
  printDebugIfActive(battery_SwAssertion, "ERROR: An internal software assertion has failed");
  printDebugIfActive(battery_CrashEvent, "ERROR: crash signal is detected by HVP");
  printDebugIfActive(battery_OverDchgCurrentFault,
                     "ERROR: Pack discharge current is above the safe max discharge current limit!");
  printDebugIfActive(battery_OverChargeCurrentFault,
                     "ERROR: Pack charge current is above the safe max charge current limit!");
  printDebugIfActive(battery_OverCurrentFault, "ERROR: Pack current (discharge or charge) is above max current limit!");
  printDebugIfActive(battery_OverTemperatureFault,
                     "ERROR: A pack module temperature is above the max temperature limit!");
  printDebugIfActive(battery_OverVoltageFault, "ERROR: A brick voltage is above maximum voltage limit");
  printDebugIfActive(battery_UnderVoltageFault, "ERROR: A brick voltage is below minimum voltage limit");
  printDebugIfActive(battery_PrimaryBmbMiaFault,
                     "ERROR: Voltage and temperature readings from primary BMB chain are mia");
  printDebugIfActive(battery_SecondaryBmbMiaFault,
                     "ERROR: Voltage and temperature readings from secondary BMB chain are mia");
  printDebugIfActive(battery_BmbMismatchFault,
                     "ERROR: Primary and secondary BMB chain readings don't match with each other");
  printDebugIfActive(battery_BmsHviMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  //printDebugIfActive(battery_CpMiaFault, "ERROR: CP node is mia on HVS CAN"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsMiaFault, "ERROR: PCS node is mia on HVS CAN");
  //printDebugIfActive(battery_BmsFault, "ERROR: BmsFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsFault, "ERROR: PcsFault is active");
  //printDebugIfActive(battery_CpFault, "ERROR: CpFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_ShuntHwMiaFault, "ERROR: Shunt current reading is not available");
  printDebugIfActive(battery_PyroMiaFault, "ERROR: Pyro squib is not connected");
  printDebugIfActive(battery_hvsMiaFault, "ERROR: Pack contactor hw fault");
  printDebugIfActive(battery_hviMiaFault, "ERROR: FC contactor hw fault");
  printDebugIfActive(battery_Supply12vFault, "ERROR: Low voltage (12V) battery is below minimum voltage threshold");
  printDebugIfActive(battery_VerSupplyFault, "ERROR: Energy reserve voltage supply is below minimum voltage threshold");
  printDebugIfActive(battery_HvilFault, "ERROR: High Voltage Inter Lock fault is detected");
  printDebugIfActive(battery_BmsHvsMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  printDebugIfActive(battery_PackVoltMismatchFault,
                     "ERROR: Pack voltage doesn't match approximately with sum of brick voltages");
  //printDebugIfActive(battery_EnsMiaFault, "ERROR: ENS line is not connected to HVC"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackPosCtrArcFault, "ERROR: HVP detectes series arc at pack contactor");
  printDebugIfActive(battery_packNegCtrArcFault, "ERROR: HVP detectes series arc at FC contactor");
  printDebugIfActive(battery_ShuntHwAndBmsMiaFault, "ERROR: ShuntHwAndBmsMiaFault is active");
  printDebugIfActive(battery_fcContHwFault, "ERROR: fcContHwFault is active");
  printDebugIfActive(battery_robinOverVoltageFault, "ERROR: robinOverVoltageFault is active");
  printDebugIfActive(battery_packContHwFault, "ERROR: packContHwFault is active");
  printDebugIfActive(battery_pyroFuseBlown, "ERROR: pyroFuseBlown is active");
  printDebugIfActive(battery_pyroFuseFailedToBlow, "ERROR: pyroFuseFailedToBlow is active");
  //printDebugIfActive(battery_CpilFault, "ERROR: CpilFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackContactorFellOpen, "ERROR: PackContactorFellOpen is active");
  printDebugIfActive(battery_FcContactorFellOpen, "ERROR: FcContactorFellOpen is active");
  printDebugIfActive(battery_packCtrCloseBlocked, "ERROR: packCtrCloseBlocked is active");
  printDebugIfActive(battery_fcCtrCloseBlocked, "ERROR: fcCtrCloseBlocked is active");
  printDebugIfActive(battery_packContactorForceOpen, "ERROR: packContactorForceOpen is active");
  printDebugIfActive(battery_fcContactorForceOpen, "ERROR: fcContactorForceOpen is active");
  printDebugIfActive(battery_dcLinkOverVoltage, "ERROR: dcLinkOverVoltage is active");
  printDebugIfActive(battery_shuntOverTemperature, "ERROR: shuntOverTemperature is active");
  printDebugIfActive(battery_passivePyroDeploy, "ERROR: passivePyroDeploy is active");
  printDebugIfActive(battery_logUploadRequest, "ERROR: logUploadRequest is active");
  printDebugIfActive(battery_packCtrCloseFailed, "ERROR: packCtrCloseFailed is active");
  printDebugIfActive(battery_fcCtrCloseFailed, "ERROR: fcCtrCloseFailed is active");
  printDebugIfActive(battery_shuntThermistorMia, "ERROR: shuntThermistorMia is active");
  // 0x320 800 BMS_alertMatrix
  printDebugIfActive(BMS_a017_SW_Brick_OV, "ERROR: BMS_a017_SW_Brick_OV");
  printDebugIfActive(BMS_a018_SW_Brick_UV, "ERROR: BMS_a018_SW_Brick_UV");
  printDebugIfActive(BMS_a019_SW_Module_OT, "ERROR: BMS_a019_SW_Module_OT");
  printDebugIfActive(BMS_a021_SW_Dr_Limits_Regulation, "ERROR: BMS_a021_SW_Dr_Limits_Regulation");
  //printDebugIfActive(BMS_a022_SW_Over_Current, "ERROR: BMS_a022_SW_Over_Current");
  printDebugIfActive(BMS_a023_SW_Stack_OV, "ERROR: BMS_a023_SW_Stack_OV");
  printDebugIfActive(BMS_a024_SW_Islanded_Brick, "ERROR: BMS_a024_SW_Islanded_Brick");
  printDebugIfActive(BMS_a025_SW_PwrBalance_Anomaly, "ERROR: BMS_a025_SW_PwrBalance_Anomaly");
  printDebugIfActive(BMS_a026_SW_HFCurrent_Anomaly, "ERROR: BMS_a026_SW_HFCurrent_Anomaly");
  printDebugIfActive(BMS_a034_SW_Passive_Isolation, "ERROR: BMS_a034_SW_Passive_Isolation");
  printDebugIfActive(BMS_a035_SW_Isolation, "ERROR: BMS_a035_SW_Isolation");
  printDebugIfActive(BMS_a036_SW_HvpHvilFault, "ERROR: BMS_a036_SW_HvpHvilFault");
  printDebugIfActive(BMS_a037_SW_Flood_Port_Open, "ERROR: BMS_a037_SW_Flood_Port_Open");
  printDebugIfActive(BMS_a039_SW_DC_Link_Over_Voltage, "ERROR: BMS_a039_SW_DC_Link_Over_Voltage");
  printDebugIfActive(BMS_a041_SW_Power_On_Reset, "ERROR: BMS_a041_SW_Power_On_Reset");
  printDebugIfActive(BMS_a042_SW_MPU_Error, "ERROR: BMS_a042_SW_MPU_Error");
  printDebugIfActive(BMS_a043_SW_Watch_Dog_Reset, "ERROR: BMS_a043_SW_Watch_Dog_Reset");
  printDebugIfActive(BMS_a044_SW_Assertion, "ERROR: BMS_a044_SW_Assertion");
  printDebugIfActive(BMS_a045_SW_Exception, "ERROR: BMS_a045_SW_Exception");
  printDebugIfActive(BMS_a046_SW_Task_Stack_Usage, "ERROR: BMS_a046_SW_Task_Stack_Usage");
  printDebugIfActive(BMS_a047_SW_Task_Stack_Overflow, "ERROR: BMS_a047_SW_Task_Stack_Overflow");
  printDebugIfActive(BMS_a048_SW_Log_Upload_Request, "ERROR: BMS_a048_SW_Log_Upload_Request");
  //printDebugIfActive(BMS_a050_SW_Brick_Voltage_MIA, "ERROR: BMS_a050_SW_Brick_Voltage_MIA");
  printDebugIfActive(BMS_a051_SW_HVC_Vref_Bad, "ERROR: BMS_a051_SW_HVC_Vref_Bad");
  printDebugIfActive(BMS_a052_SW_PCS_MIA, "ERROR: BMS_a052_SW_PCS_MIA");
  printDebugIfActive(BMS_a053_SW_ThermalModel_Sanity, "ERROR: BMS_a053_SW_ThermalModel_Sanity");
  printDebugIfActive(BMS_a054_SW_Ver_Supply_Fault, "ERROR: BMS_a054_SW_Ver_Supply_Fault");
  printDebugIfActive(BMS_a059_SW_Pack_Voltage_Sensing, "ERROR: BMS_a059_SW_Pack_Voltage_Sensing");
  printDebugIfActive(BMS_a060_SW_Leakage_Test_Failure, "ERROR: BMS_a060_SW_Leakage_Test_Failure");
  printDebugIfActive(BMS_a061_robinBrickOverVoltage, "ERROR: BMS_a061_robinBrickOverVoltage");
  printDebugIfActive(BMS_a062_SW_BrickV_Imbalance, "ERROR: BMS_a062_SW_BrickV_Imbalance");
  //printDebugIfActive(BMS_a063_SW_ChargePort_Fault, "ERROR: BMS_a063_SW_ChargePort_Fault");
  printDebugIfActive(BMS_a064_SW_SOC_Imbalance, "ERROR: BMS_a064_SW_SOC_Imbalance");
  printDebugIfActive(BMS_a069_SW_Low_Power, "ERROR: BMS_a069_SW_Low_Power");
  printDebugIfActive(BMS_a071_SW_SM_TransCon_Not_Met, "ERROR: BMS_a071_SW_SM_TransCon_Not_Met");
  printDebugIfActive(BMS_a075_SW_Chg_Disable_Failure, "ERROR: BMS_a075_SW_Chg_Disable_Failure");
  printDebugIfActive(BMS_a076_SW_Dch_While_Charging, "ERROR: BMS_a076_SW_Dch_While_Charging");
  printDebugIfActive(BMS_a077_SW_Charger_Regulation, "ERROR: BMS_a077_SW_Charger_Regulation");
  printDebugIfActive(BMS_a081_SW_Ctr_Close_Blocked, "ERROR: BMS_a081_SW_Ctr_Close_Blocked");
  printDebugIfActive(BMS_a082_SW_Ctr_Force_Open, "ERROR: BMS_a082_SW_Ctr_Force_Open");
  printDebugIfActive(BMS_a083_SW_Ctr_Close_Failure, "ERROR: BMS_a083_SW_Ctr_Close_Failure");
  printDebugIfActive(BMS_a084_SW_Sleep_Wake_Aborted, "ERROR: BMS_a084_SW_Sleep_Wake_Aborted");
  printDebugIfActive(BMS_a087_SW_Feim_Test_Blocked, "ERROR: BMS_a087_SW_Feim_Test_Blocked");
  printDebugIfActive(BMS_a088_SW_VcFront_MIA_InDrive, "ERROR: BMS_a088_SW_VcFront_MIA_InDrive");
  printDebugIfActive(BMS_a089_SW_VcFront_MIA, "ERROR: BMS_a089_SW_VcFront_MIA");
  printDebugIfActive(BMS_a090_SW_Gateway_MIA, "ERROR: BMS_a090_SW_Gateway_MIA");
  //printDebugIfActive(BMS_a091_SW_ChargePort_MIA, "ERROR: BMS_a091_SW_ChargePort_MIA");
  //printDebugIfActive(BMS_a092_SW_ChargePort_Mia_On_Hv, "ERROR: BMS_a092_SW_ChargePort_Mia_On_Hv");
  //printDebugIfActive(BMS_a094_SW_Drive_Inverter_MIA, "ERROR: BMS_a094_SW_Drive_Inverter_MIA");
  printDebugIfActive(BMS_a099_SW_BMB_Communication, "ERROR: BMS_a099_SW_BMB_Communication");
  printDebugIfActive(BMS_a105_SW_One_Module_Tsense, "ERROR: BMS_a105_SW_One_Module_Tsense");
  printDebugIfActive(BMS_a106_SW_All_Module_Tsense, "ERROR: BMS_a106_SW_All_Module_Tsense");
  printDebugIfActive(BMS_a107_SW_Stack_Voltage_MIA, "ERROR: BMS_a107_SW_Stack_Voltage_MIA");
  printDebugIfActive(BMS_a121_SW_NVRAM_Config_Error, "ERROR: BMS_a121_SW_NVRAM_Config_Error");
  printDebugIfActive(BMS_a122_SW_BMS_Therm_Irrational, "ERROR: BMS_a122_SW_BMS_Therm_Irrational");
  printDebugIfActive(BMS_a123_SW_Internal_Isolation, "ERROR: BMS_a123_SW_Internal_Isolation");
  printDebugIfActive(BMS_a127_SW_shunt_SNA, "ERROR: BMS_a127_SW_shunt_SNA");
  printDebugIfActive(BMS_a128_SW_shunt_MIA, "ERROR: BMS_a128_SW_shunt_MIA");
  printDebugIfActive(BMS_a129_SW_VSH_Failure, "ERROR: BMS_a129_SW_VSH_Failure");
  printDebugIfActive(BMS_a130_IO_CAN_Error, "ERROR: BMS_a130_IO_CAN_Error");
  printDebugIfActive(BMS_a131_Bleed_FET_Failure, "ERROR: BMS_a131_Bleed_FET_Failure");
  printDebugIfActive(BMS_a132_HW_BMB_OTP_Uncorrctbl, "ERROR: BMS_a132_HW_BMB_OTP_Uncorrctbl");
  printDebugIfActive(BMS_a134_SW_Delayed_Ctr_Off, "ERROR: BMS_a134_SW_Delayed_Ctr_Off");
  printDebugIfActive(BMS_a136_SW_Module_OT_Warning, "ERROR: BMS_a136_SW_Module_OT_Warning");
  printDebugIfActive(BMS_a137_SW_Brick_UV_Warning, "ERROR: BMS_a137_SW_Brick_UV_Warning");
  printDebugIfActive(BMS_a139_SW_DC_Link_V_Irrational, "ERROR: BMS_a139_SW_DC_Link_V_Irrational");
  printDebugIfActive(BMS_a141_SW_BMB_Status_Warning, "ERROR: BMS_a141_SW_BMB_Status_Warning");
  printDebugIfActive(BMS_a144_Hvp_Config_Mismatch, "ERROR: BMS_a144_Hvp_Config_Mismatch");
  printDebugIfActive(BMS_a145_SW_SOC_Change, "INFO: BMS_a145_SW_SOC_Change");
  printDebugIfActive(BMS_a146_SW_Brick_Overdischarged, "ERROR: BMS_a146_SW_Brick_Overdischarged");
  printDebugIfActive(BMS_a149_SW_Missing_Config_Block, "ERROR: BMS_a149_SW_Missing_Config_Block");
  printDebugIfActive(BMS_a151_SW_external_isolation, "ERROR: BMS_a151_SW_external_isolation");
  printDebugIfActive(BMS_a156_SW_BMB_Vref_bad, "ERROR: BMS_a156_SW_BMB_Vref_bad");
  printDebugIfActive(BMS_a157_SW_HVP_HVS_Comms, "ERROR: BMS_a157_SW_HVP_HVS_Comms");
  printDebugIfActive(BMS_a158_SW_HVP_HVI_Comms, "ERROR: BMS_a158_SW_HVP_HVI_Comms");
  printDebugIfActive(BMS_a159_SW_HVP_ECU_Error, "ERROR: BMS_a159_SW_HVP_ECU_Error");
  printDebugIfActive(BMS_a161_SW_DI_Open_Request, "ERROR: BMS_a161_SW_DI_Open_Request");
  printDebugIfActive(BMS_a162_SW_No_Power_For_Support, "ERROR: BMS_a162_SW_No_Power_For_Support");
  printDebugIfActive(BMS_a163_SW_Contactor_Mismatch, "ERROR: BMS_a163_SW_Contactor_Mismatch");
  printDebugIfActive(BMS_a164_SW_Uncontrolled_Regen, "ERROR: BMS_a164_SW_Uncontrolled_Regen");
  printDebugIfActive(BMS_a165_SW_Pack_Partial_Weld, "ERROR: BMS_a165_SW_Pack_Partial_Weld");
  printDebugIfActive(BMS_a166_SW_Pack_Full_Weld, "ERROR: BMS_a166_SW_Pack_Full_Weld");
  printDebugIfActive(BMS_a167_SW_FC_Partial_Weld, "ERROR: BMS_a167_SW_FC_Partial_Weld");
  printDebugIfActive(BMS_a168_SW_FC_Full_Weld, "ERROR: BMS_a168_SW_FC_Full_Weld");
  printDebugIfActive(BMS_a169_SW_FC_Pack_Weld, "ERROR: BMS_a169_SW_FC_Pack_Weld");
  //printDebugIfActive(BMS_a170_SW_Limp_Mode, "ERROR: BMS_a170_SW_Limp_Mode");
  printDebugIfActive(BMS_a171_SW_Stack_Voltage_Sense, "ERROR: BMS_a171_SW_Stack_Voltage_Sense");
  printDebugIfActive(BMS_a174_SW_Charge_Failure, "ERROR: BMS_a174_SW_Charge_Failure");
  printDebugIfActive(BMS_a176_SW_GracefulPowerOff, "ERROR: BMS_a176_SW_GracefulPowerOff");
  printDebugIfActive(BMS_a179_SW_Hvp_12V_Fault, "ERROR: BMS_a179_SW_Hvp_12V_Fault");
  printDebugIfActive(BMS_a180_SW_ECU_reset_blocked, "ERROR: BMS_a180_SW_ECU_reset_blocked");
}

void TeslaModel3YBattery::setup(void) {  // Performs one time setup at startup

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }

  //0x7FF GTW CAN frame values
  //Mux1
  write_signal_value(&TESLA_7FF_Mux1, 16, 16, user_selected_tesla_GTW_country, false);
  write_signal_value(&TESLA_7FF_Mux1, 11, 1, user_selected_tesla_GTW_rightHandDrive, false);
  //Mux3
  write_signal_value(&TESLA_7FF_Mux3, 8, 4, user_selected_tesla_GTW_mapRegion, false);
  write_signal_value(&TESLA_7FF_Mux3, 18, 3, user_selected_tesla_GTW_chassisType, false);
  write_signal_value(&TESLA_7FF_Mux3, 32, 5, user_selected_tesla_GTW_packEnergy, false);

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
  } else {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
  }
}

void TeslaModelSXBattery::setup(void) {
  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
}
