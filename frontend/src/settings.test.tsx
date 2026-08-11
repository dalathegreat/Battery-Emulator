// Browserless tests for src/settings.tsx, run with `bun test`.
//
// The settings page talks to the device through src/utils/api.tsx; those tests
// replace that module (bun mock.module) with an in-memory MockBackend so the
// page runs exactly as in the browser but against a controllable backend.
//
// The form's labels are not associated with their controls (no `for`, no
// wrapping), so controls are queried by `name` attribute (see formControl).
import { describe, test, expect, beforeEach, mock } from "bun:test";
import { render, screen, fireEvent, waitFor, cleanup } from "@testing-library/preact";

import {
    MockBackend,
    formControl,
} from "../test/settings-fixtures.ts";

// Base settings the backend serves; plausible defaults matching a Nissan LEAF
// battery + SolaX inverter setup (see mock_api.tsx for the real demo state).
const BASE_SETTINGS = {
    BATTTYPE: "21", BATTCOMM: "3", INVTYPE: "18", INVCOMM: "3",
    BATTERY_WH_MAX: "24000", MAXCHARGEAMP: "10", MAXDISCHARGEAMP: "100",
    BATTCHEM: "1", CHGTYPE: "0", SHUNTTYPE: "0",
    SSID: "HomeWifi", WIFIAPENABLED: "0", HOSTNAME: "battery-emulator",
    STATICIP: "0", LOCALIP: "", GATEWAY: "", SUBNET: "", DNS: "",
    WEBAUTH: "0", MQTTENABLED: "0", WEBENABLED: "1", USBENABLED: "1",
    LOWPASSFILTER: "1", MQTTSERVER: "mqtt.local",
};

const backend = new MockBackend();

// Swap the real fetch-based api.tsx for our in-memory backend before the page
// module is ever imported.
mock.module("./utils/api.tsx", () => ({
    useGetApi: backend.useGetApi,
    apiPost: backend.apiPost,
    refreshApi: backend.refreshApi,
}));

// settings.tsx is imported dynamically so the module mock above is in place
// first.
const { Settings } = await import("./settings.tsx");

async function renderSettings(settings: Record<string, string> = BASE_SETTINGS) {
    backend.reset(settings);
    render(<Settings />);
    // Wait for the GET payload to arrive and the form to be populated
    // (data-initialized is only set once the Form's initial-value effect ran).
    await waitFor(() => {
        expect(formControl("BATTTYPE", "select").value).not.toBe("");
        expect(screen.getByRole("button", { name: "Save settings" })).toBeTruthy();
    });
    return backend;
}

function type(name: string, value: string) {
    const el = formControl(name);
    // Set the value directly on the element: passing { target: {...} } to
    // fireEvent makes happy-dom replace event.target with that plain object.
    el.value = value;
    fireEvent.input(el);
    fireEvent.change(el);
}

/** Flip a checkbox and dispatch the change/input events the Form listens to. */
function setChecked(name: string, checked: boolean) {
    const el = formControl(name);
    if (el.checked === checked) return;
    el.checked = checked;
    fireEvent.change(el);
    fireEvent.input(el);
}

async function clickSave() {
    const save = screen.getByRole("button", { name: "Save settings" }) as HTMLButtonElement;
    fireEvent.click(save);
}

beforeEach(() => {
    cleanup(); // unmount anything left over from the previous test
    backend.reset(BASE_SETTINGS);
});

// ---------------------------------------------------------------------------
// Field population
// ---------------------------------------------------------------------------

