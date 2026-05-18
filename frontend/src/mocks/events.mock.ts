import type { EventsResponse } from "../types/events.types.js";

export function mockEvents(): EventsResponse {
  return {
    events: [
      {
        type: "EVENT_WIFI_CONNECT",
        severity: "INFO",
        age_ms: 5000,
        count: 1,
        data: 0,
        message: "WiFi connected",
      },
      {
        type: "EVENT_DUMMY_WARNING",
        severity: "WARNING",
        age_ms: 120000,
        count: 2,
        data: 0,
        message: "Mock warning",
      },
    ],
  };
}
