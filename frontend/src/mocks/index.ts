import { mockBatteryCommands, mockBatteryStatus, mockCellMonitor } from "./battery.mock.js";
import { mockCanlogStatus } from "./canlog.mock.js";
import { mockDashboard } from "./dashboard.mock.js";
import { mockEvents } from "./events.mock.js";
import { mockSettings } from "./settings.mock.js";

const origFetch = window.fetch.bind(window);

function jsonResponse(data: unknown, status = 200): Response {
  return new Response(JSON.stringify(data), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

function textResponse(text: string, status = 200): Response {
  return new Response(text, {
    status,
    headers: { "Content-Type": "text/plain" },
  });
}

function pathnameOf(input: RequestInfo | URL): string {
  if (typeof input === "string") {
    try {
      return new URL(input, window.location.origin).pathname;
    } catch {
      return input;
    }
  }
  if (input instanceof URL) return input.pathname;
  return new URL(input.url, window.location.origin).pathname;
}

window.fetch = async (input, init) => {
  const path = pathnameOf(input);
  const method = init?.method?.toUpperCase() || "GET";

  /* Advanced battery commands: PUT /identifier with body 0|1|2 */
  if (method === "PUT" && /^\/[a-zA-Z0-9_]+$/.test(path)) {
    return textResponse("Command performed. (mock)");
  }

  if (path === "/GetFirmwareInfo" && method === "GET") {
    return jsonResponse({
      hardware: "MOCK_HW",
      firmware: "0.0.0-mock",
    });
  }

  if (path === "/api/battery/status" && method === "GET") {
    return jsonResponse(mockBatteryStatus());
  }
  if (path === "/api/dashboard" && method === "GET") {
    return jsonResponse(mockDashboard());
  }
  if (path === "/api/battery/extra" && method === "GET") {
    return jsonResponse({
      batteries: [
        {
          index: 0,
          rows: [
            { label: "§ Mock section", value: "" },
            { label: "Field A", value: "123" },
            { label: "Field B", value: "ok" },
          ],
        },
      ],
    });
  }
  if (path === "/api/can/viewer/enable" && method === "POST") {
    return jsonResponse({ ok: true });
  }
  if (path === "/api/can/messages" && method === "GET") {
    return jsonResponse({ text: "mock can line\n", can_id_cutoff: 0, can_logging_active: true });
  }
  if (path === "/api/debug/log" && method === "GET") {
    return jsonResponse({ text: "mock debug\n" });
  }
  if (path === "/saveSettings" && method === "POST") {
    return new Response(null, { status: 302, headers: { Location: "/?settingsSaved=1#/settings" } });
  }
  if (path === "/factoryReset" && method === "POST") {
    return textResponse("OK");
  }
  if (path === "/api/battery/cellmonitor" && method === "GET") {
    return jsonResponse(mockCellMonitor());
  }
  if (path === "/api/battery/commands" && method === "GET") {
    return jsonResponse(mockBatteryCommands());
  }
  if (path === "/api/settings" && method === "GET") {
    return jsonResponse(mockSettings());
  }
  if (path === "/api/events" && method === "GET") {
    return jsonResponse(mockEvents());
  }
  if (path === "/api/events/clear" && method === "POST") {
    return jsonResponse({ ok: true });
  }
  if (path === "/api/canlog/status" && method === "GET") {
    return jsonResponse(mockCanlogStatus());
  }
  if (path === "/startReplay" && method === "GET") {
    return textResponse("CAN replay started! (mock)");
  }
  if (path === "/stopReplay" && method === "GET") {
    return textResponse("CAN replay stopped! (mock)");
  }
  if (path === "/setCANInterface" && method === "GET") {
    return textResponse("New interface selected (mock)");
  }
  if (path === "/stop_can_logging" && method === "GET") {
    return textResponse("Logging stopped (mock)");
  }

  return origFetch(input, init);
};
