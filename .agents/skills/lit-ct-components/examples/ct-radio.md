---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-radio.js'

const selectedValue = ref('option1')
</script>

# ct-radio

A customizable radio button web component built with LitElement.

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
import "@conectate/components/ct-radio.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 0.5rem; margin: 1rem 0;">
  <ct-radio name="group1" value="option1" :checked="selectedValue === 'option1'" @checked="selectedValue = 'option1'">Option 1</ct-radio>
  <ct-radio name="group1" value="option2" :checked="selectedValue === 'option2'" @checked="selectedValue = 'option2'">Option 2</ct-radio>
  <ct-radio name="group1" value="option3" disabled>Disabled Option</ct-radio>
</div>
<p style="color: #666;">Selected: {{ selectedValue }}</p>

```vue
<template>
	<div class="group">
		<ct-radio 
			name="my-group" 
			value="A" 
			:checked="choice === 'A'" 
			@checked="choice = 'A'">
			Choice A
		</ct-radio>
		<ct-radio 
			name="my-group" 
			value="B" 
			:checked="choice === 'B'" 
			@checked="choice = 'B'">
			Choice B
		</ct-radio>
	</div>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-radio.js";

const choice = ref('A')
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-radio.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private selected = "one";

	render() {
		return html`
			<div id="container">
				<ct-radio 
					name="group" 
					.checked=${this.selected === "one"} 
					@checked=${() => this.selected = "one"}>
					One
				</ct-radio>
				<ct-radio 
					name="group" 
					.checked=${this.selected === "two"} 
					@checked=${() => this.selected = "two"}>
					Two
				</ct-radio>
			</div>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-radio.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-radio': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					checked?: boolean;
					disabled?: boolean;
					name?: string;
					value?: any;
					onChecked?: (e: CustomEvent<{checked: boolean}>) => void;
				},
				HTMLElement
			>;
		}
	}
}

export function RadioGroup() {
	const [selected, setSelected] = useState('1');

	return (
		<div>
			<ct-radio 
				name="r-group" 
				checked={selected === '1'} 
				onChecked={() => setSelected('1')}>
				First
			</ct-radio>
			<ct-radio 
				name="r-group" 
				checked={selected === '2'} 
				onChecked={() => setSelected('2')}>
				Second
			</ct-radio>
		</div>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `checked` | `checked` | `boolean` | `false` | Current state of the radio |
| `disabled` | `disabled` | `boolean` | `false` | Disables the radio button |
| `name` | `name` | `string` | - | Group name for mutual exclusivity |
| `value` | - | `any` | - | Value associated with this radio |
| `parent` | - | `HTMLElement`| - | Container used to find other radios in the group |

## Methods

| Method | Description |
| ------ | ----------- |
| `click()` | Programmatically clicks the radio button |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `checked` | `{ checked: boolean }` | Fired when the radio is selected |

## CSS Variables

```css
ct-radio {
	/* Size */
	--ct-checkbox-box-size: 24px;
	
	/* Colors */
	--color-primary: #2cb5e8; /* Selection indicator color */
	--color-on-background: #535353; /* Border color when unchecked */
}
```

## Accessibility (a11y)

The component uses a hidden native checkbox/radio mechanism to handle focus and keyboard interactions. Group mutual exclusivity is handled automatically within the shared parent container.


