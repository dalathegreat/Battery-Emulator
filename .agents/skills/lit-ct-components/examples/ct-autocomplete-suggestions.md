---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-autocomplete-suggestions.js'
import { html } from 'lit'

const text = ref('')
const source = ref([
  { text: 'Apple', value: 'apple' },
  { text: 'Banana', value: 'banana' },
  { text: 'Cherry', value: 'cherry' },
  { text: 'Date', value: 'date' }
])
</script>

# ct-autocomplete-suggestions

A component that displays a list of suggestions based on a query, typically used with input fields to provide autocomplete functionality.

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
import "@conectate/components/ct-autocomplete-suggestions.js";
```

### Example with Vue

<div style="margin: 1rem 0; position: relative;">
  <input type="text" v-model="text" placeholder="Type 'a' to see suggestions..." style="width: 100%; padding: 8px; border: 1px solid #ccc; border-radius: 4px;">
  <ct-autocomplete-suggestions :text="text" :source="source"></ct-autocomplete-suggestions>
</div>

```vue
<template>
	<div style="position: relative;">
		<input type="text" v-model="text" placeholder="Type something...">
		<ct-autocomplete-suggestions :text="text" :source="source"></ct-autocomplete-suggestions>
	</div>
</template>

<script setup>
import { ref } from 'vue';
import "@conectate/components/ct-autocomplete-suggestions.js";

const text = ref('');
const source = ref([
	{ text: 'Apple', value: 'apple' },
	{ text: 'Banana', value: 'banana' },
	{ text: 'Cherry', value: 'cherry' }
]);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, property } from "lit";
import "@conectate/components/ct-autocomplete-suggestions.js";

@customElement("my-search-element")
class MySearchElement extends LitElement {
	@property() query = "";
	@property() items = ["Frontend", "Backend", "Mobile", "DevOps"];

	render() {
		return html`
			<div style="position: relative;">
				<input @input=${(e: any) => this.query = e.target.value} .value=${this.query}>
				<ct-autocomplete-suggestions 
					.text=${this.query} 
					.source=${this.items}>
				</ct-autocomplete-suggestions>
			</div>
		`;
	}
}
```

## Properties

| Property     | Attribute | Type      | Default                                         | Description                                              |
| ------------ | --------- | --------- | ----------------------------------------------- | -------------------------------------------------------- |
| `text`       | `text`    | `string`  | `""`                                            | Current query text to filter suggestions                 |
| `source`     | -         | `array`   | `[]`                                            | List of items to filter. Can be strings or objects       |
| `remote`     | `remote`  | `boolean` | `false`                                         | If `true`, filtering is expected to be handled externally |
| `renderItem` | -         | `object`  | `(item, index) => html<button>item ${index}</button>` | Custom template for rendering each suggestion item        |

## CSS Variables

```css
ct-autocomplete-suggestions {
	--color-surface: #fff; /* Suggestions background color */
	--color-on-surface: #333; /* Suggestions text color */
	--color-blur-surface: rgba(255, 255, 255, 0.5); /* Background color with backdrop-filter */
}
```

### CSS Variables Reference Table

| Variable               | Default                       | Description                                |
| ---------------------- | ----------------------------- | ------------------------------------------ |
| `--color-surface`      | `#fff`                        | Background color of the suggestions card   |
| `--color-on-surface`   | `#333`                        | Text color of the suggestions              |
| `--color-blur-surface` | `rgba(255, 255, 255, 0.5)`     | Background color when blur is supported    |

## Methods

| Method           | Description                                                        |
| ---------------- | ------------------------------------------------------------------ |
| `queryFn(source, query)` | Manually trigger the filtering logic and return matches          |
| `hiddeSugg()`    | Clears the source array, effectively hiding the suggestions popup |
| `removeAcento(text)` | Utility to remove accents from a string for easier comparison    |

## Custom Rendering

You can customize how each suggestion is rendered by providing a custom `renderItem` function that returns a Lit `html` template.

```ts
const suggestions = document.querySelector('ct-autocomplete-suggestions');
suggestions.renderItem = (item, index) => html`
	<div class="custom-item">
		<strong>${item.text}</strong>
		<small>Value: ${item.value}</small>
	</div>
`;
```

## Accessibility (a11y)

The suggestions are displayed in a container with a high `z-index`. When integrating with an input, ensure you handle arrow key navigation and `Enter` to select an item for a complete accessible experience.


