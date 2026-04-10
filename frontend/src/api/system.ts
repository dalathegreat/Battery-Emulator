import { xget } from "../xFetch.js";
import type { FirmwareInfo } from "../types/system.types.js";

export class SystemApi {
  async firmwareInfo() {
    return xget<FirmwareInfo>("/GetFirmwareInfo");
  }

  /** Plain GET (e.g. `/pause?value=true`) returning text or JSON. */
  async simpleGet(pathWithQuery: string) {
    return xget<unknown>(pathWithQuery);
  }
}
