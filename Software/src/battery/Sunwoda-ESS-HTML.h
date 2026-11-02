#ifndef _SUNWODA_ESS_HTML_H
#define _SUNWODA_ESS_HTML_H

#include "../datalayer/datalayer.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

/** Extra data parsed from the Sunwoda BCMU CAN protocol that has no datalayer home.
 *  Filled by SunwodaBattery, displayed by SunwodaHtmlRenderer.
 *
 *  Contactor bit meanings come from the "Switch status information" (gIoSwhInfo_51)
 *  entry of the vendor CAN map (0x0C50FF33): bit0 negative contactor, bit1 main
 *  contactor, bit2 precharge contactor, bit8/9 hard contacts 1/2. Safety switch bits
 *  from the same frame: bit0 disconnecting switch, bit1 surge protector.
 */
struct SunwodaExtendedData {
  // Contactor / switch status, 0x0C50FF33
  bool negative_contactor_closed = false;
  bool main_contactor_closed = false;
  bool precharge_contactor_closed = false;
  bool hard_contact_1_closed = false;
  bool hard_contact_2_closed = false;
  bool disconnect_switch_closed = false;
  bool surge_protector_closed = false;
  uint16_t raw_output_switch_status = 0;
  uint16_t raw_input_switch_status = 0;

  // Contactor self-test status, 0x0C50FF36 (0 = no self-test run, 1 = self-test completed)
  uint16_t contactor_selftest_status = 0;

  // System status, gStateInfo_50 (0x0C50FF32). Confirmed from the vendor's BCMU_APP CAN
  // variable map ("Current status" sheet). operatingStatus is the key field for diagnosing
  // why contactors are/aren't closed: the BCMU only closes them once it reaches Running (3),
  // which requires the host to send a Start command (see ID_SYSTEM_CONTROL in Sunwoda-ESS.h).
  uint8_t batteryProtectionStatus = 0;  // 0 Normal, 1 Fully charged, 2 Empty, 3 Standby, 4 Protection
  uint8_t operatingStatus = 0;          // 0 Initialization, 1 Stop, 2 Starting, 3 Running, 4 Stopping, 5 Fault
  uint8_t chargeDischargeStatus = 0;    // 0 Idle, 1 Charging, 2 Discharging
  uint8_t operatingMode = 0;            // 0 Normal, 1 Nuclear capacity, 2 Equalization, 8 Debugging
  uint8_t controlMode = 0;              // 0 Remote, 1 Local

  // Alarm (gAlarmInfo_52, 0x0C50FF34) and Fault (gFaultInfo_53, 0x0C50FF35) words, one per
  // subindex: [0] external 0, [1] external 1, [2] internal 0, [3] internal 1. Bit meanings
  // come from the vendor's BCMU_APP CAN variable map (tools/H102025_P02_BCMU_APP_V1.16_
  // FerroAMP_20210420_4BMU translated.xlsx, "Fault alarm information" sheet) - see the
  // SUNWODA_ALARM_BIT_NAMES/SUNWODA_FAULT_BIT_NAMES tables below.
  uint16_t alarmWords[4] = {0, 0, 0, 0};
  uint16_t faultWords[4] = {0, 0, 0, 0};
  bool alarmActive = false;
  bool faultActive = false;

  // Software/hardware version, raw value = actual version * 10 (e.g. 21 -> v2.1). See the
  // comment above ID_SOFTWARE_VERSION/ID_HARDWARE_VERSION in Sunwoda-ESS.h for how these were
  // identified - best-effort inference, not confirmed from the vendor documentation.
  uint16_t softwareVersion = 0;
  uint16_t can_speed_kbps = 500;  // host bus speed currently in use (auto-detected)
  uint16_t charge_taper_dA = 0;   // charge current cap from the highest-cell taper
  uint16_t hardwareVersion = 0;

  // Individual temperature sensor readings, 0x0C50FF57 (gTempArray_87 - not in either vendor
  // document, identified from a real CAN capture: same 3-values-per-frame/sequential-start-index
  // layout as ID_CELL_VOLTAGE_0 at 0x0C50FF55, with values in the same 0.1 degC units as the min/
  // max readings at 0x0C50FF52, so this is assumed to be a per-sensor breakdown of that summary).
  // Not confirmed against vendor documentation - treat as best-effort.
  static const uint8_t MAX_TEMP_SENSORS = 64;
  int16_t temperatures_dC[MAX_TEMP_SENSORS] = {0};
  uint8_t temp_sensor_count = 0;
};

