#include "BMW-PHEV-BATTERY.h"
#include <Arduino.h>
#include <cstring>  //For unit test
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/*
INFO

V0.1  very basic implementation reading Gen3/4 BMW PHEV SME. 
Supported:
-Pre/Post contactor voltage
-Min/ Cell Temp
-SOC

VEHICLE CANBUS
i3 messages don't affect contact close block except 0x380 which gives an error on PHEV (Possibly VIN number) 0x56, 0x5A, 0x37, 0x39, 0x34, 0x34, 0x34

BROADCAST MAP
0x112 20ms Status Of High-Voltage Battery 2
0x1F1 1000ms Status Of High-Voltage Battery 1 //PHEV doesn't seem to send this - at leasst as 0x1F1
0x239 200ms predicted charge condition and predicted target
0x295 1000ms ? [1] Alive Counter 50-5F?
0x2A5 200ms ?
0x2F5 100ms High-Voltage Battery Charge/Discharge Limitations
0x33e 5000ms?
0x40D 1000ms ?
0x430 1000ms Charging status of high-voltage battery - 2
0x431 200ms Data High-Voltage Battery Unit
0x432 200ms SOC% info
0x12F 100ms Terminal Data (possibly needed for contactor close)
0x10B 20ms  Contactor command - ACTUAL DRIVER of contactor close (see CONTACTOR CLOSE notes below)
0x53A 200ms Associated vehicle/contactor STATE indication (states A/B/C/D - see CONTACTOR CLOSE notes below)

VEHICLE TX MAP - frames WE transmit to keep the SME happy (silence CAD4xx "No message" DTCs)
Captured on the vehicle bus; the SME raises a "No message" DTC for each of these if absent.
NOTE: the source canlog was captured during a CHARGING session, so these payloads represent the
vehicle-condition / environment seen while charging (not driving). Values that depend on vehicle
state (e.g. 0x3A0 vehicle condition) may differ in other modes - revisit if behaviour differs.
Only STATIC frames are transmitted. Any frame that needs a rolling counter (and usually a CRC)
is NOT sent yet, because a wrong/frozen counter can trigger a "signal invalid" DTC that is worse
than the "No message" DTC - those need counter (and CRC where noted) logic added before enabling.
ID    DTC      Meaning                         TX     DLC  Sample payload            Period   Sent?  Notes
0x10B CAD415/  EME status / e-motor 1          EME    3    XX Cn FC open /           ~100ms   YES*   byte1 hi-nibble=state (C=open/idle,
      CAD416   (HV spec + alive)                                XX Dn FC closed                       D=close/charge), lo-nibble=rolling
                                                                                                      0..E, byte0=CRC, byte2=FC const.
                                                                                                      *Sent as the contactor driver (20ms),
                                                                                                      not as a generic vehicle frame -
                                                                                                      already has alive counter + CRC.
0x1A1 CAD40A   Vehicle speed (DSC)             DSC    5    00 C0 00 00 8A            ~100ms   NO     NEEDS COUNTER: byte1 lo-nibble=rolling
                                                                                                      0..E each frame. bytes2-3=speed
                                                                                                      (00 00=standstill), byte4=8A const.
                                                                                                      To enable: increment byte1 lo-nibble
                                                                                                      every 100ms tick (no CRC observed).
0x3A0 CAD408   Vehicle condition               BDC    8    FF FF CF FF FF FF FF FD   ~1000ms  YES    Static. mostly FF (signals not-active);
                                                                                                      byte2 CF, byte7 FD are the live bits.
0x328 CAD402   Relative time / clock           KOMBI  6    <live>                    ~1000ms  YES    Live counter (ported from i3):
                                                                                                      bytes0-3=seconds since system start
                                                                                                      (LE), bytes4-5=day counter (LE, day 1
                                                                                                      =1.1.2000). Incremented each 1000ms.
0x3CA CAD429   Driving-info forecast           KOMBI  8    B7 60 01 0F 0F 31 FF FF   ~1000ms  YES    Static forecast payload.
0x433 CAD413   HV-battery specification        EME    4    FF 0C 0C F1               ~1000ms  NO     NEEDS COUNTER: byte1 toggles 0c<->0d
                                                                                                      each frame, bytes2-3=0C F1 spec const.
                                                                                                      To enable: alternate byte1 0x0C/0x0D
                                                                                                      every 1000ms tick.
0x2CA CAD401   Ambient temperature             KOMBI  2    6E 6F                     ~1000ms  YES    byte0=temp (T+40)*2 @ 0.5C/bit -40
                                                                                                      offset; sent 0x6E=15C (in-range, no
                                                                                                      cold/hot charge derating). Static -
                                                                                                      6F/6E variance in log was noise.
0x37B CAD409   Enable HV-battery cooling       IHKA   6    00 FF 00 00 00 00         ~1000ms  NO     Reference only - NOT transmitted.
                                                                                                      byte1 FF=cooling-request/availability;
                                                                                                      00 elsewhere=no active demand.
0x3E8 CAD405   OBD diagnosis, engine ctrl      EME    2    F1 FF                     ~1000ms  YES    Static; F1 FF=no DTC / diag idle.
Notes:
 - Sent=YES (static): 0x3A0 + 0x3CA in the 1000ms loop, 0x3E8 in the 1000ms loop. 0x2CA is also
   sent static in the 1000ms loop (no rolling counter needed). 0x10B is sent as the contactor
   driver in the 20ms loop (it already maintains its own alive counter + CRC).
 - 0x328 (relative-time clock) is now transmitted every 1000ms with a live seconds + day counter
   (ported from the i3). A frozen/absent clock was a likely trigger for the idle precharge block.
 - Sent=NO (needs counter): 0x1A1, 0x433 - frame definitions exist in the header for
   reference, but they are NOT transmitted until rolling-counter logic is added per the
   "To enable" note on each. Sending them frozen risks a "signal invalid" DTC.
 - 0x37B (cooling enable) is reference only and not transmitted.

CONTACTOR CLOSE - BETA / OBSERVED CANLOG BEHAVIOUR
Two messages are involved. Bench testing shows 0x10B is what actually drives the contactors;
0x53A reflects an associated vehicle/contactor state (contactors close even while 0x53A still
streams STATE A "40 3A 04 00 00 00 01 00"). Both are documented below.

0x10B (20ms) - ACTUAL CONTACTOR DRIVER  [CURRENT IMPLEMENTATION]
 - Base data {0xCD, 0x00, 0xFC}. byte[0]=CRC (SAE J1850, init 0x3F, over 3 bytes).
 - byte[1] high nibble = contactor command, low nibble = alive counter (0..14).
 - High nibble 0x1 (0x10) = "Close contactors". 0xD0 ("Close contactors v2") was an
   alternative seen on the bus - kept commented for reference.
 - Close is only commanded when ALL of the following are true:
     * contactor_control_enabled == false  (when true, GPIO drives the physical contactors,
       so we must not also command close over CAN)
     * contactorCloseReq == true            (set by the WebUI user request or inverter request)
     * startup_counter_contactor >= 160     (~3.2s boot delay to let the SME wake)
   Otherwise the high nibble stays 0x0 (open / no close request).

0x53A (200ms) - ASSOCIATED VEHICLE/CONTACTOR STATE INDICATION
Captured on the vehicle bus (DLC8). The differentiating bytes are byte[5] and byte[6].
Observed state sequence during a close event:
  STATE A - idle / off        40 3A 04 00 00 00 01 00   (byte5=0x00, byte6=0x01)  default when contactors not requested
  STATE B - close requested   40 3A 04 00 00 80 01 00   (byte5=0x80, byte6=0x01)  CONFIRMED - held ~3.5s, ~500ms cadence
        ---- ~10.5s gap on 0x53A; physical close occurs here (seen on 0x112 / 0x3A4 / 0x326) ----
  STATE C - escalated         40 3A 04 00 00 84 01 00   (byte5=0x84, byte6=0x01)  single transient frame
  STATE D - steady / on       40 3A 04 00 00 84 00 00   (byte5=0x84, byte6=0x00)  held after physical close
Notes:
 - byte[0] (0x40) looks static; byte[1] (0x3A) matches the low byte of the ID.
 - Contactors observed to close even while 0x53A streams STATE A, confirming 0x53A is a STATE
   indication and 0x10B is the actual driver.
 - 0x53A STATE SEQUENCE [CURRENT IMPLEMENTATION]: a state machine now replays A -> B -> C -> D on a
   close request, and 0x10B is held open until the machine reaches STATE C/D (i.e. after STATE B
   has been announced for CONTACTOR_CLOSE_REQUEST_DURATION_MS, ~3.5s). This mirrors the OEM order
   (announce "close requested" on 0x53A BEFORE 0x10B drives the contactors) and was added to fix an
   intermittent open->close safety warning seen when 0x53A jumped straight to STATE D.
   The inter-state timing is compressed vs the OEM bus (which left a ~10.5s gap before STATE C);
   only the B-before-close ordering is reproduced. C ESCALATED is emitted as a single transient.
 - Timing/handshake not fully confirmed - beta.

UDS MAP
22 D6 CF - CSC Temps
22 DD C0 - Min Max temps
22 DF A5 - All Cell voltages
22 E5 EA - Alternate all cell voltages
22 DE 7E - Voltage limits.   62 DD 73 9D 5A 69 26 = 269.18V - 402.82V
22 DD 7D - Current limits 62 DD 7D 08 20 0B EA = 305A max discharge 208A max charge
22 E5 E9 DD 7D - Individual cell SOC
22 DD 69 - Current in Amps 62 DD 69 00 00 00 00 = 0 Amps
22 DD 7B - SOH  62 DD 7B 62 = 98%
22 DD 62 - HVIL Status 62 DD 64 01 = OK/Closed
22 DD BF - Cell min max alt - f18 ZELLSPANNUNGEN_MIN_MAX < doesn't work
22 DD 6A - Isolation values  62 DD 6A 07 D0 07 D0 07 D0 01 01 01 = in operation plausible/2000kOhm, in follow up plausible/2000kohm, internal iso open contactors (measured on request) pluasible/2000kohm
31 03 AD 61 - Isolation measurement status  71 03 AD 61 00 FF = Nmeasurement status - not successful / fault satate - not defined
22 DF A0 - Cell voltage and temps summary including min/max/average, Ah,  (ZUSTAND_SPEICHER)


TODO:
 BMWPHEV_6F1_REQUEST_LAST_ISO_READING - add results to advanced
 BMWPHEV_6F1_REQUEST_PACK_INFO - add cell count reading
 Find current measurement reading

*/

