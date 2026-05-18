import { execSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { defineConfig, loadEnv } from "vite";
import cssInjectedByJsPlugin from "vite-plugin-css-injected-by-js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default defineConfig(({ mode }) => {
	const env = loadEnv(mode, __dirname, "");
	const proxyTarget = env.VITE_DEV_PROXY_TARGET || "http://192.168.4.1";

	return {
		root: __dirname,
		server: {
			port: 5173,
			proxy: {
				"/api": { target: proxyTarget, changeOrigin: true },
				"/GetFirmwareInfo": { target: proxyTarget, changeOrigin: true },
				"/startReplay": { target: proxyTarget, changeOrigin: true },
				"/stopReplay": { target: proxyTarget, changeOrigin: true },
				"/setCANInterface": { target: proxyTarget, changeOrigin: true },
				"/stop_can_logging": { target: proxyTarget, changeOrigin: true }
			}
		},
		build: {
			outDir: "dist",
			emptyOutDir: true,
			cssCodeSplit: false,
			rollupOptions: {
				input: path.resolve(__dirname, "index.html"),
				output: {
					entryFileNames: "app.bundle.js",
					chunkFileNames: "app.bundle.js",
					assetFileNames: "app.bundle.[ext]",
					inlineDynamicImports: true
				}
			}
		},
		plugins: [
			cssInjectedByJsPlugin(),
			{
				name: "generate-app-bundle-cpp",
				closeBundle() {
					const script = path.resolve(__dirname, "scripts/generate-cpp.mjs");
					execSync(`node "${script}"`, { stdio: "inherit", cwd: __dirname });
				}
			}
		]
	};
});
