---
outline: deep
---

# ct-qr-tools

A collection of utility functions for writing and reading QR codes using dynamic imports for performance.

## Installation

Install via npm or pnpm along with the required peer dependencies:

```sh
pnpm i @conectate/components qrcode jsqr
# or
npm i @conectate/components qrcode jsqr
```

## Basic Usage

The tools use dynamic imports to load large libraries only when needed.

### Writing a QR Code

Uses the `qrcode` library to generate QR codes.

```ts
import { writeQR } from "@conectate/components/ct-qr-tools.js";

async function generateQR(text: string, canvasElement: HTMLCanvasElement) {
	const QRCode = await writeQR();
	await QRCode.toCanvas(canvasElement, text, {
		width: 256,
		margin: 2
	});
}
```

### Reading a QR Code

Uses the `jsqr` library to parse QR code data from image pixels.

```ts
import { readQR } from "@conectate/components/ct-qr-tools.js";

async function scanQR(imageData: ImageData) {
	const jsQR = await readQR();
	const code = jsQR(imageData.data, imageData.width, imageData.height);
	
	if (code) {
		console.log("Found QR code:", code.data);
		return code.data;
	}
	return null;
}
```

## API Reference

### `writeQR()`

**Returns**: `Promise<typeof import('qrcode')>`
Returns the `qrcode` module instance.

### `readQR()`

**Returns**: `Promise<typeof import('jsqr').default>`
Returns the `jsqr` default export function.

## Performance Note

Both functions use `import()` to dynamically load the underlying libraries. This ensures that the heavy QR processing code is not included in your main application bundle until it's actually required.


