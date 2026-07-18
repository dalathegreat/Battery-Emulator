---
outline: deep
---

# ct-helpers

A collection of utility functions and classes used across the Conectate Elements library for browser detection, geolocation, and more.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Browser and OS Detection

### `getClient(userAgent?)`

Parses a user agent string to identify the browser and operating system.

```ts
import { getClient } from "@conectate/components/ct-helpers.js";

const client = getClient();
console.log(client.browser); // 'chrome', 'firefox', etc.
console.log(client.os);      // 'ios', 'android', 'windows', etc.
console.log(client.isMobile); // true/false
```

### `browserCapabilities(userAgent)`

Returns a set of features supported by the browser based on its version.

```ts
import { browserCapabilities } from "@conectate/components/ct-helpers.js";

const caps = browserCapabilities(navigator.userAgent);
if (caps.has('modules')) {
	// Browser supports native JS modules
}
```

## Geolocation

### `getGeoLocation()`

A wrapper around the native Geolocation API that returns a Promise.

```ts
import { getGeoLocation } from "@conectate/components/ct-helpers.js";

try {
	const { lat, lon } = await getGeoLocation();
	console.log(`Location: ${lat}, ${lon}`);
} catch (err) {
	console.error(err.error);
}
```

## Utilities

### `sleep(ms)`

A simple promise-based delay function.

```ts
import { sleep } from "@conectate/components/ct-helpers.js";

async function demo() {
	console.log("Waiting...");
	await sleep(2000); // Wait 2 seconds
	console.log("Done!");
}
```

### `PushID`

A class to generate sortable, unique IDs (similar to Firebase's push IDs).

```ts
import { PushID } from "@conectate/components/ct-helpers.js";

const pushId = new PushID();
const id = pushId.next(); // e.g. "-M1234567890abcdefgh"
```

## Interfaces

### `UAClientDescription`

| Property | Type | Description |
| -------- | ---- | ----------- |
| `browser`| `string`| Name of the browser |
| `browserVersion`| `number`| Main version number |
| `isMobile`| `boolean`| True if it's a mobile device |
| `os`| `string`| Operating system name |
| `osVersion`| `number`| OS version number |
| `device`| `string` (opt)| Device model (if detected) |


