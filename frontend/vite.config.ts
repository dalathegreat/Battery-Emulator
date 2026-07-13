import minifyHTML from "@lit-labs/rollup-plugin-minify-html-literals";
import { transformSync } from "esbuild";
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

	// Every backend route the SPA talks to. Plain prefixes plus two regex
	// entries: the advanced-battery commands (PUT /{cmd}) and the legacy
	// live-edit GET routes (/update*, /Bal*, ...).
	const proxyPaths = [
		"/api",
		"/GetFirmwareInfo",
		"/startReplay",
		"/stopReplay",
		"/setCANInterface",
		"/stop_can_logging",
		"/import_can_log",
		"/export_can_log",
		"/export_log",
		"/set_can_id_cutoff",
		"/saveSettings",
		"/factoryReset",
		"/pause",
		"/equipmentStop",
		"/enableRecoveryMode",
		"/enableNewWebUI",
		"/reboot",
		"/logout",
		// PUT /{cmd} battery commands (see battery_commands in advanced_battery_html.cpp)
		"^/(clearIsolation|calibrateSOC|chademoRestart|chademoStop|resetBMS|resetSOC|resetCrash|resetNVROL|resetContactor|resetDTC|startBalancing|endBalancing|startBalancingRequest|stopBalancingRequest|isolationTest|readDTC|resetBECM|contactorClose|contactorOpen|resetSOH|setFactoryMode|toggleSOC|resetEnergySavingMode)$",
		// Live-edit GET routes registered by init_webserver()
		"^/(updateBatterySize|updateUseScaledSOC|updateSocMax|updateSocMin|updateMaxChargeA|updateMaxDischargeA|updateUseVoltageLimit|updateMaxChargeVoltage|updateMaxDischargeVoltage|updateBMSresetDuration|updateFakeBatteryVoltage|updateChargeSetpointV|updateChargeSetpointA|updateChargeEndA|updateChargerHvEnabled|updateChargerAux12vEnabled|TeslaBalAct|BalTime|BalFloatPower|BalMaxPackV|BalMaxCellV|BalMaxDevCellV)$"
	];

	return {
		root: __dirname,
		server: {
			port: 5173,
			proxy: Object.fromEntries(proxyPaths.map((p) => [p, { target: proxyTarget, changeOrigin: true }]))
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
			minifyHTML({
				// "options" se pasa directamente a minify-literals
				options: {
					minifyOptions: {
						// Reemplazamos el motor de clean-css por esbuild mediante una función
						minifyCSS: (text: string) => {
							try {
								// esbuild SÍ entiende el Nesting nativo (.actions-td { ct-icon { ... } })
								const result = transformSync(text, {
									loader: "css",
									minify: true
								});
								return result.code;
							} catch (err) {
								// Si esbuild falla por algún motivo, devolvemos el CSS original
								// Esto evita que todo el proceso de construcción (build) de Vite se caiga.
								console.warn("\n⚠️ Advertencia: No se pudo minificar un bloque CSS:", (err as Error).message);
								return text;
							}
						}
					}
				}
			}),
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