// Bit names for gAlarmInfo_52 / gFaultInfo_53, indexed [subindex][bit]. nullptr = unused/reserved.
// Source: vendor's BCMU_APP CAN variable map, "Fault alarm information" sheet.
static const char* const SUNWODA_ALARM_BIT_NAMES[4][16] = {
    {"Overcurrent during charging", "Overcurrent discharge", "Battery overvoltage", "Battery undervoltage",
     "High temperature during battery charging", "Low temperature during battery charging",
     "High temperature during battery discharge", "Low temperature during battery discharge",
     "Disconnect switch open", "Reserved switch 1 is off", "Reserved switch 2 disconnected",
     "BCMU high-voltage sampling anomaly", "BCMU temperature sampling anomaly", "BCMU leakage detection failure",
     "Precharge overheating", "Contactor overheating"},
    {"Fuse overheating", "Battery pack failed to start", "Charging stopped", "Battery pack self-protection",
     "Communication CAN bus disconnected with high voltage detection module", nullptr, nullptr, nullptr, nullptr,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"Charging current exceeds limit", "Discharge current exceeds limit", "Severe overcurrent during charging",
     "Severe overcurrent discharge", "Severe high pressure", "Severe low pressure",
     "Severe low temperature during charging", "Severe low temperature discharge", "Voltage consistency decline",
     "Uneven temperature distribution", "Abnormal capacity decay", "Low battery remaining capacity",
     "Battery cell failure", nullptr, nullptr, nullptr},
    {"BMU equalization feedback circuit malfunction", "BMU equalization control circuit malfunction", nullptr,
     nullptr, nullptr, nullptr, "BMU node number duplicate", "BMU software version error", "High voltage warning",
     "Low voltage warning", "Surge protector status", "Equalization voltage is normal", nullptr, nullptr, nullptr,
     nullptr},
};

static const char* const SUNWODA_FAULT_BIT_NAMES[4][16] = {
    {"BMU sampling circuit malfunction", "BMU sampling line disconnected",
     "Internal communication with BMU via CAN bus disconnection",
     "External communication with BSMU CAN bus disconnected", "Current sampling circuit malfunction",
     "Contactor feedback circuit malfunction", "Fuse blows", "Smoke alarm", "Battery pack leakage",
     "The rechargeable battery is severely overheating", "Severe overheating of the discharged battery",
     "Battery cell failure", "Negative contactor malfunction", "Main contactor malfunction",
     "Precharge contactor malfunction", "Instantaneous overcurrent protection during charging"},
    {"Discharge short circuit overcurrent protection", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"The battery pack is severely leaking current", "Negative contactor retest abnormal",
     "Main contactor retest abnormal", "Precharge contactor re-inspection abnormal", nullptr, nullptr, nullptr,
     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"BMU sampling circuit malfunction", "BMU voltage sampling line broken", "BMU temperature sampling line broken",
     nullptr, nullptr, nullptr, nullptr, nullptr, "BMU internal CAN disconnection", nullptr, nullptr, nullptr,
     nullptr, nullptr, nullptr, nullptr},
};

class SunwodaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  SunwodaHtmlRenderer(SunwodaExtendedData* d) : data(d) {}

  // Each pack has its own SunwodaExtendedData, so the page can show pack 2 and 3 as well
  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;

    static const char* openClosed[2] = {"Open", "Closed"};

    content += "<h4>Contactors:</h4>";
    content += "<h4>Negative: " + String(openClosed[data->negative_contactor_closed]) + "</h4>";
    content += "<h4>Main: " + String(openClosed[data->main_contactor_closed]) + "</h4>";
    content += "<h4>Precharge: " + String(openClosed[data->precharge_contactor_closed]) + "</h4>";
    content += "<h4>Hard contact 1: " + String(openClosed[data->hard_contact_1_closed]) + "</h4>";
    content += "<h4>Hard contact 2: " + String(openClosed[data->hard_contact_2_closed]) + "</h4>";

    content += "<h4>Safety switches:</h4>";
    content += "<h4>Disconnecting switch: " + String(openClosed[data->disconnect_switch_closed]) + "</h4>";
    content += "<h4>Surge protector: " + String(openClosed[data->surge_protector_closed]) + "</h4>";

    content += "<h4>Contactor self-test: " +
               String(data->contactor_selftest_status == 1 ? "Completed" : "Not run") + "</h4>";

    static const char* protectionStatusNames[5] = {"Normal", "Fully charged", "Empty", "Standby", "Protection"};
    static const char* operatingStatusNames[6] = {"Initialization", "Stop", "Starting", "Running", "Stopping",
                                                   "Fault"};
    static const char* chargeDischargeStatusNames[3] = {"Idle", "Charging", "Discharging"};

    content += "<h4>System status:</h4>";
    content += "<h4>Battery protection status: " + lookup(protectionStatusNames, 5, data->batteryProtectionStatus) +
               "</h4>";
    content +=
        "<h4>Operating status: " + lookup(operatingStatusNames, 6, data->operatingStatus) + "</h4>";
    content += "<h4>Charge/discharge status: " +
               lookup(chargeDischargeStatusNames, 3, data->chargeDischargeStatus) + "</h4>";
    if (data->operatingMode == 8 && data->operatingStatus != 3) {
      content +=
          "<h4 style='color:#ffb74d;'>The BCMU is in manual (Debugging) mode - contactors were opened by hand "
          "and stay open. Reboot (power-cycle) the BCMU to return to automatic mode.</h4>";
    } else if (data->operatingStatus == 0 || data->operatingStatus == 1) {
      content +=
          "<h4 style='color:#ffb74d;'>The BCMU is not Running - contactors stay open. BCMU V4.04 starts on its "
          "own about 30 s after power-up; if it stays here check the battery switch, precharge wiring and the "
          "alarms below.</h4>";
    }

    content += "<h4>CAN speed: " + String(data->can_speed_kbps) + " kbit/s</h4>";
    content += "<h4>Charge taper cap: " + String(data->charge_taper_dA / 10.0, 1) + " A (on highest cell, target 3.50 V)</h4>";
    content += "<h4>Software version: " + String(data->softwareVersion / 10.0f, 1) + "</h4>";
    content += "<h4>Hardware version: " + String(data->hardwareVersion / 10.0f, 1) + "</h4>";

    content += "<h4>Alarm active: " + String(data->alarmActive ? "Yes" : "No") + "</h4>";
    content += "<h4>Fault active: " + String(data->faultActive ? "Yes" : "No") + "</h4>";

    content += list_active_bits("Active alarms", data->alarmWords, SUNWODA_ALARM_BIT_NAMES);
    content += list_active_bits("Active faults", data->faultWords, SUNWODA_FAULT_BIT_NAMES);

    if (data->temp_sensor_count > 0) {
      int16_t min_dC = data->temperatures_dC[0];
      int16_t max_dC = data->temperatures_dC[0];
      for (uint8_t i = 1; i < data->temp_sensor_count; i++) {
        min_dC = min(min_dC, data->temperatures_dC[i]);
        max_dC = max(max_dC, data->temperatures_dC[i]);
      }
      content += "<h4>Temperature sensors (" + String(data->temp_sensor_count) + "): " +
                 String(min_dC / 10.0f, 1) + " - " + String(max_dC / 10.0f, 1) + " &deg;C</h4>";
    }

    return content;
  }

 private:
  SunwodaExtendedData* data;

  // Returns names[index] if in range, otherwise the raw numeric value as a fallback.
  static String lookup(const char* const* names, uint8_t count, uint8_t index) {
    if (index < count) {
      return String(names[index]);
    }
    return "Unknown (" + String(index) + ")";
  }

  // Renders any set bits as their vendor-documented name; falls back to "wordN.bitM" for
  // bits with no confirmed meaning, and shows "None" when every word is zero.
  static String list_active_bits(const char* title, const uint16_t words[4], const char* const names[4][16]) {
    String content = "<h4>" + String(title) + ":</h4>";
    bool any = false;
    for (uint8_t word = 0; word < 4; word++) {
      for (uint8_t bit = 0; bit < 16; bit++) {
        if ((words[word] & (1u << bit)) == 0) {
          continue;
        }
        any = true;
        const char* name = names[word][bit];
        if (name != nullptr) {
          content += "<h4>- " + String(name) + "</h4>";
        } else {
          content += "<h4>- word" + String(word) + ".bit" + String(bit) + " (unconfirmed)</h4>";
        }
      }
    }
    if (!any) {
      content += "<h4>- None</h4>";
    }
    return content;
  }
};

#endif
