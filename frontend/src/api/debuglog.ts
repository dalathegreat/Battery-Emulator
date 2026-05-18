import { xget } from "../xFetch.js";

export interface DebugLogResponse {
  text: string;
}

export class DebuglogApi {
  async get() {
    return xget<DebugLogResponse>("/api/debug/log");
  }
}
