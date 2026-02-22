import { useRef, useEffect } from "preact/hooks";

// Shows or hides its children based on the "when" prop.
export function Show({ when, indent, children }: { when: boolean | string, indent?: boolean | null, children: preact.ComponentChildren }) {
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
    return <div data-ind={indent} ref={ref}>
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

export function Form({ children, initial, changed, validate, submit }: { 
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

export function selectField(label: string, name: string, options: {[index: string]:string}) {
    return <div class="form-row">
        <label>{ label }</label>
        <select name={ name }>
            { Object.entries(options).sort(sortNoneFirst).map(([k, v]) => (
                <option value={k}>{v}</option>
            )) }
        </select>
    </div>;
}

export function checkboxField(label: string, name: string) {
    return <div class="form-row">
        <label>
            <input type="checkbox" name={ name } />
            { label }
        </label>
    </div>;
}

export function ipField(label: string, name: string) {
    return <div class="form-row">
        <label>{ label }</label>
        <input type="text" name={ name } pattern="|((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)" title="IPv4 address or blank" />
    </div>;
}

export function textPatternField(label: string, name: string, pattern: string) {
    return <div class="form-row">
        <label>{ label }</label>
        <input type="text" name={ name } pattern={ pattern || undefined } />
    </div>;
}
