/**
 * Global design tokens for the Battery Emulator UI.
 * Dark theme by default (matches the device aesthetic), light theme via `body.light`.
 * Injected once into <head> from lit-app.ts — components consume the CSS variables.
 */

import { css } from "lit";

const THEME_STORAGE_KEY = "be-theme";

const themeCss = css`
	:root {
		/* Brand */
	--color-primary: #34d399;
	--color-primary-medium: #34d399b0;
	--color-primary-light: #34d3992b;
	--color-primary-hover: #2cbd88;
	--color-primary-active: #26a578;
	--color-on-primary: #04251a;

		--color-accent: #38bdf8;
		--color-accent-light: #38bdf82b;

		/* Semantic */
		--color-success: #4ade80;
		--color-success-container: #4ade802b;
		--color-warning: #fbbf24;
		--color-warning-container: #fbbf242b;
		--color-error: #f87171;
		--color-error-container: #f871712b;
		--color-on-error: #fff;

		/* Surfaces */
		--color-background: #0b1215;
		--color-on-background: #dbe4e9;
		--color-surface: #162126;
		--color-surface-bright: #1d2b32;
		--color-surface-dim: #0d1114;
		--color-on-surface: #dbe4e9;

		--high-emphasis: #f2f7f9f5;
		--medium-emphasis: #dbe4e999;
		--color-disable: #dbe4e947;

		--color-outline: #8ba3ad2e;
		--color-scrim: #000000a6;

		/* Layout */
		--radius-card: 14px;
		--radius-control: 9px;
		--toolbar-height: 52px;
		--drawer-width: 248px;
		--font-body: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
		--font-mono: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;

		--zi-drawer: 20;
		--zi-toolbar: 15;
		--zi-scrim: 18;

		color-scheme: dark;
	}

	body.light {
	--color-primary: #059669;
	--color-primary-medium: #059669b0;
	--color-primary-light: #0596692b;
	--color-primary-hover: #04855d;
	--color-primary-active: #037451;
	--color-on-primary: #ffffff;

		--color-accent: #0284c7;
		--color-accent-light: #0284c72b;

		--color-success: #16a34a;
		--color-success-container: #16a34a24;
		--color-warning: #b45309;
		--color-warning-container: #b4530924;
		--color-error: #dc2626;
		--color-error-container: #dc262624;
		--color-on-error: #fff;

		--color-background: #eef3f4;
		--color-on-background: #29363c;
		--color-surface: #ffffff;
		--color-surface-bright: #f7fafb;
		--color-surface-dim: #e6edee;
		--color-on-surface: #29363c;

		--high-emphasis: #101b20f5;
		--medium-emphasis: #101b2099;
		--color-disable: #101b2047;

		--color-outline: #29363c26;
		--color-scrim: #0000007a;

		color-scheme: light;
	}

	html,
	body {
		margin: 0;
		padding: 0;
		height: 100%;
		background: var(--color-background);
		color: var(--color-on-background);
		font-family: var(--font-body);
	}

	body.overflow-hidden {
		overflow: hidden;
	}

	::selection {
		background: var(--color-primary-light);
	}
`;

export type ThemeName = "dark" | "light";

export function getTheme(): ThemeName {
	return localStorage.getItem(THEME_STORAGE_KEY) === "light" ? "light" : "dark";
}

export function setTheme(theme: ThemeName) {
	localStorage.setItem(THEME_STORAGE_KEY, theme);
	document.body.classList.toggle("light", theme === "light");
	const meta = document.querySelector<HTMLMetaElement>('meta[name="theme-color"]');
	if (meta) meta.content = theme === "light" ? "#eef3f4" : "#0b1215";
}

export function toggleTheme(): ThemeName {
	const next: ThemeName = getTheme() === "light" ? "dark" : "light";
	setTheme(next);
	return next;
}

export function injectTheme() {
	if (document.getElementById("be-theme")) return;
	const style = document.createElement("style");
	style.id = "be-theme";
	style.textContent = themeCss.toString();
	document.head.appendChild(style);
	setTheme(getTheme());
}
