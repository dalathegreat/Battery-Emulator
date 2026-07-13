---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-input-container.js'
</script>

# ct-input-container

A layout container for creating custom input components with consistent styling, floating labels, and support for prefixes/suffixes.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

The `ct-input-container` provides the outer shell (label, background, borders, character counter) for any custom input element.

```ts
import "@conectate/components/ct-input-container.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-input-container label="Custom Input" placeholder="Type here..." value="has value">
    <span slot="prefix">👉</span>
    <input slot="input" style="background:none; border:none; outline:none; width:100%; color:inherit;" placeholder="Native input">
    <span slot="suffix">👈</span>
  </ct-input-container>
</div>

```vue
<template>
	<ct-input-container label="My Custom Field" placeholder="Hint text">
		<span slot="prefix">Prefix</span>
		<input slot="input" @input="handleInput" placeholder="Placeholder">
		<span slot="suffix">Suffix</span>
	</ct-input-container>
</template>

<script setup>
import "@conectate/components/ct-input-container.js";

const handleInput = (e) => console.log(e.target.value);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-input-container.js";

@customElement("my-custom-field")
class MyCustomField extends LitElement {
	render() {
		return html`
			<ct-input-container label="Event Date" required>
				<ct-icon slot="prefix" icon="event"></ct-icon>
				<input slot="input" type="date">
			</ct-input-container>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-input-container.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-input-container': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					placeholder?: string;
					value?: string;
					required?: boolean;
					invalid?: boolean;
					disabled?: boolean;
					charCounter?: boolean;
					countChar?: number;
					maxlength?: number;
					block?: boolean;
				},
				HTMLElement
			>;
		}
	}
}

export function CustomInput() {
	return (
		<ct-input-container label="React Custom" placeholder="Placeholder">
			<input slot="input" />
		</ct-input-container>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Title label shown above input |
| `placeholder`| `placeholder`| `string`| `""` | Floating label text |
| `value` | `value` | `string` | `""` | Current value (used for floating label) |
| `required` | `required` | `boolean` | `false` | Shows required indicator |
| `invalid` | `invalid` | `boolean` | `false` | Sets error state colors |
| `disabled` | `disabled` | `boolean` | `false` | Disables interaction |
| `charCounter`| `char-counter`| `boolean`| `false` | Shows character counter |
| `countChar` | `count-char` | `number` | `0` | Current character count |
| `maxlength` | `maxlength` | `number` | `5000` | Maximum characters |
| `block` | `block` | `boolean` | `false` | Full-width display |

## Slots

- **prefix**: Content at the start
- **input**: The actual input element (e.g., `<input>`, `<select>`, etc.)
- **suffix**: Content at the end

## CSS Variables

```css
ct-input-container {
	/* Colors */
	--color-on-surface: #535353; /* Text color */
	--color-primary: #2cb5e8; /* Caret and active color */
	--ct-indicator: "*"; /* Required field character */

	/* Layout */
	--border-radius: 16px; /* Container radius */
	--ct-input-padding: 0em 1em; /* Internal padding */
	--ct-input-height: 3.3em; /* Height of input area */
}
```

## Accessibility (a11y)

The container handles ARIA roles if provided by the child input. Ensure the child element has proper accessibility attributes.


