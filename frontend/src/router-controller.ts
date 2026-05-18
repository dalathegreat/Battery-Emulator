import { LitElement, ReactiveController, html } from "lit";
import { LitRouter, Page } from "./@system/lit-router/lit-router.js";

export class BatteryEmulatorRouterController implements ReactiveController {
	appName = "Battery Emulator";
	prefix = "";
	litRouter!: LitRouter;
	host: { currentView: any; islogged: boolean };

	constructor(host: { currentView: any; islogged: boolean } & LitElement) {
		host.addController(this);
		this.host = host;
	}
	hostConnected() {}

	setupRouter() {
		if (this.litRouter) return;
		let pages: Page[] = [
			{
				path: `${this.prefix}/dashboard`,
				element: html` <app-dashboard></app-dashboard> `,
				from: () => import("./components/app-dashboard.js"),
				auth: true,
				title: () => `Dashboard • ${this.appName}`
			},
			{
				path: `${this.prefix}/cellmonitor`,
				element: html` <app-cellmonitor></app-cellmonitor> `,
				from: () => import("./components/app-cellmonitor.js"),
				auth: true,
				title: () => `Cell Monitor • ${this.appName}`
			},
			{
				path: `${this.prefix}/settings`,
				element: html` <app-settings></app-settings> `,
				from: () => import("./components/app-settings.js"),
				auth: true,
				title: () => `Settings • ${this.appName}`
			},
			{
				path: `${this.prefix}/events`,
				element: html` <app-events></app-events> `,
				from: () => import("./components/app-events.js"),
				auth: true,
				title: () => `Events • ${this.appName}`
			},
			{
				path: `${this.prefix}/canlog`,
				element: html` <app-canlog></app-canlog> `,
				from: () => import("./components/app-canlog.js"),
				auth: true,
				title: () => `CAN Log • ${this.appName}`
			},
			{
				path: `${this.prefix}/debuglog`,
				element: html` <app-debuglog></app-debuglog> `,
				from: () => import("./components/app-debuglog.js"),
				auth: true,
				title: () => `Debug Log • ${this.appName}`
			},
			{
				path: `${this.prefix}/advanced`,
				element: html` <app-advanced></app-advanced> `,
				from: () => import("./components/app-advanced.js"),
				auth: true,
				title: () => `Advanced • ${this.appName}`
			}
		];

		this.litRouter = new LitRouter(pages, this.host.islogged);
		this.litRouter.on("login-needed", async () => {
			console.log("login-needed");
			this.loginNeeded();
		});
		this.litRouter.on("loading", async (e: CustomEvent<boolean>) => {
			this.isLoading(e);
		});
		this.litRouter.on("error-import", async () => {
			this.errorImport();
		});
		window.addEventListener("location-changed", async () => {
			this.host.currentView = this.litRouter.currentView;
			// this.host.innerHTML = this.litRouter.currentView.toString();
		});
		// @ts-ignore
		window.ctrouter = this.litRouter;
		this.litRouter.isLogged = this.host.islogged;
		this.litRouter.install();
	}

	isLoading(e: CustomEvent<boolean>) {
		console.log("isLoading", e.detail);
	}

	errorImport() {
		console.error("errorImport");
	}

	loginNeeded() {
		console.log("loginNeeded");
	}
}
