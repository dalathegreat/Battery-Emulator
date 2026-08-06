import * as L from "./datalayer/DATALAYER_INFO_TESLA.ts";
import { type Field, lookup } from "./consts.ts";

// ---- Enum lookup tables (matching TESLA-HTML.h) ----

const contactorText = ["UNKNOWN(0)", "OPEN", "CLOSING", "BLOCKED", "OPENING",
  "CLOSED", "UNKNOWN(6)", "WELDED", "POS_CL", "NEG_CL",
  "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"];

const hvilStatusState = ["UNKNOWN or CONTACTORS OPEN", "STATUS_OK",
  "CURRENT_SOURCE_FAULT", "INTERNAL_OPEN_FAULT", "VEHICLE_OPEN_FAULT",
  "PENTHOUSE_LID_OPEN_FAULT", "UNKNOWN_LOCATION_OPEN_FAULT",
  "VEHICLE_NODE_FAULT", "NO_12V_SUPPLY",
  "VEHICLE_OR_PENTHOUSE_LID_OPENFAULT",
  "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)", "UNKNOWN(13)", "UNKNOWN(14)", "UNKNOWN(15)"];

const contactorState = ["SNA", "OPEN", "PRECHARGE", "BLOCKED", "PULLED_IN",
  "OPENING", "ECONOMIZED", "WELDED", "UNKNOWN(8)", "UNKNOWN(9)",
  "UNKNOWN(10)", "UNKNOWN(11)"];

const BMS_state = ["STANDBY", "DRIVE", "SUPPORT", "CHARGE", "FEIM",
  "CLEAR_FAULT", "FAULT", "WELD", "TEST", "SNA"];

const BMS_contactorState = ["SNA", "OPEN", "OPENING", "CLOSING", "CLOSED", "WELDED", "BLOCKED"];

const BMS_hvState = ["DOWN", "COMING_UP", "GOING_DOWN", "UP_FOR_DRIVE",
  "UP_FOR_CHARGE", "UP_FOR_DC_CHARGE", "UP"];

const BMS_uiChargeStatus = ["DISCONNECTED", "NO_POWER", "ABOUT_TO_CHARGE",
  "CHARGING", "CHARGE_COMPLETE", "CHARGE_STOPPED"];

const PCS_dcdcStatus = ["IDLE", "ACTIVE", "FAULTED"];

const PCS_dcdcMainState = ["STANDBY", "12V_SUPPORT_ACTIVE", "PRECHARGE_STARTUP",
  "PRECHARGE_ACTIVE", "DIS_HVBUS_ACTIVE", "SHUTDOWN", "FAULTED"];

const PCS_dcdcSubState = ["PWR_UP_INIT", "STANDBY", "12V_SUPPORT_ACTIVE", "DIS_HVBUS",
  "PCHG_FAST_DIS_HVBUS", "PCHG_SLOW_DIS_HVBUS", "PCHG_DWELL_CHARGE",
  "PCHG_DWELL_WAIT", "PCHG_DI_RECOVERY_WAIT", "PCHG_ACTIVE",
  "PCHG_FLT_FAST_DIS_HVBUS", "SHUTDOWN", "12V_SUPPORT_FAULTED",
  "DIS_HVBUS_FAULTED", "PCHG_FAULTED", "CLEAR_FAULTS", "FAULTED", "NUM"];

const BMS_powerLimitState = ["NOT_CALCULATED_FOR_DRIVE", "CALCULATED_FOR_DRIVE"];

const HVP_contactor = ["NOT_ACTIVE", "ACTIVE", "COMPLETED"];

const noYes = ["No", "Yes"];
const falseTrue = ["False", "True"];

// ---- DTC JSON loader script (from BatteryHtmlRenderer::get_dtc_json_loader_html) ----

const DTC_BASE_URL = "https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/web_data/dtc/";
const DTC_JSON_FILE = "tesla_model3y_dtc.json";

