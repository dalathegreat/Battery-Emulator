---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-select-dialog.js'
import { showCtSelect, CtSelectBuilder } from '@conectate/components/ct-select-dialog.js'

const singleResult = ref('')
const multiResult = ref([])

const items = [
  { text: 'Option One', value: 1 },
  { text: 'Option Two', value: 2 },
  { text: 'Option Three', value: 3 },
  { text: 'Option Four', value: 4 }
]

const handleSingleSelect = async () => {
  const { result } = showCtSelect(
    "Select Option",
    items,
    singleResult.value,
    "Apply",
    "Cancel",
    { searchable: true, multi: false, searchPlaceholder: "Find option...", textProperty: "text", valueProperty: "value" }
  )
  const val = await result
  if (val !== undefined) singleResult.value = val
}

const handleMultiSelect = async () => {
  const select = CtSelectBuilder.Builder()
    .title("Select Multiple")
    .items(items)
    .value(multiResult.value)
    .multi()
    .searchable()
    .build()
  
  const { result } = CtSelectBuilder.show(select)
  const val = await result
  if (val !== undefined) multiResult.value = val
}
</script>

# ct-select-dialog

A powerful selection dialog component and utility that supports searching, single/multi-selection, and custom item rendering.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

You can use the functional API or the fluent Builder API.

### Using `showCtSelect`

```ts
import { showCtSelect } from "@conectate/components/ct-select-dialog.js";

const items = [{ text: 'Item 1', id: 1 }, { text: 'Item 2', id: 2 }];

const { result } = showCtSelect(
	"Choose Item",
	items,
	undefined, // Initial value
	"OK",
	"Cancel",
	{ 
		searchable: true, 
		multi: false, 
		searchPlaceholder: "Search...",
		textProperty: "text",
		valueProperty: "id"
	}
);

const selectedValue = await result;
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; gap: 1rem;">
  <button @click="handleSingleSelect" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Single Select
  </button>
  <button @click="handleMultiSelect" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Multi Select
  </button>
</div>
<div style="color: #666;">
  Single: {{ singleResult || 'None' }}<br>
  Multi: {{ multiResult.join(', ') || 'None' }}
</div>

```vue
<template>
	<button @click="openPicker">Pick Item</button>
</template>

<script setup>
import { showCtSelect } from "@conectate/components/ct-select-dialog.js";

const openPicker = async () => {
	const items = [/* ... */];
	const { result } = showCtSelect("Title", items, undefined, "OK", "Cancel", {
		searchable: true,
		multi: false,
		textProperty: "name",
		valueProperty: "id"
	});
	const val = await result;
};
</script>
```

## CtSelectBuilder API

The Builder provides a fluent interface for more complex configurations.

```ts
import { CtSelectBuilder } from "@conectate/components/ct-select-dialog.js";

const builder = CtSelectBuilder.Builder()
	.title("Select User")
	.items(users)
	.valueProperty("uid")
	.textProperty("displayName")
	.multi()
	.searchable({ placeholder: "Find by name..." })
	.renderItem((user, index, array, selected) => html`
		<div style="padding: 12px;">
			<strong>${user.displayName}</strong><br>
			<small>${user.email}</small>
		</div>
	`);

const dialog = builder.build();
const { result } = CtSelectBuilder.show(dialog);
```

## Properties (`CtSelectDialog`)

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `ttl` | `ttl` | `string` | - | Dialog title |
| `items` | - | `any[]` | `[]` | List of items to display |
| `value` | - | `any` | - | Current selected value (single) |
| `multi` | `multi` | `boolean` | `false` | Enable multiple selection |
| `multiValue` | - | `any[]` | `[]` | Current selected values (multi) |
| `searchable` | `searchable`| `boolean`| `false` | Show search input |
| `textProperty`| - | `string` | `"text"` | Property name for display |
| `valueProperty`| - | `string` | `"value"`| Property name for value |
| `renderItem` | - | `function` | - | Custom item renderer |

## CSS Variables

```css
ct-select-dialog {
	/* Inherits from ct-card and ct-button */
	--color-primary: #00aeff;
	--color-surface: #fff;
	--color-on-surface: #535353;
}
```

## Accessibility (a11y)

The component handles:
- **Search Focus**: Automatically focuses the search input when the dialog opens.
- **Visual Feedback**: Clearly indicates selected states for items.
- **Keyboard**: Uses standard `ct-dialog` behavior for closing via `Escape`.