const char* BmwPhevBattery::getUDSRequestName(CAN_frame* frame) {
  if (frame == &BMWPHEV_6F1_REQUEST_ISO_READING1)
    return "ISO_READING1";
  if (frame == &BMWPHEV_6F1_REQUEST_ISO_READING2)
    return "ISO_READING2";
  if (frame == &BMWPHEV_6F1_REQUEST_CURRENT_LIMITS)
    return "CURRENT_LIMITS";
  if (frame == &BMWPHEV_6F1_REQUEST_SOH)
    return "SOH";
  if (frame == &BMWPHEV_6F1_REQUEST_CELLS_INDIVIDUAL_VOLTS)
    return "CELLS_INDIVIDUAL_VOLTS";
  if (frame == &BMWPHEV_6F1_REQUEST_CELL_TEMP)
    return "CELL_TEMP";
  if (frame == &BMWPHEV_6F1_REQUEST_BALANCING_STATUS)
    return "BALANCING_STATUS";
  if (frame == &BMWPHEV_6F1_REQUEST_PAIRED_VIN)
    return "PAIRED_VIN";
  if (frame == &BMWPHEV_6F1_REQUEST_CELLSUMMARY)
    return "CELLSUMMARY";
  if (frame == &BMWPHEV_6F1_REQUEST_SOC)
    return "SOC";
  if (frame == &BMWPHEV_6F1_REQUEST_CURRENT)
    return "CURRENT";
  if (frame == &BMWPHEV_6F1_REQUEST_VOLTAGE_LIMITS)
    return "VOLTAGE_LIMITS";
  if (frame == &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR)
    return "MAINVOLTAGE_PRE";
  if (frame == &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR)
    return "MAINVOLTAGE_POST";
  if (frame == &BMWPHEV_6F1_REQUEST_READ_DTC)
    return "READ_DTC";
  return "UNKNOWN";
}

// Function to check if a value has gone stale over a specified time period
bool BmwPhevBattery::isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
  unsigned long currentTime = millis();

  // Check if the value has changed
  if (currentValue != lastValue) {
    // Update the last change time and value
    lastChangeTime = currentTime;
    lastValue = currentValue;
    return false;  // Value is fresh because it has changed
  }

  // Check if the value has stayed the same for the specified staleness period
  return (currentTime - lastChangeTime >= STALE_PERIOD_CONFIG);
}

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table_SAE_J1850_ZER0[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

static uint8_t increment_uds_req_id_counter(uint8_t index, int numReqs) {
  index++;
  if (index >= numReqs) {
    index = 0;
  }
  return index;
}

/* --------------------------------------------------------------------------
   UDS Multi-Frame Helpers
   -------------------------------------------------------------------------- */

void BmwPhevBattery::startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID) {
  gUDSContext.UDS_inProgress = true;
  gUDSContext.UDS_expectedLength = totalLength;
  gUDSContext.UDS_bytesReceived = 0;
  gUDSContext.UDS_moduleID = moduleID;
  memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
  gUDSContext.UDS_lastFrameMillis = millis();  // if you want to track timeouts
}

