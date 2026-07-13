---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-loading.js'
import { showCtLoading } from '@conectate/components/ct-loading.js'

const showExample = () => {
  const dialog = showCtLoading(undefined, "Processing Data")
  setTimeout(() => {
    dialog.close()
  }, 3000)
}
</script>

# ct-loading

A modal dialog component that displays a loading spinner with customizable text, typically used for long-running operations.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

The most common way to use this component is via the `showCtLoading` helper function.

### Using the Helper Function

```ts
import { showCtLoading } from "@conectate/components/ct-loading.js";

// Show dialog
const dialog = showCtLoading();

// Do something...
// await myAsyncOperation();

// Close dialog
dialog.close();
```

### Example with Vue

<div style="margin: 1rem 0;">
  <button @click="showExample" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Show Loading Dialog (3s)
  </button>
</div>

```vue
<template>
	<button @click="startLoading">Process Order</button>
</template>

<script setup>
import { showCtLoading } from "@conectate/components/ct-loading.js";

const startLoading = async () => {
	const dialog = showCtLoading(undefined, "Processing...");
	try {
		await performTask();
	} finally {
		dialog.close();
	}
};
</script>
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import { showCtLoading } from "@conectate/components/ct-loading.js";

export function ActionButton() {
	const handleClick = async () => {
		const loading = showCtLoading(undefined, "Saving...");
		try {
			await saveToServer();
		} finally {
			loading.close();
		}
	};

	return <button onClick={handleClick}>Save Changes</button>;
}
```

## Properties

The `ct-loading` component has the following property (usually set via the helper function):

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `ttl` | `ttl` | `string` | `"Loading"` | Text to display next to the spinner |

## Helper Function API

### `showCtLoading(id?, text?)`

| Parameter | Type | Description |
| --------- | ---- | ----------- |
| `id` | `string` (opt)| Unique identifier for the dialog |
| `text` | `string` (opt)| Custom loading text (default: "Loading") |

**Returns**: `CtDialog` instance.

## Accessibility (a11y)

The component is rendered within a `ct-dialog` with `role="alert"`, ensuring screen readers announce the loading state.


