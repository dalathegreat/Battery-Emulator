---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-button-menu.js'
import '@conectate/components/ct-list-item.js'
import '@conectate/components/ct-icon.js'

const isMenuOpen = ref(false)
</script>

# ct-button-menu

A button that opens a dropdown menu when clicked, supporting keyboard navigation and custom triggers.

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
import "@conectate/components/ct-button-menu.js";
import "@conectate/components/ct-list-item.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-button-menu icon="more_vert">
    <ct-list-item>Option 1</ct-list-item>
    <ct-list-item>Option 2</ct-list-item>
    <hr>
    <ct-list-item>Option 3</ct-list-item>
  </ct-button-menu>
</div>

```vue
<template>
	<ct-button-menu icon="more_vert">
		<ct-list-item>Option 1</ct-list-item>
		<ct-list-item>Option 2</ct-list-item>
		<hr>
		<ct-list-item>Option 3</ct-list-item>
	</ct-button-menu>
</template>

<script setup>
import "@conectate/components/ct-button-menu.js";
import "@conectate/components/ct-list-item.js";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-button-menu.js";
import "@conectate/components/ct-list-item.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-button-menu icon="expand_more">
				<ct-list-item @click=${() => console.log('Edit')}>Edit</ct-list-item>
				<ct-list-item @click=${() => console.log('Delete')}>Delete</ct-list-item>
			</ct-button-menu>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-button-menu.js";
import "@conectate/components/ct-list-item.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-button-menu': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					icon?: string;
					rotate?: boolean;
					open?: boolean;
					from?: string;
					keep?: boolean;
					title?: string;
				},
				HTMLElement
			>;
			'ct-list-item': React.DetailedHTMLProps<React.HTMLAttributes<HTMLElement>, HTMLElement>;
		}
	}
}

export function MyMenu() {
	return (
		<ct-button-menu icon="settings" rotate>
			<ct-list-item onClick={() => alert('Profile')}>Profile</ct-list-item>
			<ct-list-item onClick={() => alert('Logout')}>Logout</ct-list-item>
		</ct-button-menu>
	);
}
```

## Variants

### Menu Direction

Use the `from` property to control where the menu appears relative to the trigger.

- `bottom-left` (Default)
- `bottom-right`
- `bottom`
- `top-left`
- `top-right`
- `top`

### Interactive Example

<div style="margin: 1rem 0; display: flex; gap: 2rem;">
  <ct-button-menu from="bottom-right" icon="arrow_drop_down">
    <ct-list-item>Appears Bottom Right</ct-list-item>
    <ct-list-item>Option B</ct-list-item>
  </ct-button-menu>
  
  <ct-button-menu rotate icon="settings" title="Rotate on focus">
    <ct-list-item>Icon rotates</ct-list-item>
    <ct-list-item>on focus</ct-list-item>
  </ct-button-menu>
</div>

## Properties

| Property         | Attribute   | Type      | Default                 | Description                                    |
| ---------------- | ----------- | --------- | ----------------------- | ---------------------------------------------- |
| `icon`           | `icon`      | `string`  | `"expand_more"`         | Icon name to show in the button                |
| `rotate`         | `rotate`    | `boolean` | `false`                 | If `true`, rotates the icon 180deg when opened |
| `from`           | `from`      | `string`  | `"bottom-left"`         | Location from where the popup is opened        |
| `open`           | `open`      | `boolean` | `false`                 | Current state of the menu                      |
| `keep`           | `keep`      | `boolean` | `false`                 | Keep popup open after click                    |
| `title`          | `title`     | `string`  | `"Open for more actions"` | Tooltip title for the button                   |
| `anim_selector` | `anim-selector` | `string` | `"ct-list-item"` | Selector for items that support roving index |

## Slots

- **Default slot**: Menu content (usually `ct-list-item` or `hr`).
- **dropdown**: Custom dropdown trigger (requires `use_slot`).
- **trigger**: Custom trigger element (requires `use_slot`).

## CSS Variables

```css
ct-button-menu {
	/* Animation speeds */
	--in-speed: 250ms;
	--out-speed: 250ms;

	/* Popup Styling */
	--color-menu: var(--color-surface, #fff); /* Menu background */
	--color-on-surface: #535353; /* Text color */
	--ct-button-menu-box-shadow: #0a0e1d05 0px 8px 16px 0px, #0a0e1d0f 0px 8px 40px 0px;
	--ct-button-menu-radius: 26px; /* Trigger border radius */
	--ct-button-menu-popup-radius: 16px; /* Popup border radius */
	--menu-outline: none; /* Focus outline for the menu */
}
```

### CSS Variables Reference Table

| Variable                        | Default                                                                 | Description                               |
| ------------------------------- | ----------------------------------------------------------------------- | ----------------------------------------- |
| `--in-speed`                    | `250ms`                                                                 | Animation duration when opening           |
| `--out-speed`                   | `250ms`                                                                 | Animation duration when closing           |
| `--color-menu`                  | `var(--color-surface, #fff)`                                            | Background color of the dropdown popup    |
| `--ct-button-menu-box-shadow`   | `#0a0e1d05 0px 8px 16px 0px...`                                         | Box shadow applied to the dropdown        |
| `--ct-button-menu-radius`       | `26px`                                                                  | Border radius of the trigger area         |
| `--ct-button-menu-popup-radius` | `16px`                                                                  | Border radius of the dropdown popup       |

## Accessibility (a11y)

The component uses `aria-expanded` to indicate its state to screen readers. It also supports keyboard navigation:
- `Enter` or `Click` to open.
- `Escape` to close.
- Arrow keys to navigate between items (via `rovingIndex` helper).

## Event Handling

#### Vue Example

```vue
<template>
	<ct-button-menu @focusin="onOpen" @focusout="onClose">
		<ct-list-item>Item</ct-list-item>
	</ct-button-menu>
</template>

<script setup>
const onOpen = () => console.log('Menu focused/opening');
const onClose = () => console.log('Menu blurred/closing');
</script>
```


