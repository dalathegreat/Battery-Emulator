# Battery Emulator web UI

- **Dev with fake data:** `npm install` then `npm run dev`. Uses `.env.development` (`VITE_MOCK=true`) so `fetch` is intercepted and no ESP32 is needed.
- **Dev against a real device:** Set `VITE_MOCK=false` in `.env.development.local` and set `VITE_DEV_PROXY_TARGET` to the emulator IP (default `http://192.168.4.1`). The Vite dev server proxies `/api/*` and related routes to the device.
- **Production bundle:** `npm run build` produces `dist/app.bundle.js` and runs `scripts/generate-cpp.mjs`, which writes `Software/src/devboard/webserver/app_bundle.{cpp,h}` for the firmware.

PlatformIO runs `npm install` + `npm run build` automatically via `scripts/build_frontend.py` before each compile.
