---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-confirm.js'
import { showCtConfirm, showCtConfirmCupertino } from '@conectate/components/ct-confirm.js'

const result = ref('')

const handleConfirm = async () => {
  const confirmed = await showCtConfirm(
    "Confirm Action",
    "Are you sure you want to proceed with this action?",
    "Confirm",
    "Cancel"
  )
  result.value = confirmed === true ? "Confirmed!" : confirmed === false ? "Cancelled" : "Dismissed"
}

const handleCupertino = async () => {
  const confirmed = await showCtConfirmCupertino(
    "iOS Style",
    "This is a Cupertino-style confirmation dialog.",
    "OK",
    "Cancel"
  )
  result.value = confirmed === true ? "Cupertino OK" : "Cupertino Cancelled"
}
</script>

# ct-confirm

A confirmation dialog component and utility for displaying modal confirmations with various styles.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

The most common way to use this component is through the helper functions `showCtConfirm` and `showCtConfirmCupertino`.

### Using Helper Functions

```ts
import { showCtConfirm, showCtConfirmCupertino } from "@conectate/components/ct-confirm.js";

// Standard Material Design style
const confirmed = await showCtConfirm("Title", "Are you sure?", "Yes", "No");

// iOS style
const confirmedIOS = await showCtConfirmCupertino("Title", "Are you sure?", "Yes", "No");
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; gap: 1rem;">
  <button @click="handleConfirm" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Standard Confirm
  </button>
  <button @click="handleCupertino" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Cupertino Confirm
  </button>
</div>
<p v-if="result" style="font-weight: bold; color: #555;">Result: {{ result }}</p>

```vue
<template>
	<button @click="openConfirm">Open Confirm</button>
</template>

<script setup>
import { showCtConfirm } from "@conectate/components/ct-confirm.js";

const openConfirm = async () => {
	const result = await showCtConfirm(
		"Confirm Action",
		"Do you want to save changes?",
		"Save",
		"Cancel"
	);
	if (result) {
		console.log("Saved!");
	}
};
</script>
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import { showCtConfirm } from "@conectate/components/ct-confirm.js";

export function ConfirmButton() {
	const handleClick = async () => {
		const ok = await showCtConfirm("Delete", "Are you sure?", "Delete", "Cancel");
		if (ok) {
			// Perform delete
		}
	};

	return <button onClick={handleClick}>Delete Item</button>;
}
```

## Variants

- **Standard (`showCtConfirm`)**: Follows Material Design patterns.
- **Cupertino (`showCtConfirmCupertino`)**: Follows iOS design patterns with blur effects.

## Helper Functions API

### `showCtConfirm(title, body, ok?, cancel?, neutral?, options?)`
### `showCtConfirmCupertino(title, body, ok?, cancel?, neutral?)`

| Parameter | Type | Default | Description |
| --------- | ---- | ------- | ----------- |
| `title` | `string` | - | Dialog title |
| `body` | `string` | - | Content (supports HTML) |
| `ok` | `string` | `"Ok"` | Positive button text |
| `cancel` | `string` | - | Negative button text (hidden if not provided) |
| `neutral` | `string` | - | Neutral button text (hidden if not provided) |
| `options` | `object` | `{history: true}` | Additional options (only for standard confirm) |

**Returns**: `Promise<boolean | null | undefined>`
- `true`: OK clicked
- `false`: Cancel clicked
- `null`: Neutral clicked
- `undefined`: Dialog dismissed

## CSS Variables

The components used by these functions can be customized:

```css
ct-confirm {
	/* Colors */
	--color-primary: #00aeff; /* Primary button color */
	--color-on-primary: #fff; /* Text on primary button */
	--medium-emphasis: #666; /* Body text color */
}

ct-confirm-cupertino {
	--color-background: rgba(255, 255, 255, 0.85); /* Dialog background */
	--color-blur: rgba(255, 255, 255, 0.72); /* Background when blur is supported */
	--color-outline: rgba(0, 0, 0, 0.1); /* Separator color */
}
```

## Accessibility (a11y)

The dialogs are set as `role="alert"` or `role="dialog"` with proper focus management. Keyboard users can dismiss the dialog using the `Escape` key.


