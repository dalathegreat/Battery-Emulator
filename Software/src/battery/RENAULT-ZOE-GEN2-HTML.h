#ifndef _RENAULT_ZOE_GEN2_HTML_H
#define _RENAULT_ZOE_GEN2_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen2HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>soc: " + String(datalayer_extended.zoePH2.battery_soc) + "</h4>";
    tr_h4(content, TrKey::DRV_USABLE_SOC, String(datalayer_extended.zoePH2.battery_usable_soc));
    content += "<h4>soh: " + String(datalayer_extended.zoePH2.battery_soh) + "</h4>";
    tr_h4(content, TrKey::DRV_PACK_VOLTAGE, String(datalayer_extended.zoePH2.battery_pack_voltage));
    tr_h4(content, TrKey::DRV_MAX_CELL_VOLTAGE, String(datalayer_extended.zoePH2.battery_max_cell_voltage));
    tr_h4(content, TrKey::DRV_MIN_CELL_VOLTAGE, String(datalayer_extended.zoePH2.battery_min_cell_voltage));
    content += "<h4>12v: " + String(datalayer_extended.zoePH2.battery_12v) + "</h4>";
    tr_h4(content, TrKey::DRV_AVG_TEMP, String(datalayer_extended.zoePH2.battery_avg_temp));
    tr_h4(content, TrKey::DRV_MIN_TEMP, String(datalayer_extended.zoePH2.battery_min_temp));
    tr_h4(content, TrKey::DRV_MAX_TEMP, String(datalayer_extended.zoePH2.battery_max_temp));
    tr_h4(content, TrKey::DRV_MAX_POWER, String(datalayer_extended.zoePH2.battery_max_power));
    tr_h4_colon(content, TrKey::DRV_INTERLOCK, String(datalayer_extended.zoePH2.battery_interlock));
    content += "<h4>kwh: " + String(datalayer_extended.zoePH2.battery_kwh) + "</h4>";
    content += "<h4>current: " + String(datalayer_extended.zoePH2.battery_current) + "</h4>";
    tr_h4(content, TrKey::DRV_CURRENT_OFFSET, String(datalayer_extended.zoePH2.battery_current_offset));
    tr_h4(content, TrKey::DRV_MAX_GENERATED, String(datalayer_extended.zoePH2.battery_max_generated));
    tr_h4(content, TrKey::DRV_MAX_AVAILABLE, String(datalayer_extended.zoePH2.battery_max_available));
    tr_h4(content, TrKey::DRV_CURRENT_VOLTAGE, String(datalayer_extended.zoePH2.battery_current_voltage));
    tr_h4(content, TrKey::DRV_CHARGING_STATUS, String(datalayer_extended.zoePH2.battery_charging_status));
    tr_h4(content, TrKey::DRV_REMAINING_CHARGE, String(datalayer_extended.zoePH2.battery_remaining_charge));
    tr_h4(content, TrKey::DRV_BALANCE_CAPACITY_TOTAL, String(datalayer_extended.zoePH2.battery_balance_capacity_total));
    tr_h4(content, TrKey::DRV_BALANCE_TIME_TOTAL, String(datalayer_extended.zoePH2.battery_balance_time_total));
    tr_h4(content, TrKey::DRV_BALANCE_CAPACITY_SLEEP, String(datalayer_extended.zoePH2.battery_balance_capacity_sleep));
    tr_h4(content, TrKey::DRV_BALANCE_TIME_SLEEP, String(datalayer_extended.zoePH2.battery_balance_time_sleep));
    tr_h4(content, TrKey::DRV_BALANCE_CAPACITY_WAKE, String(datalayer_extended.zoePH2.battery_balance_capacity_wake));
    tr_h4(content, TrKey::DRV_BALANCE_TIME_WAKE, String(datalayer_extended.zoePH2.battery_balance_time_wake));
    tr_h4(content, TrKey::DRV_BMS_STATE, String(datalayer_extended.zoePH2.battery_bms_state));
    tr_h4(content, TrKey::DRV_ENERGY_COMPLETE, String(datalayer_extended.zoePH2.battery_energy_complete));
    tr_h4(content, TrKey::DRV_ENERGY_PARTIAL, String(datalayer_extended.zoePH2.battery_energy_partial));
    tr_h4(content, TrKey::DRV_SLAVE_FAILURES, String(datalayer_extended.zoePH2.battery_slave_failures));
    content += "<h4>mileage: " + String(datalayer_extended.zoePH2.battery_mileage) + "</h4>";
    tr_h4(content, TrKey::DRV_FAN_SPEED, String(datalayer_extended.zoePH2.battery_fan_speed));
    tr_h4(content, TrKey::DRV_FAN_PERIOD, String(datalayer_extended.zoePH2.battery_fan_period));
    tr_h4(content, TrKey::DRV_FAN_CONTROL, String(datalayer_extended.zoePH2.battery_fan_control));
    tr_h4(content, TrKey::DRV_FAN_DUTY, String(datalayer_extended.zoePH2.battery_fan_duty));
    content += "<h4>time: " + String(datalayer_extended.zoePH2.battery_time) + "</h4>";
    tr_h4(content, TrKey::DRV_PACK_TIME, String(datalayer_extended.zoePH2.battery_pack_time));
    content += "<h4>soc min: " + String(datalayer_extended.zoePH2.battery_soc_min) + "</h4>";
    content += "<h4>soc max: " + String(datalayer_extended.zoePH2.battery_soc_max) + "</h4>";
    content += "<h4>temporisation: ";
    if (datalayer_extended.zoePH2.battery_temporisation == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_READ_YET);
    } else if (datalayer_extended.zoePH2.battery_temporisation == 0) {
      tr_h4_end(content, TrKey::DRV_0_ACTIVATED);
    } else if (datalayer_extended.zoePH2.battery_temporisation == 1) {
      tr_h4_end(content, TrKey::DRV_1_DISABLED);
    } else {
      content += String(datalayer_extended.zoePH2.battery_temporisation) + "</h4>";
    }

    return content;
  }
};

#endif