function dtcLoaderScript(): string {
  const url = DTC_BASE_URL + DTC_JSON_FILE;
  const k = "dtcJson:" + url;
  return `<script>(function(){var u='${url}';var k='${k}';` +
    `var S=document.getElementById('dtcJsonStatus');` +
    `var C=document.getElementById('dtcJsonFileContainer');` +
    `var I=document.getElementById('dtcJsonFile');` +
    `function A(a,b){var m={};a.forEach(function(e){if(e.code!=null)m[e.code]=e;if(e.dtc)m[e.dtc]=e;});` +
    `var L=document.querySelectorAll('[data-dtc-code]');var n=0;` +
    `L.forEach(function(t){var e=m[t.getAttribute('data-dtc-code')];` +
    `if(e){var d=e.l_dsc;if(e.s_dsc)d+='<br /><em style=\\'color:#aaa;font-size:0.85em\\'>'+e.s_dsc+'</em>';t.innerHTML=d;n++;}});` +
    `S.innerHTML='Loaded '+a.length+' entries, '+n+'/'+L.length+' DTCs matched'+(b?' (cached)':' (fetched)')+
    '. <a href=\\'#\\' id=\\'dtcRefresh\\' style=\\'color:#aaa;font-size:0.85em;\\'>Refresh</a>';` +
    `S.style.color=n>0?'#4CAF50':'#ff9800';` +
    `document.getElementById('dtcRefresh').addEventListener('click',function(e){` +
    `e.preventDefault();try{localStorage.removeItem(k);}catch(x){}F();});}` +
    `function P(msg){S.textContent=msg;S.style.color='#ff9800';C.style.display='';}` +
    `function F(){S.textContent='Fetching DTC descriptions from GitHub...';S.style.color='#aaa';` +
    `fetch(u).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.text();})` +
    `.then(function(t){try{localStorage.setItem(k,t);}catch(x){}A(JSON.parse(t),false);})` +
    `.catch(function(err){P('GitHub unavailable ('+err.message+') - load from local file:');});}` +
    `if(u.length>0){var g=null;try{g=localStorage.getItem(k);}catch(x){}` +
    `if(g){try{A(JSON.parse(g),true);}catch(x){F();}}else{F();}}else{C.style.display='';}` +
    `I.addEventListener('change',function(e){var f=e.target.files[0];if(!f)return;` +
    `S.textContent='Loading...';S.style.color='#aaa';var R=new FileReader();` +
    `R.onload=function(v){try{A(JSON.parse(v.target.result),false);}` +
    `catch(err){S.textContent='Parse error: '+err.message;S.style.color='#d32f2f';}};` +
    `R.onerror=function(){S.textContent='File read error';S.style.color='#d32f2f';};` +
    `R.readAsText(f);});})();</script>`;
}

// ---- Component ----

