#include "BMW-IX-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

// Function to check if a value has gone stale over a specified time period
bool BmwIXBattery::isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
  unsigned long currentTime = millis();

  // Check if the value has changed
  if (currentValue != lastValue) {
    // Update the last change time and value
    lastChangeTime = currentTime;
    lastValue = currentValue;
    return false;  // Value is fresh because it has changed
  }

  // Check if the value has stayed the same for the specified staleness period
  return (currentTime - lastChangeTime >= STALE_PERIOD);
}

uint8_t BmwIXBattery::increment_uds_req_id_counter(uint8_t index) {
  index++;
  if (index >= numUDSreqs) {
    index = 0;
  }
  return index;
}

uint8_t BmwIXBattery::increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

// UDS Multi-Frame Reception Helper Functions
void BmwIXBattery::startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID) {
  gUDSContext.UDS_inProgress = true;
  gUDSContext.UDS_expectedLength = totalLength;
  gUDSContext.UDS_bytesReceived = 0;
  gUDSContext.UDS_sequenceNumber = 1;  // Next expected sequence is 1
  gUDSContext.UDS_moduleID = moduleID;
  memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
  gUDSContext.UDS_lastFrameMillis = millis();  // Track timeout
}

bool BmwIXBattery::storeUDSPayload(const uint8_t* payload, uint8_t length) {
  if (gUDSContext.UDS_bytesReceived + length > sizeof(gUDSContext.UDS_buffer)) {
    logging.println("UDS buffer overflow prevented");
    gUDSContext.UDS_inProgress = false;
    return false;
  }

  memcpy(&gUDSContext.UDS_buffer[gUDSContext.UDS_bytesReceived], payload, length);
  gUDSContext.UDS_bytesReceived += length;
  gUDSContext.UDS_lastFrameMillis = millis();

  // If we've reached or exceeded the expected length, mark complete
  if (gUDSContext.UDS_bytesReceived >= gUDSContext.UDS_expectedLength) {
    gUDSContext.UDS_inProgress = false;
  }
  return true;
}

