import { useRef, useEffect } from "preact/hooks";

import { useGetApi } from "./utils/api.tsx";


function Form({ children, initial, validate, submit }: { 
    children: preact.ComponentChildren, 
    initial?: any,
    validate?: (data: FormData) => any,
    submit?: (data: FormData) => any
}) {    	
    //const input = useRef(null);
	const form = useRef(null);

    useEffect(() => {
        if(form.current) {
            for(const [k, v] of Object.entries(initial ?? {})) {
                const el = (form.current[k] as any);
                if(el) {
                    el.value = "" + v;
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
            console.log('errors', errors);
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
            (form.current as HTMLFormElement).setAttribute(
                'data-' + inp.name,
                inp.type === 'checkbox' ? (inp.checked ? '1' : '0') : inp.value
            );
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

function ipField(label: string, name: string) {
    return <div class="form-row">
        <label>{ label }</label>
        <input type="text" name={ name } pattern="|((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)" title="IPv4 address or blank" />
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

    // var initial = useGetApi('api/settings.json');
    // if(!initial) {
    //     return <div></div>;
    // }

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

    const submit = (data: any) => {
        splitIp(data, 'LOCALIP');
        splitIp(data, 'GATEWAY');
        splitIp(data, 'SUBNET');

        console.log('data is ', ...data);

        fetch(import.meta.env.VITE_API_BASE + '/api/settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(Object.fromEntries(data)),
        });
    };

    const batteries: {[index: string]:string} = {};
    for(let i=0; i<settings?.batteries.length; i++) {
        if(settings.batteries[""+i]) batteries[""+i] = settings.batteries[i];
    }
    const inverters: {[index: string]:string} = {};
    for(let i=0; i<settings?.inverters.length; i++) {
        if(settings.inverters[i]) inverters[i] = settings.inverters[i];
    }

    return <div>
        <h2>Settings</h2>

        { !!settings && <div>

        <Form initial={settings.settings}
              validate={validate}
              submit={submit}
              >
        
        <div class="panel">
            <h3>Battery</h3>
            <div class="form-row">
                <label>Battery</label>
                <select name="BATTTYPE">
                    { Object.entries(batteries).sort(sortNoneFirst).map(([k, v]) => (
                        <option value={k}>{v}</option>
                    )) }
                </select>
            </div>
            { selectField("Battery interface", "BATTCOMM", INTERFACES) }
            { selectField("Battery chemistry", "BATTCHEM", {
                "3": "LFP",
                "1": "NCA",
                "2": "NMC",
            }) }
            <div class="form-row">
                <label>
                    <input type="checkbox" name="DBLBTR" />
                    Double battery
                </label>
            </div>
            { selectField("Second battery interface", "BATT2COMM", INTERFACES) }
{/* 
            <div class="form-row">
                <label>Testo</label>
                <input name="testo" pattern="[0-9]*" />
            </div> */}
        </div>

        <div class="panel">
            <h3>Inverter</h3>
            { selectField("Inverter protocol", "INVTYPE", inverters) }
            { selectField("Inverter interface", "INVCOMM", INTERFACES) }
        </div>

        <div class="panel">
            <h3>Connectivity</h3>
            <div class="form-row">
                <label>
                    <input type="checkbox" name="WIFIAPENABLED" />
                    Enable WiFi access point
                </label>
            </div>
            <div class="form-row">
                <label>WiFi access point password </label>
                <input type="text" name="APPASSWORD" pattern="|.{8,}" title="at least 8 characters" />
            </div>
            <div class="form-row">
                <label>WiFi channel (0 for automatic)</label>
                <input type="text" name="WIFICHANNEL" pattern="[0-9]|1[0-4]" title="number" />
            </div>
            <div class="form-row">
                <label>Custom WiFi hostname (blank for default)</label>
                <input type="text" name="HOSTNAME" pattern="[a-zA-Z0-9\-]*" title="letters, numbers, hyphen" />
            </div>
            <div class="form-row">
                <label>
                    <input type="checkbox" name="STATICIP" />
                    Use static IP address
                </label>
            </div>
            { ipField("IP address", "LOCALIP") }
            { ipField("Gateway", "GATEWAY") }
            { ipField("Subnet", "SUBNET") }

        </div>


        <button type="submit" style="margin-top: 1rem;">Save settings</button>


        </Form>

        </div> }

    </div>
}
