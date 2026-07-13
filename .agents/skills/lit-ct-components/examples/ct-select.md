---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-select.js'

const singleValue = ref(1)
const multiValue = ref([1, 2])
const items = [
  { text: 'Option 1', value: 1 },
  { text: 'Option 2', value: 2 },
  { text: 'Option 3', value: 3 },
  { text: 'Option 4', value: 4 }
]
</script>

# ct-select

A versatile selection component that opens a search-enabled dialog for choosing one or more items.

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
import "@conectate/components/ct-select.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
  <ct-select 
    label="Single Selection" 
    :items="items" 
    v-model="singleValue"
    searchable
  ></ct-select>
  
  <ct-select 
    label="Multiple Selection" 
    :items="items" 
    v-model="multiValue" 
    multi 
    searchable
    selected-placeholder="items picked"
  ></ct-select>
</div>

<p style="color: #666;">
  Single: {{ singleValue }} | Multi: {{ multiValue }}
</p>

```vue
<template>
	<ct-select 
		label="Category" 
		:items="categories" 
		v-model="selectedId"
		searchable
	></ct-select>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-select.js";

const selectedId = ref(null)
const categories = [
	{ text: 'Electronics', value: 'elec' },
	{ text: 'Books', value: 'books' }
]
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-select.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private items = [
		{ name: "High", id: 1 },
		{ name: "Medium", id: 2 },
		{ name: "Low", id: 3 }
	];
	@state() private value = 2;

	render() {
		return html`
			<ct-select 
				label="Priority" 
				.items=${this.items} 
				.value=${this.value}
				textProperty="name"
				valueProperty="id"
				@value=${(e: any) => this.value = e.detail.value}
			></ct-select>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-select.js";

export function SelectField() {
	const [val, setVal] = useState(1);
	const items = [{ text: 'Yes', value: 1 }, { text: 'No', value: 0 }];

	return (
		<ct-select 
			label="Confirm" 
			items={items} 
			value={val} 
			onValue={(e: any) => setVal(e.detail.value)}
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `items` | - | `T[]` | `[]` | Array of items to select from |
| `value` | `value` | `any` | - | Current selected value (single) or array (multi) |
| `multi` | `multi` | `boolean` | `false` | Enable multiple selection |
| `searchable` | `searchable`| `boolean`| `false` | Show search in the dialog |
| `required` | `required` | `boolean` | `false` | Mark as required |
| `invalid` | `invalid` | `boolean` | `false` | Current validation state |
| `placeholder` | `placeholder`| `string`| `""` | Floating placeholder text |
| `textProperty`| `text-property`| `string`| `"text"` | Property to use for item display |
| `valueProperty`| `value-property`| `string`| `"value"`| Property to use for item value |
| `disabled` | `disabled` | `boolean` | `false` | Disable interactions |
| `searchPlaceholder`| `search-placeholder`| `string`| `"Search..."`| Placeholder in search input |
| `selectedPlaceholder`| `selected-placeholder`| `string`| `"Items selected"`| Text used when >3 items picked |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation and returns `true` if valid |
| `showDialog()` | Programmatically opens the selection dialog |
| `bounce()` | Triggers a visual "bounce" animation (useful for errors) |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `value` | `{ value, old }` | Fired when selection changes |
| `dismiss` | `{}` | Fired when dialog is closed without selection |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--ct-input-height`| `3.3em` | Component height |
| `--ct-input-padding`| `0em 1em`| Internal padding |
| `--border-radius` | `16px` | Container border radius |
| `--color-error` | `#ed4f32`| Required indicator and error color |
| `--ct-indicator` | `"*"` | Character for required marker |

## Accessibility (a11y)

The component opens a high-accessibility dialog (`ct-dialog`) with keyboard support and focus management.


