// Preloaded by `bun test` (see bunfig.toml [test] preload) before every test
// file: provides a browserless DOM (happy-dom) and stubs polyfills the few
// browser APIs the frontend uses that happy-dom does not implement.

import { GlobalRegistrator } from "@happy-dom/global-registrator";

// A real-looking origin so relative URLs, FormData and fetch-adjacent code
// behave as they would in the browser.
GlobalRegistrator.register({ url: "http://localhost/" });

// The settings page scrolls to the top after saving a form.
if (typeof window.scrollTo !== "function") {
    window.scrollTo = () => {};
}

// alert() is used on save-failure paths in settings.tsx.
if (typeof window.alert !== "function") {
    window.alert = () => {};
}

// Form validation in components/forms.tsx calls setCustomValidity() (happy-dom
// implements it) and reportValidity() (implemented nowhere headless).
if (!HTMLFormElement.prototype.reportValidity) {
    HTMLFormElement.prototype.reportValidity = function () {
        return this.checkValidity();
    };
}
if (!HTMLElement.prototype.reportValidity) {
    HTMLElement.prototype.reportValidity = function () {
        return this.checkValidity();
    };
}

// forms.tsx iterates `ev.target.form` (`for (const el of form)`), which works
// in browsers because HTMLFormElement is iterable (over its controls).
if (typeof HTMLFormElement.prototype[Symbol.iterator] !== "function") {
    HTMLFormElement.prototype[Symbol.iterator] = function* () {
        yield* Array.from(this.elements);
    };
}

// happy-dom's ValidityState.patternMismatch does `value.replace(new RegExp(pattern),
// '').length > 0` which is not the HTML algorithm and rejects patterns whose
// first alternative is empty (the app uses e.g. "|[1-9][0-9]*" for the
// battery capacity field). Browsers match the pattern against the whole value.
(() => {
    const firstInput = document.createElement("input");
    const proto = Object.getPrototypeOf(firstInput.validity);
    const desc = Object.getOwnPropertyDescriptor(proto, "patternMismatch");
    if (desc && typeof desc.get === "function") {
        Object.defineProperty(proto, "patternMismatch", {
            configurable: true,
            enumerable: true,
            get() {
                const el = (this as any).element as HTMLInputElement;
                const pattern = el.getAttribute("pattern");
                if (el.localName !== "input" || !pattern || el.value.length === 0) return false;
                return !new RegExp(`^(?:${pattern})$`, "u").test(el.value);
            },
        });
    }
})();

// Forms.tsx does `new FormData(form)` to collect the submitted values. Node's
// FormData does not accept a <form> element; if the environment's FormData
// doesn't either, wrap it with a form-aware shim.
(() => {
    try {
        const f = document.createElement("form");
        f.innerHTML = '<input name="probe" value="ok">';
        const fd = new FormData(f);
        if (fd.get("probe") === "ok") return; // native supports forms
    } catch {
        // fall through to the shim
    }
    const NativeFormData = FormData;
    class FormDataShim extends NativeFormData {
        constructor(form?: any) {
            if (form && form.tagName === "FORM") {
                super();
                for (const el of Array.from(form.elements)) {
                    const e = el as HTMLInputElement;
                    if (!e.name || e.disabled) continue;
                    if (e.type === "checkbox" || e.type === "radio") {
                        if (e.checked) this.append(e.name, e.value);
                    } else if (e.type === "select-multiple") {
                        for (const opt of Array.from((e as HTMLSelectElement).options)) {
                            if (opt.selected) this.append(e.name, opt.value);
                        }
                    } else {
                        this.append(e.name, e.value);
                    }
                }
            } else {
                super(form);
            }
        }
    }
    Object.setPrototypeOf(FormDataShim, NativeFormData);
    globalThis.FormData = FormDataShim as any;
})();