bool BmwPhevBattery::storeUDSPayload(const uint8_t* payload, uint8_t length) {
  if (gUDSContext.UDS_bytesReceived + length > sizeof(gUDSContext.UDS_buffer)) {
    // Overflow => abort
    gUDSContext.UDS_inProgress = false;
    logging.println("UDS Payload Overflow");
    return false;
  }
  memcpy(&gUDSContext.UDS_buffer[gUDSContext.UDS_bytesReceived], payload, length);
  gUDSContext.UDS_bytesReceived += length;
  gUDSContext.UDS_lastFrameMillis = millis();

  // If we’ve reached or exceeded the expected length, mark complete
  if (gUDSContext.UDS_bytesReceived >= gUDSContext.UDS_expectedLength) {
    gUDSContext.UDS_inProgress = false;
    //     logging.println("Recived all expected UDS bytes");
  }
  return true;
}

bool BmwPhevBattery::isUDSMessageComplete() {
  return (!gUDSContext.UDS_inProgress && gUDSContext.UDS_bytesReceived > 0);
}

uint8_t BmwPhevBattery::increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

/* --------------------------------------------------------------------------
   Beta CAN-based contactor close (0x53A)
   Streams an idle frame by default and a closing frame while a close is
   requested (by the inverter or the user via WebUI). See CONTACTOR CLOSE
   notes in the INFO section above. The exact close handshake/timing is not
   yet fully characterised, so this simply toggles between the two observed
   payloads - sequencing may be needed in future.
   -------------------------------------------------------------------------- */

void BmwPhevBattery::PhevCloseContactors(void) {
  // Ignore if already closed/steady - don't restart the handshake and re-arm the stop burst,
  // which would briefly open 0x10B and re-run the ~3.5s 0x53A announce before closing again.
  if (contactorCloseReq && phev_53a_state == PHEV_53A_STEADY) {
    logging.println("Close contactors requested but already closed - ignoring");
    return;
  }
  logging.println("Closing contactors (0x10B)");
  // Balancing BLOCKS contactor close in the SME. Before the 0x10B driver is allowed to close, we
  // must make sure balancing is cancelled. Clear the user flag (so the 10s UDS loop sends stopRoutine
  // instead of startRoutine) AND kick off a short pre-close phase that sends several guarded
  // stopRoutine frames (one per UDS-guard window). 0x10B close is held off until those are sent
  // (see phev_pre_close_stops_remaining gating in the 20ms block).
  datalayer.battery.settings.user_requests_balancing = false;
  phev_pre_close_stops_remaining = PHEV_PRE_CLOSE_STOP_COUNT;
  phev_pre_close_stop_last_ms = 0;  // 0 = send the first stop immediately
  contactorCloseReq = true;
  contactor_close_start_ms = millis();  // Start of STATE B closing phase
}

void BmwPhevBattery::PhevOpenContactors(void) {
  logging.println("Opening contactors (0x10B)");
  contactorCloseReq = false;
}

void BmwPhevBattery::HandleIncomingUserRequest(void) {
  if ((userRequestContactorClose == false) && (userRequestContactorOpen == false)) {
    // do nothing
  } else if ((userRequestContactorClose == true) && (userRequestContactorOpen == false)) {
    PhevCloseContactors();
    userRequestContactorClose = false;
  } else if ((userRequestContactorClose == false) && (userRequestContactorOpen == true)) {
    PhevOpenContactors();
    userRequestContactorOpen = false;
  } else {
    // Both flags should never be set together - opening is the safest state
    PhevOpenContactors();
    userRequestContactorClose = false;
    userRequestContactorOpen = false;
    logging.println("Error: contactor close+open both requested. Contactors opened.");
  }
}

void BmwPhevBattery::HandleIncomingInverterRequest(void) {
  InverterContactorCloseRequest.present = datalayer.system.status.inverter_allows_contactor_closing;
  // Detect edge
  if (InverterContactorCloseRequest.previous == false && InverterContactorCloseRequest.present == true) {
    logging.println("Inverter req. to close contactors");
    PhevCloseContactors();
  } else if (InverterContactorCloseRequest.previous == true && InverterContactorCloseRequest.present == false) {
    logging.println("Inverter req. to open contactors");
    PhevOpenContactors();
  }  // else: do nothing
  InverterContactorCloseRequest.previous = InverterContactorCloseRequest.present;
}
void BmwPhevBattery::parseDTCResponse() {
  // Check for negative response
  if (gUDSContext.UDS_buffer[0] == 0x7F) {
    logging.print("DTC request rejected by battery. Reason code: 0x");
    logging.print(gUDSContext.UDS_buffer[2], HEX);
    logging.println();
    datalayer_extended.bmwphev.dtc_read_failed = true;
    return;
  }

  if (gUDSContext.UDS_buffer[0] != 0x59 || gUDSContext.UDS_buffer[1] != 0x02) {
    logging.println("Invalid DTC response header");
    datalayer_extended.bmwphev.dtc_read_failed = true;
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
    validDtcCount++;  //  Increment only for valid DTCs
  }

  datalayer_extended.bmwphev.dtc_count = validDtcCount;  //  Store actual count

  logging.print("Total valid DTCs: ");
  logging.println(validDtcCount);

  datalayer_extended.bmwix.dtc_last_read_millis = millis();  //Note we re-use ix struct to save memory
  datalayer_extended.bmwphev.dtc_read_failed = false;
}
void BmwPhevBattery::processCellVoltages() {
  const int startByte = 3;     // Start reading at byte 3
  const int numVoltages = 96;  // Number of cell voltage values to process
  int voltage_index = 0;       // Starting index for the destination array

  // Loop through 96 voltage values
  for (int i = 0; i < numVoltages; i++) {
    // Calculate the index of the first and second bytes in the input array
    int byteIndex = startByte + (i * 2);

    // Combine two bytes to form a 16-bit value
    uint16_t voltageRaw = (gUDSContext.UDS_buffer[byteIndex] << 8) | gUDSContext.UDS_buffer[byteIndex + 1];

    // Store the result in the destination array
    datalayer.battery.status.cell_voltages_mV[voltage_index] = voltageRaw;

    // Increment the destination array index
    voltage_index++;
  }
}

void BmwPhevBattery::wake_battery_via_canbus() {
  //TJA1055 transceiver remote wake requires pulses on the bus of
  // Dominant for at least ~7 µs (min) and at most ~38 µs (max)
  // Followed by a Recessive interval of at least ~3 µs (min) and at most ~10 µs (max)
  // Then a second dominant pulse of similar timing.
  static unsigned long wakeup_start_time = 0;
  static bool waiting_for_completion = false;

  if (!waiting_for_completion) {
    logging.println("Setting Canbus to 100kbps");
    change_can_speed(CAN_Speed::CAN_SPEED_100KBPS);
    transmit_can_frame(&BMW_PHEV_BUS_WAKEUP_REQUEST);
    transmit_can_frame(&BMW_PHEV_BUS_WAKEUP_REQUEST);
    logging.println("Sent magic wakeup packet to SME at 100kbps");
    wakeup_start_time = millis();
    waiting_for_completion = true;
    return;
  }

  if (millis() - wakeup_start_time >= 50) {
    logging.println("Resetting Canbus speed");
    change_can_speed(CAN_Speed::CAN_SPEED_500KBPS);
    logging.println("CAN speed restored, ready for operation");
    waiting_for_completion = false;
  }
}

void BmwPhevBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = avg_soc_state;
  datalayer.battery.status.voltage_dV = battery_voltage;
  datalayer.battery.status.current_dA = battery_current / 100;  // battery_current is in mA, convert to dA
  datalayer.battery.info.total_capacity_Wh = (battery_energy_content_maximum_kWh * 1000);  // Convert kWh to Wh
  datalayer.battery.status.remaining_capacity_Wh = battery_predicted_energy_charge_condition;
  datalayer.battery.status.soh_pptt = min_soh_state;
  datalayer.battery.status.max_discharge_power_W = battery_BEV_available_power_longterm_discharge;

  //datalayer.battery.status.max_charge_power_W = 3200; //10000; //Aux HV Port has 100A Fuse  Moved to Ramping

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > user_set_rampdown_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        battery_BEV_available_power_longterm_charge *
        (1 - (datalayer.battery.status.real_soc - user_set_rampdown_SOC) / (10000.0 - user_set_rampdown_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = battery_BEV_available_power_longterm_charge;
  }

  datalayer.battery.status.temperature_min_dC = battery_temperature_min * 10;  // Add a decimal
  datalayer.battery.status.temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  //Check stale values. As values dont change much during idle only consider stale if both parts of this message freeze.
  bool isMinCellVoltageStale =
      isStale(min_cell_voltage, datalayer.battery.status.cell_min_voltage_mV, min_cell_voltage_lastchanged);
  bool isMaxCellVoltageStale =
      isStale(max_cell_voltage, datalayer.battery.status.cell_max_voltage_mV, max_cell_voltage_lastchanged);

  if (isMinCellVoltageStale && isMaxCellVoltageStale &&
      battery_current != 0) {                             //Ignore stale values if there is no current flowing
    datalayer.battery.status.cell_min_voltage_mV = 9999;  //Stale values force stop
    datalayer.battery.status.cell_max_voltage_mV = 9999;  //Stale values force stop
    set_event(EVENT_STALE_VALUE, 0);
    logging.println("Stale Min/Max voltage values detected during charge/discharge sending - 9999mV...");
  } else {

    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;  //Value is alive
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;  //Value is alive
  }

  datalayer.battery.info.max_design_voltage_dV = max_design_voltage;

  datalayer.battery.info.min_design_voltage_dV = min_design_voltage;

  datalayer_extended.bmwphev.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  //datalayer_extended.bmwphev.hvil_status = hvil_status; //TODO, not implemented

  datalayer_extended.bmwphev.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwphev.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwphev.balancing_status = balancing_status;

  // Map PHEV balancing_status raw value to the shared datalayer enum so MQTT picks it up.
  // PHEV values (from BMW-PHEV-HTML.h): 0=inactive not needed, 1=active, 2=not resting,
  // 3=inactive, 4=unknown/qualifier invalid.
  switch (balancing_status) {
    case 1:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_ACTIVE;
      break;
    case 0:  // Inactive - not needed
    case 3:  // Inactive
      datalayer.battery.status.balancing_status = BALANCING_STATUS_READY;
      break;
    case 2:  // Cells not at rest — balancing blocked until they settle
      datalayer.battery.status.balancing_status = BALANCING_STATUS_BLOCKED;
      break;
    case 4:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_ERROR;
      break;
    default:
      datalayer.battery.status.balancing_status = BALANCING_STATUS_UNKNOWN;
      break;
  }

  datalayer_extended.bmwphev.battery_voltage_after_contactor = battery_voltage_after_contactor;

  // Update webserver datalayer

  datalayer_extended.bmwphev.ST_iso_ext = battery_status_error_isolation_external_Bordnetz;
  datalayer_extended.bmwphev.ST_iso_int = battery_status_error_isolation_internal_Bordnetz;
  datalayer_extended.bmwphev.ST_valve_cooling = battery_status_valve_cooling;
  datalayer_extended.bmwphev.ST_interlock = battery_status_error_locking;
  datalayer_extended.bmwphev.ST_precharge = battery_status_precharge_locked;
  datalayer_extended.bmwphev.ST_DCSW = battery_status_disconnecting_switch;
  datalayer_extended.bmwphev.ST_EMG = battery_status_emergency_mode;
  datalayer_extended.bmwphev.ST_WELD = battery_status_error_disconnecting_switch;
  datalayer_extended.bmwphev.ST_isolation = battery_status_warning_isolation;
  datalayer_extended.bmwphev.ST_cold_shutoff_valve = battery_status_cold_shutoff_valve;
  datalayer_extended.bmwphev.iso_safety_int_kohm = iso_safety_int_kohm;
  datalayer_extended.bmwphev.iso_safety_ext_kohm = iso_safety_ext_kohm;
  datalayer_extended.bmwphev.iso_safety_trg_kohm = iso_safety_trg_kohm;
  datalayer_extended.bmwphev.iso_safety_ext_plausible = iso_safety_ext_plausible;
  datalayer_extended.bmwphev.iso_safety_int_plausible = iso_safety_int_plausible;
  datalayer_extended.bmwphev.iso_safety_kohm = iso_safety_kohm;
  datalayer_extended.bmwphev.iso_safety_kohm_quality = iso_safety_kohm_quality;
  datalayer_extended.bmwphev.battery_request_open_contactors = battery_request_open_contactors;
  datalayer_extended.bmwphev.battery_request_open_contactors_instantly = battery_request_open_contactors_instantly;
  datalayer_extended.bmwphev.battery_request_open_contactors_fast = battery_request_open_contactors_fast;
  datalayer_extended.bmwphev.battery_charging_condition_delta = battery_charging_condition_delta;

  if (pack_limit_info_available) {
    // If we have pack limit data from battery - override the defaults to suit
    datalayer.battery.info.max_design_voltage_dV = max_design_voltage;
    datalayer.battery.info.min_design_voltage_dV = min_design_voltage;
  }
}
void BmwPhevBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  //battery_awake = true; //look for specific messages
  switch (rx_frame.ID) {
    case 0x112:  //BMS [20ms] Status Of High-Voltage Battery 2
      battery_awake = true;
      battery_current = ((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) - 8192) * 10;  //deciAmps to milliAmps
      battery_request_open_contactors = (rx_frame.data.u8[5] & 0xC0) >>
                                        6;  //00 Keine Aussage möglich   01 nicht aktiv    10 aktiv   11 Signal ungültig
      battery_request_open_contactors_instantly = (rx_frame.data.u8[6] & 0x03);
      battery_request_open_contactors_fast = (rx_frame.data.u8[6] & 0x0C) >> 2;
      battery_charging_condition_delta = (rx_frame.data.u8[6] & 0xF0) >> 4;
      //battery_DC_link_voltage = rx_frame.data.u8[7];
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //This message is only sent if 30C (Wakeup pin on battery) is energized with 12V
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x239:                                                                                      //BMS [200ms]
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery - 2
      battery_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x431:  //BMS [200ms] Data High-Voltage Battery Unit
      battery_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery_energy_content_maximum_kWh = (((rx_frame.data.u8[6] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
      break;
    case 0x432:  //BMS [200ms] SOC% info
      battery_awake = true;
      battery_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery_display_SOC = rx_frame.data.u8[4];
      break;
    case 0x607:  //SME responds to UDS requests on 0x607
    {
      // UDS Multi Frame vars - Top nibble indicates Frame Type: SF (0), FF (1), CF (2), FC (3)
      // Extended addressing => data[0] is ext address, data[1] is PCI
      //uint8_t extAddr = rx_frame.data.u8[0];  // e.g., 0xF1
      uint8_t pciByte = rx_frame.data.u8[1];  // e.g., 0x10, 0x21, etc.
      uint8_t pciType = pciByte >> 4;         // top nibble => 0=SF,1=FF,2=CF,3=FC
      uint8_t pciLower = pciByte & 0x0F;      // bottom nibble => length nibble or sequence

      switch (pciType) {
        case 0x0: {
          // Single Frame reponse
          // SF payload length is in pciLower
          //uint8_t sfLength = pciLower;
          //uint8_t moduleID = rx_frame.data.u8[5];

          if (rx_frame.DLC == 8 && rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xDD &&
              rx_frame.data.u8[4] == 0xC4) {  // SOC%
            avg_soc_state = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
          }
          if (rx_frame.DLC == 6 && rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xDD &&
              rx_frame.data.u8[4] == 0x7B) {  // SOH%
            min_soh_state = (rx_frame.data.u8[5]) * 100;
          }

          if (rx_frame.DLC == 8 && rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xD6 &&
              rx_frame.data.u8[4] == 0xD9) {                                     // Isolation Reading 2
            iso_safety_kohm = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);  //STAT_R_ISO_ROH_01_WERT
            iso_safety_kohm_quality =
                (rx_frame.data.u8[7]);  //STAT_R_ISO_ROH_QAL_01_INFO Quality of measurement 0-21 (higher better)
          }

          if (rx_frame.DLC == 7 && rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xDD &&
              rx_frame.data.u8[4] == 0xB4) {  //Main Battery Voltage (Pre Contactor)
            battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
          }

          if (rx_frame.DLC == 7 && rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xDD &&
              rx_frame.data.u8[4] == 0x66) {  //Main Battery Voltage (Post Contactor)
            battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
          }

          if (rx_frame.DLC == 7 && rx_frame.data.u8[1] == 0x05 && rx_frame.data.u8[2] == 0x71 &&
              rx_frame.data.u8[3] == 0x03 &&
              rx_frame.data.u8[4] == 0xAD) {  //Balancing Status  01 Active 03 Not Active    7DLC F1 05 71 03 AD 6B 01
            balancing_status = (rx_frame.data.u8[6]);
          }

          // Single-frame DTC response ($19 -> $59), e.g. F1 03 59 02 FF when few/no DTCs are
          // stored. The multi-frame path (FF/CF) only triggers when the list is long, so without
          // this branch a short/empty DTC reply was silently dropped and never parsed.
          if (rx_frame.data.u8[2] == 0x59 && rx_frame.data.u8[3] == 0x02) {
            uint8_t sfLen = pciLower;  // SF payload length (e.g. 3 for 59 02 FF)
            memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
            if (sfLen > 0 && sfLen <= 6) {
              memcpy(gUDSContext.UDS_buffer, &rx_frame.data.u8[2], sfLen);
              gUDSContext.UDS_bytesReceived = sfLen;
              gUDSContext.UDS_inProgress = false;
              parseDTCResponse();
            }
          }

          break;
        }
        case 0x1: {
          // total length = (pciLower << 8) + data[2]
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
            moduleID = rx_frame.data.u8[5];  // 0xA5, 0x69, 0xA0, etc.
          }
#if defined(UDS_LOG)
          logging.print("FF arrived! moduleID=0x");
          logging.print(moduleID, HEX);
          logging.print(", totalLength=");
          logging.println(totalLength);
#endif  //  UDS_LOG

          // Start the multi-frame
          startUDSMultiFrameReception(totalLength, moduleID);
          gUDSContext.receivedInBatch = 0;  // Reset batch count
          // The FF payload is at data[3..7] (5 bytes) for an 8-byte CAN frame in extended addressing
          const uint8_t* ffPayload = &rx_frame.data.u8[3];
          uint8_t ffPayloadSize = 5;
          storeUDSPayload(ffPayload, ffPayloadSize);

#if defined(UDS_LOG)
          logging.print("After FF, UDS_bytesReceived=");
          logging.println(gUDSContext.UDS_bytesReceived);
#endif  //  UDS_LOG
#if defined(UDS_LOG)
          logging.println("Requesting continue frame...");
#endif  //  UDS_LOG
          transmit_can_frame(&BMW_6F1_REQUEST_CONTINUE_MULTIFRAME);
          break;
        }

        case 0x2: {
          // The sequence number is in (data[0] & 0x0F), but we often don’t need it if frames are in order.
          // Make sure we *are* in progress
          if (!gUDSContext.UDS_inProgress) {
// Unexpected CF. Possibly ignore or reset.
#if defined(UDS_LOG)
            uint8_t seq = pciByte & 0x0F;
            logging.print("Unexpected CF --- seq=0x");
            logging.print(seq, HEX);
            logging.print(" for moduleID=0x");
            logging.println(gUDSContext.UDS_moduleID, HEX);
#endif  // UDS_LOG
            return;
          }
#if defined(UDS_LOG)
          uint8_t seq = pciByte & 0x0F;
          logging.print("CF seq=0x");
          logging.print(seq, HEX);
          logging.print("CF pcibyte=0x");
          logging.print(pciByte, HEX);
          logging.print(" for moduleID=0x");
          logging.println(gUDSContext.UDS_moduleID, HEX);
#endif  // UDS_LOG

          storeUDSPayload(&rx_frame.data.u8[2], 6);
          // Increment batch counter
          gUDSContext.receivedInBatch++;
#if defined(UDS_LOG)
          logging.print("After CF seq=0x");
          logging.print(seq, HEX);
          logging.print(", moduleID=0x");
          logging.print(gUDSContext.UDS_moduleID, HEX);
          logging.print(", UDS_bytesReceived=");
          logging.println(gUDSContext.UDS_bytesReceived);
#endif  // UDS_LOG

          // Check if the batch is complete
          if (gUDSContext.receivedInBatch >= 3) {  //BMW PHEV Using batch size of 3 in continue message
                                                   // Send the next Flow Control
#if defined(UDS_LOG)
            logging.println("Batch Complete - Requesting continue frame...");
#endif  // UDS_LOG
            transmit_can_frame(&BMW_6F1_REQUEST_CONTINUE_MULTIFRAME);
            gUDSContext.receivedInBatch = 0;  // Reset batch count
#if defined(UDS_LOG)
            logging.println("Sent FC for next batch of 3 frames.");
#endif  // UDS_LOG
          }

          break;
        }

        case 0x3: {
          // Flow Control Frame from ECU -> Tester (rare in a typical request/response flow)
          // Typically we only *send* FC. If the ECU sends one, parse or ignore here.
          break;
        }
      }

      // Optionally, check if message is complete
      if (isUDSMessageComplete()) {
        // We have a complete UDS/ISO-TP response in gUDSContext.UDS_buffer
#if defined(UDS_LOG)
        logging.print("UDS message complete for module ID 0x");
        logging.println(gUDSContext.UDS_moduleID, HEX);

        logging.print("Total bytes: ");
        logging.println(gUDSContext.UDS_bytesReceived);

        logging.print("Received data: ");
        for (uint16_t i = 0; i < gUDSContext.UDS_bytesReceived; i++) {
          // Optional leading zero for single-digit hex
          if (gUDSContext.UDS_buffer[i] < 0x10) {
            logging.print("0");
          }
          logging.print(gUDSContext.UDS_buffer[i], HEX);
          logging.print(" ");
        }
        logging.println();  // new line at the end
#endif                      // DEBUG_LOG

        //Cell Voltages
        if (gUDSContext.UDS_moduleID == 0xA5) {  //We have a complete set of cell voltages - pass to data layer
#if defined(UDS_LOG)
          logging.printf("Received cell voltage data");
#endif  // DEBUG_LOG

          processCellVoltages();
        }
        //Current measurement
        if (gUDSContext.UDS_moduleID == 0x69) {  //Current (32bit mA?  negative = discharge)

          //Battery current now reading from 0x112 message
          // battery_current = ((int32_t)((gUDSContext.UDS_buffer[3] << 24) | (gUDSContext.UDS_buffer[4] << 16) |
          //                              (gUDSContext.UDS_buffer[5] << 8) | gUDSContext.UDS_buffer[6])) *
          //                   0.1;
          // logging.printf("Received current/amps measurement data: %d\n", battery_current);
          // logging.printf(" - ");

          for (uint16_t i = 0; i < gUDSContext.UDS_bytesReceived; i++) {
            // Optional leading zero for single-digit hex
            if (gUDSContext.UDS_buffer[i] < 0x10) {
              //logging.printf("0");
            }
            //logging.print(gUDSContext.UDS_buffer[i], HEX);
            //logging.printf(" ");
          }
          //logging.println("");  // new line at the end
        }

        //Cell Min/Max
        if (gUDSContext.UDS_moduleID == 0xA0) {
          uint16_t min_voltage_raw = (gUDSContext.UDS_buffer[9] << 8 | gUDSContext.UDS_buffer[10]);
          uint16_t max_voltage_raw = (gUDSContext.UDS_buffer[11] << 8 | gUDSContext.UDS_buffer[12]);

          // Check combined 16-bit values instead of individual bytes
          if (min_voltage_raw != 0xFFFF && min_voltage_raw != 0x0000 && max_voltage_raw != 0xFFFF &&
              max_voltage_raw != 0x0000) {
            uint16_t new_min_voltage = min_voltage_raw / 10;
            uint16_t new_max_voltage = max_voltage_raw / 10;

            // Update timestamps when values change
            if (new_min_voltage != min_cell_voltage) {
              min_cell_voltage_lastchanged = millis();
            }
            if (new_max_voltage != max_cell_voltage) {
              max_cell_voltage_lastchanged = millis();
            }

            min_cell_voltage = new_min_voltage;
            max_cell_voltage = new_max_voltage;

          } else {
            logging.println("Cell Min Max Invalid 65535 or 0...");
          }
        }

        // DTC Response ($19 returns $02)
        if (gUDSContext.UDS_moduleID == 0x02) {  //  Changed from 0x02 to 0x59
          logging.println("=== DTC Response Received ===");
          logging.print("Total bytes: ");
          logging.println(gUDSContext.UDS_bytesReceived);

          parseDTCResponse();
        }
        if (gUDSContext.UDS_moduleID == 0x7E) {  // Voltage Limits
          max_design_voltage = (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]) / 10;
          min_design_voltage = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]) / 10;
          pack_limit_info_available = true;
        }

        if (gUDSContext.UDS_moduleID == 0x7D) {  // Current Limits
          allowable_charge_amps = (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]) / 10;
          allowable_discharge_amps = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]) / 10;
        }

        if (gUDSContext.UDS_moduleID == 0x90) {  // Paired VIN
          for (int i = 0; i < 17; i++) {
            paired_vin[i] = gUDSContext.UDS_buffer[4 + i];
          }
        }
        if (gUDSContext.UDS_moduleID == 0x6A) {  // Iso Reading 1
          iso_safety_int_kohm =
              (gUDSContext.UDS_buffer[7] << 8 | gUDSContext.UDS_buffer[8]);  //STAT_ISOWIDERSTAND_INT_WERT
          iso_safety_ext_kohm =
              (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]);  //STAT_ISOWIDERSTAND_EXT_STD_WERT
          iso_safety_trg_kohm = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]);
          iso_safety_ext_plausible = gUDSContext.UDS_buffer[9];   //STAT_ISOWIDERSTAND_EXT_TRG_PLAUS
          iso_safety_trg_plausible = gUDSContext.UDS_buffer[10];  //STAT_ISOWIDERSTAND_EXT_TRG_WERT
          iso_safety_int_plausible = gUDSContext.UDS_buffer[11];  //STAT_ISOWIDERSTAND_EXT_TRG_WERT
        }
      }

      break;
    }
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
      battery_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery_request_service = (rx_frame.data.u8[2] & 0x03);
      battery_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      //battery_temperature_HV = (rx_frame.data.u8[4] - 50);
      //battery_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      if (rx_frame.data.u8[6] > 0 && rx_frame.data.u8[6] < 255) {
        battery_temperature_min = (rx_frame.data.u8[6] - 50);
      } else {
        logging.println("Pre parsed Cell Temp Min is Invalid ");
      }
      if (rx_frame.data.u8[7] > 0 && rx_frame.data.u8[7] < 255) {
        battery_temperature_max = (rx_frame.data.u8[7] - 50);
      } else {
        logging.println("Pre parsed Cell Temp Max is Invalid ");
      }

      break;
    default:
      break;
  }
}

