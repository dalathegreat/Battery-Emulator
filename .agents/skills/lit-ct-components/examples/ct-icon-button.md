---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-icon-button.js'
import '@conectate/components/ct-icon.js'

const count = ref(0)
</script>

# ct-icon-button

A circular button component that displays a Material Icon or custom SVG with hover and active states.

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
import "@conectate/components/ct-icon-button.js";
```

### Example with Vue

<div style="display: flex; gap: 1rem; align-items: center; margin: 1rem 0;">
  <ct-icon-button icon="add" @click="count++"></ct-icon-button>
  <span>Count: {{ count }}</span>
  <ct-icon-button icon="remove" @click="count--"></ct-icon-button>
  <ct-icon-button icon="delete" disabled></ct-icon-button>
</div>

```vue
<template>
	<ct-icon-button icon="search" @click="handleSearch"></ct-icon-button>
	<ct-icon-button icon="delete" disabled></ct-icon-button>
</template>

<script setup>
import "@conectate/components/ct-icon-button.js";

const handleSearch = () => console.log('Search clicked');
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-icon-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-icon-button icon="settings" @click=${this.onSettings}></ct-icon-button>
		`;
	}

	onSettings() {
		console.log("Settings clicked");
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-icon-button.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-icon-button': React.DetailedHTMLProps<
				React.ButtonHTMLAttributes<HTMLButtonElement> & {
					icon?: string;
					svg?: string;
					disabled?: boolean;
					'aria-label'?: string;
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	return (
		<ct-icon-button 
			icon="favorite" 
			aria-label="Add to favorites" 
			onClick={() => console.log('Favorited!')} 
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `icon` | `icon` | `string` | - | Name of the Material Icon |
| `svg` | `svg` | `string` | - | Custom SVG content |
| `disabled` | `disabled` | `boolean` | `false` | Whether the button is disabled |
| `ariaLabel`| `aria-label`| `string`| `""`| Accessible label |

## Slots

- **Default slot**: Additional content (rarely used)
- **icon slot**: Custom icon content (use when providing SVG as a slot)

## CSS Variables

```css
ct-icon-button {
	/* Size */
	--ct-icon-size: 24px; /* Size of the icon */
	
	/* Layout & Spacing */
	/* Padding is automatically calculated based on icon size: calc((var(--ct-icon-size, 24px) * 2 - var(--ct-icon-size, 24px)) / 2) */
}
```

## Accessibility (a11y)

Always provide an `aria-label` for icon buttons since they often don't have visible text. If not provided, the component falls back to using the icon name as the label.

```html
<ct-icon-button icon="menu" aria-label="Open menu"></ct-icon-button>
```


