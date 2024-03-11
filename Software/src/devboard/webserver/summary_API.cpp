#include "summary_API.h"
#include <Arduino.h>

AsyncJsonResponse* summaryAPI_GET() {
	
    wl_status_t status = WiFi.status();
    
    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    
    root["Software Version"] = String(version_number);
    root["Led Colour"] = getLEDColour();
    
    // WiFi Details
    JsonObject nestedWifi = root.createNestedObject("WiFi");
    nestedWifi["ssid"] = String(ssid);
    nestedWifi["status"] = wirelessStatusDescription(status);
    
    if (status == WL_CONNECTED) {
      nestedWifi["ip"] = WiFi.localIP().toString();
      nestedWifi["signalStrength"] = WiFi.RSSI();
      nestedWifi["channel"] = WiFi.channel();
    }
    
    // Inverter / Battery / Charger details
    String inverterProtocol = inverterProtocolDescription();
    String batteryProtocol = batteryProtocolDescription();
    String chargerProtocol = chargerProtocolDescription();
    
    if (inverterProtocol != "") {
      root["inverterProtocol"] = inverterProtocol;
    }
    if (batteryProtocol != "") {
      root["batteryProtocol"] = batteryProtocol;
    }
    if (chargerProtocol != "") {
      root["chargerProtocol"] = chargerProtocol;
    }
	
	// Battery Statistics
	
    float socRealFloat = static_cast<float>(system_real_SOC_pptt) / 100.0;      // Convert to float and divide by 100
    float socScaledFloat = static_cast<float>(system_scaled_SOC_pptt) / 100.0;  // Convert to float and divide by 100
    float sohFloat = static_cast<float>(system_SOH_pptt) / 100.0;               // Convert to float and divide by 100
    float voltageFloat = static_cast<float>(system_battery_voltage_dV) / 10.0;  // Convert to float and divide by 10
    float currentFloat = static_cast<float>(system_battery_current_dA) / 10.0;  // Convert to float and divide by 10
    float powerFloat = static_cast<float>(system_active_power_W);               // Convert to float
    float tempMaxFloat = static_cast<float>(system_temperature_max_dC) / 10.0;  // Convert to float
    float tempMinFloat = static_cast<float>(system_temperature_min_dC) / 10.0;  // Convert to float

	JsonObject nestedBatteryStats = root.createNestedObject("Battery Statistics");
	
    nestedBatteryStats["Real SOC"] = socRealFloat;
    nestedBatteryStats["Scaled SOC"] = socScaledFloat;
    nestedBatteryStats["SOH"] = sohFloat;
    nestedBatteryStats["Voltage"] = voltageFloat;
    nestedBatteryStats["Current"] = currentFloat;
	
    nestedBatteryStats["Power"] = scalePowerValue(powerFloat, "", 1);
    nestedBatteryStats["Total capacity"] = scalePowerValue(system_capacity_Wh, "h", 0);
    nestedBatteryStats["Remaining capacity"] = scalePowerValue(system_remaining_capacity_Wh, "h", 1);
    nestedBatteryStats["Max charge power"] =  scalePowerValue(system_max_charge_power_W, "", 1);
    nestedBatteryStats["Max discharge power"] = scalePowerValue(system_max_discharge_power_W, "", 1);

    nestedBatteryStats["Cell max"] = String(system_cell_max_voltage_mV) + " mV";
    nestedBatteryStats["Cell min"] = String(system_cell_min_voltage_mV) + " mV";
    nestedBatteryStats["Temperature max"] = String(tempMaxFloat, 1) + " C";
    nestedBatteryStats["Temperature min"] = String(tempMinFloat, 1) + " C";
		
    // BMS Status
    if (system_bms_status == ACTIVE) {
      nestedBatteryStats["BMS Status"] = "OK";
    } else if (system_bms_status == UPDATING) {
      nestedBatteryStats["BMS Status"] = "UPDATING";
    } else {
      nestedBatteryStats["BMS Status"] = "FAULT";
    }
	
	// Battery chargins/discharging/idle state
    if (system_battery_current_dA == 0) {
      nestedBatteryStats["Battery State"] = "Idle";
    } else if (system_battery_current_dA < 0) {
      nestedBatteryStats["Battery State"] = "Discharging";
    } else {  // > 0
      nestedBatteryStats["Battery State"] = "Charging";
    }

    // Automatic contactor closing allowed
    JsonObject nestedContactorNode = nestedBatteryStats.createNestedObject("Automatic contactor closing allowed");
    nestedContactorNode["Battery"] = batteryAllowsContactorClosing;
    nestedContactorNode["Inverter"] = inverterAllowsContactorClosing;
	return response;
}