bool BmwIXBattery::isUDSMessageComplete() {
  return (!gUDSContext.UDS_inProgress && gUDSContext.UDS_bytesReceived > 0);
}
CAN_frame BmwIXBattery::generate_433_datetime_message() {
  CAN_frame frame_433;
  frame_433.ID = 0x433;
  frame_433.DLC = 8;
  frame_433.ext_ID = false;
  frame_433.FD = true;
  // Hardcoded reference start time: 2025-02-21 17:00:00
  const uint16_t startYear = 2025;
  const uint8_t startMonth = 2;
  const uint8_t startDay = 21;
  const uint8_t startHour = 17;
  const uint8_t startMinute = 0;
  const uint8_t startSecond = 0;

  // Calculate elapsed time since boot in seconds
  unsigned long elapsedSeconds = millis() / 1000;

  // Add elapsed seconds to reference time
  uint32_t totalSeconds = startSecond + elapsedSeconds;
  uint8_t second = totalSeconds % 60;
  uint32_t totalMinutes = startMinute + (totalSeconds / 60);
  uint8_t minute = totalMinutes % 60;
  uint32_t totalHours = startHour + (totalMinutes / 60);
  uint8_t hour = totalHours % 24;
  uint32_t totalDays = startDay + (totalHours / 24);

  // Simple month/year calculation (not accounting for varying month lengths)
  // For production, you'd want a proper datetime library
  uint8_t month = startMonth;
  uint16_t year = startYear;

  // Rough day overflow handling (assumes 30 days per month for simplicity)
  while (totalDays > 30) {
    totalDays -= 30;
    month++;
    if (month > 12) {
      month = 1;
      year++;
    }
  }
  uint8_t day = totalDays;

  // Byte 1: Hour (0-23)
  frame_433.data.u8[0] = hour;

  // Byte 2: Minute (0-59)
  frame_433.data.u8[1] = minute;

  // Byte 3: Second (0-59)
  frame_433.data.u8[2] = second;

  // Byte 4: Day of month (1-31)
  frame_433.data.u8[3] = day;

  // Byte 5: Month (upper nibble) + Year low digit (lower nibble)
  // Month in upper 4 bits, year last digit in lower 4 bits
  uint8_t yearLowDigit = year % 10;
  frame_433.data.u8[4] = (month << 4) | yearLowDigit;

  // Byte 6 + 7: Full year as 16-bit little-endian
  frame_433.data.u8[5] = year & 0xFF;         // Low byte
  frame_433.data.u8[6] = (year >> 8) & 0xFF;  // High byte

  // Byte 8: Terminator/checksum (constant 0xF5 based on samples)
  frame_433.data.u8[7] = 0xF5;

  return frame_433;
}
CAN_frame BmwIXBattery::generate_442_time_counter_message() {
  CAN_frame frame_442;
  frame_442.ID = 0x442;
  frame_442.DLC = 6;
  frame_442.ext_ID = false;
  frame_442.FD = true;

  // Calculate elapsed time in seconds (counter increments at 1 Hz)
  // millis() returns milliseconds, so divide by 1000 to get seconds
  uint32_t timeCounter = millis() / 1000;

  // Bytes 1-4: Time counter in little-endian format (seconds since boot)
  frame_442.data.u8[0] = timeCounter & 0xFF;  // LSB
  frame_442.data.u8[1] = (timeCounter >> 8) & 0xFF;
  frame_442.data.u8[2] = (timeCounter >> 16) & 0xFF;
  frame_442.data.u8[3] = (timeCounter >> 24) & 0xFF;  // MSB

  // Bytes 5-6: Constant signature
  frame_442.data.u8[4] = 0xE0;
  frame_442.data.u8[5] = 0x23;

  return frame_442;
}
void BmwIXBattery::parseDTCResponse() {
  // Check for negative response
  if (gUDSContext.UDS_buffer[0] == 0x7F) {
    logging.print("DTC request rejected by battery. Reason code: 0x");
    logging.print(gUDSContext.UDS_buffer[2], HEX);
    logging.println();
    datalayer_extended.bmwix.dtc_read_failed = true;
    datalayer_extended.bmwix.dtc_read_in_progress = false;
    return;
  }

  if (gUDSContext.UDS_buffer[0] != 0x59 || gUDSContext.UDS_buffer[1] != 0x02) {
    logging.println("Invalid DTC response header");
    datalayer_extended.bmwix.dtc_read_failed = true;
    datalayer_extended.bmwix.dtc_read_in_progress = false;
    return;
  }

  int dtcStartIndex = 3;  // Skip 59 02 FF
  int availableBytes = gUDSContext.UDS_bytesReceived - dtcStartIndex;
  int maxDtcCount = availableBytes / 4;

  if (maxDtcCount > MAX_DTC_COUNT) {
    maxDtcCount = MAX_DTC_COUNT;
    logging.println("DTC count exceeds buffer, truncating");
  }

  int validDtcCount = 0;  // Track actual valid DTCs

  logging.print("Parsing DTCs (max ");
  logging.print(maxDtcCount);
  logging.println("):");

  for (int i = 0; i < maxDtcCount; i++) {
    int offset = dtcStartIndex + (i * 4);

    // Bounds check
    if (offset + 3 > gUDSContext.UDS_bytesReceived) {
      logging.println("DTC parsing: offset exceeds buffer, stopping");
      break;
    }

    // Combine 3 bytes into single uint32
    uint32_t dtcCode = ((uint32_t)gUDSContext.UDS_buffer[offset] << 16) |
                       ((uint32_t)gUDSContext.UDS_buffer[offset + 1] << 8) |
                       (uint32_t)gUDSContext.UDS_buffer[offset + 2];

    uint8_t dtcStatus = gUDSContext.UDS_buffer[offset + 3];

    // Skip invalid DTCs (0x000000 or status 0x00)
    if (dtcCode == 0x000000 || dtcStatus == 0x00) {
      logging.print("  Skipping invalid DTC at offset ");
      logging.println(offset);
      continue;  // Don't store this one
    }

    // Store valid DTC
    datalayer_extended.bmwix.dtc_codes[validDtcCount] = dtcCode;
    datalayer_extended.bmwix.dtc_status[validDtcCount] = dtcStatus;

    // Log each DTC for debugging
    logging.print("  DTC #");
    logging.print(validDtcCount + 1);
    logging.print(": 0x");
    if (dtcCode < 0x100000)
      logging.print("0");
    if (dtcCode < 0x10000)
      logging.print("0");
    if (dtcCode < 0x1000)
      logging.print("0");
    if (dtcCode < 0x100)
      logging.print("0");
    if (dtcCode < 0x10)
      logging.print("0");
    logging.print(dtcCode, HEX);
    logging.print(" Status: 0x");
    if (dtcStatus < 0x10)
      logging.print("0");
    logging.print(dtcStatus, HEX);
    logging.println();

    validDtcCount++;  // Increment only for valid DTCs
  }

  datalayer_extended.bmwix.dtc_count = validDtcCount;  // Store actual count

  logging.print("Total valid DTCs: ");
  logging.println(validDtcCount);

  datalayer_extended.bmwix.dtc_last_read_millis = millis();
  datalayer_extended.bmwix.dtc_read_failed = false;
  datalayer_extended.bmwix.dtc_read_in_progress = false;
}

