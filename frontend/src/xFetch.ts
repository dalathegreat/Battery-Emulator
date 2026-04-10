/** Simplified xFetch (no Firebase / dayjs / ct-confirm). */

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

export function getAPIBaseURL(): string {
  const w = window as Window & { apiURL?: string };
  let apiURL = w.apiURL || sessionStorage.apiURL || localStorage.apiURL;
  if (apiURL) return apiURL;
  return import.meta.env.VITE_ESP32_URL || "";
}

function toQueryString(obj: Record<string, unknown>) {
  const parts: string[] = [];
  for (const i in obj) {
    if (Object.prototype.hasOwnProperty.call(obj, i) && obj[i] !== undefined) {
      const v = obj[i];
      parts.push(
        `${encodeURIComponent(i)}=${v === null ? "" : encodeURIComponent(String(v))}`
      );
    }
  }
  return parts.join("&");
}

function cleanObject(obj?: Record<string, string>) {
  if (!obj) return obj;
  const out: Record<string, string> = { ...obj };
  for (const key in out) {
    if (out[key] === null || out[key] === undefined) {
      delete out[key];
    }
  }
  return out;
}

function tzOffsetHours(): string {
  return String(-new Date().getTimezoneOffset() / 60);
}

export namespace xFetch {
  export function createResponse<T, E>(
    json: T,
    error: xError<E> | undefined,
    response: Response
  ): xResponse<T, E> {
    return Object.assign([json, error, response], {
      response,
      json,
      error,
    }) as xResponse<T, E>;
  }

  export interface xResponse<T = unknown, E = unknown> extends Array<unknown> {
    json: T;
    error?: xError<E>;
    response: Response;
    0: T;
    1: xError<E> | undefined;
    2: Response;
  }

  export interface xError<E = unknown> {
    number: number;
    status: number;
    error_type: string;
    error_msg: string;
    data?: E;
  }

  export interface Pagination {
    token?: string;
    limit?: string;
    skip?: string;
  }
}

export async function xget<T = unknown, E = string>(
  url: string,
  params?: Record<string, unknown>,
  options?: {
    skipBaseURL?: boolean;
    noauth?: boolean;
    headers?: Record<string, string>;
    retries?: number;
  }
): Promise<xFetch.xResponse<T, E>> {
  let parsedURL: URL;
  let retries = options?.retries ?? 4;
  if (retries < 1) retries = 1;
  let rs: Promise<xFetch.xResponse<T, E>>;
  do {
    rs = new Promise((solve) => {
      (async () => {
        try {
          if (options?.skipBaseURL) parsedURL = new URL(url);
          else parsedURL = new URL(getAPIBaseURL() + url, window.location.origin);
          if (params) parsedURL.search = toQueryString(params);
          const lang = localStorage.lang || navigator.language.substring(0, 2);
          const p = await fetch(parsedURL.href, {
            headers: {
              Accept: "application/json, text/javascript, */*; q=0.01",
              "Content-Type": "application/json",
              "Cache-Control": "no-cache",
              Pragma: "no-cache",
              "accept-language": lang,
              ...(options?.noauth
                ? {}
                : {
                    "x-lang": lang,
                    "x-timezone": tzOffsetHours(),
                    ...cleanObject(options?.headers),
                  }),
            },
            method: "GET",
            mode: "cors",
          });

          let jsonResult: unknown = {};
          const ct = p.headers.get("content-type") || "";
          try {
            if (ct.includes("application/json")) {
              jsonResult = await p.json();
            } else {
              jsonResult = await p.text();
            }
          } catch {
            /* empty */
          }

          const r: xFetch.xResponse<T, E> = xFetch.createResponse(
            jsonResult as T,
            undefined,
            p
          );
          if (p.status >= 400) {
            r.error = jsonResult as xFetch.xError<E>;
            r[1] = r.error;
          }
          solve(r);
        } catch (error) {
          console.error(error);
          const jsonResult: unknown = {};
          let msg = `${error}`;
          if (msg.includes("Failed to fetch")) msg = "No hay conexión a internet.";
          const r: xFetch.xResponse<T, E> = Object.assign(
            [jsonResult as T, undefined, {} as Response],
            {
              json: jsonResult as T,
              response: {} as Response,
              error: {
                status: 0,
                error_msg: msg,
                error_type: msg,
                number: 0,
              },
            }
          ) as xFetch.xResponse<T, E>;
          r[1] = r.error;
          solve(r);
        }
      })();
    });
    const [, error] = await rs;
    if (error && error.status === 0) {
      retries--;
      await sleep(1500);
      console.log(retries, "Retrying", url);
      continue;
    }
    break;
  } while (retries > 0);
  return rs;
}

