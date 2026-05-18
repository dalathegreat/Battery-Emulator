import { TemplateResult } from "lit";
import { installRouter } from "pwa-helpers/router";

import { EvaluateParams, pathToRegExpAndGroups } from "./path_to_regexp";

declare global {
	interface Window {
		__litRouter: LitRouter;
	}
}
export interface Page {
	path: string | RegExp;
	element?: string | TemplateResult | Element | HTMLElement | any;
	from?: () => Promise<any>;
	auth?: boolean | null;
	title?: (() => string) | (() => null);
	description?: string;
	JSON_LD?: () => Promise<object>;
	groups?: string[];
	extra?: Record<string, any>;
	children?: Page[];
}
export interface LocationChanged {
	//patternMatched like a: /:profile/preferences
	path: string;
	// pathname like a: /herberthobregon/preferences
	pathname: string;
	// if path is /home?hello=word then queryParams is { hello : "world" }
	queryParams?: { [x: string]: string };
	// if href is /herberth/preference and path is /:username/preference then params is { username : "herberth" }
	params?: { [x: string]: string };
}

interface Routes {
	path: string | RegExp;
	element?: string | TemplateResult | Element | HTMLElement | any;
	from?: () => Promise<any>;
	auth?: boolean | null;
	title?: (() => string) | (() => null);
	description?: string;
	JSON_LD?: () => Promise<object>;
	c2regexp: { regexp: RegExp; groups: string[] };
}

type CtRouterListeners = "beforeunload" | "change" | "login-needed" | "loading" | "error-import";
/**
 * ## `lit-router`
 * It's a simple routing system that changes the viewport depending on the route given
 *
 * @element lit-router
 * @event login-needed It triggers when a page requires authentication but the user is not yet logged in
 * @event loading It fires when a page is imported diamicamente and it is fired again when it finishes loading the page
 * @event location-changed it shoots when the route changes
 * @event error-import it fires when the page is not found
 */
export class LitRouter {
	/**
	 * This is a dictionary of routes linked to its corresponding elements.
	 */
	private _routes: { [x: string]: Routes } = {};
	/**
	 * This holds the next route to be viewed after the url has been changed.
	 */
	private patternMatched: string = "";
	/**
	 * This holds the current route that is being viewed
	 */
	private _currentView: any = "";
	/**
	 * This is a indication if the viewport is loading a page or not.
	 */
	loading: boolean = false;
	get currentView() {
		return this._currentView;
	}
	set currentView(view: any) {
		this._currentView = view;
	}
	/**
	 * Current Path
	 */
	private pathname: string = "/";

	/**
	 * Login needed fallback path
	 */
	public loginFallback = "/login";
	/**
	 * Array de elementos {path,element(HTML),from,auth,title}
	 */
	public pages: Page[] = [];
	/**
	 * This is an object that holds the values of the query parameters in the url
	 */
	public queryParams: { [x: string]: string } = {};
	/**
	 * This is an object that holds the values of the parameters in the route
	 * pattern set in the current page element being viewed.
	 */
	public params: { [x: string]: string } = {};
	/**
	 * Parametro para verificar el auto
	 */
	private _isLogged: boolean = false;

	public get isLogged(): boolean {
		return this._isLogged;
	}

	public set isLogged(v: boolean) {
		this._isLogged = v;
		this._setPath(location.pathname);
		this._setQueryParams(getQueryParams());
		this.href(this.pathname);
	}

	private listeners: { name: CtRouterListeners; callback: (param?: any) => Promise<boolean | void>; id: number }[] = [];
	private listenerID = 0;

	constructor(pages: Page[], auth?: boolean) {
		this.pages = this.flattenPages(pages);
		this.isLogged = auth || false;
	}

	flattenPages(pages: Page[], parentPath: string | RegExp = ""): Page[] {
		let pagesFlat: Page[] = [];
		for (const p of pages) {
			const fullPath = p.path instanceof RegExp ? p.path : `${parentPath}${p.path}`;
			if (p.element) {
				pagesFlat.push({ ...p, path: fullPath });
			}
			if (p.children) {
				pagesFlat.push(...this.flattenPages(p.children, fullPath));
			}
		}
		return pagesFlat;
	}

