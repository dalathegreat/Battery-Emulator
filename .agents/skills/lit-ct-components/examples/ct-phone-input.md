---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-phone-input.js'

const phoneValue = ref('502,12345678')
</script>

# ct-phone-input

A phone number input component with a separate country code field and a combined value property.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the component:

```ts
import "@conectate/components/ct-phone-input.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-phone-input 
    label="Phone Number" 
    v-model="phoneValue"
    required
  ></ct-phone-input>
  <p style="margin-top: 1rem; color: #666;">
    Combined Value (code,phone): {{ phoneValue }}
  </p>
</div>

```vue
<template>
	<ct-phone-input 
		label="Mobile Phone" 
		v-model="phoneValue"
		required
	></ct-phone-input>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-phone-input.js";

const phoneValue = ref('502,12345678')
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-phone-input.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private value = "502,87654321";

	render() {
		return html`
			<ct-phone-input 
				label="Contact Number" 
				.value=${this.value}
				@value=${(e: any) => this.value = e.detail.value}
			></ct-phone-input>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-phone-input.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-phone-input': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					value?: string | null;
					code?: string;
					phone?: string;
					required?: boolean;
					invalid?: boolean;
					errorMessage?: string;
				},
				HTMLElement
			>;
		}
	}
}

export function PhoneField() {
	const [value, setValue] = useState('502,12345678');

	return (
		<ct-phone-input 
			label="Phone" 
			value={value} 
			onValue={(e: any) => setValue(e.detail.value)}
			required
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `value` | `value` | `string \| null`| - | Combined value as `"code,phone"` |
| `code` | `code` | `string` | `"502"` | Country calling code |
| `phone` | `phone` | `string` | `""` | Phone number part |
| `required` | `required` | `boolean` | `false` | Mark as required |
| `invalid` | `invalid` | `boolean` | `false` | Current validation state |
| `errorMessage`| `errorMessage`| `string`| `""` | Error message to display |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation and returns `true` if valid |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `value` | `{ value: string }` | Fired when the combined value changes |

## CSS Variables

The component uses internal styles and relies on `ct-input-container`.

```css
ct-phone-input {
	/* Colors */
	--color-on-surface: #535353;
	
	/* Layout */
	--ct-input-height: 3.3em;
}
```

## Accessibility (a11y)

The component uses `<input type="tel">` for both code and phone fields, which triggers the appropriate numeric keyboard on mobile devices.


