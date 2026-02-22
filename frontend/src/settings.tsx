import { useState } from "preact/hooks";
// import { signal } from '@preact/signals';
// console.log(signal);

import { Button } from "./components/button.tsx";
import { Show, Form, selectField, checkboxField, ipField, textPatternField } from "./components/forms.tsx";

import { useGetApi } from "./utils/api.tsx";
import { reboot } from "./utils/reboot.tsx";

const INTERFACES = {
    "6": "CAN FD MCP 2518 add-on",
    "5": "CAN MCP 2515 add-on",
    "1": "Modbus RTU (RS485)",
    "3": "Native CAN",
    "4": "Native CAN FD",
    "2": "Modbus TCP"
};

export function Settings() {
    const settings = useGetApi('/api/settings');
    const [savedSettings, setSavedSettings] = useState<any>(null);
    const [current, setCurrent] = useState<{[index: string]:string}>({});

    const validate = (data: any) => {
        const errors: any = {};
        if(data.get('inverter')==='2') {
            errors.inverter = "don't choose 2";
        }
        return errors;
    };

    const splitIp = (data: any, field: string) => {
        // LOCALIP=1.2.3.4 gets split into LOCALIP1=1, LOCALIP2=2, LOCALIP3=3, LOCALIP4=4
        const v = data.get(field);
        if(v) {
            const parts = (v as string).split('.');
            for(let i=0; i<4; i++) {
                data.set(field + (i+1), parts.length===4 ? parts[i] : '0');
            }
        }
        data.delete(field);
    }

    const submit = async (data: any) => {
        splitIp(data, 'LOCALIP');
        splitIp(data, 'GATEWAY');
        splitIp(data, 'SUBNET');

        const r = await fetch(import.meta.env.VITE_API_BASE + '/api/settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(Object.fromEntries(data)),
        });
        if(r.status>=400) {
            alert('Failed to save settings: ' + JSON.stringify(await r.json()));
            return;
        }
        const rr = await r.json();
        // Successful save
        setCurrent({});
        setSavedSettings(rr);
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

    const reboot_required = settings?.reboot_required || savedSettings?.reboot_required;
    const merged = { ...settings?.settings, ...savedSettings?.settings, ...current };

    const custom_bms = ["6", "11", "22", "23", "24", "31"].includes(""+merged.BATTTYPE);
    const estimated = ["3", "4", "6", "14", "16", "24", "32", "33"].includes(""+merged.BATTTYPE);
    const tesla = ["32", "33"].includes(""+merged.BATTTYPE);
    const socestimated = ""+merged.BATTTYPE==="16";
    const pylonish = ["4", "10", "19"].includes(""+merged.INVTYPE);
    const byd = ""+merged.INVTYPE==="2";
    const kostal = ""+merged.INVTYPE==="9";
    const pylon = ""+merged.INVTYPE==="10";
    const sofar = ""+merged.INVTYPE==="17";
    const solax = ""+merged.INVTYPE==="18";

    return <div>
        <h2>Settings</h2>

        { !!settings && <div>

        { reboot_required && <div class="alert">
            Settings saved, reboot to apply.
            <Button onClick={reboot}>Reboot now</Button>
        </div> }

        <Form initial={settings.settings}
              changed={(k, v) => {
                setCurrent({
                    ...current,
                    [k]: v
                });
              }}
              validate={validate}
              submit={submit}
              >
        
        <div class="panel">
            <h3>Battery</h3>
            { selectField("Battery", "BATTTYPE", batteries) }
            <Show when={""+merged.BATTTYPE!=="0"}>
                <Show when={custom_bms}>
                    { textPatternField("Battery max design voltage (V)", "BATTPVMAX", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Battery min design voltage (V)", "BATTPVMIN", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Cell max design voltage (mV)", "BATTCVMAX", "[0-9]+") }
                    { textPatternField("Cell min design voltage (mV)", "BATTCVMIN", "[0-9]+") }
                </Show>
                { checkboxField("Double battery", "DBLBTR") }
                <Show indent={true} when={merged.DBLBTR}>
                    { selectField("Second battery interface", "BATT2COMM", INTERFACES) }
                    { checkboxField("Triple battery", "TRIBTR") }
                    <Show indent={true} when={merged.TRIBTR}>
                        { selectField("Third battery interface", "BATT3COMM", INTERFACES) }
                    </Show>
                </Show>
                { textPatternField("Battery capacity (Wh)", "BATTERY_WH_MAX", "|[1-9][0-9]*") }
                { checkboxField("Rescale SoC", "USE_SCALED_SOC") }
                <Show indent={true} when={merged.USE_SCALED_SOC}>
                    { textPatternField("SoC max percentage", "MAXPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                    { textPatternField("SoC min percentage", "MINPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                </Show>
                { textPatternField("Max charge current (A)", "MAXCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                { textPatternField("Max discharge current (A)", "MAXDISCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                { checkboxField("Manual voltage limits", "USEVOLTLIMITS") }
                <Show indent={true} when={merged.USEVOLTLIMITS}>
                    { textPatternField("Max charge voltage (V)", "TARGETCHVOLT", "[0-9]+(\\.[0-9]+)?") }
                    { textPatternField("Min discharge voltage (V)", "TARGETDISCHVOLT", "[0-9]+(\\.[0-9]+)?") }
                </Show>
                <Show when={""+merged.BATTTYPE=="21"}>
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
                { selectField("Battery interface", "BATTCOMM", INTERFACES) }
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
            <Show when={""+merged.INVTYPE!=="0"}>
                { selectField("Inverter interface", "INVCOMM", INTERFACES) }
                <Show when={sofar}>
                    { textPatternField("Sofar Battery ID (0-15)", "SOFAR_ID", "[0-9]{1,2}") }
                </Show>
                <Show when={pylon}>
                    { textPatternField("Pylon, send group (0-1)", "PYLONSEND", "[0-1]") }
                    { checkboxField("Pylon, 30k offset", "PYLONOFFSET") }
                    { checkboxField("Pylon, invert byteorder", "PYLONORDER") }
                </Show>
                <Show when={byd}>
                    { checkboxField("Deye offgrid specific fixes", "DEYEBYD") }
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
                <Show when={solax||kostal}>
                    { checkboxField("Prevent inverter opening contactors", "INVICNT") }
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
            <Show when={""+merged.CHGTYPE!=="0"}>
                { selectField("Charger interface", "CHGCOMM", INTERFACES) }
            </Show>
            { selectField("Shunt", "SHUNTTYPE", {
                "0": "None",
                "1": "BMW SBOX",
            }) }
            <Show when={""+merged.SHUNTTYPE!=="0"}>
                { selectField("Shunt interface", "SHUNTCOMM", INTERFACES) }
            </Show>
        </div>

        <div class="panel">
            <h3>Hardware</h3>
            { checkboxField("Use CAN FD as classic CAN", "CANFDASCAN") }
            { textPatternField("CAN addon crystal (Mhz)", "CANFREQ", "[0-9]{1,2}") }
            { textPatternField("CAN-FD-addon crystal (Mhz)", "CANFDFREQ", "[0-9]{1,2}") }
            { selectField("Equipment stop button", "EQSTOP", {
                "0": "Not connected",
                "1": "Latching switch",
                "2": "Momentary switch",
            }) }

            { checkboxField("Contactor control via GPIO", "CNTCTRL") }
            <Show when={merged.DBLBTR}>
                { checkboxField("Double-Battery Contactor control via GPIO", "CNTCTRLDBL") }
            </Show>
            <Show when={merged.TRIBTR}>
                { checkboxField("Triple-Battery Contactor control via GPIO", "CNTCTRLTRI") }
            </Show>
           
            <Show indent={true} when={merged.CNTCTRL}>
                { textPatternField("Precharge time (ms)", "PRECHGMS", "[0-9]+") }
                { checkboxField("Normally Closed (NC) contactors", "NCCONTACTOR") }
                { checkboxField("PWM contactor control", "PWMCNTCTRL") }
                <Show indent={true} when={merged.PWMCNTCTRL}>
                    { textPatternField("PWM Frequency (Hz)", "PWMFREQ", "[0-9]+") }
                    { textPatternField("PWM Hold (0-1023)", "PWMHOLD", "[0-9]+") }
                </Show>
            </Show>

            { checkboxField("Double-Battery Contactor control via GPIO", "CNTCTRLDBL") }

            { checkboxField("Periodic BMS reset every 24h", "PERBMSRESET") }
            { textPatternField("Periodic BMS reset off time (s)", "BMSRESETDUR", "[0-9]+") }
            { checkboxField("External precharge via HIA4V1", "EXTPRECHARGE") }
            <Show indent={true} when={merged.EXTPRECHARGE}>
                { textPatternField("Precharge, maximum ms before fault", "MAXPRETIME", "[0-9]+") }
                { checkboxField("Normally Open (NO) inverter disconnect contactor", "NOINVDISC") }
            </Show>
                
            { selectField("Status LED pattern", "LEDMODE", {
                "0": "Classic",
                "1": "Energy Flow",
                "2": "Heartbeat",
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
        </div>

        <div class="panel">
            <h3>Connectivity</h3>
            <div class="form-row">
                <label>WiFi SSID</label>
                <input type="text" name="SSID" pattern="[ -~]{1,63}" />
            </div>
            <div class="form-row">
                <label>WiFi password</label>
                <input type="text" name="PASSWORD" pattern="[ -~]{8,63}" title="at least 8 printable ASCII characters" />
            </div>
            { checkboxField("Enable WiFi access point", "WIFIAPENABLED") }
            <Show indent={true} when={merged.WIFIAPENABLED}>
                <div class="form-row">
                    <label>WiFi access point name</label>
                    <input type="text" name="APNAME" pattern="[ -~]{1,63}" />
                </div>
                <div class="form-row">
                    <label>WiFi access point password</label>
                    <input type="text" name="APPASSWORD" pattern="[ -~]{8,63}" title="at least 8 printable ASCII characters" />
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
            <Show indent={true} when={merged.STATICIP}>
                { ipField("IP address", "LOCALIP") }
                { ipField("Gateway", "GATEWAY") }
                { ipField("Subnet", "SUBNET") }
            </Show>

            { checkboxField("Enable ESPNow", "ESPNOWENABLED") }

            { checkboxField("Enable MQTT", "MQTTENABLED") }
            <Show indent={true} when={merged.MQTTENABLED}>
                { textPatternField("MQTT server", "MQTTSERVER", "") }
                { textPatternField("MQTT port", "MQTTPORT", "[0-9]+") }
                { textPatternField("MQTT user", "MQTTUSER", "") }
                { textPatternField("MQTT password", "MQTTPASSWORD", "") }
                { textPatternField("MQTT timeout (ms)", "MQTTTIMEOUT", "[0-9]+") }
                { checkboxField("Send all cell voltages via MQTT", "MQTTCELLV") }
                { checkboxField("Remote BMS reset via MQTT allowed", "REMBMSRESET") }
                { checkboxField("Customized MQTT topics", "MQTTTOPICS") }
                <Show indent={true} when={merged.MQTTTOPICS}>
                    { textPatternField("Topic name", "MQTTTOPIC", "") }
                    { textPatternField("Prefix for object ID", "MQTTOBJIDPREFIX", "") }
                    { textPatternField("Home Assistant device name", "MQTTDEVICENAME", "") }
                    { textPatternField("Home Assistant device ID", "HADEVICEID", "") }
                </Show>
                { checkboxField("Enable Home Assistant auto discovery", "HADISC") }
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
        </div>

        <div class="actions">
            <button type="submit" disabled={!Object.keys(current).length}>Save settings</button>
        </div>


        </Form>

        </div> }

    </div>
}