describe("field population", () => {
    test("text, select and checkbox fields are populated from the GET payload", async () => {
        await renderSettings();

        expect(formControl("SSID").value).toBe("HomeWifi");
        expect(formControl("HOSTNAME").value).toBe("battery-emulator");
        expect(formControl("MAXCHARGEAMP").value).toBe("10");

        // BATTTYPE 21 = Nissan LEAF battery
        const battery = formControl<HTMLSelectElement>("BATTTYPE", "select");
        expect(battery.value).toBe("21");
        expect(battery.selectedOptions[0].textContent).toBe("Nissan LEAF battery");

        // INVTYPE 18 = SolaX Triple Power LFP over CAN bus
        const inverter = formControl<HTMLSelectElement>("INVTYPE", "select");
        expect(inverter.value).toBe("18");

        expect(formControl("WEBENABLED").checked).toBe(true);   // 1
        expect(formControl("MQTTENABLED").checked).toBe(false); // 0
    });

    test("fields hidden by the battery type are disabled (not just hidden)", async () => {
        await renderSettings();

        // BATTTYPE 21 is not in the custom-BMS (6/11/22/23/24/...), estimated
        // (3/4/6/8/14/16/24/32/33/40/41/50/51) or Tesla (32/33) groups.
        expect(formControl("BATTCVMAX").disabled).toBe(true);   // custom BMS block
        expect(formControl("GTWCOUNTRY").disabled).toBe(true);  // Tesla block
        expect(formControl("CHGPOWER").disabled).toBe(true);    // estimated block
        // ...but the battery interface itself is available
        expect(formControl("BATTCOMM", "select").disabled).toBe(false);
    });

    test("battery-type dependent fields enable with the matching battery type", async () => {
        // Tesla Model S/X (32): Tesla-only fields appear
        await renderSettings({ ...BASE_SETTINGS, BATTTYPE: "32" });
        expect(formControl("GTWCOUNTRY").disabled).toBe(false);
        expect(formControl("GTWCHASSIS").disabled).toBe(false);
        expect(formControl("DALYPWRPCT").disabled).toBe(true); // Daly-only

        // DALY RS485 (23): Daly fields appear, Tesla fields do not
        cleanup();
        backend.reset({ ...BASE_SETTINGS, BATTTYPE: "23" });
        render(<Settings />);
        await waitFor(() => expect(formControl("DALYPWRPCT").disabled).toBe(false));
        expect(formControl("GTWCOUNTRY").disabled).toBe(true);
    });

    test("inverter-type dependent fields enable with the matching inverter type", async () => {
        // Pylontech HV (10): Pylon manufacturer + group fields
        await renderSettings({ ...BASE_SETTINGS, INVTYPE: "10" });
        expect(formControl("PYLONBRAND", "select").disabled).toBe(false);
        expect(formControl("PYLONBAUD").disabled).toBe(false);
        expect(formControl("INVBTYPE").disabled).toBe(true); // SolaX-only

        // SolaX (18): reported battery type field
        cleanup();
        backend.reset({ ...BASE_SETTINGS, INVTYPE: "18" });
        render(<Settings />);
        await waitFor(() => expect(formControl("INVBTYPE").disabled).toBe(false));
        expect(formControl("PYLONBRAND", "select").disabled).toBe(true);
    });

    test("saved-changes banner is shown when the backend says a reboot is required", async () => {
        backend.setSettings(BASE_SETTINGS, true);
        render(<Settings />);
        expect(await screen.findByText("Settings saved, reboot to apply.")).toBeTruthy();
        expect(screen.getByRole("button", { name: "Reboot now" })).toBeTruthy();
    });
});

// ---------------------------------------------------------------------------
// Saving back
// ---------------------------------------------------------------------------

