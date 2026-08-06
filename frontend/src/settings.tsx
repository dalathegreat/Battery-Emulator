import { useEffect, useMemo, useRef, useState } from "preact/hooks";

import { Button } from "./components/button.tsx";
import { Show, Form, selectField, checkboxField, ipField, textPatternField, passwordField } from "./components/forms.tsx";

import { apiPost, refreshApi, useGetApi } from "./utils/api.tsx";
import { reboot } from "./utils/reboot.tsx";

const INTERFACES : [string, string][] = [
    ["3", "Native CAN"],
    ["4", "Native CAN FD"],
    ["5", "CAN MCP 2515 add-on"],
    ["6", "CAN FD MCP 2518 add-on"],
    ["1", "Modbus RTU (RS485)"],
    ["2", "Modbus TCP"]
];

function validate(data: any) {
    const errors: any = {};
    if(data.get('inverter')==='2') {
        errors.inverter = "don't choose 2";
    }
    if(data.get('HTTPPASS') !== data.get('HTTPPASSCONFIRM')) {
        errors.HTTPPASSCONFIRM = "Web interface passwords do not match";
    }
    return errors;
};

export function Settings() {
    const settings = useGetApi('/api/internal/settings');
    const status = useGetApi('/api/status');
    // The current modifications the user has made
    const [current, setCurrent] = useState<{[index: string]:string}>({});

    // When we save, we mustn't clear the modifications ('current') until we
    // get the refreshed settings back from the GET endpoint, else it flickers
    // back to the old values briefly.
    // Use a flag which does the actual clearing once the settings update.
    const pendingClear = useRef(false);
    useEffect(() => {
        if(pendingClear.current) {
            pendingClear.current = false;
            setCurrent({});
        }
    }, [settings?.settings]);

    const submit = async () => {
        // Only submit actually changed values that we have been tracking
        // ourselves (the form's own data is ignored).
        
        const post_data = new FormData();
        for(const [k, v] of Object.entries(current)) {
            post_data.set(k, v);
        }

        try {
            await apiPost('/api/internal/settings', Object.fromEntries(post_data));
        } catch (e: any) {
            alert(e.message || "Failed to save settings");
            return;
        }

        // Indicate that `current` should be cleared once the refreshed settings
        // are received.
        pendingClear.current = true;
        refreshApi();
        window.scrollTo(0,0);
    };

    const batteries: {[index: string]:string} = {};
    for(let i=0; i<settings?.batteries.length; i++) {
        if(settings.batteries[""+i]) batteries[""+i] = settings.batteries[i];
    }
    const inverters: {[index: string]:string} = {};
    for(let i=0; i<settings?.inverters.length; i++) {
        if(settings.inverters[i]) inverters[i] = settings.inverters[i];
    }

    const reboot_required = settings?.reboot_required;
    
    // Transform the retrieved settings, applying any local changes at the time
    // that the settings were retrieved. The 'current' dict is deliberately not
    // in the sensitivity list - we don't want this updating everytime a field
    // is changed or it'll fight the user.
    // This value is then used to populate the form fields.
    const initial_settings = useMemo(() => {
        const s = { ...settings?.settings, ...current };
        return s;
    }, [settings?.settings]);
    // Merge the retrieved settings with any unsaved changes - this is used by
    // dependent fields to decide whether to hide/show, and thus does update as
    // the user edits.
    // This is used for dynamic hiding/showing of fields based on other field
    // values, and thus must update as the user edits.
    const live_settings = { ...settings?.settings, ...current };

    const custom_bms = ["6", "11", "22", "23", "24", "31", "41", "48", "49", "51"].includes(""+live_settings.BATTTYPE);
    const estimated = ["3", "4", "6", "8", "14", "16", "24", "32", "33", "40", "41", "50", "51"].includes(""+live_settings.BATTTYPE);
    const tesla = ["32", "33"].includes(""+live_settings.BATTTYPE);
    const daly = ["23"].includes(""+live_settings.BATTTYPE);
    const socestimated = ["16", "26", "41", "42"].includes(""+live_settings.BATTTYPE);
    const manual_balancing = tesla;
    const pylonish = ["4", "10", "19"].includes(""+live_settings.INVTYPE);
    const byd = ""+live_settings.INVTYPE==="2";
    const bydmodbus = ""+live_settings.INVTYPE==="3";
    const kostal = ""+live_settings.INVTYPE==="9";
    const pylon = ""+live_settings.INVTYPE==="10";
    const sofar = ""+live_settings.INVTYPE==="17";
    const solax = ""+live_settings.INVTYPE==="18";
    const sungrow = ""+live_settings.INVTYPE==="21";
    const foxess = ""+live_settings.INVTYPE==="5";

    const custom_clamp = ""+live_settings.SHUNTTYPE==="3";

    // Current DHCP lease, used to hint and prefill the static IP fields.
    // Nothing is suggested without a station link (0.0.0.0 is not an address
    // worth pinning).
    const pickLease = (v: any) => (typeof v === 'string' && v.includes('.') && v !== '0.0.0.0') ? v : undefined;
    const lease: {[name: string]: string|undefined} = {
        LOCALIP: pickLease(status?.ip),
        GATEWAY: pickLease(status?.gateway),
        SUBNET: pickLease(status?.subnet),
        DNS: pickLease(status?.dns),
    };

    return <div>
        <h2>Settings</h2>

        { !!settings && <div>

        <Form initial={initial_settings}
              changed={(k, v) => {
                if(k==='STATICIP' && v==='1') {
                    // Ticking "Use static IP address" adopts the addresses
                    // currently in use (the DHCP lease), like the old page.
                    const cur = {...current};
                    for(const n of ['LOCALIP', 'GATEWAY', 'SUBNET', 'DNS']) {
                        const el = document.querySelector<HTMLInputElement>(`input[name="${n}"]`);
                        if(!el || el.value) continue;
                        const ph = lease[n];
                        if(ph) {
                            el.value = ph;   // show it in the field
                            cur[n] = ph;     // and track it for submission
                        }
                    }
                    cur.STATICIP = '1';
                    setCurrent(cur);
                    return;
                }
                if(v==='' && (settings.settings[k]===undefined || settings.settings[k]==='')) {
                    const cur = {...current};
                    delete cur[k];
                    setCurrent(cur);
                } else {
                    setCurrent({
                        ...current,
                        [k]: v
                    });
                }
              }}
              validate={validate}
              submit={submit}
              >
        
        <div class="form__with-aside"><div class="form__main">

        <div class="panel">
            <h3>Battery</h3>
            { selectField("Battery", "BATTTYPE", batteries) }
            <Show when={parseInt(live_settings.BATTTYPE)>0}>
                { selectField("Battery interface", "BATTCOMM", INTERFACES) }
                <Show when={custom_bms}>
                    { textPatternField("Battery max design voltage (V)", "BATTPVMAX", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Battery min design voltage (V)", "BATTPVMIN", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Cell max design voltage (mV)", "BATTCVMAX", "[0-9]+") }
                    { textPatternField("Cell min design voltage (mV)", "BATTCVMIN", "[0-9]+") }
                </Show>
                <Show when={""+live_settings.BATTTYPE==="34"}>
                    { textPatternField("Fake battery voltage (V)", "TMP_FAKEBATTERYV", "[0-9]+(\\.[0-9]+)?") }
                </Show>
                { checkboxField("Double battery", "DBLBTR") }
                <Show indent={true} when={live_settings.DBLBTR}>
                    { selectField("Second battery interface", "BATT2COMM", INTERFACES) }
                    { checkboxField("Triple battery", "TRIBTR") }
                    <Show indent={true} when={live_settings.TRIBTR}>
                        { selectField("Third battery interface", "BATT3COMM", INTERFACES) }
                    </Show>
                </Show>
                { textPatternField("Battery capacity (Wh)", "BATTERY_WH_MAX", "|[1-9][0-9]*") }
                { checkboxField("Rescale SoC", "USE_SCALED_SOC") }
                <Show indent={true} when={live_settings.USE_SCALED_SOC}>
                    { textPatternField("SoC max percentage", "MAXPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                    { textPatternField("SoC min percentage", "MINPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                </Show>
                { textPatternField("Max charge current (A)", "MAXCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                { textPatternField("Max discharge current (A)", "MAXDISCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                <Show when={daly}>
                    { textPatternField("Daly: power limit per % SOC (W/%)", "DALYPWRPCT", "[0-9]+") }
                    { textPatternField("Daly: voltage difference for start of voltage based discharge limit (dV)", "DALYDVSTART", "[0-9]+") }
                    { textPatternField("Daly: max power per dV distance from minimum voltage (W/dV)", "DALYPWRDV", "[0-9]+") }
                    { textPatternField("Daly: power change per °C above/below 0°C (W/°C)", "DALYPWRDEG", "[0-9]+") }
                    { textPatternField("Daly: power at 0°C (W)", "DALYPWR0C", "[0-9]+") }
                </Show>
                { checkboxField("Manual voltage limits", "USEVOLTLIMITS") }
                <Show indent={true} when={live_settings.USEVOLTLIMITS}>
                    { textPatternField("Max charge voltage (V)", "TARGETCHVOLT", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Min discharge voltage (V)", "TARGETDISCHVOLT", "[0-9]+(\\.[0-9]+)?") }
                </Show>
                <Show when={""+live_settings.BATTTYPE=="21"}>
                    { checkboxField("Require interlock", "INTERLOCKREQ") }
                </Show>
                <Show when={tesla}>
                    { checkboxField("Digital HVIL (2024+)", "DIGITALHVIL") }
                    { checkboxField("Right hand drive", "GTWRHD") }
                    { textPatternField("Country code", "GTWCOUNTRY", "[0-9]{5}") }
                    { textPatternField("Map region", "GTWMAPREG", "[0-9]") }
                    { textPatternField("Chassis type", "GTWCHASSIS", "[0-9]") }
                    { textPatternField("Pack type", "GTWPACK", "[0-9]") }
                </Show>
                <Show when={estimated}>
                    { textPatternField("Manual charging power (W)", "CHGPOWER", "[0-9]+") }
                    { textPatternField("Manual discharging power (W)", "DCHGPOWER", "[0-9]+") }
                </Show>
                <Show when={socestimated}>
                    { checkboxField("Use estimated SoC", "SOCESTIMATED") }
                </Show>
                <Show when={manual_balancing}>
                    { checkboxField("Manual LFP balancing enabled", "TMP_BALANCE") }
                    { textPatternField("Balancing max time (min)", "TMP_BALTIME", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Balancing float power (W)", "TMP_BALFLOATPOWER", "[0-9]+") }
                    { textPatternField("Max battery voltage (V)", "TMP_BALMAXPACKV", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Max cell voltage (mV)", "TMP_BALMAXCELLV", "[0-9]+") }
                    { textPatternField("Max cell voltage deviation (mV)", "TMP_BALMAXDEVCELLV", "[0-9]+") }
                </Show>
                { selectField("Battery chemistry", "BATTCHEM", {
                    "3": "LFP",
                    "1": "NCA",
                    "2": "NMC",
                }) }
            </Show>
        </div>

        <div class="panel">
            <h3>Inverter</h3>
            { selectField("Inverter protocol", "INVTYPE", inverters) }
            <Show when={parseInt(live_settings.INVTYPE)>0}>
                { selectField("Inverter interface", "INVCOMM", INTERFACES) }
                { checkboxField("Inverter limits low pass filter", "LOWPASSFILTER") }
                { checkboxField("Charge power tapering based on SOC", "CHGTAPERSOC") }
                <Show indent={true} when={live_settings.CHGTAPERSOC}>
                    { textPatternField("Start tapering at SOC (percent)", "CHGTAPERSTART", "[0-9]+", "95 = full power until 95%, then linear reduction reaching 0W at 100% scaled SOC") }
                    { textPatternField("Float charge power (W)", "CHGTAPERFLOOR", "[0-9]+", "Minimum charge power held during tapering. 0 disables the floor") }
                </Show>
                { checkboxField("Allow longer CAN timeout", "SLOWCANINV") }
                <Show when={sofar}>
                    { textPatternField("Sofar Battery ID (0-15)", "SOFAR_ID", "[0-9]{1,2}") }
                </Show>
                <Show when={pylon}>
                    { selectField("Pylon manufacturer", "PYLONBRAND", {
                        "0": "PYLONTECH",
                        "1": "PYLON",
                        "2": "DEYE",
                    }) }
                    { textPatternField("Pylon, send group (0-1)", "PYLONSEND", "[0-1]") }
                    { checkboxField("Pylon, 30k offset", "PYLONOFFSET") }
                    { checkboxField("Pylon, invert byteorder", "PYLONORDER") }
                    { textPatternField("Pylon CAN baudrate (kbps)", "PYLONBAUD", "[0-9]+", "Usually 500, sometimes 250") }
                </Show>
                { checkboxField("Inverter run entirely offgrid", "INVOFFGRID") }
                <Show when={byd}>
                    { checkboxField("Deye offgrid specific fixes", "DEYEBYD") }
                </Show>
                <Show when={bydmodbus}>
                    { checkboxField("Fronius Primo, 450V maxvoltage cap", "PRIMOGEN24") }
                </Show>
                <Show when={pylonish}>
                    { textPatternField("Reported cell count (0 for default)", "INVCELLS", "[0-9]+") }
                </Show>
                <Show when={pylonish||solax}>
                    { textPatternField("Reported module count (0 for default)", "INVMODULES", "[0-9]+") }
                </Show>
                <Show when={pylonish}>
                    { textPatternField("Reported cells per module (0 for default)", "INVCELLSPER", "[0-9]+") }
                    { textPatternField("Reported voltage level (0 for default)", "INVVLEVEL", "[0-9]+") }
                    { textPatternField("Reported Ah capacity (0 for default)", "INVCAPACITY", "[0-9]+") }
                </Show>
                <Show when={solax}>
                    { textPatternField("Reported battery type (in decimal)", "INVBTYPE", "[0-9]+") }
                </Show>
                <Show when={foxess}>
                    { textPatternField("FoxESS battery type (0 for default)", "FOXESSTYPE", "[0-9]+") }
                    { textPatternField("FoxESS battery subtype (0 for default)", "FOXESSSUBTYPE", "[0-9]+") }
                    { textPatternField("FoxESS module count (0 for default)", "FOXESSMODULES", "[0-9]+") }
                </Show>
                <Show when={solax||kostal}>
                    { selectField("Inverter contactor workaround", "INVICNT", {
                        "0": "No Workaround",
                        "1": "Keep contactors always closed",
                        "2": "Lock contactors closed after first close request",
                    }) }
                </Show>
                <Show when={sungrow}>
                    { selectField("Sungrow model", "INVSUNTYPE", {
                        "0": "SBR064 (6.4 kWh, 2 modules)",
                        "1": "SBR096 (9.6 kWh, 3 modules)",
                        "2": "SBR128 (12.8 kWh, 4 modules)",
                        "3": "SBR160 (16.0 kWh, 5 modules)",
                        "4": "SBR192 (19.2 kWh, 6 modules)",
                        "5": "SBR224 (22.4 kWh, 7 modules)",
                        "6": "SBR256 (25.6 kWh, 8 modules)",
                    }) }
                </Show>
            </Show>
        </div>

        <div class="panel">
            <h3>Charger/shunt</h3>
            { selectField("Charger", "CHGTYPE", {
                "0": "None",
                "2": "Chevy Volt Gen1 Charger",
                "1": "Nissan LEAF 2013-2024 PDM charger",
            }) }
            <Show when={parseInt(live_settings.CHGTYPE)>0}>
                { selectField("Charger interface", "CHGCOMM", INTERFACES) }
                { textPatternField("Charging voltage (V)", "TMP_CHARGERSETPOINTV", "[0-9]+(\\.[0-9]+)?") }
                { textPatternField("Charging current (A)", "TMP_CHARGERSETPOINTA", "[0-9]+(\\.[0-9]+)?") }
                { textPatternField("Stop charging below current (A)", "TMP_CHARGERENDA", "[0-9]+(\\.[0-9]+)?") }
                { checkboxField("Enable HV DC output", "TMP_CHARGERHVENABLED") }
                { checkboxField("Enable 12V aux output", "TMP_CHARGERAUX12VENABLED") }
            </Show>
            { selectField("Shunt", "SHUNTTYPE", {
                "0": "None",
                "1": "BMW SBOX",
                "2": "Using inverter values",
                "3": "Custom Clamp"
            }) }
            <Show when={parseInt(live_settings.SHUNTTYPE)>0}>
                { selectField("Shunt interface", "SHUNTCOMM", INTERFACES) }
            </Show>
            <Show when={custom_clamp}>
                { textPatternField("CT Clamp offset (mV)", "CTOFFSET", "-?[0-9]+", "Voltage offset required to calibrate 0A reading. -1 = auto-detect") }
                { textPatternField("CT Clamp nominal voltage (dV)", "CTVNOM", "[1-5]?[0-9]?[0-9]") }
                { textPatternField("CT Clamp nominal current (A)", "CTANOM", "[1-5]?[0-9]?[0-9]") }
                { selectField("ESP32 pin attenuation", "CTATTEN", {
                    "0": "0 dB (max 950 mV)",
                    "1": "2.5 dB (max 1250 mV)",
                    "2": "6 dB (max 1750 mV)",
                    "3": "11 dB (max 3100 mV)",
                }) }
                { checkboxField("Invert CT current (charging positive)", "CTINVERT") }
            </Show>
        </div>

        <div class="panel">
            <h3>Hardware</h3>
            { selectField("Equipment stop button", "EQSTOP", {
                "0": "Not connected",
                "1": "Latching switch",
                "2": "Momentary switch",
            }) }

            { checkboxField("Contactor control via GPIO", "CNTCTRL") }
            <Show when={live_settings.DBLBTR}>
                { checkboxField("Double-Battery Contactor control via GPIO", "CNTCTRLDBL") }
            </Show>
            <Show when={live_settings.TRIBTR}>
                { checkboxField("Triple-Battery Contactor control via GPIO", "CNTCTRLTRI") }
            </Show>
           
            <Show indent={true} when={live_settings.CNTCTRL}>
                { textPatternField("Precharge time (ms)", "PRECHGMS", "[0-9]+") }
                { checkboxField("Normally Closed (NC) contactors", "NCCONTACTOR") }
                { checkboxField("PWM contactor control", "PWMCNTCTRL") }
                <Show indent={true} when={live_settings.PWMCNTCTRL}>
                    { textPatternField("PWM Frequency (Hz)", "PWMFREQ", "[0-9]+") }
                    { textPatternField("PWM Hold (0-1023)", "PWMHOLD", "[0-9]+") }
                </Show>
            </Show>
            
            { checkboxField("Periodic BMS reset every 24h", "PERBMSRESET") }
            <Show indent={true} when={live_settings.PERBMSRESET}>
                { selectField("Periodic BMS reset interval", "PERBMSRESETH", {
                    "24": "24h",
                    "48": "48h",
                }) }
                { checkboxField("Defer reset if SOC less than 15%", "PERBMSDEFSOC") }
                { checkboxField("Skip reset for one period if balancing", "PERBMSSKIPBAL") }
            </Show>
            { textPatternField("BMS reset off time (ms)", "BMSRESETDUR", "[0-9]+") }
            { checkboxField("Start undercharged emergency recovery mode", "TMP_RECOVERYMODE") }
            { checkboxField("External precharge via HIA4V1", "EXTPRECHARGE") }
            <Show indent={true} when={live_settings.EXTPRECHARGE}>
                { textPatternField("Precharge, maximum ms before fault", "MAXPRETIME", "[0-9]+") }
                { textPatternField("Precharge, maximum PWM frequency (Hz)", "MAXPREFREQ", "[0-9]+") }
                { checkboxField("Normally Open (NO) inverter disconnect contactor", "NOINVDISC") }
            </Show>

            { checkboxField("Measure CPU temperature", "MEASURECPUTEMP") }
            <Show indent={true} when={live_settings.MEASURECPUTEMP}>
                { textPatternField("CPU temperature calibration offset (°C)", "CPUTEMPOFFSET", "-?[0-9]+") }
            </Show>
            
            { selectField("Status LED pattern", "LEDMODE", {
                "0": "Classic",
                "1": "Energy Flow",
                "2": "Heartbeat",
                "3": "GRB Classic (2CAN only)",
                "4": "GRB Energy Flow (2CAN only)",
                "5": "GRB Heartbeat (2CAN only)",
            }) }

            { selectField("Configurable port", "GPIOOPT1", {
                "0": "WUP1 / WUP2",
                "1": "I2C Display (SSD1306)",
                "2": "E-Stop / BMS Power",
            }) }

            { selectField("BMS power pin", "GPIOOPT2", {
                "0": "Pin 18",
                "1": "Pin 25",
            }) }

            { selectField("SMA enable pin", "GPIOOPT3", {
                "0": "Pin 5",
                "1": "Pin 33",
            }) }

            { selectField("uSD Slot", "GPIOOPT4", {
                "0": "uSD Card",
                "1": "I2C Display (SSD1306)",
            }) }

            { selectField("BMS power pin (Stark CMR)", "GPIOOPT5", {
                "0": "Pin 23 (BMS POWER)",
                "1": "Pin 25 (PRECHARGE)",
            }) }

            { selectField("GPIO 1/2 function (Waveshare)", "GPIOOPT6", {
                "0": "Status LED (GPIO2)",
                "1": "I2C Display SSD1306 (GPIO1=SDA, GPIO2=SCL)",
            }) }
        </div>

        <div class="panel">
            <h3>Connectivity</h3>
            <div class="form-row">
                <label>WiFi SSID</label>
                <input type="text" name="SSID" pattern="[ -~]{1,63}" />
            </div>
            <div class="form-row">
                <label>WiFi password</label>
                <input type="password" name="PASSWORD" pattern="[ -~]{8,63}" title="at least 8 printable ASCII characters" placeholder="unchanged" />
            </div>
            { checkboxField("Enable WiFi access point", "WIFIAPENABLED") }
            <Show indent={true} when={live_settings.WIFIAPENABLED}>
                <div class="form-row">
                    <label>WiFi access point name</label>
                    <input type="text" name="APNAME" pattern="[ -~]{1,63}" />
                </div>
                <div class="form-row">
                    <label>WiFi access point password</label>
                    <input type="password" name="APPASSWORD" pattern="[ -~]{8,63}" title="at least 8 printable ASCII characters" placeholder="unchanged" />
                </div>
                <div class="form-row">
                    <label>WiFi channel (0 for automatic)</label>
                    <input type="text" name="WIFICHANNEL" pattern="[0-9]|1[0-4]" title="number" />
                </div>
            </Show>
            <div class="form-row">
                <label>Custom WiFi hostname (blank for default)</label>
                <input type="text" name="HOSTNAME" pattern="[A-Za-z0-9\-]+" title="letters, numbers, hyphen" />
            </div>
            { checkboxField("Use static IP address", "STATICIP") }
            <Show indent={true} when={live_settings.STATICIP}>
                { ipField("IP address", "LOCALIP", lease.LOCALIP) }
                { ipField("Gateway", "GATEWAY", lease.GATEWAY) }
                { ipField("Subnet", "SUBNET", lease.SUBNET) }
                { ipField("DNS server", "DNS", lease.DNS) }
            </Show>

            { checkboxField("Enable password protection", "WEBAUTH") }
            <Show indent={true} when={live_settings.WEBAUTH}>
                { textPatternField("Web interface username", "HTTPUSER", "[ -~]{1,32}") }
                { passwordField("Web interface password", "HTTPPASS") }
                { passwordField("Repeat web interface password", "HTTPPASSCONFIRM") }
            </Show>

            { checkboxField("Enable ESPNow", "ESPNOWENABLED") }

            { checkboxField("Enable MQTT", "MQTTENABLED") }
            <Show indent={true} when={live_settings.MQTTENABLED}>
                { textPatternField("MQTT server", "MQTTSERVER", "") }
                { textPatternField("MQTT port", "MQTTPORT", "[0-9]+") }
                { textPatternField("MQTT user", "MQTTUSER", "") }
                { passwordField("MQTT password", "MQTTPASSWORD") }
                { textPatternField("MQTT timeout (ms)", "MQTTTIMEOUT", "[0-9]+") }
                { checkboxField("Send all cell voltages via MQTT", "MQTTCELLV") }
                { textPatternField("MQTT publish interval (ms)", "MQTTPUBLISHMS", "[0-9]+") }
                { checkboxField("Remote BMS reset via MQTT allowed", "REMBMSRESET") }
                { checkboxField("Enable Home Assistant auto discovery", "HADISC") }
                { textPatternField("Home Assistant auto discovery topic", "HADISCTOPIC", "[A-Za-z0-9_\\-]+") }
            </Show>
        </div>

        <div class="panel">
            <h3>Debug</h3>
            { checkboxField("Enable performance profiling on main page", "PERFPROFILE") }
            { checkboxField("Enable CAN message logging via USB serial", "CANLOGUSB") }
            { checkboxField("Enable general logging via USB serial", "USBENABLED") }
            { checkboxField("Enable general logging via Webserver", "WEBENABLED") }
            { checkboxField("Enable CAN message logging via SD card", "CANLOGSD") }
            { checkboxField("Enable general logging via SD card", "SDLOGENABLED") }
            { checkboxField("Enable general logging via syslog", "SYSLOGEN") }
            <Show indent={true} when={live_settings.SYSLOGEN}>
                { ipField("Syslog server IP", "SYSLOGIP") }
                { textPatternField("Syslog UDP port", "SYSLOGPORT", "[0-9]+") }
                { textPatternField("Syslog facility", "SYSLOGFAC", "[0-9]+") }
            </Show>
        </div>

        </div><div class="form__aside"><div>

            { reboot_required && <div class="alert">
                Settings saved, reboot to apply.
                <Button onClick={reboot}>Reboot now</Button>
            </div> }

            <button type="submit" disabled={!Object.keys(current).length}>Save settings</button>
        
        </div></div></div>


        </Form>

        </div> }

    </div>
}
