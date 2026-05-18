import { xget, xhttp } from "../xFetch.js";
import type { EventsResponse } from "../types/events.types.js";

export class EventsApi {
  async list() {
    return xget<EventsResponse>("/api/events");
  }

  async clear() {
    return xhttp<{ ok: boolean }>("/api/events/clear", "POST", {});
  }
}
