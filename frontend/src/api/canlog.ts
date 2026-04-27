import { xget, xhttp } from "../xFetch.js";
import type { CanMessagesResponse } from "../types/canlog.types.js";
import type { CanlogStatusResponse } from "../types/system.types.js";

export class CanlogApi {
  async status() {
    return xget<CanlogStatusResponse>("/api/canlog/status");
  }

  async startReplay(loop = false) {
    const q = loop ? "?loop=1" : "";
    return xget<string>(`/startReplay${q}`);
  }

  async stopReplay() {
    return xget<string>("/stopReplay");
  }

  async setCANInterface(iface: number) {
    return xget<string>(`/setCANInterface?interface=${iface}`);
  }

  async stopCanLogging() {
    return xget<string>("/stop_can_logging");
  }

  async enableViewer() {
    return xhttp<{ ok?: boolean }>("/api/can/viewer/enable", "POST");
  }

  async messages() {
    return xget<CanMessagesResponse>("/api/can/messages");
  }

  async setCanIdCutoff(id: number) {
    return xget<string>(`/set_can_id_cutoff?value=${encodeURIComponent(String(id))}`);
  }
}
