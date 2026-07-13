---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-button.js'
import '@conectate/components/ct-button-split.js'
import '@conectate/components/ct-button-menu.js'
import '@conectate/components/ct-list-item.js'

const message = ref('')
</script>

# ct-button-split

A container component that combines a standard button with a dropdown menu to create a split button.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the component and its dependencies:

```ts
import "@conectate/components/ct-button-split.js";
import "@conectate/components/ct-button.js";
import "@conectate/components/ct-button-menu.js";
```

### Example with Vue

<div style="display: flex; gap: 1rem; flex-wrap: wrap; margin: 1rem 0;">
  <ct-button-split raised>
    <ct-button>Main Action</ct-button>
    <ct-button-menu>
      <ct-list-item @click="message = 'Option 1 clicked'">Option 1</ct-list-item>
      <ct-list-item @click="message = 'Option 2 clicked'">Option 2</ct-list-item>
    </ct-button-menu>
  </ct-button-split>
</div>
<p v-if="message" style="color: green; font-weight: bold;">{{ message }}</p>

```vue
<template>
	<ct-button-split raised>
		<ct-button @click="handleMainAction">Main Action</ct-button>
		<ct-button-menu>
			<ct-list-item @click="handleOption(1)">Option 1</ct-list-item>
			<ct-list-item @click="handleOption(2)">Option 2</ct-list-item>
		</ct-button-menu>
	</ct-button-split>
</template>

<script setup>
import "@conectate/components/ct-button-split.js";
import "@conectate/components/ct-button.js";
import "@conectate/components/ct-button-menu.js";
import "@conectate/components/ct-list-item.js";

const handleMainAction = () => console.log('Main action clicked');
const handleOption = (id) => console.log(`Option ${id} clicked`);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-button-split.js";
import "@conectate/components/ct-button.js";
import "@conectate/components/ct-button-menu.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-button-split raised>
				<ct-button @click=${() => console.log('Main action')}>Save</ct-button>
				<ct-button-menu>
					<ct-list-item @click=${() => console.log('Save and publish')}>Save and publish</ct-list-item>
					<ct-list-item @click=${() => console.log('Save as draft')}>Save as draft</ct-list-item>
				</ct-button-menu>
			</ct-button-split>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-button-split.js";
import "@conectate/components/ct-button.js";
import "@conectate/components/ct-button-menu.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-button-split': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					raised?: boolean;
					shadow?: boolean;
					flat?: boolean;
					light?: boolean;
				},
				HTMLElement
			>;
		}
	}
}

export function MySplitButton() {
	return (
		<ct-button-split raised>
			<ct-button onClick={() => console.log('Main action')}>Main Action</ct-button>
			<ct-button-menu>
				<ct-list-item onClick={() => console.log('Option 1')}>Option 1</ct-list-item>
				<ct-list-item onClick={() => console.log('Option 2')}>Option 2</ct-list-item>
			</ct-button-menu>
		</ct-button-split>
	);
}
```

## Variants

The split button reflects the same style variants as `ct-button`:

- **Default**: Standard split button
- **Raised**: Primary colored split button
- **Shadow**: Split button with opaque black background
- **Flat**: Split button with primary color borders
- **Light**: Split button with primary color borders (alternative style)

## Properties

| Property | Attribute | Type      | Default | Description                         |
| -------- | --------- | --------- | ------- | ----------------------------------- |
| `raised` | `raised`  | `boolean` | `false` | Raised Style (primary color)        |
| `shadow` | `shadow`  | `boolean` | `false` | Shown with opaque black background. |
| `flat`   | `flat`    | `boolean` | `false` | Shown with a primary color border   |
| `light`  | `light`   | `boolean` | `false` | Shown with a primary color border   |

## Slots

- **Default slot**: Should contain a `ct-button` for the primary action and a `ct-button-menu` for the dropdown.

## CSS Variables

The component uses styles from `ct-button` and provides some internal overrides for its children.

```css
ct-button-split {
	/* Internal overrides for children */
	--ct-button-padding: 0.4em 0.5em 0.4em 1em;
	--ct-button-radius: 26px 4px 4px 26px;
}
```

## Accessibility (a11y)

The component relies on the accessibility features of its children (`ct-button` and `ct-button-menu`). Ensure you provide appropriate `aria-label` or titles where necessary.

## Event Handling

Events are handled by the individual components within the split button.

<div style="margin: 1rem 0;">
  <ct-button-split raised>
    <ct-button @click="message = 'Primary action triggered'">Primary Action</ct-button>
    <ct-button-menu>
      <ct-list-item @click="message = 'Secondary action 1 triggered'">Secondary 1</ct-list-item>
      <ct-list-item @click="message = 'Secondary action 2 triggered'">Secondary 2</ct-list-item>
    </ct-button-menu>
  </ct-button-split>
</div>


