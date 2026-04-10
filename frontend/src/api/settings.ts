import { xget, xhttp } from "../xFetch.js";
import type { SettingsResponse } from "../types/settings.types.js";

export class SettingsApi {
  async get() {
    return xget<SettingsResponse>("/api/settings");
  }

  /** Same field names as legacy `settings_html` / `POST /saveSettings`. */
  async save(form: FormData) {
    return xhttp<unknown>("/saveSettings", "POST", form);
  }
}
