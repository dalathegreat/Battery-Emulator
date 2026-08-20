#ifndef _CMFA_EV_HTML_H
#define _CMFA_EV_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CmfaEvHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    tr_h4(content, TrKey::DRV_SOC_U, String(datalayer_extended.CMFAEV.soc_u), "percent");
    tr_h4(content, TrKey::DRV_SOC_Z, String(datalayer_extended.CMFAEV.soc_z), "percent");
    tr_h4(content, TrKey::DRV_SOH_AVERAGE, String(datalayer_extended.CMFAEV.soh_average), "pptt");
    tr_h4(content, TrKey::DRV_12V_VOLTAGE, String(datalayer_extended.CMFAEV.lead_acid_voltage), "mV");
    tr_h4(content, TrKey::DRV_HIGHEST_CELL_NUMBER, String(datalayer_extended.CMFAEV.highest_cell_voltage_number));
    tr_h4(content, TrKey::DRV_LOWEST_CELL_NUMBER, String(datalayer_extended.CMFAEV.lowest_cell_voltage_number));
    tr_h4(content, TrKey::DRV_SUM_CELLVOLTAGES, String(datalayer_extended.CMFAEV.average_voltage_of_cells));
    tr_h4(content, TrKey::DRV_MAX_REGEN_POWER, String(datalayer_extended.CMFAEV.max_regen_power));
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_POWER, String(datalayer_extended.CMFAEV.max_discharge_power));
    tr_h4(content, TrKey::DRV_MAX_CHARGE_POWER, String(datalayer_extended.CMFAEV.maximum_charge_power));
    tr_h4(content, TrKey::DRV_SOH_AVAILABLE_POWER, String(datalayer_extended.CMFAEV.SOH_available_power));
    tr_h4(content, TrKey::DRV_SOH_GENERATED_POWER, String(datalayer_extended.CMFAEV.SOH_generated_power));
    tr_h4(content, TrKey::DRV_AVERAGE_TEMPERATURE, String(datalayer_extended.CMFAEV.average_temperature), "dC");
    tr_h4(content, TrKey::DRV_MAXIMUM_TEMPERATURE, String(datalayer_extended.CMFAEV.maximum_temperature), "dC");
    tr_h4(content, TrKey::DRV_MINIMUM_TEMPERATURE, String(datalayer_extended.CMFAEV.minimum_temperature), "dC");
    tr_h4(content, TrKey::DRV_CUMULATIVE_ENERGY_DISCHARGED,
          String(datalayer_extended.CMFAEV.cumulative_energy_when_discharging), "Wh");
    tr_h4(content, TrKey::DRV_CUMULATIVE_ENERGY_CHARGED,
          String(datalayer_extended.CMFAEV.cumulative_energy_when_charging), "Wh");
    tr_h4(content, TrKey::DRV_CUMULATIVE_ENERGY_REGEN, String(datalayer_extended.CMFAEV.cumulative_energy_in_regen),
          "Wh");

    return content;
  }
};

#endif