	install() {
		if (window.__litRouter) {
			console.error("lit-router is already installed");
			return;
		}
		window.__litRouter = this;
		installRouter(l => this.handleRoutes(l));
		window.addEventListener("href-fire", () => this.handleRoutes(window.location));
		this.addContent(this.pages);
	}

	on(listener: CtRouterListeners, func: (param?: any) => Promise<boolean | void>) {
		let id = this.listenerID++;
		this.listeners.push({ name: listener, callback: func, id });
		return id;
	}
	deleteListener(id: number) {
		this.listeners = this.listeners.filter(l => l.id != id);
	}

	updated(m: Map<string, any>) {
		if (m.has("pages")) {
			this.addContent(this.pages || []);
		}
	}

	replaceState(data: object, title: string, url: string) {
		window.history?.replaceState(data, title, url);
		this._setPath(url);
	}

	async handleRoutes(location: Location) {
		/*
        Esto es por si hago un push state y luego hago un back entonces no cambie la URL
        ej: /herberth -> /herberth#showDialog -> /herberth
        */
		this._setQueryParams(getQueryParams());
		if (location.pathname != this.pathname) {
			let beforeunload = this.listeners.find(l => l.name == "beforeunload");
			if (beforeunload) {
				let cont = await beforeunload.callback();
				if (cont) {
					this._setPath(location.pathname);
					this.href(this.pathname);
				} else {
					window.history?.replaceState(null, document.title, this.pathname);
				}
			} else {
				this._setPath(location.pathname);
				this.href(this.pathname);
			}
		} else {
			this.dispatchLocationChanged();
		}
	}

	/**
	 * Sets the path property
	 */
	_setPath(path: string) {
		this.pathname = path || "/";
	}

	/**
	 * Sets the queryParams property and updates the element's params and queryParams
	 * properties. It also calls the page element's updateView method if it exists
	 */
	_setQueryParams(queryParams: { [x: string]: string }) {
		let original = this.queryParams;
		if (JSON.stringify(original) != JSON.stringify(queryParams)) {
			let ce = new CustomEvent("query-changed", {
				detail: { queryParams: queryParams }
			});
			window.dispatchEvent(ce);
		}
		this.queryParams = queryParams || {};
	}

	/**
	 * Called in the attached lifecycle method to put the children to the dictionary
	 * for easy referencing
	 */
	addContent(pages: Page[]) {
		let newContentRoutes: { [key: string]: boolean } = {};
		for (let el of pages) {
			if (el && el.path) {
				let regExpAndGroups: { regexp: RegExp; groups: string[] };
				// Custom regex and keys
				if (el.path instanceof RegExp) {
					regExpAndGroups = { regexp: el.path, groups: el.groups || [] };
				} else {
					regExpAndGroups = pathToRegExpAndGroups(el.path);
				}
				if (!newContentRoutes[el.path.toString()]) {
					newContentRoutes[el.path.toString()] = true;
					this._routes[el.path.toString()] = {
						path: el.path,
						element: el.element,
						from: el.from,
						auth: el.auth,
						title: el.title,
						description: el.description,
						JSON_LD: el.JSON_LD,
						c2regexp: regExpAndGroups
					};
				} else {
					console.warn(new Error(`The Path: '${el.path}' already use`));
				}
			}
		}
		// cuando termina de agregar nuevo contenido entonces acciono el pathChanged para ver si hay un mejor match
		this.href(this.pathname);
	}

