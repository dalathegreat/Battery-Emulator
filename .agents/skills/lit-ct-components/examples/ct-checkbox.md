---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-checkbox.js'

const checked1 = ref(false)
const checked2 = ref(true)
const checked3 = ref(false)
</script>

# ct-checkbox

A customizable checkbox web component built with LitElement.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

If you are using frameworks like Lit, React, or Vue, import the component:

```ts
import "@conectate/components/ct-checkbox.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
	<ct-checkbox v-model="checked1">Basic checkbox</ct-checkbox>
	<ct-checkbox v-model="checked2" checked>Pre-checked</ct-checkbox>
	<ct-checkbox label="Using label property"></ct-checkbox>
	<ct-checkbox indeterminate>Indeterminate</ct-checkbox>
	<ct-checkbox disabled>Disabled</ct-checkbox>
</div>

```vue
<template>
	<ct-checkbox v-model="checked">Basic checkbox</ct-checkbox>
	<ct-checkbox checked>Pre-checked</ct-checkbox>
	<ct-checkbox label="Using label property"></ct-checkbox>
	<ct-checkbox indeterminate>Indeterminate</ct-checkbox>
	<ct-checkbox disabled>Disabled</ct-checkbox>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-checkbox";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-checkbox.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-checkbox @checked=${this.handleChecked}>Check me</ct-checkbox>
		`;
	}

	handleChecked(e) {
		console.log("Checked:", e.detail.checked);
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-checkbox.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-checkbox': React.DetailedHTMLProps<
				React.InputHTMLAttributes<HTMLInputElement> & {
					checked?: boolean;
					indeterminate?: boolean;
					disabled?: boolean;
					label?: string;
					name?: string;
					value?: any;
					onChecked?: (event: CustomEvent<{checked: boolean}>) => void;
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	const [checked, setChecked] = useState(false);

	const handleChecked = (e: CustomEvent<{checked: boolean}>) => {
		setChecked(e.detail.checked);
		console.log('Checked:', e.detail.checked);
	};

	return (
		<ct-checkbox checked={checked} onChecked={handleChecked}>
			Check me
		</ct-checkbox>
	);
}
```

## States

The checkbox component supports three states:

- **Unchecked**: Default state
- **Checked**: When the checkbox is selected
- **Indeterminate**: When the checkbox is in an intermediate state

## Properties

| Property        | Attribute       | Type      | Default | Description                        |
| --------------- | --------------- | --------- | ------- | ---------------------------------- |
| `checked`       | `checked`       | `boolean` | `false` | Checkbox checked state             |
| `indeterminate` | `indeterminate` | `boolean` | `false` | Checkbox indeterminate state       |
| `disabled`      | `disabled`      | `boolean` | `false` | Disables the checkbox              |
| `value`         | `value`         | `any`     | -       | Value associated with the checkbox |
| `name`          | `name`          | `string`  | `""`    | Name attribute for form submission |
| `label`         | `label`         | `string`  | `""`    | Text label (alternative to slot)   |

## Slots

- **Default slot**: Content is used as the checkbox label

### Example with Slots

<div style="margin: 1rem 0;">
	<ct-checkbox>
		<span>Custom label content</span>
	</ct-checkbox>
</div>

```html
<ct-checkbox>
	<span>Custom label content</span>
</ct-checkbox>
```

## CSS Variables

You can customize the component using the following CSS variables:

```css
ct-checkbox {
	/* Size and Shape */
	--ct-checkbox-box-size: 24px; /* Size of the checkbox */
	--ct-checkbox-box-border-radius: 8px; /* Border radius of checkbox */
	--ct-checkbox-height: var(--ct-checkbox-box-size); /* Height of the checkbox component */
	--ct-checkbox-box-border-size: 3px; /* Border size of unchecked box */

	/* Colors */
	--color-primary: #2cb5e8; /* Color used for the checked state */
	--color-on-primary: #fff; /* Color of the checkmark */
	--color-on-background: #535353; /* Color of the unchecked checkbox border */
}
```

### CSS Variables Reference Table

| Variable                          | Default                       | Description                            |
| --------------------------------- | ----------------------------- | -------------------------------------- |
| `--ct-checkbox-box-size`          | `24px`                        | Size of the checkbox                   |
| `--ct-checkbox-box-border-radius` | `8px`                         | Border radius of checkbox              |
| `--ct-checkbox-height`            | `var(--ct-checkbox-box-size)` | Height of the checkbox component       |
| `--ct-checkbox-box-border-size`   | `3px`                         | Border size of unchecked box           |
| `--color-primary`                 | `#2cb5e8`                     | Color used for the checked state       |
| `--color-on-primary`              | `#fff`                        | Color of the checkmark                 |
| `--color-on-background`           | `#535353`                     | Color of the unchecked checkbox border |

### Custom Styling Examples

<div style="margin: 1rem 0;">
	<div style="display: flex; flex-direction: column; gap: 1rem;">
		<ct-checkbox style="--ct-checkbox-box-size: 32px; --color-primary: #ff6b6b;">Large Custom Color</ct-checkbox>
		<ct-checkbox style="--ct-checkbox-box-border-radius: 4px;">Square Checkbox</ct-checkbox>
	</div>
</div>

```html
<!-- Large checkbox with custom color -->
<ct-checkbox style="--ct-checkbox-box-size: 32px; --color-primary: #ff6b6b;">Large Custom Color</ct-checkbox>

<!-- Square checkbox -->
<ct-checkbox style="--ct-checkbox-box-border-radius: 4px;">Square Checkbox</ct-checkbox>
```

## Methods

| Name      | Description                          |
| --------- | ------------------------------------ |
| `click()` | Programmatically clicks the checkbox |

## Events

| Event Name | Detail                      | Description                      |
| ---------- | --------------------------- | -------------------------------- |
| `checked`  | `{checked: boolean}`        | Fires when checked state changes |
| `change`   | Native event (redispatched) | Standard input change event      |

## Accessibility (a11y)

The component has proper focus states and keyboard navigation support. Use `aria-label` for improved accessibility when needed.

### Example with aria-label

```html
<ct-checkbox aria-label="Accept terms and conditions">Accept terms</ct-checkbox>
```

## Event Handling

The checkbox supports standard change events and can be used with any framework:

#### Vue Example

```vue
<template>
	<ct-checkbox @checked="handleChecked">Check me</ct-checkbox>
</template>

<script setup>
import "@conectate/components/ct-checkbox";

const handleChecked = (e) => {
	console.log('Checked:', e.detail.checked);
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-checkbox.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-checkbox @checked=${this.handleChecked}>Check me</ct-checkbox>
		`;
	}

	handleChecked(e) {
		console.log("Checked:", e.detail.checked);
	}
}
```

#### React Example

```tsx
import React from 'react';
import "@conectate/components/ct-checkbox.js";

function MyComponent() {
	const handleChecked = (e: CustomEvent<{checked: boolean}>) => {
		console.log('Checked:', e.detail.checked);
	};

	return (
		<ct-checkbox onChecked={handleChecked}>Check me</ct-checkbox>
	);
}
```
