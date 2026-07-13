---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-card-dialog.js'
import { showCtCardDialog } from '@conectate/components/ct-card-dialog.js'

const showExample = () => {
  const content = document.createElement('div')
  content.style.padding = '24px'
  content.innerHTML = `
    <h2 style="margin-top:0">Card Dialog Title</h2>
    <p>This is a dialog that looks like a card. It can contain any HTML element.</p>
    <button id="close-btn" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">Close Dialog</button>
  `
  const dialog = showCtCardDialog(content)
  content.querySelector('#close-btn').onclick = () => dialog.close()
}
</script>

# ct-card-dialog

A card-styled dialog component and utility for displaying content in a floating card format.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

### Using the helper function (Recommended)

The easiest way to show a card dialog is using the `showCtCardDialog` helper function.

```ts
import { showCtCardDialog } from "@conectate/components/ct-card-dialog.js";

const content = document.createElement('div');
content.innerHTML = `
  <div style="padding: 24px;">
    <h2>Hello World</h2>
    <p>This is content inside a card dialog.</p>
  </div>
`;

const dialog = showCtCardDialog(content);
// Close it later
// dialog.close();
```

### Example with Vue

<div style="margin: 1rem 0;">
  <button @click="showExample" style="padding: 8px 16px; cursor: pointer; background: #00aeff; color: white; border: none; border-radius: 4px;">
    Show Card Dialog
  </button>
</div>

```vue
<template>
	<button @click="openDialog">Open Card Dialog</button>
</template>

<script setup>
import { showCtCardDialog } from "@conectate/components/ct-card-dialog.js";

const openDialog = () => {
	const content = document.createElement('div');
	content.innerHTML = '<div style="padding: 24px;"><h3>Card Content</h3><p>Content goes here...</p></div>';
	showCtCardDialog(content);
};
</script>
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import { showCtCardDialog } from "@conectate/components/ct-card-dialog.js";

export function MyComponent() {
	const openDialog = () => {
		const div = document.createElement('div');
		div.style.padding = '20px';
		div.innerHTML = '<h2>Title</h2><p>Description</p>';
		showCtCardDialog(div);
	};

	return (
		<button onClick={openDialog}>Open Dialog</button>
	);
}
```

## Properties

The `ct-card-dialog` component is usually managed by the helper function, but it has the following property:

| Property | Attribute | Type     | Default | Description                              |
| -------- | --------- | -------- | ------- | ---------------------------------------- |
| `el`     | -         | `object` | -       | The HTML element to display in the card |

## Helper Function API

### `showCtCardDialog(el, id?, history?)`

| Parameter | Type               | Description                                           |
| --------- | ------------------ | ----------------------------------------------------- |
| `el`      | `HTMLElement`      | The element to show inside the dialog                 |
| `id`      | `string` (opt)     | Unique identifier for the dialog                      |
| `history` | `ConectateHistory` | History object for browser history integration (back) |

**Returns**: `CtDialog` instance.

## CSS Variables

You can customize the card dialog using these variables:

```css
ct-card-dialog {
	/* Colors */
	--color-background: #fff; /* Background color */
	--color-on-surface: #000; /* Text color */
	--color-primary: #00aeff; /* Scrollbar thumb color */

	/* Layout */
	--border-radius: 16px; /* Card border radius */
}
```

## Accessibility (a11y)

The component uses internal dialog mechanisms to handle focus and layering. Ensure the content passed to the dialog is accessible.