void BmwIXBattery::handleISOTPFrame(CAN_frame& rx_frame) {
  uint8_t pciByte = rx_frame.data.u8[1];  // e.g., 0x10, 0x21, etc.
  uint8_t pciType = pciByte >> 4;         // top nibble => 0=SF,1=FF,2=CF,3=FC

  // Only process multi-frame ISO-TP messages (FF and CF)
  // Single-frame messages are handled directly in case 0x607
  if (pciType != 0x1 && pciType != 0x2) {
    return;  // Not a multi-frame message we care about
  }

  switch (pciType) {
    case 0x1: {
      // First Frame (FF)
      uint8_t pciLower = pciByte & 0x0F;
      uint16_t totalLength = ((uint16_t)pciLower << 8) | rx_frame.data.u8[2];

      uint8_t serviceResponse = rx_frame.data.u8[3];  // Service response byte (0x59, 0x62, etc.)
      uint8_t moduleID;

      // Determine which byte to use for module ID based on service response
      if (serviceResponse == 0x59) {
        // Standard UDS DTC response (0x19 -> 0x59)
        // Use sub-function byte as module ID
        moduleID = rx_frame.data.u8[4];  // 0x02 for reportDTCByStatusMask
      } else {
        // BMW proprietary responses (0x22 -> 0x62)
        // Use parameter byte as module ID
        moduleID = rx_frame.data.u8[5];  // 0x54, 0x53, etc.
      }

      // logging.print("FF arrived! moduleID=0x");
      // logging.print(moduleID, HEX);
      // logging.print(", totalLength=");
      // logging.println(totalLength);

      // Start the multi-frame reception
      startUDSMultiFrameReception(totalLength, moduleID);
      gUDSContext.receivedInBatch = 0;  // Reset batch count

      // Store the FF payload (starts at data[3] for extended addressing)
      const uint8_t* ffPayload = &rx_frame.data.u8[3];
      uint8_t ffPayloadSize = rx_frame.DLC - 3;
      storeUDSPayload(ffPayload, ffPayloadSize);

      // Request continuation
      transmit_can_frame(&BMWiX_6F4_CONTINUE_DATA);
      break;
    }

    case 0x2: {
      // Consecutive Frame (CF)
      if (!gUDSContext.UDS_inProgress) {
        logging.println("Unexpected CF - not in progress");
        return;  // Unexpected CF, ignore
      }

      //uint8_t seq = pciByte & 0x0F;

      // logging.print("CF seq=0x");
      // logging.print(seq, HEX);
      // logging.print(" for moduleID=0x");
      // logging.println(gUDSContext.UDS_moduleID, HEX);

      // Store CF payload (starts at byte 2)
      storeUDSPayload(&rx_frame.data.u8[2], rx_frame.DLC - 2);

      // Increment batch counter
      gUDSContext.receivedInBatch++;

      // logging.print("After CF, UDS_bytesReceived=");
      // logging.println(gUDSContext.UDS_bytesReceived);

      // Check if batch is complete (iX uses 2 frames per batch based on 0x30 0x00 0x02)
      if (gUDSContext.receivedInBatch >= 2) {
        //logging.println("Batch complete - requesting continue frame...");
        transmit_can_frame(&BMWiX_6F4_CONTINUE_DATA);
        gUDSContext.receivedInBatch = 0;
      }
      break;
    }
  }
}

void BmwIXBattery::processCompletedUDSResponse() {
  uint8_t* buf = gUDSContext.UDS_buffer;
  uint16_t len = gUDSContext.UDS_bytesReceived;

  // Route based on moduleID (set during First Frame reception)
  if (gUDSContext.UDS_moduleID == 0x02) {
    // DTC Response (0x19 0x02 -> 0x59 0x02)
    logging.println("=== DTC Response Received ===");
    logging.print("Total bytes: ");
    logging.println(len);
    parseDTCResponse();

  } else if (gUDSContext.UDS_moduleID == 0x54) {
    // Cell Voltages (0x22 0xE5 0x54 -> 0x62 0xE5 0x54)
    // logging.print("Parsing cell voltages - Total bytes: ");
    // logging.println(len);
    int voltage_index = 0;
    for (int i = 3; i < len - 1; i += 2) {
      if (voltage_index >= 108)
        break;
      uint16_t voltage = (buf[i] << 8) | buf[i + 1];
      if (voltage < 10000) {
        datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
      }
      voltage_index++;
    }
    // logging.print("Parsed ");
    // logging.print(voltage_index);
    // logging.println(" cell voltages");

    // Check for 96S vs 108S detection
    if (voltage_index >= 97) {
      int byte_offset = 3 + (96 * 2);
      if (byte_offset + 1 < len) {
        if (buf[byte_offset] == 0xFF && buf[byte_offset + 1] == 0xFF) {
          detected_number_of_cells = 96;
          //logging.println("Detected 96S battery");
        } else {
          detected_number_of_cells = 108;
          // logging.println("Detected 108S battery");
        }
      }
    }

  } else if (gUDSContext.UDS_moduleID == 0xCE) {
    // SOC Response (0x22 0xE5 0xCE -> 0x62 0xE5 0xCE)
    if (len >= 9) {
      avg_soc_state = (buf[3] << 8 | buf[4]);
      min_soc_state = (buf[5] << 8 | buf[6]);
      max_soc_state = (buf[7] << 8 | buf[8]);
      logging.println("SOC data updated");
    }

  } else if (gUDSContext.UDS_moduleID == 0xCA) {
    // Balancing Data (0x22 0xE4 0xCA -> 0x62 0xE4 0xCA)
    if (len >= 4) {
      balancing_status = buf[3];
      logging.println("Balancing status updated");
    }

    // REMOVE THIS BLOCK - Isolation data comes as single CAN FD frame, not ISO-TP
    // } else if (gUDSContext.UDS_moduleID == 0x60) {
    //   // Safety Isolation (0x22 0xA8 0x60 -> 0x62 0xA8 0x60)
    //   if (len >= 43) {
    //     iso_safety_positive = (buf[31] << 24) | (buf[32] << 16) | (buf[33] << 8) | buf[34];
    //     iso_safety_negative = (buf[35] << 24) | (buf[36] << 16) | (buf[37] << 8) | buf[38];
    //     iso_safety_parallel = (buf[39] << 24) | (buf[40] << 16) | (buf[41] << 8) | buf[42];
    //     logging.println("ISO safety data updated");
    //   }

  } else if (gUDSContext.UDS_moduleID == 0xC0) {
    // Uptime (0x22 0xE4 0xC0 -> 0x62 0xE4 0xC0)
    if (len >= 11) {
      sme_uptime = (buf[7] << 24) | (buf[8] << 16) | (buf[9] << 8) | buf[10];
      logging.println("SME uptime updated");
    }

  } else if (gUDSContext.UDS_moduleID == 0x45) {
    // SOH Data (0x22 0xE5 0x45 -> 0x62 0xE5 0x45)
    if (len >= 11) {
      min_soh_state = (buf[5] << 8 | buf[6]);
      avg_soh_state = (buf[7] << 8 | buf[8]);
      max_soh_state = (buf[9] << 8 | buf[10]);
      logging.println("SOH data updated");
    }
  }

  // Reset buffer after processing
  gUDSContext.UDS_bytesReceived = 0;
}

