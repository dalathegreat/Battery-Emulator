import "./components/app-root.js";

async function boot() {
  if (import.meta.env.DEV && import.meta.env.VITE_MOCK === "true") {
    await import("./mocks/index.js");
  }
}

boot();
