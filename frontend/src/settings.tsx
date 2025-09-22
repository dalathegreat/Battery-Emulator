import { useRef, useEffect, useState } from "preact/hooks";
// import { signal } from '@preact/signals';
// console.log(signal);


import { useGetApi } from "./utils/api.tsx";

// Shows or hides its children based on the "when" prop.
function Show({ when, children }: { when: boolean | string, children: preact.ComponentChildren }) {
    const ref = useRef<HTMLDivElement>(null);
    const prev = useRef<boolean | null>(null);
    const b = when === true || when === "1";
    useEffect(() => {
        if(!ref.current) return;
        if(b) {
            if(prev.current!==null) {
                // fade in
                ref.current.style.opacity = "0";
                ref.current.style.transition = "opacity 0.4s ease";
                setTimeout(() => {
                    if(ref.current) ref.current.style.opacity = "1";
                }, 50);
            }
            ref.current.style.display = "";
        } else {
            ref.current.style.display = "none";
        }
        prev.current = b;
    }, [ref, b]);
    return <div ref={ref}>
        { children }
    </div>;
}

// A semi-controlled form wrapper component. Populates initial values into the
// form fields on mount, but not thereafter.
//
// Accepts a "changed" callback that is called whenever a field changes, which
// can be used to show/hide fields via CSS (they should remain in the DOM or
// they'll lose their initial values).
//
// On submit, calls a "validate" callback if provided, which should return an
// object mapping field names to errors. If empty, validation passes.
//
// If validation passes, calls "submit" callback if provided, passing the
// FormData object.

function Form({ children, initial, changed, validate, submit }: { 
    children: preact.ComponentChildren, 
    initial?: any,
    changed?: (key: string, value: string) => void,
    validate?: (data: FormData) => any,
    submit?: (data: FormData) => any
}) {
	const form = useRef(null);

    useEffect(() => {
        if(form.current) {
            for(const [k, v] of Object.entries(initial ?? {})) {
                const el = (form.current[k] as any);
                if(el) {
                    if(el.type==='checkbox') {
                        el.checked = (""+v)==='1' || (""+v)==='true';
                    } else {
                        el.value = "" + v;
                    }
                }
            }
            (form.current as HTMLFormElement).setAttribute('data-initialized', '1');
        }
    }, [initial, form]);

    function handleSubmit(ev: Event) {
        ev.preventDefault();
        const target = ev.target as HTMLFormElement;
        const data = new FormData(target);

        for(const el of target) {
            if(el instanceof HTMLInputElement && el.type==='checkbox') {
                data.set(el.name, el.checked ? '1' : '0');
            }
        }

        if(validate) {
            const errors = validate(data);
            for(const [k, v] of Object.entries(errors)) {
                if(target[k]) {
                    target[k].setCustomValidity(v);
                    target[k].reportValidity();
                }
            }
            if(Object.keys(errors).length) {
                return;
            }
        }

        if(submit) {
            submit(data);
        }

    }

    function handleChange(ev: Event) {
        // reset custom validity of every field
        for(const el of (ev.target as HTMLFormElement).form) {
            if(el.setCustomValidity) {
                el.setCustomValidity("");
            }
        }

        if(form.current) {
            const inp = (ev.target as HTMLInputElement);
            if(changed) changed(inp.name, inp.type === 'checkbox' ? (inp.checked ? '1' : '0') : inp.value);
        }
    }

    // preload the form fields as uncontrolled inputs?

    return <form ref={form} action="#" onSubmit={ handleSubmit } onChange={ handleChange } data-initialized="0">
        {children}
    </form>
}

function sortNoneFirst(a: [string, string], b: [string, string]) {
    if(a[0]==='0') return -1;
    if(b[0]==='0') return 1;
    return (a[1] as string).localeCompare(b[1] as string);
}

function selectField(label: string, name: string, options: {[index: string]:string}) {
    return <div class="form-row">
        <label>{ label }</label>
        <select name={ name }>
            { Object.entries(options).sort(sortNoneFirst).map(([k, v]) => (
                <option value={k}>{v}</option>
            )) }
        </select>
    </div>;
}

function checkboxField(label: string, name: string) {
    return <div class="form-row">
        <label>
            <input type="checkbox" name={ name } />
            { label }
        </label>
    </div>;
}

function ipField(label: string, name: string) {
    return <div class="form-row">
        <label>{ label }</label>
        <input type="text" name={ name } pattern="|((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)" title="IPv4 address or blank" />
    </div>;
}