export async function xhttp<T = unknown, E = null>(
  url: string,
  method: "POST" | "PUT" | "DELETE" | "PATCH" | "SEARCH",
  body?: object | FormData | string,
  options?: {
    skipBaseURL?: boolean;
    headers?: Record<string, string>;
    retries?: number;
    server_send_events?: (event: string) => void;
  }
): Promise<xFetch.xResponse<T, E>> {
  let parsedURL: URL;
  let retries = options?.retries ?? 4;
  if (retries < 1) retries = 1;
  let rs: Promise<xFetch.xResponse<T, E>>;
  do {
    rs = new Promise((solve) => {
      (async () => {
        try {
          let sendBody: BodyInit | undefined;
          if (body === undefined) {
            sendBody = undefined;
          } else if (typeof body === "string") {
            sendBody = body;
          } else if (body instanceof FormData) {
            sendBody = body;
          } else {
            sendBody = JSON.stringify(body);
          }
          if (options?.skipBaseURL) parsedURL = new URL(url);
          else parsedURL = new URL(getAPIBaseURL() + url, window.location.origin);
          const lang = localStorage.lang || navigator.language.substring(0, 2);
          const baseHeaders: Record<string, string> = {
            Accept: "application/json, text/javascript, */*;",
            "Cache-Control": "no-cache",
            Pragma: "no-cache",
            "accept-language": lang,
            "x-lang": lang,
            "x-timezone": tzOffsetHours(),
            ...cleanObject(options?.headers),
          };
          if (body instanceof FormData) {
            /* browser sets multipart boundary */
          } else if (typeof body === "string") {
            baseHeaders["Content-Type"] = "text/plain";
          } else if (body !== undefined) {
            baseHeaders["Content-Type"] = "application/json";
          }
          const response = await fetch(parsedURL.href, {
            headers: baseHeaders,
            body: sendBody,
            method,
            mode: "cors",
          });

          if (options?.server_send_events) {
            const reader = response.body?.getReader();
            const decoder = new TextDecoder();
            if (!reader) return;
            while (true) {
              const { done, value } = await reader.read();
              if (done) break;
              const chunk = decoder.decode(value);
              const lines = chunk.split("\n\n");
              lines.forEach((line) => {
                if (line.startsWith("data: ")) {
                  try {
                    const json = JSON.parse(line.substring(6));
                    if (json.text) options.server_send_events!(json.text);
                  } catch (e) {
                    console.error("No se pudo parsear el JSON del chunk:", line, e);
                  }
                }
              });
            }
            const rsse: xFetch.xResponse<T, E> = xFetch.createResponse(
              {} as T,
              undefined,
              response
            );
            solve(rsse);
            return;
          }

          let jsonResult: unknown = {};
          const ct = response.headers.get("content-type") || "";
          try {
            if (ct.includes("application/json")) {
              jsonResult = await response.json();
            } else {
              jsonResult = await response.text();
            }
          } catch {
            /* empty */
          }

          const r: xFetch.xResponse<T, E> = xFetch.createResponse(
            jsonResult as T,
            undefined,
            response
          );
          if (response.status >= 400) {
            r.error = jsonResult as xFetch.xError<E>;
            r[1] = r.error;
          }
          solve(r);
        } catch (error) {
          console.error(error);
          const jsonResult: unknown = {};
          let msg = `${error}`;
          if (msg.includes("fetch")) msg = "No hay conexión a internet.";
          const r: xFetch.xResponse<T, E> = Object.assign(
            [jsonResult as T, undefined, {} as Response],
            {
              json: jsonResult as T,
              response: {} as Response,
              error: {
                status: 0,
                error_msg: msg,
                error_type: msg,
                number: 0,
              },
            }
          ) as xFetch.xResponse<T, E>;
          r[1] = r.error;
          solve(r);
        }
      })();
    });
    const [, error] = await rs;
    if (error && error.status === 0) {
      retries--;
      await sleep(1500);
      console.log(retries, "Retrying", url);
      continue;
    }
    break;
  } while (retries > 0);
  return rs;
}
