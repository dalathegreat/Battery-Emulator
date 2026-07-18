---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-input-phone.js'

const phone = ref('')
const code = ref(502)
</script>

# ct-input-phone

A specialized phone number input component with a separate country code field.

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
import "@conectate/components/ct-input-phone.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-input-phone 
    label="Phone Number" 
    v-model:phone="phone" 
    v-model:code="code"
    required
  ></ct-input-phone>
  <p style="margin-top: 1rem; color: #666;">
    Selected Value: +{{ code }} {{ phone }}
  </p>
</div>

```vue
<template>
	<ct-input-phone 
		label="Mobile Phone" 
		v-model:phone="phoneNumber" 
		v-model:code="countryCode"
		required
	></ct-input-phone>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-input-phone.js";

const phoneNumber = ref('')
const countryCode = ref(502)
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-input-phone.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private phone = "";
	@state() private code = 502;

	render() {
		return html`
			<ct-input-phone 
				label="Contact Number" 
				.phone=${this.phone}
				.code=${this.code}
				@phone-changed=${(e: any) => this.phone = e.detail.value}
				@code-changed=${(e: any) => this.code = e.detail.value}
			></ct-input-phone>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-input-phone.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-input-phone': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					phone?: string;
					code?: number;
					required?: boolean;
					invalid?: boolean;
					errorMessage?: string;
					autocomplete?: string;
				},
				HTMLElement
			>;
		}
	}
}

export function PhoneField() {
	const [phone, setPhone] = useState('');
	const [code, setCode] = useState(1);

	return (
		<ct-input-phone 
			label="Phone" 
			phone={phone} 
			code={code}
			required
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `phone` | `phone` | `string` | `""` | Phone number part |
| `code` | `code` | `number` | `502` | Country calling code |
| `required` | `required` | `boolean` | `false` | Mark as required |
| `invalid` | `invalid` | `boolean` | `false` | Current validation state |
| `errorMessage`| `errorMessage`| `string`| `""` | Error message to display |
| `autocomplete`| `autocomplete`| `string`| `"off"` | Autocomplete attribute |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation and returns `true` if valid |

## CSS Variables

The component uses internal styles and relies on `ct-input-container`.

```css
ct-input-phone {
	/* Colors */
	--color-on-surface: #535353;
	
	/* Layout */
	--ct-input-height: 3.3em;
}
```

## Accessibility (a11y)

The component uses `<input type="tel">` for both code and phone fields, which triggers the appropriate numeric keyboard on mobile devices.


