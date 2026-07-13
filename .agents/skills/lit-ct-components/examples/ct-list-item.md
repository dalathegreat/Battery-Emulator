---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-list-item.js'
import '@conectate/components/ct-icon.js'
</script>

# ct-list-item

A versatile list item component that can display icons, text, and additional content, with support for navigation links.

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
import "@conectate/components/ct-list-item.js";
```

### Example with Vue

<div style="margin: 1rem 0; border: 1px solid #ddd; border-radius: 8px; overflow: hidden; width: 300px;">
  <ct-list-item icon="settings" text="Settings"></ct-list-item>
  <ct-list-item icon="person" text="Profile"></ct-list-item>
  <ct-list-item icon="help" text="Help & Support" hideoutline></ct-list-item>
</div>

```vue
<template>
	<div class="list">
		<ct-list-item icon="settings" text="Settings" @click="openSettings"></ct-list-item>
		<ct-list-item icon="home" text="Home" href="/home"></ct-list-item>
		<ct-list-item icon="logout" text="Logout" hideoutline></ct-list-item>
	</div>
</template>

<script setup>
import "@conectate/components/ct-list-item.js";
import "@conectate/components/ct-icon.js";

const openSettings = () => console.log('Settings clicked');
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-list-item.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-list-item 
				icon="person" 
				text="User Profile" 
				href="/profile"
			></ct-list-item>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-list-item.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-list-item': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					text?: string;
					icon?: string;
					svg?: string;
					href?: string;
					target?: '_self' | '_top' | '_blank';
					'keep-open'?: boolean;
					hideoutline?: boolean;
					role?: 'button' | 'menuitem';
				},
				HTMLElement
			>;
		}
	}
}

export function SidebarItem() {
	return (
		<ct-list-item 
			icon="dashboard" 
			text="Dashboard" 
			onClick={() => console.log('Navigating...')} 
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `text` | `text` | `string` | `""` | Primary text to display |
| `icon` | `icon` | `string` | - | Material icon name |
| `svg` | `svg` | `string` | `""` | Custom SVG content for icon |
| `href` | `href` | `string` | - | Navigation URL |
| `target` | `target` | `string` | - | Link target (_blank, etc) |
| `keepOpen`| `keep-open`| `boolean`| `false` | Don't close parent menu on click |
| `hideoutline`| `hideoutline`| `boolean`| `false` | Hides the bottom separator line |
| `role` | `role` | `string` | `"menuitem"`| ARIA role (button or menuitem) |

## Slots

- **Default slot**: Content placed next to the primary text.
- **prefix**: Content placed before the icon/text.
- **suffix**: Content placed at the far end of the item.

## CSS Variables

```css
ct-list-item {
	/* Size */
	--ct-icon-size: 21px; /* Size of the icon */
	
	/* Colors */
	--color-outline: transparent; /* Bottom separator color */
	--color-primary: inherit; /* Focus outline color */
	
	/* Layout */
	--ct-list-item--white-space: nowrap; /* Text wrapping behavior */
	--ct-list-item--padding: 8px 16px 8px 0; /* Padding around the text */
}
```

## Accessibility (a11y)

The component uses appropriate ARIA roles (`menuitem` by default or `button`) and provides `aria-label` automatically based on the `text` property.


