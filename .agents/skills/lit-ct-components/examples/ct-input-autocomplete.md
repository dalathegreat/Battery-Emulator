---
outline: deep
---

<script setup>
import { ref } from 'vue'
import { html } from 'lit'
import '@conectate/components/ct-input-autocomplete.js'
import '@conectate/components/ct-list-item.js'

const text = ref('')
const value = ref('')
const source = [
  { text: 'Apple', value: 'apple' },
  { text: 'Banana', value: 'banana' },
  { text: 'Cherry', value: 'cherry' },
  { text: 'Date', value: 'date' }
]

const renderItem = (item, index) => html`<ct-list-item>#${index} ${item.text}</ct-list-item>`
</script>

# ct-input-autocomplete

An input component with integrated autocomplete suggestions.

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
import "@conectate/components/ct-input-autocomplete.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-input-autocomplete 
    label="Search Fruit" 
    :source="source" 
    v-model:text="text"
    v-model:value="value"
    compute
  ></ct-input-autocomplete>
  <p style="margin-top: 1rem; color: #666;">
    Selected Text: {{ text }}<br>
    Selected Value: {{ value }}
  </p>
</div>

```vue
<template>
	<ct-input-autocomplete 
		label="Search Fruit" 
		:source="fruitSource" 
		v-model:text="searchText"
		v-model:value="selectedValue"
		compute
	></ct-input-autocomplete>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-input-autocomplete.js";

const searchText = ref('')
const selectedValue = ref('')
const fruitSource = [
	{ text: 'Apple', value: 'apple' },
	{ text: 'Banana', value: 'banana' },
	{ text: 'Cherry', value: 'cherry' }
]
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-input-autocomplete.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private source = [
		{ text: 'Option 1', value: 1 },
		{ text: 'Option 2', value: 2 }
	];

	render() {
		return html`
			<ct-input-autocomplete 
				label="Select Option" 
				.source=${this.source}
				@value=${(e: CustomEvent) => console.log('Value:', e.detail.value)}
				compute
			></ct-input-autocomplete>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-input-autocomplete.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-input-autocomplete': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					placeholder?: string;
					source?: any[];
					text?: string;
					value?: any;
					compute?: boolean;
					remote?: boolean;
					required?: boolean;
					errorMessage?: string;
					renderItem?: (item: any, index: number) => any;
				},
				HTMLElement
			>;
		}
	}
}

export function AutocompleteDemo() {
	const [text, setText] = useState('');
	const source = [{ text: 'One', value: 1 }, { text: 'Two', value: 2 }];

	return (
		<ct-input-autocomplete 
			label="Choose" 
			source={source} 
			text={text}
			onText={(e: any) => setText(e.detail.value)}
			compute
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `text` | `text` | `string` | `""` | Current text in the input |
| `value` | `value` | `any` | `undefined`| Selected value from source |
| `source` | - | `any[]` | `[]` | List of items for suggestions |
| `placeholder` | `placeholder` | `string` | `""` | Placeholder text |
| `compute` | `compute` | `boolean` | `false` | Auto-match value when text changes |
| `remote` | `remote` | `boolean` | `false` | Handle filtering server-side |
| `required` | `required` | `boolean` | `false` | Mark as required |
| `errorMessage`| `errorMessage`| `string`| `""` | Error message to display |
| `textProperty`| `textProperty`| `string`| `"text"` | Property name for display text |
| `valueProperty`| `valueProperty`| `string`| `"value"`| Property name for value |
| `renderItem` | - | `function` | - | Custom item renderer |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `text` | `{ value: string }` | Fired when text changes |
| `value` | `{ value: any }` | Fired when value changes |

## Accessibility (a11y)

The component uses `role="combobox"` and manages accessibility attributes for the suggestions list automatically.