/*
static uint8_t increment_C0_counter(uint8_t counter) {
  counter++;
  // Reset to 0xF0 if it exceeds 0xFE
  if (counter > 0xFE) {
    counter = 0xF0;
  }
  return counter;
}
*/
void BmwIXBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = avg_soc_state;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh = max_capacity;

  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity;

  datalayer.battery.status.soh_pptt = min_soh_state;

  datalayer.battery.status.max_discharge_power_W =
      datalayer.battery.status.override_discharge_power_W;  //TODO: Estimated from UI

  // Estimated charge power is set in Settings page. Ramp power on top
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        datalayer.battery.status.override_charge_power_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  datalayer.battery.status.temperature_min_dC = min_battery_temperature;

  datalayer.battery.status.temperature_max_dC = max_battery_temperature;

  //Check stale values. As values dont change much during idle only consider stale if both parts of this message freeze.
  bool isMinCellVoltageStale =
      isStale(min_cell_voltage, datalayer.battery.status.cell_min_voltage_mV, min_cell_voltage_lastchanged);
  bool isMaxCellVoltageStale =
      isStale(max_cell_voltage, datalayer.battery.status.cell_max_voltage_mV, max_cell_voltage_lastchanged);

  if (isMinCellVoltageStale && isMaxCellVoltageStale) {
    datalayer.battery.status.cell_min_voltage_mV = 9999;  //Stale values force stop
    datalayer.battery.status.cell_max_voltage_mV = 9999;  //Stale values force stop
    set_event(EVENT_STALE_VALUE, 0);
  } else {
    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;  //Value is alive
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;  //Value is alive
  }

  if (terminal30_12v_voltage < 1100) {  //11.000V
    set_event(EVENT_12V_LOW, terminal30_12v_voltage);
  }

  if ((datalayer.battery.status.cell_voltages_mV[77] > 1000) &&
      (datalayer.battery.status.cell_voltages_mV[78] < 1000)) {
    //If we detect cellvoltage on cell78, but nothing on 79, we can confirm we are on SE12
    detected_number_of_cells = 78;  //We are on 78S SE12 battery from BMW IX1
  }  //Sidenote, detection of 96S and 108S batteries happen inside the cellvoltage reading blocks

  datalayer.battery.info.number_of_cells = detected_number_of_cells;

  if (detected_number_of_cells == 78) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_78S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_78S_DV;
  }
  if (detected_number_of_cells == 96) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
  }
  if (detected_number_of_cells == 108) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
  }

  // Map BMW IX balancing status to standard balancing status enum
  // 0 = No balancing mode active (Ready)
  // 1/2/3 = Various balancing modes active (Active)
  // 4 = No balancing mode active, qualifier invalid (Error)
  // default = Unknown
  switch (balancing_status) {
    case 0:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_READY;
      break;
    case 1:
    case 2:
    case 3:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;
      break;
    case 4:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_ERROR;
      break;
    default:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_UNKNOWN;
      break;
  }
}

void BmwIXBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x12B8D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x20B:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2E2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x31F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x453:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x486:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x49C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4BB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x507:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x587:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x8F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xD0D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x607:  //SME responds to UDS requests on 0x607
      // Removed immediate cell voltage parsing blocks - now handled by ISO-TP multi-frame handler below

      if ((rx_frame.DLC == 7) && (rx_frame.data.u8[4] == 0x4D)) {  //Main Battery Voltage (Pre Contactor)
        battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if ((rx_frame.DLC == 7) && (rx_frame.data.u8[4] == 0x4A)) {  //Main Battery Voltage (After Contactor)
        battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0x61)) {  //Current amps 32bit signed MSB. dA . negative is discharge
        battery_current = ((int32_t)((rx_frame.data.u8[6] << 24) | (rx_frame.data.u8[7] << 16) |
                                     (rx_frame.data.u8[8] << 8) | rx_frame.data.u8[9])) *
                          0.1;
      }

      if ((rx_frame.DLC == 64) && (rx_frame.data.u8[4] == 0xE4) && (rx_frame.data.u8[5] == 0xCA)) {  //Balancing Data
        balancing_status = (rx_frame.data.u8[6]);  //4 = No symmetry mode active, invalid qualifier
      }
      if ((rx_frame.DLC >= 6) && (rx_frame.data.u8[2] == 0x62) && (rx_frame.data.u8[3] == 0x10) &&
          (rx_frame.data.u8[4] == 0x0A)) {
        energy_saving_mode_status = rx_frame.data.u8[5];  // Store the energy saving mode status byte
      }
      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[4] == 0xE5) && (rx_frame.data.u8[5] == 0xCE)) {  //Min/Avg/Max SOC%
        min_soc_state = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        avg_soc_state = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        max_soc_state = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]);
      }

      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0xC7)) {  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
        remaining_capacity = ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) * 10) - 50000;
        max_capacity = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) * 10) - 50000;
      }

      if ((rx_frame.DLC == 20) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0x45)) {  //SOH Max Min Mean Request
        min_soh_state = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]));
        avg_soh_state = ((rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]));
        max_soh_state = ((rx_frame.data.u8[12] << 8 | rx_frame.data.u8[13]));
      }

      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0x62)) {  //Max allowed charge and discharge current - Signed 16bit
        allowable_charge_amps = (int16_t)((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7])) / 10;
        allowable_discharge_amps = (int16_t)((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9])) / 10;
      }

      if ((rx_frame.DLC == 9) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0x4B)) {                 //Max allowed charge and discharge current - Signed 16bit
        voltage_qualifier_status = (rx_frame.data.u8[8]);  // Request HV Voltage Qualifier
      }

      if ((rx_frame.DLC == 64) && (rx_frame.data.u8[4] == 0xA8) &&
          (rx_frame.data.u8[5] == 0x60)) {  // Safety Isolation Measurements
        iso_safety_positive = (rx_frame.data.u8[34] << 24) | (rx_frame.data.u8[35] << 16) |
                              (rx_frame.data.u8[36] << 8) | rx_frame.data.u8[37];  //Assuming 32bit
        iso_safety_negative = (rx_frame.data.u8[38] << 24) | (rx_frame.data.u8[39] << 16) |
                              (rx_frame.data.u8[40] << 8) | rx_frame.data.u8[41];  //Assuming 32bit
        iso_safety_parallel = (rx_frame.data.u8[42] << 24) | (rx_frame.data.u8[43] << 16) |
                              (rx_frame.data.u8[44] << 8) | rx_frame.data.u8[45];  //Assuming 32bit
      }

      if ((rx_frame.DLC == 48) && (rx_frame.data.u8[4] == 0xE4) &&
          (rx_frame.data.u8[5] == 0xC0)) {  // Uptime and Vehicle Time Status
        sme_uptime = (rx_frame.data.u8[10] << 24) | (rx_frame.data.u8[11] << 16) | (rx_frame.data.u8[12] << 8) |
                     rx_frame.data.u8[13];  //Assuming 32bit
      }

      if ((rx_frame.DLC == 8) && (rx_frame.data.u8[3] == 0xAC) && (rx_frame.data.u8[4] == 0x93)) {  // Pyro Status
        pyro_status_pss1 = (rx_frame.data.u8[5]);
        pyro_status_pss4 = (rx_frame.data.u8[6]);
        pyro_status_pss6 = (rx_frame.data.u8[7]);
      }

      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[4] == 0xE5) &&
          (rx_frame.data.u8[5] == 0x53)) {  //Min and max cell voltage   10V = Qualifier Invalid

        datalayer.battery.status.CAN_battery_still_alive =
            CAN_STILL_ALIVE;  //This is the most important safety values, if we receive this we reset CAN alive counter.

        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) == 10000 ||
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) == 10000) {  //Qualifier Invalid Mode - Request Reboot
          logging.println("Cell MinMax Qualifier Invalid - Requesting BMS Reset");
          //set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, (millis())); //Eventually need new Info level event type
          transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET);
        } else {  //Only ingest values if they are not the 10V Error state
          min_cell_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
          max_cell_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        }
      }

      if ((rx_frame.DLC == 16) && (rx_frame.data.u8[4] == 0xDD) &&
          (rx_frame.data.u8[5] == 0xC0)) {  //Battery Temperature
        min_battery_temperature = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) / 10;
        avg_battery_temperature = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]) / 10;
        max_battery_temperature = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) / 10;
      }
      if ((rx_frame.DLC == 7) &&
          (rx_frame.data.u8[4] == 0xA3)) {  //Main Contactor Temperature CHECK FINGERPRINT 2 LEVEL
        main_contactor_temperature = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if ((rx_frame.DLC == 7) && (rx_frame.data.u8[4] == 0xA7)) {  //Terminal 30 Voltage (12V SME supply)
        terminal30_12v_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if ((rx_frame.DLC == 6) && (rx_frame.data.u8[0] == 0xF4) && (rx_frame.data.u8[1] == 0x04) &&
          (rx_frame.data.u8[2] == 0x62) && (rx_frame.data.u8[3] == 0xE5) &&
          (rx_frame.data.u8[4] == 0x69)) {  //HVIL Status
        hvil_status = (rx_frame.data.u8[5]);
      }

      if ((rx_frame.DLC == 12) && (rx_frame.data.u8[2] == 0x07) && (rx_frame.data.u8[3] == 0x62) &&
          (rx_frame.data.u8[4] == 0xE5) && (rx_frame.data.u8[5] == 0x4C)) {  //Pack Voltage Limits
        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) < 4700 &&
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) > 2600) {  //Make sure values are plausible
          battery_info_available = true;
          max_design_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);  //Not valid on all iX
          min_design_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);  //Not valid on all iX
        }
      }

      if ((rx_frame.DLC == 16) && (rx_frame.data.u8[3] == 0xF1) &&
          (rx_frame.data.u8[4] == 0x8C)) {  //Battery Serial Number
        //Convert hex bytes to ASCII characters and combine them into a string
        char numberString[11];  // 10 characters + null terminator
        for (int i = 0; i < 10; i++) {
          numberString[i] = char(rx_frame.data.u8[i + 6]);
        }
        numberString[10] = '\0';  // Null-terminate the string
        // Step 3: Convert the string to an unsigned long integer
        battery_serial_number = strtoul(numberString, NULL, 10);
      }

      // Handle single-frame DTC response (service 0x59)
      if (rx_frame.data.u8[3] == 0x59 && rx_frame.data.u8[4] == 0x02) {
        // Single-frame DTC response: F4 00 XX 59 02 FF [DTCs...]
        // Copy to UDS buffer and parse
        uint8_t sfLength = rx_frame.data.u8[2];  // Length byte
        if (sfLength > 0 && sfLength <= (rx_frame.DLC - 3)) {
          // Copy response data starting from byte 3 (service ID)
          memcpy(gUDSContext.UDS_buffer, &rx_frame.data.u8[3], sfLength);
          gUDSContext.UDS_bytesReceived = sfLength;
          gUDSContext.UDS_moduleID = 0x02;  // DTC response
          gUDSContext.UDS_inProgress = false;

          logging.println("=== Single-Frame DTC Response Received ===");
          logging.print("Total bytes: ");
          logging.println(gUDSContext.UDS_bytesReceived);

          parseDTCResponse();
        }
      }

      // Handle ISO-TP multi-frame messages
      handleISOTPFrame(rx_frame);

      // Check if complete UDS response is ready to process
      if (isUDSMessageComplete()) {
        processCompletedUDSResponse();
      }
      break;
    default:
      break;
  }
}

