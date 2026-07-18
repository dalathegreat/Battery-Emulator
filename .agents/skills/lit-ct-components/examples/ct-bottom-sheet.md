---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-bottom-sheet.js'
import '@conectate/components/ct-button.js'

const isOpened = ref(false)
</script>

# ct-bottom-sheet

Bottom sheets slide up from the bottom of the screen to reveal more content.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the component in your project:

```ts
import "@conectate/components/ct-bottom-sheet.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-button raised @click="isOpened = true">Open Bottom Sheet</ct-button>
  
  <ct-bottom-sheet :opened="isOpened" @transitionend="if(!isOpened) isOpened = false" label="Bottom Sheet Title" @click="isOpened = false">
    <div style="padding: 1rem;">
      <p>This is the content of the bottom sheet.</p>
      <ct-button flat @click="isOpened = false">Close</ct-button>
    </div>
  </ct-bottom-sheet>
</div>

```vue
<template>
	<ct-button raised @click="isOpened = true">Open Bottom Sheet</ct-button>

	<ct-bottom-sheet :opened="isOpened" label="Bottom Sheet Title">
		<div style="padding: 1rem;">
			<p>This is the content of the bottom sheet.</p>
			<ct-button flat @click="isOpened = false">Close</ct-button>
		</div>
	</ct-bottom-sheet>
</template>

<script setup>
import { ref } from 'vue';
import "@conectate/components/ct-bottom-sheet.js";
import "@conectate/components/ct-button.js";

const isOpened = ref(false);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, query } from "lit";
import "@conectate/components/ct-bottom-sheet.js";
import { CtBottomSheet } from "@conectate/components/ct-bottom-sheet.js";

@customElement("my-element")
class MyElement extends LitElement {
	@query("ct-bottom-sheet") bottomSheet!: CtBottomSheet;

	render() {
		return html`
			<ct-button raised @click=${() => this.bottomSheet.open()}>Open</ct-button>
			<ct-bottom-sheet label="Settings">
				<div style="padding: 1rem;">
					<p>Configuration options go here.</p>
				</div>
			</ct-bottom-sheet>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useRef } from 'react';
import "@conectate/components/ct-bottom-sheet.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-bottom-sheet': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					closelabel?: string;
					'no-padding'?: boolean;
					'fit-bottom'?: boolean;
					'center-bottom'?: boolean;
					withbackdrop?: boolean;
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	const sheetRef = useRef<any>(null);

	return (
		<div>
			<button onClick={() => sheetRef.current?.open()}>Open Sheet</button>
			<ct-bottom-sheet ref={sheetRef} label="React Bottom Sheet" withbackdrop>
				<div style={{ padding: '20px' }}>
					<h3>Hello from React!</h3>
					<button onClick={() => sheetRef.current?.close()}>Close</button>
				</div>
			</ct-bottom-sheet>
		</div>
	);
}
```

## Variants

The bottom sheet supports different positioning and styling attributes:

- **fit-bottom**: Positioned at the bottom with full width and no border radius.
- **center-bottom**: Positioned at the bottom and centered.
- **withbackdrop**: Shows a backdrop behind the sheet.
- **no-padding**: Removes internal padding from the scrollable area.

### Interactive Example

<div style="margin: 1rem 0;">
  <ct-button raised @click="isOpened = true">Open Centered with Backdrop</ct-button>
  
  <ct-bottom-sheet class="center-bottom" withbackdrop :opened="isOpened" label="Centered Sheet" @click="isOpened = false">
    <div style="padding: 1rem; max-width: 400px;">
      <p>This sheet is centered and has a backdrop.</p>
    </div>
  </ct-bottom-sheet>
</div>

## Properties

| Property     | Attribute    | Type      | Default   | Description                                       |
| ------------ | ------------ | --------- | --------- | ------------------------------------------------- |
| `label`      | `label`      | `string`  | `""`      | The title displayed at the top of the sheet       |
| `closelabel` | `closelabel` | `string`  | `"Close"` | Text for the close button at the bottom           |
| `noPadding`  | `no-padding` | `boolean` | `false`   | Removes internal padding from the scrollable area |

## Attributes

| Attribute       | Description                                                  |
| --------------- | ------------------------------------------------------------ |
| `fit-bottom`    | Positions the sheet at the bottom of the app with full width |
| `center-bottom` | Positions the sheet at the bottom centered on the page       |
| `withbackdrop`  | Displays a backdrop behind the bottom sheet                  |
| `fixed-height`  | Applies a fixed height to the sheet                          |
| `full-width`    | Makes the sheet take up the full width of its container      |

## Slots

- **Default slot**: The main content area of the bottom sheet.

## CSS Variables

```css
ct-bottom-sheet {
	/* Colors */
	--bottom-sheet-background-color: #fff; /* Background color */
	--bottom-sheet-color: #323232; /* Text color */
	--bottom-sheet-label-color: rgba(0, 0, 0, 0.54); /* Title text color */
	--bottom-sheet-draggable-background-color: #9292922d; /* Draggable handle color */
	--bottom-sheet-border-color: #8181811c; /* Border color for the close button */

	/* Layout & Spacing */
	--bottom-sheet-max-width: auto; /* Maximum width */
	--bottom-sheet-max-height: auto; /* Maximum height */
	--bottom-sheet-box-shadow: 0 2px 5px 0 rgba(0, 0, 0, 0.26); /* Shadow effect */
	--border-radius: 16px; /* Draggable handle border radius */
}
```

### CSS Variables Reference Table

| Variable                                    | Default                           | Description                                 |
| ------------------------------------------- | --------------------------------- | ------------------------------------------- |
| `--bottom-sheet-background-color`           | `#fff`                            | Background color of the sheet               |
| `--bottom-sheet-color`                      | `#323232`                         | Main text color                             |
| `--bottom-sheet-label-color`                | `rgba(0, 0, 0, 0.54)`             | Color of the title label                    |
| `--bottom-sheet-max-width`                  | `auto`                            | Maximum width of the element                |
| `--bottom-sheet-max-height`                 | `auto`                            | Maximum height of the element               |
| `--bottom-sheet-box-shadow`                 | `0 2px 5px 0 rgba(0, 0, 0, 0.26)` | Box shadow of the element                   |
| `--bottom-sheet-draggable-background-color` | `#9292922d`                       | Color of the top draggable indicator handle |

## Accessibility (a11y)

Ensure that interactive elements within the bottom sheet have appropriate labels. When `withbackdrop` is used, the backdrop helps focus the user's attention on the sheet content.

## Event Handling

#### Vue Example

```vue
<template>
	<ct-bottom-sheet @transitionend="onTransitionEnd">
		<p>Content</p>
	</ct-bottom-sheet>
</template>

<script setup>
const onTransitionEnd = (e) => {
  console.log('Transition finished', e);
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-bottom-sheet.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-bottom-sheet @transitionend=${this.handleTransition}>
				<p>Sheet Content</p>
			</ct-bottom-sheet>
		`;
	}

	handleTransition(e: TransitionEvent) {
		console.log("Animation complete", e);
	}
}
```

