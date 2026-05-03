#ifndef _CHADEMO_BATTERY_HTML_H
#define _CHADEMO_BATTERY_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class ChademoBatteryHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>Vehicle current requested: " + String(datalayer_extended.chademo.CurrentRequested) + " A</h4>";
    content += "<h4>Vehicle voltage requested: " + String(datalayer_extended.chademo.VoltageRequested) + " V</h4>";
    content += "<h4>Pin 7 Connection Check Status: " +
               String(datalayer_extended.chademo.ConnectionCheckStatus ? "Connected" : "Disconnected") + "</h4>";
    content +=
        "<h4>Connector Lock Status: " + String(datalayer_extended.chademo.ConnectorLockStatus ? "Locked" : "Unlocked") +
        "</h4>";
    content += "<h4>Pin 2 D1 Status: " + String(datalayer_extended.chademo.D1Status ? "On" : "Off") + "</h4>";
    content += "<h4>Pin 4 Charge Enable Status: " +
               String(datalayer_extended.chademo.ChargeEnableStatus ? "Enabled" : "Disabled") + "</h4>";
    content += "<h4>Pin 10 D2 Status: " + String(datalayer_extended.chademo.D2Status ? "On" : "Off") + "</h4>";
    content += "<h4>Chademo state: ";
    switch (datalayer_extended.chademo.CHADEMO_Status) {
      case 0:
        content += String("FAULT</h4>");
        break;
      case 1:
        content += String("STOP</h4>");
        break;
      case 2:
        content += String("IDLE</h4>");
        break;
      case 3:
        content += String("CONNECTED</h4>");
        break;
      case 4:
        content += String("INIT</h4>");
        break;
      case 5:
        content += String("NEGOTIATE</h4>");
        break;
      case 6:
        content += String("EV ALLOWED</h4>");
        break;
      case 7:
        content += String("EVSE PREPARE</h4>");
        break;
      case 8:
        content += String("EVSE START</h4>");
        break;
      case 9:
        content += String("EVSE CONTACTORS ENABLED</h4>");
        break;
      case 10:
        content += String("POWERFLOW</h4>");
        break;
      case 11:
        content += String("POWERFLOW SUSPENDED</h4>");
        break;
      default:
        content += String("Unknown</h4>");
        break;
    }
    if (datalayer_extended.chademo.FaultBatteryCurrentDeviation) {
      content += "<h4>FAULT: Battery Current Deviation</h4>";
    }
    if (datalayer_extended.chademo.FaultBatteryOverVoltage) {
      content += "<h4>FAULT: Battery Overvoltage</h4>";
    }
    if (datalayer_extended.chademo.FaultBatteryUnderVoltage) {
      content += "<h4>FAULT: Battery Undervoltage</h4>";
    }
    if (datalayer_extended.chademo.FaultBatteryVoltageDeviation) {
      content += "<h4>FAULT: Battery Voltage Deviation</h4>";
    }
    if (datalayer_extended.chademo.FaultHighBatteryTemperature) {
      content += "<h4>FAULT: Battery Temperature</h4>";
    }

    // report stop reason if we are in STOP state
    if (datalayer_extended.chademo.CHADEMO_Status == 1) {
      content += "<h4>Stop reason: ";
      switch (datalayer_extended.chademo.StopReason) {
        case EV_REQUEST:
          content += "EV requested";
          break;
        case BATTERY_VOLTAGE:
          content += "Battery voltage";
          break;
        case VEHICLE_FAULT:
          content += "Vehicle fault";
          break;
        case EVSE_FAULT:
          content += "EVSE fault";
          break;
        case SHIFTER_POSITION:
          content += "Shifter position";
          break;
        case USER_REQUESTED:
          content += "User requested";
          break;
        case UNPLUGGED:
          content += "Unplugged";
          break;
        case OTHER:
        default:
          content += "Other";
          break;
      }
      content += "</h4>";
    }

    content += "<h4>Protocol: " + String(datalayer_extended.chademo.ControlProtocolNumberEV) + "</h4>";

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 5000);";
    content += "</script>";

    return content;
  }
};

#endif