void BmwIXBattery::transmit_can(unsigned long currentMillis) {
  // Perform startup BMS reset after 3 seconds, before allowing contactor close
  if (!startup_reset_complete && (currentMillis - startup_time > 3000)) {
    logging.println("Performing startup BMS reset");
    transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET);
    startup_reset_complete = true;
    // Allow contactors to close after reset
    datalayer.system.status.battery_allows_contactor_closing = true;
  }

  // Timeout check for stuck UDS transfers
  if (gUDSContext.UDS_inProgress) {
    if (currentMillis - gUDSContext.UDS_lastFrameMillis > 2000) {  // 2 second timeout
      logging.println("UDS transfer timeout - aborting");
      gUDSContext.UDS_inProgress = false;
      gUDSContext.UDS_bytesReceived = 0;
    }
  }

  // We can always send CAN as the iX BMS will wake up on vehicle comms
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    ContactorCloseRequest.present = contactorCloseReq;
    // Detect edge
    if (ContactorCloseRequest.previous == false && ContactorCloseRequest.present == true) {
      // Rising edge detected
      logging.println("Rising edge detected. Resetting 10ms counter.");
      counter_10ms = 0;  // reset counter
    } else if (ContactorCloseRequest.previous == true && ContactorCloseRequest.present == false) {
      // Dropping edge detected
      logging.println("Dropping edge detected. Resetting 10ms counter.");
      counter_10ms = 0;  // reset counter
    }
    ContactorCloseRequest.previous = ContactorCloseRequest.present;
    HandleBmwIxCloseContactorsRequest(counter_10ms);
    HandleBmwIxOpenContactorsRequest(counter_10ms);
    counter_10ms++;

    // prevent counter overflow: 2^16-1 = 65535
    if (counter_10ms == 65535) {
      counter_10ms = 1;  // set to 1, to differentiate the counter being set to 0 by the functions above
    }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    HandleIncomingInverterRequest();

    //Loop through and send a different UDS request once the contactors are closed
    if (contactorCloseReq == true &&
        ContactorState.closed ==
            true) {  // Do not send unless the contactors are requested to be closed and are closed, as sending these does not allow the contactors to close
      uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
      transmit_can_frame(
          UDS_REQUESTS100MS[uds_req_id_counter]);  // FIXME: sending these does not allow the contactors to close
    } else {  // FIXME: hotfix: If contactors are not requested to be closed, ensure the battery is reported as alive, even if no CAN messages are received
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    }

    // Keep contactors closed if needed
    BmwIxKeepContactorsClosed(counter_100ms);
    counter_100ms++;
    if (counter_100ms == 140) {
      counter_100ms = 0;  // reset counter every 14 seconds
    }

    //Send SME Keep alive values 100ms
    //transmit_can_frame(&BMWiX_510);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    //Send SME Keep alive values 200ms
    //BMWiX_C0.data.u8[0] = increment_C0_counter(BMWiX_C0.data.u8[0]);  //Keep Alive 1
    //transmit_can_frame(&BMWiX_C0);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
    CAN_frame BMWiX_433 = generate_433_datetime_message();
    transmit_can_frame(&BMWiX_433);
    CAN_frame BMWiX_442 = generate_442_time_counter_message();
    transmit_can_frame(&BMWiX_442);
    HandleIncomingUserRequest();
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;

    // Send slow UDS requests (like DTC reading) regardless of contactor state
    uds_req_id_counter_slow++;
    if (uds_req_id_counter_slow >= numUDSreqsSlow) {
      uds_req_id_counter_slow = 0;
    }
    // Add logging to see which request is being sent
    //logging.print("Sending slow UDS request #");
    //logging.println(uds_req_id_counter_slow);
    //transmit_can_frame(UDS_REQUESTS_SLOW[uds_req_id_counter_slow]); no messages needed on slow loop right now

    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START2);
    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START);
  }
  // Handle user DTC read request
  if (UserRequestDTCRead) {
    logging.println("User requested DTC read");
    transmit_can_frame(&BMWiX_6F4_REQUEST_READ_DTC);
    UserRequestDTCRead = false;

    // Set flags in datalayer for HTML renderer
    datalayer_extended.bmwix.dtc_read_in_progress = true;
    datalayer_extended.bmwix.dtc_read_failed = false;
  }

  // Handle user DTC reset request
  if (UserRequestDTCreset) {
    logging.println("User requested DTC reset");
    transmit_can_frame(&BMWiX_6F4_REQUEST_CLEAR_DTC);
    UserRequestDTCreset = false;
  }

  // Handle user BMS reset request
  if (UserRequestBMSReset) {
    logging.println("User requested BMS reset");
    transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET);
    UserRequestBMSReset = false;
  }
  // Handle user Energy Saving Mode reset request
  if (UserRequestEnergySavingModeReset) {
    logging.println("User requested Energy Saving Mode reset to normal");
    transmit_can_frame(&BMWiX_6F4_SET_ENERGY_SAVING_MODE_NORMAL);
    UserRequestEnergySavingModeReset = false;
  }
}

void BmwIXBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  startup_time = millis();
  startup_reset_complete = false;

  //Before we have started up and detected which battery is in use, use largest deviation possible to avoid errors
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_78S_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = false;  // Don't allow contactors until reset is done
}

void BmwIXBattery::HandleIncomingUserRequest(void) {
  // Debug user request to open or close the contactors
  if (userRequestContactorClose) {
    logging.printf("User request: contactor close");
  }
  if (userRequestContactorOpen) {
    logging.printf("User request: contactor open");
  }
  if ((userRequestContactorClose == false) && (userRequestContactorOpen == false)) {
    // do nothing
  } else if ((userRequestContactorClose == true) && (userRequestContactorOpen == false)) {
    BmwIxCloseContactors();
    // set user request to false
    userRequestContactorClose = false;
  } else if ((userRequestContactorClose == false) && (userRequestContactorOpen == true)) {
    BmwIxOpenContactors();
    // set user request to false
    userRequestContactorOpen = false;
  } else if ((userRequestContactorClose == true) && (userRequestContactorOpen == true)) {
    // these flasgs should not be true at the same time, therefore open contactors, as that is the safest state
    BmwIxOpenContactors();
    // set user request to false
    userRequestContactorClose = false;
    userRequestContactorOpen = false;
    // print error, as both these flags shall not be true at the same time
    logging.println(
        "Error: user requested contactors to close and open at the same time. Contactors have been opened.");
  }
}

void BmwIXBattery::HandleIncomingInverterRequest(void) {
  InverterContactorCloseRequest.present = datalayer.system.status.inverter_allows_contactor_closing;
  // Detect edge
  if (InverterContactorCloseRequest.previous == false && InverterContactorCloseRequest.present == true) {
    // Rising edge detected
    logging.println("Inverter requests to close contactors");
    BmwIxCloseContactors();
  } else if (InverterContactorCloseRequest.previous == true && InverterContactorCloseRequest.present == false) {
    // Falling edge detected
    logging.println("Inverter requests to open contactors");
    BmwIxOpenContactors();
  }  // else: do nothing

  // Update state
  InverterContactorCloseRequest.previous = InverterContactorCloseRequest.present;
}
void BmwIXBattery::BmwIxCloseContactors(void) {
  logging.println("Closing contactors");
  contactorCloseReq = true;
}

void BmwIXBattery::BmwIxOpenContactors(void) {
  logging.println("Opening contactors");
  contactorCloseReq = false;
  counter_100ms = 0;  // reset counter, such that keep contactors closed message sequence starts from the beginning
}

void BmwIXBattery::HandleBmwIxCloseContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == true) {  // Only when contactor close request is set to true
    if (ContactorState.closed == false &&
        ContactorState.open ==
            true) {  // Only when the following commands have not been completed yet, because it shall not be run when commands have already been run, AND only when contactor open commands have finished
      // Initially 0x510[2] needs to be 0x02, and 0x510[5] needs to be 0x00
      BMWiX_510.data = {0x40, 0x10,
                        0x02,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                        0x00, 0x00,
                        0x00,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                        0x01,   // 0x01 at contactor closing
                        0x00};  // Explicit declaration, to prevent modification by other functions
      BMWiX_16E.data = {
          0x00,  // Almost any possible number in 0x00 and 0xFF
          0xA0,  // Almost any possible number in 0xA0 and 0xAF
          0xC9, 0xFF, 0x60,
          0xC9, 0x3A, 0xF7};  // Explicit declaration of default values, to prevent modification by other functions

      if (counter_10ms == 0) {
        // @0 ms
        transmit_can_frame(&BMWiX_510);
        logging.println("Transmitted 0x510 - 1/6");
      } else if (counter_10ms == 5) {
        // @50 ms
        transmit_can_frame(&BMWiX_276);
        logging.println("Transmitted 0x276 - 2/6");
      } else if (counter_10ms == 10) {
        // @100 ms
        BMWiX_510.data.u8[2] = 0x04;  // TODO: check if needed
        transmit_can_frame(&BMWiX_510);
        logging.println("Transmitted 0x510 - 3/6");
      } else if (counter_10ms == 20) {
        // @200 ms
        BMWiX_510.data.u8[2] = 0x10;  // TODO: check if needed
        BMWiX_510.data.u8[5] = 0x80;  // needed to close contactors
        transmit_can_frame(&BMWiX_510);
        logging.println("Transmitted 0x510 - 4/6");
      } else if (counter_10ms == 30) {
        // @300 ms
        BMWiX_16E.data.u8[0] = 0x6A;
        BMWiX_16E.data.u8[1] = 0xAD;
        transmit_can_frame(&BMWiX_16E);
        logging.println("Transmitted 0x16E - 5/6");
      } else if (counter_10ms == 50) {
        // @500 ms
        BMWiX_16E.data.u8[0] = 0x03;
        BMWiX_16E.data.u8[1] = 0xA9;
        transmit_can_frame(&BMWiX_16E);
        logging.println("Transmitted 0x16E - 6/6");
        ContactorState.closed = true;
        ContactorState.open = false;
      }
    }
  }
}

