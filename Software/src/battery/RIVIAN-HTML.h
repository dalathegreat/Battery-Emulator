#ifndef _RIVIAN_HTML_H
#define _RIVIAN_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RivianHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    tr_h4(content, TrKey::DRV_VOLTAGE_PRE_CONTACTORS, String(datalayer_extended.rivian.pre_contactor_voltage), " dV");
    tr_h4(content, TrKey::DRV_VOLTAGE_MAIN_CONTACTORS, String(datalayer_extended.rivian.main_contactor_voltage), " dV");
    tr_h4(content, TrKey::DRV_VOLTAGE_REFERENCE, String(datalayer_extended.rivian.voltage_reference), " dV");
    tr_h4(content, TrKey::DRV_VOLTAGE_DCFC_CONTACTORS, String(datalayer_extended.rivian.DCFC_contactor_voltage), " dV");

    tr_h4(content, TrKey::DRV_SOC_MAX, String(datalayer_extended.rivian.battery_SOC_max), " pptt");
    tr_h4(content, TrKey::DRV_SOC_MIN, String(datalayer_extended.rivian.battery_SOC_min), " pptt");

    if (datalayer_extended.rivian.NACS_charger_detected) {
      tr_h4(content, TrKey::DRV_NACS_CHARGER_DETECTED);
    }

    tr_h4_open(content, TrKey::DRV_ISOLATION_MEASUREMENT_ONGOING);
    if (datalayer_extended.rivian.IsolationMeasurementOngoing) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }

    tr_h4_open(content, TrKey::DRV_ISOLATION_STATUS);
    if (datalayer_extended.rivian.isolation_fault_status == 0) {
      content += TR(TrKey::DRV_UNDEFINED);
    } else if (datalayer_extended.rivian.isolation_fault_status == 1) {
      content += TR(TrKey::DRV_STABLE);
    } else if (datalayer_extended.rivian.isolation_fault_status == 2) {
      content += TR(TrKey::DRV_NO_FAULT);
    } else if (datalayer_extended.rivian.isolation_fault_status == 3) {
      content += TR(TrKey::DRV_HIGH_SIDE_FAULT);
    } else if (datalayer_extended.rivian.isolation_fault_status == 4) {
      content += TR(TrKey::DRV_LOW_SIDE_FAULT);
    } else if (datalayer_extended.rivian.isolation_fault_status == 5) {
      content += TR(TrKey::DRV_DUAL_SIDE_FAULT);
    } else if (datalayer_extended.rivian.isolation_fault_status == 6) {
      content += TR(TrKey::DRV_ISO_CIRCUIT_FAILURE);
    } else if (datalayer_extended.rivian.isolation_fault_status == 7) {
      content += TR(TrKey::DRV_ISO_CIRCUIT_CHECK_TIMEOUT);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_INTERLOCK_STATUS);
    if (datalayer_extended.rivian.HVIL == 0) {
      content += TR(TrKey::DRV_NOT_OK);
    } else if (datalayer_extended.rivian.HVIL == 1) {
      content += TR(TrKey::DRV_NOT_OK);
    } else if (datalayer_extended.rivian.HVIL == 2) {
      content += TR(TrKey::DRV_NOT_OK);
    } else if (datalayer_extended.rivian.HVIL == 3) {
      content += TR(TrKey::DRV_OK);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_BMS_STATE);
    if (datalayer_extended.rivian.BMS_state == 0) {
      content += TR(TrKey::DRV_SLEEP);
    } else if (datalayer_extended.rivian.BMS_state == 1) {
      content += TR(TrKey::DRV_STANDBY);
    } else if (datalayer_extended.rivian.BMS_state == 2) {
      content += TR(TrKey::DRV_READY);
    } else if (datalayer_extended.rivian.BMS_state == 3) {
      content += TR(TrKey::DRV_GO);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_CONTACTOR_STATUS);
    if (datalayer_extended.rivian.contactor_state == 0) {
      content += TR(TrKey::DRV_OPEN);
    } else if (datalayer_extended.rivian.contactor_state == 1) {
      content += TR(TrKey::DRV_CLOSED);
    } else if (datalayer_extended.rivian.contactor_state == 2) {
      content += TR(TrKey::DRV_PRECHARGE);
    } else if (datalayer_extended.rivian.contactor_state == 3) {
      content += TR(TrKey::DRV_TURNING_OFF);
    } else if (datalayer_extended.rivian.contactor_state == 4) {
      content += TR(TrKey::DRV_INITIALIZATION);
    } else if (datalayer_extended.rivian.contactor_state == 5) {
      content += TR(TrKey::DRV_FAILURE);
    }
    content += "</h4>";

    tr_h4(content, TrKey::DRV_ACTIVE_ERRORS_FAULTS);

    if (datalayer_extended.rivian.error_relay_open) {
      tr_h4(content, TrKey::DRV_ERROR_RELAY_OPEN);
    }
    if (datalayer_extended.rivian.error_flags_from_BMS & 0x01) {
      tr_h4(content, TrKey::DRV_ERROR_ISOLATION_SINGLE);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x02) >> 1) {
      tr_h4(content, TrKey::DRV_ERROR_ISOLATION_DOUBLE);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x04) >> 2) {
      tr_h4(content, TrKey::DRV_ERROR_EMERGENCY_OFF_CRASH);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x08) >> 3) {
      tr_h4(content, TrKey::DRV_ERROR_EMERGENCY_OFF_PILOT);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x10) >> 4) {
      tr_h4(content, TrKey::DRV_ERROR_EMERGENCY_OFF_REQUEST);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x20) >> 5) {
      tr_h4(content, TrKey::DRV_ERROR_EMERGENCY_OFF);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x40) >> 6) {
      tr_h4(content, TrKey::DRV_ERROR_CONTACTORS_WELDED);
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x80) >> 7) {
      tr_h4(content, TrKey::DRV_ERROR_LIMITED_POWER);
    }

    //HMI errors / status codes, bundle them also under errors

    if ((datalayer_extended.rivian.HMI_part1 & 0x10) >> 4) {
      tr_h4(content, TrKey::DRV_VEHICLE_BATTERY_ISSUE);
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x20) >> 5) {
      tr_h4(content, TrKey::DRV_CRITICAL_BATTERY_ISSUE);
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x40) >> 6) {
      tr_h4(content, TrKey::DRV_AC_PERFORMANCE_LIMITED);
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x80) >> 7) {
      tr_h4(content, TrKey::DRV_DC_PERFORMANCE_LIMITED);
    }
    if (datalayer_extended.rivian.HMI_part2 & 0x01) {
      tr_h4(content, TrKey::DRV_DC_CHARGING_DISABLED);
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x02) >> 1) {
      tr_h4(content, TrKey::DRV_ELECTRIC_HAZARD);
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x04) >> 2) {
      tr_h4(content, TrKey::DRV_FIRE_RISK);
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x08) >> 3) {
      tr_h4(content, TrKey::DRV_VEHICLE_SYSTEM_FAULT);
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x10) >> 4) {
      tr_h4(content, TrKey::DRV_BATTERY_ELECTRIC_MALFUNCTION);
    }

    //Misc
    if (datalayer_extended.rivian.system_safe_state > 1) {
      tr_h4(content, TrKey::DRV_SYSTEM_SAFE_STATE_ACTIVE);
    }
    if (datalayer_extended.rivian.puncture_fault) {
      tr_h4(content, TrKey::DRV_PUNCTURE_FAULT_DETECTED);
    }
    if (datalayer_extended.rivian.liquid_fault) {
      tr_h4(content, TrKey::DRV_LIQUID_FAULT_DETECTED);
    }
    if (datalayer_extended.rivian.contactor_DCFC_welded) {
      tr_h4(content, TrKey::DRV_DCFC_CONTACTOR_WELDED);
    }

    if (datalayer_extended.rivian.slewrate_potential_violation) {
      tr_h4(content, TrKey::DRV_SLEWRATE_POTENTIAL_VIOLATION);
    }
    if (datalayer_extended.rivian.minimum_power_potential_violation) {
      tr_h4(content, TrKey::DRV_MIN_POWER_POTENTIAL_VIOLATION);
    }
    if (datalayer_extended.rivian.operation_limit_violation_warning) {
      tr_h4(content, TrKey::DRV_OPERATION_LIMIT_VIOLATION_WARNING);
    }

    return content;
  }
};

#endif
