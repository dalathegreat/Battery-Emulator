/// <reference types="vite/client" />

declare module "*.svg" {
  export const value: string;
  export default value;
}

declare module "*.webp" {
  const src: string;
  export default src;
}

interface ImportMetaEnv {
  readonly VITE_MOCK: string;
  readonly VITE_ESP32_URL: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