void BmwIXBattery::BmwIxKeepContactorsClosed(uint8_t counter_100ms) {
  if ((ContactorState.closed == true) && (ContactorState.open == false)) {
    BMWiX_510.data = {0x40, 0x10,
                      0x04,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                      0x00, 0x00,
                      0x80,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                      0x01,   // 0x01 at contactor closing
                      0x00};  // Explicit declaration, to prevent modification by other functions
    BMWiX_16E.data = {0x00,   // Almost any possible number in 0x00 and 0xFF
                      0xA0,   // Almost any possible number in 0xA0 and 0xAF
                      0xC9, 0xFF, 0x60,
                      0xC9, 0x3A, 0xF7};  // Explicit declaration, to prevent modification by other functions

    if (counter_100ms == 0) {
      logging.println("Sending keep contactors closed messages started");
      // @0 ms
      transmit_can_frame(&BMWiX_510);
    } else if (counter_100ms == 7) {
      // @ 730 ms
      BMWiX_16E.data.u8[0] = 0x8C;
      BMWiX_16E.data.u8[1] = 0xA0;
      transmit_can_frame(&BMWiX_16E);
    } else if (counter_100ms == 24) {
      // @2380 ms
      transmit_can_frame(&BMWiX_510);
    } else if (counter_100ms == 29) {
      // @ 2900 ms
      BMWiX_16E.data.u8[0] = 0x02;
      BMWiX_16E.data.u8[1] = 0xA7;
      transmit_can_frame(&BMWiX_16E);
      logging.println("Sending keep contactors closed messages finished");
    } else if (counter_100ms == 140) {
      // @14000 ms
      // reset counter (outside of this function)
    }
  }
}

void BmwIXBattery::HandleBmwIxOpenContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == false) {  // if contactors are not requested to be closed, they are requested to be opened
    if (ContactorState.open == false) {  // only if contactors are not open yet
      // message content to quickly open contactors
      if (counter_10ms == 0) {
        // @0 ms (0.00) RX0 510 [8] 40 10 00 00 00 80 00 00
        BMWiX_510.data = {0x40, 0x10, 0x00, 0x00,
                          0x00, 0x80, 0x00, 0x00};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_510);
        // set back to default values
        BMWiX_510.data = {0x40, 0x10, 0x04, 0x00, 0x00, 0x80, 0x01, 0x00};  // default values
      } else if (counter_10ms == 6) {
        // @60 ms  (0.06) RX0 16E [8] E6 A4 C8 FF 60 C9 33 F0
        BMWiX_16E.data = {0xE6, 0xA4, 0xC8, 0xFF,
                          0x60, 0xC9, 0x33, 0xF0};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_16E);
        // set back to default values
        BMWiX_16E.data = {0x00, 0xA0, 0xC9, 0xFF, 0x60, 0xC9, 0x3A, 0xF7};  // default values
        ContactorState.closed = false;
        ContactorState.open = true;
      }
    }
  }
}

// Getter implementations for HTML renderer
int BmwIXBattery::get_battery_voltage_after_contactor() const {
  return battery_voltage_after_contactor;
}
unsigned long BmwIXBattery::get_min_cell_voltage_data_age() const {
  return millis() - min_cell_voltage_lastchanged;
}
unsigned long BmwIXBattery::get_max_cell_voltage_data_age() const {
  return millis() - max_cell_voltage_lastchanged;
}
int BmwIXBattery::get_T30_Voltage() const {
  return terminal30_12v_voltage;
}
int BmwIXBattery::get_balancing_status() const {
  return balancing_status;
}
int BmwIXBattery::get_energy_saving_mode_status() const {
  return energy_saving_mode_status;
}
int BmwIXBattery::get_hvil_status() const {
  return hvil_status;
}
unsigned long BmwIXBattery::get_bms_uptime() const {
  return sme_uptime;
}
int BmwIXBattery::get_allowable_charge_amps() const {
  return allowable_charge_amps;
}
int BmwIXBattery::get_allowable_discharge_amps() const {
  return allowable_discharge_amps;
}
int BmwIXBattery::get_iso_safety_positive() const {
  return iso_safety_positive;
}
int BmwIXBattery::get_iso_safety_negative() const {
  return iso_safety_negative;
}
int BmwIXBattery::get_iso_safety_parallel() const {
  return iso_safety_parallel;
}
int BmwIXBattery::get_pyro_status_pss1() const {
  return pyro_status_pss1;
}
int BmwIXBattery::get_pyro_status_pss4() const {
  return pyro_status_pss4;
}
int BmwIXBattery::get_pyro_status_pss6() const {
  return pyro_status_pss6;
}