function textPatternField(label: string, name: string, pattern: string) {
    return <div class="form-row">
        <label>{ label }</label>
        <input type="text" name={ name } pattern={ pattern } />
    </div>;
}

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

    const reboot = () => {
        fetch(import.meta.env.VITE_API_BASE + '/api/reboot', {
            method: 'POST',
        });
        setTimeout(() => {
            window.location.href = "/";
        }, 6000);
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
    const tesla = ["32", "33"].includes(""+merged.BATTTYPE);
    const pylonish = ["4", "10", "19"].includes(""+merged.INVTYPE);
    const sofar = ""+merged.INVTYPE==="17";
    const solax = ""+merged.INVTYPE==="18";

    return <div>
        <h2>Settings</h2>

        { !!settings && <div>

        { reboot_required && <div class="alert">
            Settings saved, reboot to apply.
            <button onClick={reboot}>Reboot now</button>
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
                <Show when={merged.DBLBTR}>{ selectField("Second battery interface", "BATT2COMM", INTERFACES) }</Show>
                { textPatternField("Battery capacity (Wh)", "BATTERY_WH_MAX", "|[1-9][0-9]*") }
                { checkboxField("Rescale SoC", "USE_SCALED_SOC") }
                <Show when={merged.USE_SCALED_SOC}>
                    { textPatternField("SoC max percentage", "MAXPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                    { textPatternField("SoC min percentage", "MINPERCENTAGE", "[0-9]{1,3}(\\.[0-9])?") }
                </Show>
                { textPatternField("Max charge current (A)", "MAXCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                { textPatternField("Max discharge current (A)", "MAXDISCHARGEAMP", "[0-9]+(\\.[0-9]+)?") }
                { checkboxField("Manual voltage limits", "USEVOLTLIMITS") }
                <Show when={merged.USEVOLTLIMITS}>
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
                    { checkboxField("Inverter should ignore contactors", "INVICNT") }
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

            <Show when={merged.CNTCTRL}>
                { textPatternField("Precharge time (ms)", "PRECHGMS", "[0-9]+") }
                { checkboxField("PWM contactor control", "PWMCNTCTRL") }
                <Show when={merged.PWMCNTCTRL}>
                    { textPatternField("PWM Frequency (Hz)", "PWMFREQ", "[0-9]+") }
                    { textPatternField("PWM Hold (0-1023)", "PWMHOLD", "[0-9]+") }
                </Show>
            </Show>

            { checkboxField("Double-Battery Contactor control via GPIO", "CNTCTRLDBL") }

            { checkboxField("Periodic BMS reset every 24h", "PERBMSRESET") }
            { textPatternField("Periodic BMS reset off time (s)", "BMSRESETDUR", "[0-9]+") }
            { checkboxField("External precharge via HIA4V1", "EXTPRECHARGE") }
            <Show when={merged.EXTPRECHARGE}>
                { textPatternField("Precharge, maximum ms before fault", "MAXPRETIME", "[0-9]+") }
                { checkboxField("Normally Open (NO) inverter disconnect contactor", "NOINVDISC") }
            </Show>
                
            { selectField("Status LED pattern", "LEDMODE", {
                "0": "Classic",
                "1": "Energy Flow",
                "2": "Heartbeat",
            }) }
        </div>

        <div class="panel">
            <h3>Connectivity</h3>
            { checkboxField("Enable WiFi access point", "WIFIAPENABLED") }
            <Show when={merged.WIFIAPENABLED}>
                <div class="form-row">
                    <label>WiFi access point password </label>
                    <input type="text" name="APPASSWORD" pattern="|.{8,}" title="at least 8 characters" />
                </div>
                <div class="form-row">
                    <label>WiFi channel (0 for automatic)</label>
                    <input type="text" name="WIFICHANNEL" pattern="[0-9]|1[0-4]" title="number" />
                </div>
            </Show>
            <div class="form-row">
                <label>Custom WiFi hostname (blank for default)</label>
                <input type="text" name="HOSTNAME" pattern="[a-zA-Z0-9\-]*" title="letters, numbers, hyphen" />
            </div>
            { checkboxField("Use static IP address", "STATICIP") }
            <Show when={merged.STATICIP}>
                { ipField("IP address", "LOCALIP") }
                { ipField("Gateway", "GATEWAY") }
                { ipField("Subnet", "SUBNET") }
            </Show>

            { checkboxField("Enable MQTT", "MQTTENABLED") }
            <Show when={merged.MQTTENABLED}>
                { textPatternField("MQTT server", "MQTTSERVER", "") }
                { textPatternField("MQTT port", "MQTTPORT", "[0-9]+") }
                { textPatternField("MQTT user", "MQTTUSER", "") }
                { textPatternField("MQTT password", "MQTTPASSWORD", "") }
                { textPatternField("MQTT timeout (ms)", "MQTTTIMEOUT", "[0-9]+") }
                { checkboxField("Send all cell voltages via MQTT", "MQTTCELLV") }
                { checkboxField("Remote BMS reset via MQTT allowed", "REMBMSRESET") }
                { checkboxField("Customized MQTT topics", "MQTTTOPICS") }
                <Show when={merged.MQTTTOPICS}>
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