String inverterProtocolDescription() {

  String description = "";

#ifdef BYD_CAN
  description += "BYD Battery-Box Premium HVS over CAN Bus";
#endif
#ifdef BYD_MODBUS
  description += "BYD 11kWh HVM battery over Modbus RTU";
#endif
#ifdef LUNA2000_MODBUS
  description += "Luna2000 battery over Modbus RTU";
#endif
#ifdef PYLON_CAN
  description += "Pylontech battery over CAN bus";
#endif
#ifdef SERIAL_LINK_TRANSMITTER
  description += "Serial link to another LilyGo board";
#endif
#ifdef SMA_CAN
  description += "BYD Battery-Box H 8.9kWh, 7 mod over CAN bus";
#endif
#ifdef SOFAR_CAN
  description += "Sofar Energy Storage Inverter High Voltage BMS General Protocol (Extended Frame) over CAN bus";
#endif
#ifdef SOLAX_CAN
  description += "SolaX Triple Power LFP over CAN bus";
#endif

  return description;
}

String batteryProtocolDescription() {
  String description = "";

#ifdef BMW_I3_BATTERY
  description += "BMW i3";
#endif
#ifdef CHADEMO_BATTERY
  description += "Chademo V2X mode";
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
  description += "I-Miev / C-Zero / Ion Triplet";
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
  description += "Kia/Hyundai 64kWh";
#endif
#ifdef NISSAN_LEAF_BATTERY
  description += "Nissan LEAF";
#endif
#ifdef RENAULT_KANGOO_BATTERY
  description += "Renault Kangoo";
#endif
#ifdef RENAULT_ZOE_BATTERY
  description += "Renault Zoe";
#endif
#ifdef SERIAL_LINK_RECEIVER
  description += "Serial link to another LilyGo board";
#endif
#ifdef TESLA_MODEL_3_BATTERY
  description += "Tesla Model S/3/X/Y";
#endif
#ifdef TEST_FAKE_BATTERY
  description += "Fake battery for testing purposes";
#endif
  
  return description;
}

String chargerProtocolDescription() {
  
  String description = "";
  
#ifdef CHEVYVOLT_CHARGER
  description += "Chevy Volt Gen1 Charger";
#endif
#ifdef NISSANLEAF_CHARGER
  description += "Nissan LEAF 2013-2024 PDM charger";
#endif
  
  return description;
}

String getLEDColour() {
  switch (LEDcolor) {
    case GREEN:
      return "GREEN";
      break;
    case YELLOW:
      return "YELLOW";
      break;
    case BLUE:
      return "BLUE";
      break;
    case RED:
      return "RED";
      break;
    case TEST_ALL_COLORS:
      return  "RGB Testing loop";
      break;
    default:
      break;
  }
}

String wirelessStatusDescription(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "No WiFi Shield";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "No SSID Available";
    case WL_SCAN_COMPLETED: return "Scan Completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connection Failed";
    case WL_CONNECTION_LOST: return "Connection Lost";
    case WL_DISCONNECTED: return "Disconnected";
  }
}

template <typename S>
String scalePowerValue(S value, String unit, int precision) {

  String result = "";
  if (std::is_same<S, float>::value || std::is_same<S, uint16_t>::value || std::is_same<S, uint32_t>::value) {
    float convertedValue = static_cast<float>(value);

    if (convertedValue >= 1000.0 || convertedValue <= -1000.0) {
      result += String(convertedValue / 1000.0, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }

  result += unit;
  return result;
}