import { execSync } from "node:child_process";
import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { defineConfig, loadEnv, Plugin } from "vite";
import cssInjectedByJsPlugin from "vite-plugin-css-injected-by-js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export const svgLoader: () => Plugin = () => {
	// TODO: find better parser/tokenizer
	var regexSequences: [RegExp, string][] = [
		// Remove XML stuffs and comments
		[/<\?xml[\s\S]*?>/gi, ""],
		[/<!doctype[\s\S]*?>/gi, ""],
		[/<!--.*-->/gi, ""],

		// SVG XML -> HTML5
		[/\<([A-Za-z]+)([^\>]*)\/\>/g, "<$1$2></$1>"], // convert self-closing XML SVG nodes to explicitly closed HTML5 SVG nodes
		[/\s+/g, " "], // replace whitespace sequences with a single space
		[/\> \</g, "><"] // remove whitespace between tags
	];
	const getExtractedSVG = (svgStr: string) => regexSequences.reduce((prev, regexSequence) => prev.replace(regexSequence[0], regexSequence[1]), svgStr).trim();
	return {
		name: "vite-svg-patch-plugin",
		transform: function (code, id) {
			if (id.endsWith(".svg")) {
				const extractedSvg = readFileSync(id, "utf8");
				return `export const value = '${getExtractedSVG(extractedSvg)}'\n;export default value`;
			}
			return code;
		}
	};
};

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
			svgLoader(),
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
