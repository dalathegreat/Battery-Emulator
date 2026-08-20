#include "BMW-I3-HTML.h"
#include "../devboard/i18n/tr.h"
#include "BMW-I3-BATTERY.h"

// Helper function for safe array access
static String safeArrayAccess(const String arr[], size_t arrSize, int idx) {
  if (idx >= 0 && static_cast<size_t>(idx) < arrSize) {
    return arr[idx];
  }
  return TR(TrKey::UI_UNKNOWN);
}

String BmwI3HtmlRenderer::get_status_html() {
  String content;

  tr_h4(content, TrKey::DRV_SOC_RAW, String(batt.SOC_raw()));
  tr_h4(content, TrKey::DRV_SOC_DASH, String(batt.SOC_dash()));
  tr_h4(content, TrKey::DRV_SOC_OBD2, String(batt.SOC_OBD2()));
  const String statusText[16] = {TR(TrKey::DRV_NOT_EVALUATED),
                                 "OK",
                                 "Error!",
                                 TR(TrKey::DRV_INVALID_SIGNAL),
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "",
                                 ""};
  tr_h4_colon(content, TrKey::DRV_INTERLOCK, String(safeArrayAccess(statusText, 16, batt.ST_interlock())));
  tr_h4(content, TrKey::DRV_ISOLATION_EXTERNAL, String(safeArrayAccess(statusText, 16, batt.ST_iso_ext())));
  tr_h4(content, TrKey::DRV_ISOLATION_INTERNAL, String(safeArrayAccess(statusText, 16, batt.ST_iso_int())));
  tr_h4(content, TrKey::DRV_ISOLATION, String(safeArrayAccess(statusText, 16, batt.ST_isolation())));
  tr_h4(content, TrKey::DRV_COOLING_VALVE, String(safeArrayAccess(statusText, 16, batt.ST_valve_cooling())));
  tr_h4(content, TrKey::DRV_EMERGENCY, String(safeArrayAccess(statusText, 16, batt.ST_EMG())));
  const String prechargeText[16] = {TR(TrKey::DRV_NOT_EVALUATED),
                                    TR(TrKey::DRV_NOT_ACTIVE_CLOSING_NOT_BLOCKED),
                                    "Error precharge blocked",
                                    TR(TrKey::DRV_INVALID_SIGNAL),
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    ""};
  tr_h4_colon(content, TrKey::DRV_PRECHARGE,
              String(safeArrayAccess(prechargeText, 16, batt.ST_precharge())));  //Still unclear of enum
  const String DCSWText[16] = {TR(TrKey::DRV_CONTACTORS_OPEN),
                               TR(TrKey::DRV_PRECHARGE_ONGOING),
                               TR(TrKey::DRV_CONTACTORS_ENGAGED),
                               TR(TrKey::DRV_INVALID_SIGNAL),
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               ""};
  tr_h4(content, TrKey::DRV_CONTACTOR_STATUS, String(safeArrayAccess(DCSWText, 16, batt.ST_DCSW())));
  const String contText[16] = {TR(TrKey::DRV_CONTACTORS_OK),
                               TR(TrKey::DRV_ONE_CONTACTOR_WELDED),
                               TR(TrKey::DRV_TWO_CONTACTORS_WELDED),
                               TR(TrKey::DRV_INVALID_SIGNAL),
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               "",
                               ""};
  tr_h4(content, TrKey::DRV_CONTACTOR_WELD, String(safeArrayAccess(contText, 16, batt.ST_WELD())));
  const String valveText[16] = {"OK",
                                TR(TrKey::DRV_SHORT_CIRCUIT_GND),
                                TR(TrKey::DRV_SHORT_CIRCUIT_12V),
                                TR(TrKey::DRV_LINE_BREAK),
                                "",
                                "",
                                TR(TrKey::DRV_DRIVER_ERROR),
                                "",
                                "",
                                "",
                                "",
                                "",
                                TR(TrKey::DRV_STUCK),
                                TR(TrKey::DRV_STUCK),
                                "",
                                TR(TrKey::DRV_INVALID_SIGNAL)};
  tr_h4(content, TrKey::DRV_COLD_SHUTOFF_VALVE, String(safeArrayAccess(valveText, 16, batt.ST_cold_shutoff_valve())));
  const String balancingText[16] = {TR(TrKey::DRV_NOT_REQUESTED),
                                    TR(TrKey::DRV_REQUESTED),
                                    TR(TrKey::DRV_STARTING),
                                    TR(TrKey::DRV_EXECUTING),
                                    "4",
                                    "5",
                                    "6",
                                    "7",
                                    "8",
                                    "9",
                                    "10",
                                    "11",
                                    "12",
                                    "13",
                                    "14",
                                    "15"};
  tr_h4(content, TrKey::DRV_BALANCING_STATUS, String(safeArrayAccess(balancingText, 16, batt.ST_balancing_status())));
  tr_h4(content, TrKey::DRV_CHARGE_ABORT_REQUEST, String(batt.get_abort_charging_string()));

  return content;
}
