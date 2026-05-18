import { xget } from "../xFetch.js";
import type { DashboardResponse } from "../types/dashboard.types.js";

export class DashboardApi {
  async get() {
    return xget<DashboardResponse>("/api/dashboard");
  }
}
