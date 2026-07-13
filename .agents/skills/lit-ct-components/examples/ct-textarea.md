---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-textarea.js'

const text = ref('')
</script>

# ct-textarea

A beautiful, Material-inspired multi-line text input component that automatically grows as you type.

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
import "@conectate/components/ct-textarea.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-textarea 
    label="Description" 
    placeholder="Write something helpful..." 
    v-model="text"
    char-counter
    :maxlength="500"
    rows="3"
  ></ct-textarea>
</div>

```vue
<template>
	<ct-textarea 
		label="Comments" 
		v-model="comments"
		char-counter
		maxlength="1000"
		rows="4"
	></ct-textarea>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-textarea.js";

const comments = ref('')
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-textarea.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private val = "";

	render() {
		return html`
			<ct-textarea 
				label="Detailed Bio" 
				.value=${this.val}
				@value=${(e: any) => this.val = e.detail.value}
				required
				rows="5"
			></ct-textarea>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-textarea.js";

export function FeedbackForm() {
	const [message, setMessage] = useState('');

	return (
		<ct-textarea 
			label="Message" 
			value={message} 
			onValue={(e: any) => setMessage(e.detail.value)}
			placeholder="Enter your message here"
			rows={4}
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `value` | `value` | `string` | `""` | Current text content |
| `placeholder`| `placeholder`| `string`| `""` | Floating placeholder text |
| `rows` | `rows` | `number` | `1` | Initial number of visible rows |
| `required` | `required` | `boolean` | `false` | If true, field is required |
| `invalid` | `invalid` | `boolean` | `false` | Current validation state |
| `errorMessage`| `error-message`| `string`| `""` | Error message to display when invalid |
| `charCounter`| `char-counter`| `boolean`| `false` | Show character count at bottom right |
| `maxlength` | `maxlength`| `number` | `MAX_SAFE_INT` | Maximum character limit |
| `minlength` | `minlength`| `number` | `0` | Minimum character limit |
| `disabled` | `disabled` | `boolean` | `false` | Disables the textarea |
| `readonly` | `readonly` | `boolean` | `false` | Field is read-only |
| `autofocus` | `autofocus`| `boolean`| `false` | Focus on mount |
| `autocapitalize`| `autocapitalize`| `string`| - | Native autocapitalize attribute |
| `inputmode` | `inputmode` | `string` | `""` | Native inputmode attribute |
| `pattern` | `pattern` | `string` | - | Regex pattern for validation |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation and returns `true` if valid |
| `focus()` | Moves focus to the internal textarea |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `value` | `{ value: string }`| Fired when the text content changes |
| `enter-pressed`| `{}` | Fired when Ctrl+Enter or Meta+Enter is pressed |

## Slots

| Name | Description |
| ---- | ----------- |
| `prefix` | Content placed before the main text area |
| `suffix` | Content placed after the main text area |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--color-primary` | `#2cb5e8` | Caret and focus underline color |
| `--color-error` | `#ed4f32` | Color for error states and required marker |
| `--ct-textarea-font-family`| `inherit`| Font family for the textarea |
| `--ct-textarea-font-weight`| `inherit`| Font weight for the textarea |
| `--border-radius` | `16px` | Container border radius |

## Accessibility (a11y)

The component uses an internal `ct-textarea-autogrow` which wraps a native `<textarea>`. It handles ARIA labels, required states, and focus management automatically.


