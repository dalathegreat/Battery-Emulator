---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-menu.js'
import '@conectate/components/ct-button.js'
import '@conectate/components/ct-icon.js'

const isOpened = ref(false)
</script>

# ct-menu

A flexible dropdown menu component that displays a list of selectable items triggered by a clickable element.

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
import "@conectate/components/ct-menu.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-menu align="bottom-left">
    <ct-button slot="trigger" raised>Open Menu</ct-button>
    <button @click="console.log('Action 1')">Action 1</button>
    <button @click="console.log('Action 2')">Action 2</button>
    <hr>
    <button @click="console.log('Delete')">Delete</button>
  </ct-menu>
</div>

```vue
<template>
	<ct-menu align="bottom-left">
		<ct-button slot="trigger" raised>Options</ct-button>
		
		<button @click="handleEdit">Edit</button>
		<button @click="handleShare">Share</button>
		<hr>
		<button class="danger" @click="handleDelete">Delete</button>
	</ct-menu>
</template>

<script setup>
import "@conectate/components/ct-menu.js";
import "@conectate/components/ct-button.js";

const handleEdit = () => console.log('Edit clicked');
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-menu.js";
import "@conectate/components/ct-icon-button.js";

@customElement("my-navbar")
class MyNavbar extends LitElement {
	render() {
		return html`
			<ct-menu align="bottom-right">
				<ct-icon-button slot="trigger" icon="more_vert"></ct-icon-button>
				
				<button>Profile</button>
				<button>Settings</button>
				<button>Logout</button>
			</ct-menu>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-menu.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-menu': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					align?: 'top' | 'top-right' | 'top-left' | 'bottom' | 'bottom-right' | 'bottom-left';
					opened?: boolean;
					icon?: string;
				},
				HTMLElement
			>;
		}
	}
}

export function ToolbarMenu() {
	return (
		<ct-menu align="bottom-right">
			<button slot="trigger">⋮</button>
			<button onClick={() => alert('Option A')}>Option A</button>
			<button onClick={() => alert('Option B')}>Option B</button>
		</ct-menu>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `align` | `align` | `string` | `"top-right"` | Menu alignment relative to trigger |
| `opened` | `opened` | `boolean` | `false` | Current state of the menu |
| `icon` | `icon` | `string` | - | Icon name (internal use) |

## Slots

- **Default slot**: Menu items (usually `button`, `ct-list-item`, or `hr` elements).
- **trigger**: The element that triggers the menu when clicked.
- **dropdown-trigger**: (Deprecated) Alternative name for the trigger slot.

## CSS Variables

```css
ct-menu {
	/* Backgrounds */
	--color-surface: #fff; /* Menu background color */
	--color-blur-surface: #ffffffbd; /* Background when blur is supported */
	
	/* Colors */
	--color-on-surface: #474747; /* Item text color */
	--color-outline: #dadce0; /* Separator color */
	
	/* Layout */
	--border-radius: 8px; /* Menu border radius */
}
```

## Accessibility (a11y)

- **Keyboard Navigation**: Use the `Escape` key to close the menu.
- **Focus Management**: The menu container is focusable, and it automatically closes on blur unless configured otherwise.

## Event Handling

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `open` | `boolean` | Fired when the menu opens or closes |


