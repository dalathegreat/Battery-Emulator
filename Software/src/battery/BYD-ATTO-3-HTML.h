#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl) : byd_datalayer(dl) {}

  String get_status_html() {
    String content;

    content += "<script>";
    content += "function editComplete() {";
    content += "  alert('Update successful!');";
    content += "  setTimeout(function() { location.reload(); }, 1000);";
    content += "}";
    content += "function editError() {";
    content += "  alert('Update failed. Please try again.');";
    content += "}";
    content += "function editCalTargetSOC(){";
    content += "  var value=prompt('Enter calibration target SOC (0 to 100):');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>=0 && numValue<=100){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetSOC?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('Invalid value. Please enter a value between 0 and 100.');";
    content += "    }";
    content += "  }";
    content += "}";
    content += "function editCalTargetAH(){";
    content += "  var value=prompt('Enter calibration target AH:');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>0){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetAH?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('Invalid value. Please enter a positive number.');";
    content += "    }";
    content += "  }";
    content += "}";
    content += "</script>";

    content += "<h4>Cells configured: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content += "<h4>Charging battery state: ";
    switch (byd_datalayer->discharge_status) {
      case 0:
        content += "Ready</h4>";
        break;
      case 1:
        content += "Charging</h4>";
        break;
      case 2:
        content += "Charge finished</h4>";
        break;
      case 3:
        content += "Discharging</h4>";
        break;
      case 4:
        content += "Charge terminated</h4>";
        break;
      case 5:
        content += "Breakdown C10</h4>";
        break;
      case 6:
        content += "Breakdown charging plug</h4>";
        break;
      case 7:
        content += "Breakdown charger</h4>";
        break;
      case 8:
        content += "Breakdown AC</h4>";
        break;
      case 9:
        content += "Schedule</h4>";
        break;
      case 10:
        content += "Discharge CBU</h4>";
        break;
      case 11:
        content += "Timeout</h4>";
        break;
      case 12:
        content += "Discharge finish</h4>";
        break;
      case 13:
        content += "Charging pause</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }

    float soc_measured = static_cast<float>(byd_datalayer->SOC_highprec) * 0.1f;
    float BMS_maxChargePower = static_cast<float>(byd_datalayer->chargePower) * 0.1f;
    float BMS_maxDischargePower = static_cast<float>(byd_datalayer->dischargePower) * 0.1f;

    content += "<h4>SOC measured: " + String(soc_measured) + "&percnt;</h4>";
    content += "<h4>SOC OBD2: " + String(byd_datalayer->SOC_polled) + "&percnt;</h4>";
    content += "<h4>Voltage periodic: " + String(byd_datalayer->voltage_periodic) + " V</h4>";
    content += "<h4>Voltage OBD2: " + String(byd_datalayer->voltage_polled) + " V</h4>";
    content += "<h4>Temperature sensor 1: " + String(byd_datalayer->battery_temperatures[0]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 2: " + String(byd_datalayer->battery_temperatures[1]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 3: " + String(byd_datalayer->battery_temperatures[2]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 4: " + String(byd_datalayer->battery_temperatures[3]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 5: " + String(byd_datalayer->battery_temperatures[4]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 6: " + String(byd_datalayer->battery_temperatures[5]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 7: " + String(byd_datalayer->battery_temperatures[6]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 8: " + String(byd_datalayer->battery_temperatures[7]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 9: " + String(byd_datalayer->battery_temperatures[8]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 10: " + String(byd_datalayer->battery_temperatures[9]) + " &deg;C</h4>";
    content += "<h4>Max discharge power: " + String(BMS_maxDischargePower) + " kW</h4>";
    content += "<h4>Max charge (regen) power: " + String(BMS_maxChargePower) + " kW</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_kwh) + " kWh</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_kwh) + " kWh</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_ah) + " Ah</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_ah) + " Ah</h4>";
    content += "<h4>Charge times: " + String(byd_datalayer->charge_times) + "</h4>";
    content += "<h4>Times of full power: " + String(byd_datalayer->times_full_power) + "</h4>";
    content += "<h4>Min cell voltage number: " + String(byd_datalayer->BMS_min_cell_voltage_number) + "</h4>";
    content += "<h4>Max cell voltage number: " + String(byd_datalayer->BMS_max_cell_voltage_number) + "</h4>";
    content += "<h4>Min temp module number: " + String(byd_datalayer->BMS_min_temp_module_number) + "</h4>";
    content += "<h4>Max temp module number: " + String(byd_datalayer->BMS_max_temp_module_number) + "</h4>";
    content += "<h4>Seed: " + String(byd_datalayer->seed) + " SolvedKey: " + String(byd_datalayer->solvedKey) + "</h4>";
    if (byd_datalayer->servicemode == 0) {
      content += "<h4>ServiceMode: No command ran yet</h4>";
    } else if (byd_datalayer->servicemode == 1) {
      content += "<h4>ServiceMode: REJECTED </h4>";
    } else if (byd_datalayer->servicemode == 2) {
      content += "<h4>ServiceMode: APPROVED! </h4>";
    }
    content += "<h4>Capacity orignal: " + String((byd_datalayer->BMS_capacity_original_calibration) / 100) + "AH</h4>";
    content += "<h4>Capacity current: " + String((byd_datalayer->BMS_capacity_current_calibration) / 100) + "AH</h4>";
    content += "<h4>SOC original: " + String(byd_datalayer->BMC_SOC_original_calibration) + "&percnt;</h4>";
    content += "<h4>SOC current: " + String(byd_datalayer->BMC_SOC_current_calibration) + "&percnt;</h4>";

    content += "<h4>Calibration target SOC: " + String(byd_datalayer->calibrationTargetSOC) +
               "&percnt;"
               " </span><button onclick='editCalTargetSOC()'>Edit</button></h4>";
    content += "<h4>Calibration target capacity: " + String(byd_datalayer->calibrationTargetAH) +
               " AH</span><button onclick='editCalTargetAH()'>Edit</button></h4>";

    return content;
  }

 private:
  DATALAYER_INFO_BYDATTO3* byd_datalayer;
};

#endif