void BmwPhevBattery::transmit_can(unsigned long currentMillis) {
  // Timeout check for stuck UDS transfers
  if (gUDSContext.UDS_inProgress) {
    if (currentMillis - gUDSContext.UDS_lastFrameMillis > 2000) {  // 2 second timeout
      logging.println("UDS transfer timeout - aborting");
      gUDSContext.UDS_inProgress = false;
      gUDSContext.UDS_bytesReceived = 0;
    }
  }
  if (battery_awake) {
    // Update requests from webserver datalayer
    if (datalayer_extended.bmwphev.UserRequestDTCreset) {
      logging.println("User requested DTC reset");
      transmit_can_frame(&BMWPHEV_6F1_REQUEST_CLEAR_DTC);  // Send DTC erase command
      datalayer_extended.bmwphev.UserRequestDTCreset = false;
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
    }
    if (datalayer_extended.bmwphev.UserRequestBMSReset) {
      logging.println("User requested SME reset");
      transmit_can_frame(&BMW_6F1_REQUEST_HARD_RESET);  // Send SME reset command
      datalayer_extended.bmwphev.UserRequestBMSReset = false;
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
    }
    if (datalayer_extended.bmwphev.UserRequestIsolationTest) {
      logging.println("User requested isolation test");
      transmit_can_frame(&BMWPHEV_6F1_REQUEST_ISOLATION_TEST);  // Start isolation test routine (0xAD61)
      datalayer_extended.bmwphev.UserRequestIsolationTest = false;
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
    }

    // One-shot at boot: explicitly stop the balancing routine (UDS stopRoutine 0x31 02 AD 6B).
    // A startRoutine latches inside the SME and keeps running until stopped or the SME power-cycles.
    // Earlier firmware sent startRoutine unconditionally every 10s, so the routine can be left
    // latched ON - causing autonomous balancing + precharge block once the cells reach rest (~10 min).
    // Clearing it once on every boot guarantees we start from a known "not balancing" state.
    if (!phev_balancing_stop_sent && !datalayer.battery.settings.user_requests_balancing) {
      logging.println("Clearing any latched balancing routine in SME (stopRoutine)");
      transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_STOP);
      phev_balancing_stop_sent = true;
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
    }

    // Pre-close balancing-stop phase. When a contactor close is requested we send several guarded
    // stopRoutine frames (one per UDS-guard window) to make sure balancing is cancelled in the SME
    // before 0x10B is allowed to close. Each send silences the UDS polls so it isn't clobbered.
    if (phev_pre_close_stops_remaining > 0 &&
        (phev_pre_close_stop_last_ms == 0 ||
         currentMillis - phev_pre_close_stop_last_ms >= PHEV_PRE_CLOSE_STOP_INTERVAL_MS)) {
      logging.println("Pre-close: sending balancing stopRoutine");
      transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_STOP);
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
      phev_pre_close_stop_last_ms = currentMillis;
      phev_pre_close_stops_remaining--;
    }

    // Balancing start/stop, sent as a guarded burst on REQUEST CHANGE (like the pre-close burst above),
    // not in a timed loop - a single periodic send was getting missed by the SME. When the user
    // toggles balancing, fire several guarded frames of the new desired state (startRoutine when
    // requested, stopRoutine when cancelled), one per UDS-guard window, so the latching routine takes.
    // NOTE: balancing only works with contactors OPEN and it BLOCKS contactor close while active.
    if (datalayer.battery.settings.user_requests_balancing != phev_last_balancing_request) {
      phev_last_balancing_request = datalayer.battery.settings.user_requests_balancing;
      phev_balancing_burst_start = datalayer.battery.settings.user_requests_balancing;
      phev_balancing_bursts_remaining = PHEV_BALANCING_BURST_COUNT;
      phev_balancing_burst_last_ms = 0;  // 0 = send the first frame immediately
    }
    if (phev_balancing_bursts_remaining > 0 &&
        (phev_balancing_burst_last_ms == 0 ||
         currentMillis - phev_balancing_burst_last_ms >= PHEV_BALANCING_BURST_INTERVAL_MS)) {
      if (phev_balancing_burst_start) {
        logging.println("Balancing burst: startRoutine");
        transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_START);  // Enable Balancing
      } else {
        logging.println("Balancing burst: stopRoutine");
        transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_STOP);  // Cancel any latched balancing routine
      }
      uds_one_shot_sent_ms = currentMillis;  // Silence polls for UDS_ONE_SHOT_SILENCE_MS
      phev_balancing_burst_last_ms = currentMillis;
      phev_balancing_bursts_remaining--;
    }

    if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
      previousMillis20 = currentMillis;

      if (startup_counter_contactor < 160) {
        startup_counter_contactor++;
      }

      // 0x10B drives the physical contactors. Only command close when:
      //  - GPIO contactor control is disabled (otherwise GPIO drives the contactors directly)
      //  - a close has been requested (user via WebUI or inverter)
      //  - the ~3.2s startup boot delay has elapsed (let the SME wake)
      //  - the 0x53A state machine has finished announcing STATE B (reached ESCALATED/STEADY).
      //    The OEM bus announces "close requested" on 0x53A for ~3.5s BEFORE 0x10B closes; doing
      //    both at once was triggering an intermittent open->close safety warning.
      //  - the pre-close balancing-stop frames have all been sent (phev_pre_close_stops_remaining==0),
      //    so balancing can't be blocking the close.
      bool allow_can_close = (!contactor_control_enabled) && contactorCloseReq && (startup_counter_contactor >= 160) &&
                             (phev_53a_state == PHEV_53A_ESCALATED || phev_53a_state == PHEV_53A_STEADY) &&
                             (phev_pre_close_stops_remaining == 0);
      if (allow_can_close) {
        BMW_10B.data.u8[1] = 0x10;  // Close contactors
        //BMW_10B.data.u8[1] = 0xD0;  // Close contactors v2
      } else {
        BMW_10B.data.u8[1] = 0x00;  // Open / no close request
      }

      BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
      BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

      alive_counter_20ms = increment_alive_counter(alive_counter_20ms);

      //if (datalayer.battery.status.bms_status == FAULT) {  //ALLOW ANY TIME - TEST ONLY
      //}  //If battery is not in Fault mode, allow contactor to close by sending 10B
      //else {

      transmit_can_frame(&BMW_10B);
      //}
    }

    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      // Send 0x12F Terminal Status - counter cycles 0x20->0x2E (15 values)
      BMW_12F.data.u8[1] = 0x20 + alive_counter_100ms;
      BMW_12F.data.u8[0] = calculateCRC(BMW_12F, BMW_12F.DLC, 0x3F);

      transmit_can_frame(&BMW_12F);

      alive_counter_100ms++;
      if (alive_counter_100ms > 14) {  // Reset after 14 (0x2E is 0x20 + 14)
        alive_counter_100ms = 0;
      }

      // Beta CAN-based contactor close - evaluate inverter/user requests (edge detected)
      HandleIncomingInverterRequest();
      HandleIncomingUserRequest();
    }
    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;
      // Don't start a new UDS request while a multi-frame response is still being reassembled -
      // an incoming First Frame resets gUDSContext and clobbers the in-progress response (this is
      // why long multi-frame DTC reads never completed). A timeout prevents a lost frame from
      // wedging polling permanently.
      // Also pause during the one-shot silence window (Clear DTC / BMS Reset) so the response
      // has time to arrive before the next request is sent.
      bool uds_poll_allowed =
          !gUDSContext.UDS_inProgress && (currentMillis - uds_one_shot_sent_ms > UDS_ONE_SHOT_SILENCE_MS);
      if (gUDSContext.UDS_inProgress && (currentMillis - gUDSContext.UDS_lastFrameMillis > UDS_REASSEMBLY_TIMEOUT_MS)) {
        gUDSContext.UDS_inProgress = false;  // Abandon stalled reassembly
        uds_poll_allowed = (currentMillis - uds_one_shot_sent_ms > UDS_ONE_SHOT_SILENCE_MS);
      }
      if (uds_poll_allowed) {
        uds_fast_req_id_counter = increment_uds_req_id_counter(
            uds_fast_req_id_counter, numFastUDSreqs);  //Loop through and send a different UDS request each cycle
        transmit_can_frame(UDS_REQUESTS_FAST[uds_fast_req_id_counter]);
      }

      // Beta - stream 0x53A associated vehicle/contactor STATE indication.
      // 0x53A does NOT drive the contactors (0x10B does - see INFO), but the OEM announces a
      // close on 0x53A before 0x10B closes. We replay that handshake as a small state machine so
      // the SME sees the expected open->close sequence (avoids an intermittent safety warning):
      //   STATE A IDLE      byte5=0x00 byte6=0x01  default, contactors not requested
      //   STATE B CLOSE_REQ byte5=0x80 byte6=0x01  held ~3.5s before 0x10B is allowed to close
      //   STATE C ESCALATED byte5=0x84 byte6=0x01  single transient frame
      //   STATE D STEADY    byte5=0x84 byte6=0x00  held while contactors closed
      // 0x10B is gated on this reaching ESCALATED/STEADY (see 20ms block).
      if (!contactorCloseReq) {
        phev_53a_state = PHEV_53A_IDLE;
      } else {
        unsigned long close_elapsed = currentMillis - contactor_close_start_ms;
        switch (phev_53a_state) {
          case PHEV_53A_IDLE:
            phev_53a_state = PHEV_53A_CLOSE_REQ;  // close just requested - begin announcing STATE B
            break;
          case PHEV_53A_CLOSE_REQ:
            if (close_elapsed >= CONTACTOR_CLOSE_REQUEST_DURATION_MS) {
              phev_53a_state = PHEV_53A_ESCALATED;  // B hold done - escalate (single transient)
            }
            break;
          case PHEV_53A_ESCALATED:
            phev_53a_state = PHEV_53A_STEADY;  // escalation is a single frame - settle to steady
            break;
          case PHEV_53A_STEADY:
            break;  // hold steady while closed
        }
      }

      switch (phev_53a_state) {
        case PHEV_53A_IDLE:
          BMW_53A.data.u8[5] = 0x00;
          BMW_53A.data.u8[6] = 0x01;
          break;
        case PHEV_53A_CLOSE_REQ:
          BMW_53A.data.u8[5] = 0x80;
          BMW_53A.data.u8[6] = 0x01;
          break;
        case PHEV_53A_ESCALATED:
          BMW_53A.data.u8[5] = 0x84;
          BMW_53A.data.u8[6] = 0x01;
          break;
        case PHEV_53A_STEADY:
          BMW_53A.data.u8[5] = 0x84;
          BMW_53A.data.u8[6] = 0x00;
          break;
      }
      transmit_can_frame(&BMW_53A);
    }
    // Send 1000ms CAN Message
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      // Same guard as the fast poll: skip starting a new request while a multi-frame response is
      // still being reassembled, so the slow DTC read isn't clobbered (or doesn't clobber others).
      // Also respects the one-shot silence window.
      if (!gUDSContext.UDS_inProgress && (currentMillis - uds_one_shot_sent_ms > UDS_ONE_SHOT_SILENCE_MS)) {
        uds_slow_req_id_counter = increment_uds_req_id_counter(
            uds_slow_req_id_counter, numSlowUDSreqs);  //Loop through and send a different UDS request each cycle
        logging.print("Sending UDS_SLOW: ");
        logging.println(getUDSRequestName(UDS_REQUESTS_SLOW[uds_slow_req_id_counter]));
        transmit_can_frame(UDS_REQUESTS_SLOW[uds_slow_req_id_counter]);
      }

      // Vehicle environment frames the SME expects (silence "No message" DTCs).
      // Only STATIC frames are sent here. Frames that require a rolling counter/CRC on the real
      // bus (0x1A1, 0x433) are intentionally NOT transmitted yet - see VEHICLE TX MAP.
      transmit_can_frame(&BMW_3A0);  // CAD408 Vehicle condition (BDC) - static
      transmit_can_frame(&BMW_3CA);  // CAD429 Driving-info forecast (KOMBI) - static
      transmit_can_frame(&BMW_3E8);  // CAD405 OBD diagnosis, engine control (EME) - static

      // 0x328 Relative time / clock (KOMBI) - CAD402. Ported from the i3 implementation.
      // byte0-3 = T_SEC_COU_REL: seconds since system start (LE). byte4-5 = T_DAY_COU_ABSL: absolute
      // day counter (LE), day 1 = 1.1.2000. A live, incrementing clock keeps the SME from treating the
      // bus as stale - a frozen/absent clock is a likely trigger for the idle precharge block.
      BMW_328_seconds++;
      BMW_328.data.u8[0] = (uint8_t)(BMW_328_seconds & 0xFF);
      BMW_328.data.u8[1] = (uint8_t)((BMW_328_seconds >> 8) & 0xFF);
      BMW_328.data.u8[2] = (uint8_t)((BMW_328_seconds >> 16) & 0xFF);
      BMW_328.data.u8[3] = (uint8_t)((BMW_328_seconds >> 24) & 0xFF);
      BMW_328_seconds_to_day++;
      if (BMW_328_seconds_to_day > 86400) {
        BMW_328_days++;
        BMW_328_seconds_to_day = 0;
      }
      BMW_328.data.u8[4] = (uint8_t)(BMW_328_days & 0xFF);
      BMW_328.data.u8[5] = (uint8_t)((BMW_328_days >> 8) & 0xFF);
      transmit_can_frame(&BMW_328);  // CAD402 Relative time / clock (KOMBI) - live counter

      // 0x2CA Ambient temperature (KOMBI) - CAD401. byte0 = (T+40)*2 (0.5C/bit, -40 offset);
      // 0x6E = 15C, a moderate in-range ambient that avoids cold/hot charge derating.
      // Sent static - no rolling counter needed (the 6F/6E variance in the log was measurement noise).
      BMW_2CA.data.u8[0] = 0x6E;  // 15 C ambient
      BMW_2CA.data.u8[1] = 0x6F;
      transmit_can_frame(&BMW_2CA);
    }
    // Send 5000ms CAN Message
    if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
      previousMillis5000 = currentMillis;
    }
    // Send 10000ms CAN Message
    if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
      previousMillis10000 = currentMillis;
      // Balancing is no longer sent here - it is handled as a guarded burst on request change
      // (see "Balancing start/stop" burst near the top of the awake section).
    }
  } else {
    // Battery is asleep - try and wake it every 1 seconds
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;
      logging.println("Battery asleep, sending wakeup packet");
      wake_battery_via_canbus();
    }
  }
}

void BmwPhevBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
