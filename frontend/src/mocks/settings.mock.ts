import type { SettingsResponse } from "../types/settings.types.js";

export function mockSettings(): SettingsResponse {
  return {
    bool: {
      MQTTENABLED: false,
      WEBENABLED: true,
      WIFIAPENABLED: false,
      SDLOGENABLED: false,
    },
    uint: {
      MQTTPORT: 1883,
      WIFICHANNEL: 0,
      BATTPVMAX: 4500,
      BATTPVMIN: 2800,
    },
    string: {
      HOSTNAME: "BatteryEmulator",
      SSID: "mock-ssid",
      MQTTSERVER: "",
    },
    select: {
      INVTYPE: 0,
      INVCOMM: 3,
      BATTTYPE: 0,
      BATTCHEM: 0,
      BATTCOMM: 3,
      CHGTYPE: 0,
      CHGCOMM: 3,
      EQSTOP: 0,
      BATT2COMM: 3,
      BATT3COMM: 3,
      SHUNTTYPE: 0,
      SHUNTCOMM: 3,
      CTATTEN: 3,
    },
    select_options: {
      battery: [
        { value: 0, label: "None" },
        { value: 21, label: "Nissan LEAF" },
      ],
      inverter: [
        { value: 0, label: "None" },
        { value: 3, label: "BYD Modbus" },
      ],
      charger: [
        { value: 0, label: "None" },
        { value: 1, label: "Nissan Leaf Charger" },
      ],
      chemistry: [
        { value: 0, label: "Autodetect" },
        { value: 3, label: "LFP" },
      ],
      comm: [
        { value: 1, label: "Modbus" },
        { value: 3, label: "CAN (Native)" },
      ],
      shunt: [
        { value: 0, label: "None" },
        { value: 1, label: "BMW SBOX" },
      ],
      eqstop: [
        { value: 0, label: "Not connected" },
        { value: 1, label: "Latching switch" },
        { value: 2, label: "Momentary switch" },
      ],
      ctatten: [
        { value: 0, label: "a)   0dB 0-950mV" },
        { value: 3, label: "d)  11dB 0-3100mV" },
      ],
    },
    string_extra: { CTOFFSET: "0" },
    mqtt_publish_s: 5,
    datalayer: {
      total_capacity_Wh: 30000,
      max_charge_A: 30,
      max_discharge_A: 30,
      soc_max_pct: 100,
      soc_min_pct: 10,
      charge_voltage_V: 450,
      discharge_voltage_V: 300,
      soc_scaling_active: true,
      voltage_limits_active: false,
      manual_balancing: false,
      sofar_battery_id: 0,
    },
  };
}