	/**
	 * Here is the magic! This is called when the path changes. It tries to look for a the pattern
	 * that matches the path, then calls the _changeView method to change the view
	 */
	href(path: string) {
		let routes = this._routes;
		this.params = {};
		let routePaths = this.pages;
		if (routePaths.length == 0 || Object.keys(routes).length == 0) {
			return;
		}

		this.patternMatched = "";
		for (let i = 0; i < routePaths.length; i++) {
			let element = routes[routePaths[i].path.toString()]; // element,c2regexp,from,auth
			// console.log(routePaths[i].path,element.c2regexp.regexp);
			let c2 = EvaluateParams(path, element.c2regexp);
			if (c2 != null) {
				// Set params like a  {username : "herberthbregon", "jid" : 1234}
				this.params = c2;
				this.patternMatched = routePaths[i].path.toString();
				break;
			}
		}

		// if pattern matched is created, change the view, if exausted all
		// route patterns, make view a not-found
		// console.log(this.patternMatched);
		if (this.patternMatched) {
			// Si la vista es protegida y no esta logeado entonces lo mando a /login y no esta autenticado
			if (routes[this.patternMatched].auth && !this.isLogged) {
				console.warn("You need to log in to perform this action");
				this.patternMatched = this.patternMatched = this.loginFallback;
				this.patternMatched = this.loginFallback;
				this.pathname = this.loginFallback;
				this._currentView = this._routes[this.loginFallback]?.element;
				let ce = new CustomEvent("login-needed", {
					detail: { path: window.location.pathname }
				});
				this.dispatchEvent(ce);
				window.dispatchEvent(ce);
			} else {
				this._currentView = this._routes[this.patternMatched].element || "";
			}
		} else {
			console.log("/404");
			this.patternMatched = "/404";
			this.pathname = "/404";
			this._currentView = this._routes["/404"]?.element;
		}

		this.dispatchLocationChanged();
		if (this.patternMatched && routes[this.patternMatched]) {
			let fromImport = routes[this.patternMatched].from;
			if (fromImport) {
				this.dispatchEvent(new CustomEvent("loading", { detail: true }));
				this.loading = true;
				fromImport()
					?.then(() => {
						if (this.patternMatched) this.dispatchEvent(new CustomEvent(this.patternMatched));
						setTimeout(() => {
							this.loading = false;
							this.dispatchEvent(new CustomEvent("loading", { detail: false }));
						}, 250);
					})
					.catch((e: any) => {
						console.error(e);
						this.dispatchEvent(new CustomEvent("loading", { detail: false }));
						this.dispatchEvent(new CustomEvent("error-import", { detail: { error: e } }));
						console.error("Can't lazy-import - " + fromImport);
					});
			} else {
				this.loading = false;
			}
			let title = routes[this.patternMatched].title?.();
			if (title) {
				document.title = title;
			}
			if (routes[this.patternMatched].JSON_LD) {
				routes[this.patternMatched].JSON_LD!().then(jsonLD => {
					// find the first script tag with type application/ld+json
					let script = document.querySelector("script[type='application/ld+json']");
					if (script) {
						script.innerHTML = JSON.stringify(jsonLD);
					} else {
						let script = document.createElement("script");
						script.type = "application/ld+json";
						script.innerHTML = JSON.stringify(jsonLD);
						document.head.appendChild(script);
					}
				});
			}
		}
	}
	dispatchEvent(ce: CustomEvent) {
		let listeners = this.listeners.filter(l => l.name == ce.type);
		listeners.forEach(l => l.callback(ce));
	}

	dispatchLocationChanged() {
		let ce = new CustomEvent("location-changed", {
			detail: {
				search: location.search,
				path: this.patternMatched,
				pathname: this.pathname,
				queryParams: this.queryParams,
				params: this.params
			}
		});
		this.dispatchEvent(ce);
		window.dispatchEvent(ce);
	}
}

export function href(path: string, name: string = document.title) {
	window.history.pushState({}, name, path);
	window.dispatchEvent(new CustomEvent("href-fire"));
}

window.href = href;

export function getCtRouter() {
	return window.ctrouter;
}

export function getQuery(): URLSearchParams {
	let u = new URL(location.href);
	return u.searchParams;
}

/** @deprecated */
export function getQueryParams(): Record<string, string> {
	let pairs = window.location.search.substring(1).split("&"),
		obj: Record<string, string> = {};

	for (let i in pairs) {
		if (pairs[i] === "") continue;

		let pair = pairs[i].split("=");
		obj[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1]);
	}
	return obj || {};
}