export function TeslaExtended({view}: {view: DataView}) {
  const get = lookup(view);

  // Convert a byte-array field (u8[]) to an ASCII string, matching
  // TeslaHtmlRenderer's serial/part number decoding.
  function byteArrToStr(field: Field): string {
    const bytes = get( field) as number[];
    return bytes.map(b => (b >= 32 && b <= 126) ? String.fromCharCode(b) : '?').join('');
  }

  // Helper: enum lookup from a raw u8 index
  function enumLookup(field: Field, table: string[]): string {
    const idx = get( field) as number;
    return table[idx] ?? `UNKNOWN(${idx})`;
  }

  // Helper: bool → "Yes"/"No"
  function boolYN(field: Field): string {
    return noYes[get( field) as number ? 1 : 0];
  }

  // ---- Read all values with scaling factors ----

  const beginning_of_life = get( L.battery_beginning_of_life) as number;
  const battTempPct = (get( L.battery_battTempPct) as number) * 0.4;
  const dcdcLvBusVolt = (get( L.battery_dcdcLvBusVolt) as number) * 0.0390625;
  const dcdcHvBusVolt = (get( L.battery_dcdcHvBusVolt) as number) * 0.146484;
  const dcdcLvOutputCurrent = (get( L.battery_dcdcLvOutputCurrent) as number) * 0.1;
  const nominal_full_pack_energy = (get( L.battery_nominal_full_pack_energy) as number) * 0.1;
  const nominal_full_pack_energy_m0 = (get( L.battery_nominal_full_pack_energy_m0) as number) * 0.02;
  const nominal_energy_remaining = (get( L.battery_nominal_energy_remaining) as number) * 0.1;
  const nominal_energy_remaining_m0 = (get( L.battery_nominal_energy_remaining_m0) as number) * 0.02;
  const ideal_energy_remaining = (get( L.battery_ideal_energy_remaining) as number) * 0.1;
  const ideal_energy_remaining_m0 = (get( L.battery_ideal_energy_remaining_m0) as number) * 0.02;
  const energy_to_charge_complete = (get( L.battery_energy_to_charge_complete) as number) * 0.1;
  const energy_to_charge_complete_m1 = (get( L.battery_energy_to_charge_complete_m1) as number) * 0.02;
  const energy_buffer = (get( L.battery_energy_buffer) as number) * 0.1;
  const energy_buffer_m1 = (get( L.battery_energy_buffer_m1) as number) * 0.01;
  const expected_energy_remaining_m1 = (get( L.battery_expected_energy_remaining_m1) as number) * 0.02;
  const packMass = get( L.battery_packMass) as number;
  const platformMaxBusVoltage = (get( L.battery_platformMaxBusVoltage) as number) * 0.1 + 375;
  const bms_min_voltage = (get( L.BMS_min_voltage) as number) * 0.01 * 2;
  const bms_max_voltage = (get( L.BMS_max_voltage) as number) * 0.01 * 2;
  const max_charge_current = get( L.battery_max_charge_current) as number;
  const max_discharge_current = get( L.battery_max_discharge_current) as number;
  const soc_ave = (get( L.battery_soc_ave) as number) * 0.1;
  const soc_max = (get( L.battery_soc_max) as number) * 0.1;
  const soc_min = (get( L.battery_soc_min) as number) * 0.1;
  const soc_ui = (get( L.battery_soc_ui) as number) * 0.1;
  const BrickVoltageMax = (get( L.battery_BrickVoltageMax) as number) * 0.002;
  const BrickVoltageMin = (get( L.battery_BrickVoltageMin) as number) * 0.002;
  const isolationResistance = (get( L.BMS_isolationResistance) as number) * 10;
  const PCS_dcdcMaxOutputCurrentAllowed = (get( L.PCS_dcdcMaxOutputCurrentAllowed) as number) * 0.1;
  const PCS_dcdcTemp = (get( L.PCS_dcdcTemp) as number) * 0.1 + 40;
  const PCS_ambientTemp = (get( L.PCS_ambientTemp) as number) * 0.1 + 40;
  const PCS_chgPhATemp = (get( L.PCS_chgPhATemp) as number) * 0.1 + 40;
  const PCS_chgPhBTemp = (get( L.PCS_chgPhBTemp) as number) * 0.1 + 40;
  const PCS_chgPhCTemp = (get( L.PCS_chgPhCTemp) as number) * 0.1 + 40;
  const BMS_maxRegenPower = (get( L.BMS_maxRegenPower) as number) * 0.01;
  const BMS_maxDischargePower = (get( L.BMS_maxDischargePower) as number) * 0.013;
  const BMS_powerDissipation = (get( L.BMS_powerDissipation) as number) * 0.02;
  const BMS_flowRequest = (get( L.BMS_flowRequest) as number) * 0.3;
  const BMS_inletActiveCoolTargetT = (get( L.BMS_inletActiveCoolTargetT) as number) * 0.25 - 25;
  const BMS_inletPassiveTargetT = (get( L.BMS_inletPassiveTargetT) as number) * 0.25 - 25;
  const BMS_inletActiveHeatTargetT = (get( L.BMS_inletActiveHeatTargetT) as number) * 0.25 - 25;
  const BMS_packTMin = (get( L.BMS_packTMin) as number) * 0.25 - 25;
  const BMS_packTMax = (get( L.BMS_packTMax) as number) * 0.25 - 25;
  const PCS_dcdcMaxLvOutputCurrent = (get( L.PCS_dcdcMaxLvOutputCurrent) as number) * 0.1;
  const PCS_dcdcCurrentLimit = (get( L.PCS_dcdcCurrentLimit) as number) * 0.1;
  const PCS_dcdcLvOutputCurrentTempLimit = (get( L.PCS_dcdcLvOutputCurrentTempLimit) as number) * 0.1;
  const PCS_dcdcUnifiedCommand = (get( L.PCS_dcdcUnifiedCommand) as number) * 0.001;
  const PCS_dcdcCLAControllerOutput = (get( L.PCS_dcdcCLAControllerOutput) as number) * 0.001;
  const PCS_dcdcClaCurrentFreq = (get( L.PCS_dcdcClaCurrentFreq) as number) * 0.0976563;
  const PCS_dcdcTCommMeasured = (get( L.PCS_dcdcTCommMeasured) as number) * 0.00195313;
  const PCS_dcdcShortTimeUs = (get( L.PCS_dcdcShortTimeUs) as number) * 0.000488281;
  const PCS_dcdcHalfPeriodUs = (get( L.PCS_dcdcHalfPeriodUs) as number) * 0.000488281;
  const PCS_dcdcIntervalMaxHvBusVolt = (get( L.PCS_dcdcIntervalMaxHvBusVolt) as number) * 0.1;
  const PCS_dcdcIntervalMaxLvBusVolt = (get( L.PCS_dcdcIntervalMaxLvBusVolt) as number) * 0.1;
  const PCS_dcdcIntervalMinHvBusVolt = (get( L.PCS_dcdcIntervalMinHvBusVolt) as number) * 0.1;
  const PCS_dcdcIntervalMinLvBusVolt = (get( L.PCS_dcdcIntervalMinLvBusVolt) as number) * 0.1;
  const PCS_dcdc12vSupportLifetimekWh = (get( L.PCS_dcdc12vSupportLifetimekWh) as number) * 0.01;
  const HVP_hvp1v5Ref = (get( L.HVP_hvp1v5Ref) as number) * 0.1;
  const HVP_shuntCurrentDebug = (get( L.HVP_shuntCurrentDebug) as number) * 0.1;
  const HVP_dcLinkVoltage = (get( L.HVP_dcLinkVoltage) as number) * 0.1;
  const HVP_packVoltage = (get( L.HVP_packVoltage) as number) * 0.1;
  const HVP_packContVoltage = (get( L.HVP_packContVoltage) as number) * 0.1;
  const HVP_pyroAnalog = (get( L.HVP_pyroAnalog) as number) * 0.1;
  const HVP_hvilInVoltage = (get( L.HVP_hvilInVoltage) as number) * 0.1;
  const HVP_hvilOutVoltage = (get( L.HVP_hvilOutVoltage) as number) * 0.1;
  const HVP_packContCoilCurrent = (get( L.HVP_packContCoilCurrent) as number) * 0.1;
  const HVP_battery12V = (get( L.HVP_battery12V) as number) * 0.1;

  // ---- Derived values ----

  const soh_nomux = beginning_of_life ? nominal_full_pack_energy * 100 / beginning_of_life : 0;
  const soh_mux = beginning_of_life ? nominal_full_pack_energy_m0 * 100 / beginning_of_life : 0;

  // ---- Enum lookups ----

  const hvilStatus = enumLookup(L.hvil_status, hvilStatusState);
  const hvpContactorState = enumLookup(L.packContactorSetState, contactorText);
  const bmsContactorState = enumLookup(L.BMS_contactorState, BMS_contactorState);
  const negContactor = enumLookup(L.packContNegativeState, contactorState);
  const posContactor = enumLookup(L.packContPositiveState, contactorState);
  const bmsState = enumLookup(L.BMS_state, BMS_state);
  const bmsHvState = enumLookup(L.BMS_hvState, BMS_hvState);
  const bmsUiChargeStatus = enumLookup(L.BMS_uiChargeStatus, BMS_uiChargeStatus);
  const pcsPrechargeStatus = enumLookup(L.PCS_dcdcPrechargeStatus, PCS_dcdcStatus);
  const pcs12vSupportStatus = enumLookup(L.PCS_dcdc12VSupportStatus, PCS_dcdcStatus);
  const pcsHvBusDischargeStatus = enumLookup(L.PCS_dcdcHvBusDischargeStatus, PCS_dcdcStatus);
  const pcsMainState = enumLookup(L.PCS_dcdcMainState, PCS_dcdcMainState);
  const pcsSubState = enumLookup(L.PCS_dcdcSubState, PCS_dcdcSubState);
  const pcsInitialPrechargeSubState = enumLookup(L.PCS_dcdcInitialPrechargeSubState, PCS_dcdcSubState);
  const powerLimitState = enumLookup(L.BMS_powerLimitState, BMS_powerLimitState);
  const packCtrsRequestStatus = enumLookup(L.battery_packCtrsRequestStatus, HVP_contactor);

  // Closing blocked: show "(already CLOSED)" annotation when pack is closed (state 5)
  const packContactorSetState = get( L.packContactorSetState) as number;
  const closingBlockedSuffix = packContactorSetState === 5 ? " (already CLOSED)" : "";

  // ---- Alert matrix (DTC) ----

  const bmsAlertActive = get( L.BMS_alertMatrixActive) as boolean[];
  const pcsAlertActive = get( L.PCS_alertMatrixActive) as boolean[];
  const cpAlertActive = get( L.CP_alertMatrixActive) as boolean[];

  const alertGroups = [
    { label: "BMS 0x320", base: 100, active: bmsAlertActive, count: 100 },
    { label: "PCS 0x3A4", base: 200, active: pcsAlertActive, count: 94 },
    { label: "CP 0x31E", base: 300, active: cpAlertActive, count: 96 },
  ];

  let totalActive = 0;
  for (const g of alertGroups) {
    for (let i = 0; i < g.count; i++) {
      if (g.active[i]) totalActive++;
    }
  }

  // ---- Part/serial numbers ----

  const serialNumber = byteArrToStr(L.battery_serialNumber);
  const batteryPartNumber = byteArrToStr(L.battery_partNumber);
  const pcsPartNumber = byteArrToStr(L.PCS_partNumber);

  // ---- Render ----

  return <div>
    {/* Battery identification */}
    <h4>Battery Serial Number: {serialNumber}</h4>
    <h4>Battery Part Number: {batteryPartNumber}</h4>
    <h4>PCS Part Number: {pcsPartNumber}</h4>
    <h4>Battery Pack Mass: {packMass} KG</h4>
    <h4>Battery Total Discharge: N/A (not in extended struct)</h4>
    <h4>Battery Total Charge: N/A (not in extended struct)</h4>

    {/* 0x20A 522 HVP_contactorState + HVIL */}
    <h4>HVIL Status: {hvilStatus}</h4>
    <h4>HVP Contactor State: {hvpContactorState}</h4>
    <h4>BMS Contactor State: {bmsContactorState}</h4>
    <h4>Negative Contactor: {negContactor}</h4>
    <h4>Positive Contactor: {posContactor}</h4>
    <h4>Closing blocked: {boolYN(L.packCtrsClosingBlocked)}{closingBlockedSuffix}</h4>
    <h4>Pyrotest in progress: {boolYN(L.pyroTestInProgress)}</h4>
    <h4>Contactors Open Now Requested: {boolYN(L.battery_packCtrsOpenNowRequested)}</h4>
    <h4>Contactors Open Requested: {boolYN(L.battery_packCtrsOpenRequested)}</h4>
    <h4>Contactors Request Status: {packCtrsRequestStatus}</h4>
    <h4>Contactors Reset Request Required: {boolYN(L.battery_packCtrsResetRequestRequired)}</h4>
    <h4>DC Link Allowed to Energize: {boolYN(L.battery_dcLinkAllowedToEnergize)}</h4>

    {/* 0x352 850 BMS_energyStatus — mux vs non-mux */}
    {!get( L.BMS352_mux) ? <>
      <h3>BMS 0x352 w/o mux</h3>
      <h4>Calculated SOH: {soh_nomux.toFixed(1)}</h4>
      <h4>Nominal Full Pack Energy: {nominal_full_pack_energy} kWh</h4>
      <h4>Nominal Energy Remaining: {nominal_energy_remaining} kWh</h4>
      <h4>Ideal Energy Remaining: {ideal_energy_remaining} kWh</h4>
      <h4>Energy to Charge Complete: {energy_to_charge_complete} kWh</h4>
      <h4>Energy Buffer: {energy_buffer} kWh</h4>
      <h4>Full Charge Complete: {boolYN(L.battery_full_charge_complete)}</h4>
    </> : <>
      <h3>BMS 0x352 w/ mux</h3>
      <h4>Calculated SOH: {soh_mux.toFixed(1)}</h4>
      <h4>Nominal Full Pack Energy: {nominal_full_pack_energy_m0} kWh</h4>
      <h4>Nominal Energy Remaining: {nominal_energy_remaining_m0} kWh</h4>
      <h4>Ideal Energy Remaining: {ideal_energy_remaining_m0} kWh</h4>
      <h4>Energy to Charge Complete: {energy_to_charge_complete_m1} kWh</h4>
      <h4>Energy Buffer: {energy_buffer_m1} kWh</h4>
      <h4>Expected Energy Remaining: {expected_energy_remaining_m1} kWh</h4>
      <h4>Fully Charged: {boolYN(L.battery_fully_charged)}</h4>
    </>}

    {/* 0x212 530 BMS_status */}
    <h4>Isolation Resistance: {isolationResistance} kOhms</h4>
    <h4>BMS State: {bmsState}</h4>
    <h4>BMS HV State: {bmsHvState}</h4>
    <h4>BMS UI Charge Status: {bmsUiChargeStatus}</h4>
    <h4>BMS_buildConfigId: {get( L.BMS_info_buildConfigId)}</h4>
    <h4>BMS_hardwareId: {get( L.BMS_info_hardwareId)}</h4>
    <h4>BMS_componentId: {get( L.BMS_info_componentId)}</h4>
    {get( L.BMS_pcsPwmEnabled) && <h4>BMS PCS PWM Enabled: ACTIVE</h4>}

    {/* 0x292 658 BMS_socStates */}
    <h4>Battery Beginning of Life: {beginning_of_life} kWh</h4>
    <h4>Battery SOC UI: {soc_ui} </h4>
    <h4>Battery SOC Ave: {soc_ave} </h4>
    <h4>Battery SOC Max: {soc_max} </h4>
    <h4>Battery SOC Min: {soc_min} </h4>
    <h4>Battery Temp Percent: {battTempPct} </h4>

    {/* 0x2B4 PCS_dcdcRailStatus */}
    <h4>PCS Lv Output: {dcdcLvOutputCurrent} A</h4>
    <h4>PCS Lv Bus: {dcdcLvBusVolt} V</h4>
    <h4>PCS Hv Bus: {dcdcHvBusVolt} V</h4>

    {/* 0x392 BMS_packConfig */}
    <h4>Platform Max Bus Voltage: {platformMaxBusVoltage} V</h4>

    {/* 0x2D2 722 BMSVAlimits */}
    <h4>BMS Min Voltage: {bms_min_voltage} V</h4>
    <h4>BMS Max Voltage: {bms_max_voltage} V</h4>
    <h4>Max Charge Current: {max_charge_current} A</h4>
    <h4>Max Discharge Current: {max_discharge_current} A</h4>

    {/* 0x332 818 BMS_bmbMinMax */}
    <h4>Brick Voltage Max: {BrickVoltageMax} V</h4>
    <h4>Brick Voltage Min: {BrickVoltageMin} V</h4>
    <h4>Brick Temp Max Num: {get( L.battery_BrickTempMaxNum)} </h4>
    <h4>Brick Temp Min Num: {get( L.battery_BrickTempMinNum)} </h4>

    {/* 0x2A4 676 PCS_thermalStatus */}
    <h4>PCS dcdc Temp: {PCS_dcdcTemp} DegC</h4>
    <h4>PCS Ambient Temp: {PCS_ambientTemp} DegC</h4>
    <h4>PCS Chg PhA Temp: {PCS_chgPhATemp} DegC</h4>
    <h4>PCS Chg PhB Temp: {PCS_chgPhBTemp} DegC</h4>
    <h4>PCS Chg PhC Temp: {PCS_chgPhCTemp} DegC</h4>

    {/* 0x252 594 BMS_powerAvailable */}
    <h4>Max Regen Power: {BMS_maxRegenPower} kW</h4>
    <h4>Max Discharge Power: {BMS_maxDischargePower} kW</h4>
    <h4>Power Limit State: {powerLimitState}</h4>

    {/* 0x312 786 BMS_thermalStatus */}
    <h4>Power Dissipation: {BMS_powerDissipation} kW</h4>
    <h4>Flow Request: {BMS_flowRequest} LPM</h4>
    <h4>Inlet Active Cool Target Temp: {BMS_inletActiveCoolTargetT} DegC</h4>
    <h4>Inlet Passive Target Temp: {BMS_inletPassiveTargetT} DegC</h4>
    <h4>Inlet Active Heat Target Temp: {BMS_inletActiveHeatTargetT} DegC</h4>
    <h4>Pack Temp Min: {BMS_packTMin} DegC</h4>
    <h4>Pack Temp Max: {BMS_packTMax} DegC</h4>
    {get( L.BMS_pcsNoFlowRequest) && <h4>PCS No Flow Request: ACTIVE</h4>}
    {get( L.BMS_noFlowRequest) && <h4>BMS No Flow Request: ACTIVE</h4>}

    {/* 0x224 548 PCS_dcdcStatus */}
    <h4>Precharge Status: {pcsPrechargeStatus}</h4>
    <h4>12V Support Status: {pcs12vSupportStatus}</h4>
    <h4>HV Bus Discharge Status: {pcsHvBusDischargeStatus}</h4>
    <h4>Main State: {pcsMainState}</h4>
    <h4>Sub State: {pcsSubState}</h4>
    {get( L.PCS_dcdcFaulted) && <h4>PCS Faulted: ACTIVE</h4>}
    {get( L.PCS_dcdcOutputIsLimited) && <h4>Output Is Limited: ACTIVE</h4>}
    <h4>Max Output Current Allowed: {PCS_dcdcMaxOutputCurrentAllowed} A</h4>
    <h4>Precharge Rty Cnt: {falseTrue[get( L.PCS_dcdcPrechargeRtyCnt) as number]}</h4>
    <h4>12V Support Rty Cnt: {falseTrue[get( L.PCS_dcdc12VSupportRtyCnt) as number]}</h4>
    <h4>Discharge Rty Cnt: {falseTrue[get( L.PCS_dcdcDischargeRtyCnt) as number]}</h4>
    {get( L.PCS_dcdcPwmEnableLine) && <h4>PWM Enable Line: ACTIVE</h4>}
    {get( L.PCS_dcdcSupportingFixedLvTarget) && <h4>Supporting Fixed LV Target: ACTIVE</h4>}
    <h4>Precharge Restart Cnt: {falseTrue[get( L.PCS_dcdcPrechargeRestartCnt) as number]}</h4>
    <h4>Initial Precharge Substate: {pcsInitialPrechargeSubState}</h4>

    {/* 0x3C4 PCS_info */}
    <h4>PCS_buildConfigId: {get( L.PCS_info_buildConfigId)}</h4>
    <h4>PCS_hardwareId: {get( L.PCS_info_hardwareId)}</h4>
    <h4>PCS_componentId: {get( L.PCS_info_componentId)}</h4>

    {/* 0x2C4 708 PCS_logging */}
    <h4>PCS_dcdcMaxLvOutputCurrent: {PCS_dcdcMaxLvOutputCurrent} A</h4>
    <h4>PCS_dcdcCurrentLimit: {PCS_dcdcCurrentLimit} A</h4>
    <h4>PCS_dcdcLvOutputCurrentTempLimit: {PCS_dcdcLvOutputCurrentTempLimit} A</h4>
    <h4>PCS_dcdcUnifiedCommand: {PCS_dcdcUnifiedCommand}</h4>
    <h4>PCS_dcdcCLAControllerOutput: {PCS_dcdcCLAControllerOutput}</h4>
    <h4>PCS_dcdcTankVoltage: {get( L.PCS_dcdcTankVoltage)} V</h4>
    <h4>PCS_dcdcTankVoltageTarget: {get( L.PCS_dcdcTankVoltageTarget)} V</h4>
    <h4>PCS_dcdcClaCurrentFreq: {PCS_dcdcClaCurrentFreq} kHz</h4>
    <h4>PCS_dcdcTCommMeasured: {PCS_dcdcTCommMeasured} us</h4>
    <h4>PCS_dcdcShortTimeUs: {PCS_dcdcShortTimeUs} us</h4>
    <h4>PCS_dcdcHalfPeriodUs: {PCS_dcdcHalfPeriodUs} us</h4>
    <h4>PCS_dcdcIntervalMaxFrequency: {get( L.PCS_dcdcIntervalMaxFrequency)} kHz</h4>
    <h4>PCS_dcdcIntervalMaxHvBusVolt: {PCS_dcdcIntervalMaxHvBusVolt} V</h4>
    <h4>PCS_dcdcIntervalMaxLvBusVolt: {PCS_dcdcIntervalMaxLvBusVolt} V</h4>
    <h4>PCS_dcdcIntervalMaxLvOutputCurr: {get( L.PCS_dcdcIntervalMaxLvOutputCurr)} A</h4>
    <h4>PCS_dcdcIntervalMinFrequency: {get( L.PCS_dcdcIntervalMinFrequency)} kHz</h4>
    <h4>PCS_dcdcIntervalMinHvBusVolt: {PCS_dcdcIntervalMinHvBusVolt} V</h4>
    <h4>PCS_dcdcIntervalMinLvBusVolt: {PCS_dcdcIntervalMinLvBusVolt} V</h4>
    <h4>PCS_dcdcIntervalMinLvOutputCurr: {get( L.PCS_dcdcIntervalMinLvOutputCurr)} A</h4>
    <h4>PCS_dcdc12vSupportLifetimekWh: {PCS_dcdc12vSupportLifetimekWh} kWh</h4>

    {/* 0x310 HVP_info */}
    <h4>HVP_buildConfigId: {get( L.HVP_info_buildConfigId)}</h4>
    <h4>HVP_hardwareId: {get( L.HVP_info_hardwareId)}</h4>
    <h4>HVP_componentId: {get( L.HVP_info_componentId)}</h4>

    {/* 0x7AA 1962 HVP_debugMessage */}
    <h4>HVP_battery12V: {HVP_battery12V} V</h4>
    <h4>HVP_dcLinkVoltage: {HVP_dcLinkVoltage} V</h4>
    <h4>HVP_packVoltage: {HVP_packVoltage} V</h4>
    <h4>HVP_packContVoltage: {HVP_packContVoltage} V</h4>
    <h4>HVP_packContCoilCurrent: {HVP_packContCoilCurrent} A</h4>
    <h4>HVP_pyroAnalog: {HVP_pyroAnalog} V</h4>
    <h4>HVP_hvp1v5Ref: {HVP_hvp1v5Ref} V</h4>
    <h4>HVP_hvilInVoltage: {HVP_hvilInVoltage} V</h4>
    <h4>HVP_hvilOutVoltage: {HVP_hvilOutVoltage} V</h4>

    {/* HVP GPIO faults */}
    {get( L.HVP_gpioPassivePyroDepl) && <h4>HVP_gpioPassivePyroDepl: ACTIVE</h4>}
    {get( L.HVP_gpioPyroIsoEn) && <h4>HVP_gpioPyroIsoEn: ACTIVE</h4>}
    {get( L.HVP_gpioCpFaultIn) && <h4>HVP_gpioCpFaultIn: ACTIVE</h4>}
    {get( L.HVP_gpioPackContPowerEn) && <h4>HVP_gpioPackContPowerEn: ACTIVE</h4>}
    {get( L.HVP_gpioHvCablesOk) && <h4>HVP_gpioHvCablesOk: ACTIVE</h4>}
    {get( L.HVP_gpioHvpSelfEnable) && <h4>HVP_gpioHvpSelfEnable: ACTIVE</h4>}
    {get( L.HVP_gpioLed) && <h4>HVP_gpioLed: ACTIVE</h4>}
    {get( L.HVP_gpioCrashSignal) && <h4>HVP_gpioCrashSignal: ACTIVE</h4>}
    {get( L.HVP_gpioShuntDataReady) && <h4>HVP_gpioShuntDataReady: ACTIVE</h4>}
    {get( L.HVP_gpioFcContPosAux) && <h4>HVP_gpioFcContPosAux: ACTIVE</h4>}
    {get( L.HVP_gpioFcContNegAux) && <h4>HVP_gpioFcContNegAux: ACTIVE</h4>}
    {get( L.HVP_gpioBmsEout) && <h4>HVP_gpioBmsEout: ACTIVE</h4>}
    {get( L.HVP_gpioCpFaultOut) && <h4>HVP_gpioCpFaultOut: ACTIVE</h4>}
    {get( L.HVP_gpioPyroPor) && <h4>HVP_gpioPyroPor: ACTIVE</h4>}
    {get( L.HVP_gpioShuntEn) && <h4>HVP_gpioShuntEn: ACTIVE</h4>}
    {get( L.HVP_gpioHvpVerEn) && <h4>HVP_gpioHvpVerEn: ACTIVE</h4>}
    {get( L.HVP_gpioPackCoontPosFlywheel) && <h4>HVP_gpioPackCoontPosFlywheel: ACTIVE</h4>}
    {get( L.HVP_gpioCpLatchEnable) && <h4>HVP_gpioCpLatchEnable: ACTIVE</h4>}
    {get( L.HVP_gpioPcsEnable) && <h4>HVP_gpioPcsEnable: ACTIVE</h4>}
    {get( L.HVP_gpioPcsDcdcPwmEnable) && <h4>HVP_gpioPcsDcdcPwmEnable: ACTIVE</h4>}
    {get( L.HVP_gpioPcsChargePwmEnable) && <h4>HVP_gpioPcsChargePwmEnable: ACTIVE</h4>}
    {get( L.HVP_gpioFcContPowerEnable) && <h4>HVP_gpioFcContPowerEnable: ACTIVE</h4>}
    {get( L.HVP_gpioHvilEnable) && <h4>HVP_gpioHvilEnable: ACTIVE</h4>}
    {get( L.HVP_gpioSecDrdy) && <h4>HVP_gpioSecDrdy: ACTIVE</h4>}

    <h4>HVP_shuntCurrentDebug: {HVP_shuntCurrentDebug} A</h4>
    <h4>HVP_packCurrentMia: {boolYN(L.HVP_packCurrentMia)}</h4>
    <h4>HVP_auxCurrentMia: {boolYN(L.HVP_auxCurrentMia)}</h4>
    <h4>HVP_currentSenseMia: {boolYN(L.HVP_currentSenseMia)}</h4>
    <h4>HVP_shuntRefVoltageMismatch: {boolYN(L.HVP_shuntRefVoltageMismatch)}</h4>
    <h4>HVP_shuntThermistorMia: {boolYN(L.HVP_shuntThermistorMia)}</h4>
    <h4>HVP_shuntHwMia: {boolYN(L.HVP_shuntHwMia)}</h4>

    {/* Active alert-matrix faults (0x320 BMS / 0x3A4 PCS / 0x31E CP) */}
    <h3>Active Faults: {totalActive}</h3>
    {totalActive > 0 && <>
      <table style="border-collapse: collapse; margin: 0 auto;">
        <tr>
          <th style="text-align:left;padding:2px 20px 2px 0">ECU</th>
          <th style="text-align:left;padding:2px 0">Description</th>
        </tr>
        {alertGroups.map(g =>
          g.active.map((active, i) => {
            if (!active) return null;
            const code = g.base + i;
            return <tr>
              <td style="text-align:left;padding:2px 20px 2px 0">{g.label}</td>
              <td style="text-align:left;padding:2px 0" data-dtc-code={code}>{code}</td>
            </tr>;
          })
        )}
      </table>
      <div style="margin-top:15px;padding:12px;background:#1e1e2e;border:1px solid #444;border-radius:8px;">
        <p id="dtcJsonStatus" style="margin:0 0 8px 0;color:#aaa;font-size:.95em;"></p>
        <div id="dtcJsonFileContainer" style="display:none;">
          <p style="margin:4px 0;color:#ccc;font-size:.9em;">
            <strong>Load DTC descriptions from a local JSON file</strong>
            (e.g. <em>{DTC_JSON_FILE}</em>):
          </p>
          <input type="file" id="dtcJsonFile" accept=".json"
            style="color:#ccc;background:#2a2a3e;border:1px solid #555;border-radius:4px;padding:4px 8px;cursor:pointer;" />
        </div>
      </div>
      <div dangerouslySetInnerHTML={{__html: dtcLoaderScript()}} />
    </>}
  </div>;
}
