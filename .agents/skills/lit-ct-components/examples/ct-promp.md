---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-promp.js'
import { showCtPrompt } from '@conectate/components/ct-promp.js'

const result = ref('')

const handlePrompt = async () => {
  const value = await showCtPrompt(
    "User Feedback",
    "What do you think about Conectate Elements?",
    "Submit",
    "Cancel",
    undefined,
    { label: "Your thoughts", placeholder: "Type here..." }
  )
  result.value = value !== undefined ? `You said: ${value}` : "Prompt canceled"
}

const handleTextAreaPrompt = async () => {
  const value = await showCtPrompt(
    "Description",
    "Please provide a detailed description:",
    "Save",
    "Cancel",
    undefined,
    { label: "Details", textarea: true }
  )
  result.value = value !== undefined ? `Description saved.` : "Canceled"
}
</script>

# ct-promp

A prompt dialog component and utility for requesting single-field input from the user.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

The component is typically used via the `showCtPrompt` helper function.

### Using Helper Function

```ts
import { showCtPrompt } from "@conectate/components/ct-promp.js";

const result = await showCtPrompt(
	"Input Required",
	"Please enter your name:",
	"OK",
	"Cancel",
	undefined,
	{ label: "Name", value: "Default Name" }
);

if (result !== undefined) {
	console.log("User entered:", result);
}
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; gap: 1rem;">
  <button @click="handlePrompt" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Open Prompt
  </button>
  <button @click="handleTextAreaPrompt" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Open Textarea Prompt
  </button>
</div>
<p v-if="result" style="font-weight: bold; color: #555;">{{ result }}</p>

```vue
<template>
	<button @click="openPrompt">Ask Something</button>
</template>

<script setup>
import { showCtPrompt } from "@conectate/components/ct-promp.js";

const openPrompt = async () => {
	const result = await showCtPrompt(
		"Confirmation",
		"Please enter the access code:",
		"Verify",
		"Cancel",
		undefined,
		{ label: "Code", placeholder: "0000" }
	);
	if (result) {
		console.log("Verifying code:", result);
	}
};
</script>
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import { showCtPrompt } from "@conectate/components/ct-promp.js";

export function PromptButton() {
	const handleClick = async () => {
		const value = await showCtPrompt("Settings", "Change your username:", "Update", "Cancel");
		if (value) {
			// Update username
		}
	};

	return <button onClick={handleClick}>Edit Username</button>;
}
```

## Helper Function API

### `showCtPrompt(title, body, ok?, cancel?, neutral?, options?)`

| Parameter | Type | Default | Description |
| --------- | ---- | ------- | ----------- |
| `title` | `string` | - | Dialog title |
| `body` | `string \| TemplateResult` | - | Content to display above the input |
| `ok` | `string` | `"OK"` | Positive button text |
| `cancel` | `string` | `"Cancel"` | Negative button text (hidden if null) |
| `neutral` | `string` | - | Neutral button text (hidden if not provided) |
| `options` | `object` | - | Configuration for the input field |

#### Options Object

| Property | Type | Default | Description |
| -------- | ---- | ------- | ----------- |
| `value` | `string` | - | Initial value for the input |
| `label` | `string` | - | Label for the input field |
| `placeholder`| `string`| - | Placeholder for the input field |
| `rawplaceholder`| `string`| - | Raw placeholder for the input field |
| `textarea` | `boolean` | `false` | If `true`, uses `ct-textarea` instead of `ct-input` |
| `wordwrap` | `boolean` | `false` | Enables text wrapping in the body |

**Returns**: `Promise<string | undefined>`
- `string`: The value entered by the user
- `undefined`: If the prompt was canceled or dismissed

## CSS Variables

The prompt dialog uses `ct-card` and `ct-button` styles internally.

```css
ct-promp {
	/* Backgrounds and text inherited from theme */
	--color-primary: #00aeff;
	--color-on-surface: #fff;
}
```

## Accessibility (a11y)

The dialog uses `ct-dialog` with appropriate ARIA attributes. Focus is automatically moved to the input field when the prompt opens.