describe("saving", () => {
    test("the Save button starts disabled and enables once a field changes", async () => {
        await renderSettings();
        const save = screen.getByRole("button", { name: "Save settings" }) as HTMLButtonElement;
        expect(save.disabled).toBe(true);

        type("SSID", "NewWifi");
        expect(save.disabled).toBe(false);
    });

    test("submits only the fields that changed", async () => {
        const backend = await renderSettings();

        type("SSID", "NewWifi");
        await clickSave();

        expect(backend.apiPost).toHaveBeenCalledTimes(1);
        const [url, posted] = backend.apiPost.mock.calls[0];
        expect(url).toBe("/api/internal/settings");
        expect(posted).toEqual({ SSID: "NewWifi" });
        // Unchanged fields must not be sent back
        expect(posted.BATTTYPE).toBeUndefined();
    });

    test("submits multiple changed fields in one request", async () => {
        const backend = await renderSettings();

        type("SSID", "NewWifi");
        type("MAXCHARGEAMP", "42");
        await clickSave();

        expect(backend.apiPost).toHaveBeenCalledTimes(1);
        expect(backend.apiPost.mock.calls[0][1]).toEqual({
            SSID: "NewWifi",
            MAXCHARGEAMP: "42",
        });
    });

    test("toggling a checkbox is tracked and submitted", async () => {
        const backend = await renderSettings();

        setChecked("WEBENABLED", false);
        await clickSave();

        expect(backend.apiPost.mock.calls[0][1]).toEqual({ WEBENABLED: "0" });
    });

    test("after saving, the form refreshes from the backend and clears local changes", async () => {
        const backend = await renderSettings();

        type("SSID", "NewWifi");
        await clickSave();

        // The page must re-pull via refreshApi rather than echoing our input
        expect(backend.refreshApi).toHaveBeenCalled();

        // The backend now holds the new value (the mock applied the write)...
        expect(backend.settings.settings.SSID).toBe("NewWifi");

        // ...and the form shows the persisted value with no pending changes
        // once the refreshed GET payload lands.
        await waitFor(() => {
            expect(formControl("SSID").value).toBe("NewWifi");
            const save = screen.getByRole("button", { name: "Save settings" }) as HTMLButtonElement;
            expect(save.disabled).toBe(true);
        });

        // And the "reboot required" banner appears after any save.
        expect(await screen.findByText("Settings saved, reboot to apply.")).toBeTruthy();
    });

    test("ticking 'Use static IP address' adopts the current DHCP lease for saving", async () => {
        const backend = await renderSettings();

        setChecked("STATICIP", true);

        // The lease values come from /api/status
        expect(formControl("LOCALIP").value).toBe(backend.status.ip);
        expect(formControl("GATEWAY").value).toBe(backend.status.gateway);
        expect(formControl("SUBNET").value).toBe(backend.status.subnet);
        expect(formControl("DNS").value).toBe(backend.status.dns);

        await clickSave();
        expect(backend.apiPost.mock.calls[0][1]).toEqual({
            STATICIP: "1",
            LOCALIP: backend.status.ip,
            GATEWAY: backend.status.gateway,
            SUBNET: backend.status.subnet,
            DNS: backend.status.dns,
        });
    });
});

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

describe("validation", () => {
    test("rejects saves where the web passwords do not match", async () => {
        const backend = await renderSettings({ ...BASE_SETTINGS, WEBAUTH: "1" });

        type("HTTPPASS", "secret1");
        type("HTTPPASSCONFIRM", "secret2");
        await clickSave();

        expect(backend.apiPost).not.toHaveBeenCalled();
        // The offending field is flagged
        expect(formControl("HTTPPASSCONFIRM").validationMessage).not.toBe("");
    });

    // NOTE: settings.tsx validate() also wants to reject INVTYPE "2"
    // ("don't choose 2") but looks FormData up under a non-existent 'inverter'
    // name, so that rule never fires; it is not tested here until fixed.

    test("passes validation when nothing is wrong", async () => {
        const backend = await renderSettings({ ...BASE_SETTINGS, WEBAUTH: "1" });

        type("HTTPPASS", "matchme");
        type("HTTPPASSCONFIRM", "matchme");
        await clickSave();

        expect(backend.apiPost).toHaveBeenCalledTimes(1);
    });
});

// ---------------------------------------------------------------------------
// Dynamic show/hide while editing
// ---------------------------------------------------------------------------

describe("dynamic field visibility", () => {
    test("enabling MQTT reveals the MQTT section for editing", async () => {
        await renderSettings();

        expect(formControl("MQTTSERVER").disabled).toBe(true);

        setChecked("MQTTENABLED", true);
        expect(formControl("MQTTSERVER").disabled).toBe(false);
        // Value served by the backend is already there, just disabled
        expect(formControl("MQTTSERVER").value).toBe("mqtt.local");

        // Disabling MQTT hides the section again
        setChecked("MQTTENABLED", false);
        expect(formControl("MQTTSERVER").disabled).toBe(true);
    });

    test("double battery checkbox reveals the second battery interface", async () => {
        await renderSettings();

        expect(formControl("BATT2COMM", "select").disabled).toBe(true);

        setChecked("DBLBTR", true);
        expect(formControl("BATT2COMM", "select").disabled).toBe(false);

        setChecked("DBLBTR", false);
        expect(formControl("BATT2COMM", "select").disabled).toBe(true);
    });
});