---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-icon.js'
import '@conectate/components/ct-icon-button.js'
</script>

# ct-icon

A versatile icon component system for displaying Material Icons and custom SVGs in web applications, with support for various styles and customization options.

> IMPORTANT: Check [https://fonts.google.com/icons](https://fonts.google.com/icons) for the list of available icons, fonts and styles.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

If you are using frameworks like Lit, React, or Vue, import the component:

```ts
import "@conectate/components/ct-icon.js";
import "@conectate/components/ct-icon-button.js";
```

### Example with Vue

<div style="display: flex; gap: 1rem; align-items: center; flex-wrap: wrap; margin: 1rem 0;">
	<ct-icon icon="settings"></ct-icon>
	<ct-icon icon="home" style="--ct-icon-size: 36px;"></ct-icon>
	<ct-icon-button icon="search"></ct-icon-button>
	<ct-icon-button icon="close" disabled></ct-icon-button>
</div>

```vue
<template>
	<ct-icon icon="settings"></ct-icon>
	<ct-icon icon="home" style="--ct-icon-size: 36px;"></ct-icon>
	<ct-icon-button icon="search"></ct-icon-button>
	<ct-icon-button icon="close" disabled></ct-icon-button>
</template>

<script setup>
import "@conectate/components/ct-icon";
import "@conectate/components/ct-icon-button";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-icon.js";
import "@conectate/components/ct-icon-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-icon icon="settings"></ct-icon>
			<ct-icon-button icon="search" @click=${this.handleClick}></ct-icon-button>
		`;
	}

	handleClick() {
		console.log("Icon button clicked!");
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-icon.js";
import "@conectate/components/ct-icon-button.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
  namespace JSX {
    interface IntrinsicElements {
			'ct-icon': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					icon?: string;
					svg?: string;
					font?: 'Icons' | 'Symbols';
					fontstyle?: 'Outlined' | 'Fill' | 'Sharp' | 'Two Tone' | 'Round' | 'Rounded';
					ready?: boolean;
				},
				HTMLElement
			>;
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
		<div>
			<ct-icon icon="settings" />
			<ct-icon-button icon="search" aria-label="Search" onClick={() => console.log('Clicked')} />
		</div>
	);
}
```

## Components

This package includes the following components:

- `ct-icon`: Base icon component for displaying Material Icons or custom SVGs
- `ct-icon-button`: Icon wrapped in a circular button with hover and active states

## Icon Styles

The ct-icon component supports different styles from the Material Icons library:

- **Outlined**: Icons with an outlined appearance
- **Fill**: Solid, filled icons
- **Sharp**: Icons with sharp, straight edges
- **Two Tone**: Two-tone icons with varying opacity
- **Round**: Icons with rounded corners
- **Rounded**: Rounded icon style (default)

You can set styles globally or per component:

```javascript
// Global setting
import { CtIcon } from "@conectate/components/ct-icon";
CtIcon.FontStyle = "Outlined";

// Per component
<ct-icon icon="favorite" fontstyle="Fill"></ct-icon>
```

## Properties

### ct-icon

| Property    | Attribute   | Type                                                                    | Default     | Description                                                       |
| ----------- | ----------- | ----------------------------------------------------------------------- | ----------- | ----------------------------------------------------------------- |
| `icon`      | `icon`      | `string`                                                                | -           | Name of the Material Icon to display                              |
| `svg`       | `svg`       | `string`                                                                | -           | Custom SVG content if the icon is not available in Material Icons |
| `font`      | `font`      | `"Icons" \| "Symbols"`                                                  | `"Symbols"` | Which icon font family to use                                     |
| `fontstyle` | `fontstyle` | `"Outlined" \| "Fill" \| "Sharp" \| "Two Tone" \| "Round" \| "Rounded"` | `"Rounded"` | Style variant of the icon                                         |
| `ready`     | `ready`     | `boolean`                                                               | `false`     | Whether the font has been loaded (read-only)                      |

### ct-icon-button

| Property    | Attribute    | Type      | Default | Description                                                               |
| ----------- | ------------ | --------- | ------- | ------------------------------------------------------------------------- |
| `icon`      | `icon`       | `string`  | -       | Name of the Material Icon to display                                      |
| `svg`       | `svg`        | `string`  | -       | Custom SVG content for the button                                         |
| `disabled`  | `disabled`   | `boolean` | `false` | Whether the button is disabled                                            |
| `ariaLabel` | `aria-label` | `string`  | `""`    | Accessible label for the button (falls back to icon name if not provided) |

## Slots

### ct-icon

- **Default slot**: Custom SVG content (if provided, takes precedence over icon property)

### ct-icon-button

- **Default slot**: Custom icon content
- **slot="content"**: Additional content (rarely used)

## CSS Variables

You can customize the components using the following CSS variables:

```css
ct-icon {
	--ct-icon-size: 24px; /* Size of the icon */
}

ct-icon-button {
	/* Inherits --ct-icon-size from ct-icon */
	/* Padding is automatically calculated based on icon size */
}
```

### CSS Variables Reference Table

| Variable         | Default | Description                     |
| ---------------- | ------- | ------------------------------- |
| `--ct-icon-size` | `24px`  | Size of the icon (width/height) |

### Custom Styling Examples

<div style="margin: 1rem 0;">
	<div style="display: flex; gap: 1rem; align-items: center; flex-wrap: wrap;">
		<ct-icon icon="settings" style="--ct-icon-size: 32px; color: #ff6b6b;"></ct-icon>
		<ct-icon icon="favorite" style="--ct-icon-size: 48px; color: #51cf66;"></ct-icon>
		<ct-icon-button icon="search" style="--ct-icon-size: 20px;"></ct-icon-button>
	</div>
</div>

```html
<!-- Custom size and color -->
<ct-icon icon="settings" style="--ct-icon-size: 32px; color: #ff6b6b;"></ct-icon>

<!-- Large icon -->
<ct-icon icon="favorite" style="--ct-icon-size: 48px; color: #51cf66;"></ct-icon>

<!-- Custom icon button size -->
<ct-icon-button icon="search" style="--ct-icon-size: 20px;"></ct-icon-button>
```

## Custom SVG Icons

You can use custom SVG icons instead of Material Icons:

Example:
<ct-icon>
<svg viewBox="0 0 24 24" data-astro-cid-zjewmv2f="true"><path d="M18.901 1.153h3.68l-8.04 9.19L24 22.846h-7.406l-5.8-7.584-6.638 7.584H.474l8.6-9.83L0 1.154h7.594l5.243 6.932ZM17.61 20.644h2.039L6.486 3.24H4.298Z"></path></svg>
</ct-icon>
<ct-icon-button>
<svg viewBox="0 0 24 24" data-astro-cid-zjewmv2f="true">
<path d="M18.901 1.153h3.68l-8.04 9.19L24 22.846h-7.406l-5.8-7.584-6.638 7.584H.474l8.6-9.83L0 1.154h7.594l5.243 6.932ZM17.61 20.644h2.039L6.486 3.24H4.298Z"></path>
</svg>
</ct-icon-button>

```html
<!-- Using custom SVG content -->
<ct-icon>
	<svg viewBox="0 0 24 24" data-astro-cid-zjewmv2f="true">
		<path d="M18.901 1.153h3.68l-8.04 9.19L24 22.846h-7.406l-5.8-7.584-6.638 7.584H.474l8.6-9.83L0 1.154h7.594l5.243 6.932ZM17.61 20.644h2.039L6.486 3.24H4.298Z"></path>
	</svg>
</ct-icon>

<!-- SVG in icon button -->
<ct-icon-button>
	<svg viewBox="0 0 24 24" data-astro-cid-zjewmv2f="true">
		<path d="M18.901 1.153h3.68l-8.04 9.19L24 22.846h-7.406l-5.8-7.584-6.638 7.584H.474l8.6-9.83L0 1.154h7.594l5.243 6.932ZM17.61 20.644h2.039L6.486 3.24H4.298Z"></path>
	</svg>
</ct-icon-button>
```

## Static Properties and Methods

### ct-icon Static Properties

| Property    | Type                                                                    | Default     | Description                |
| ----------- | ----------------------------------------------------------------------- | ----------- | -------------------------- |
| `Font`      | `"Icons" \| "Symbols"`                                                  | `"Symbols"` | Global font family setting |
| `FontStyle` | `"Outlined" \| "Fill" \| "Sharp" \| "Two Tone" \| "Round" \| "Rounded"` | `"Rounded"` | Global font style setting  |

### ct-icon Static Methods

| Method        | Description                     | Parameters                            | Returns                        |
| ------------- | ------------------------------- | ------------------------------------- | ------------------------------ |
| `loadFonts()` | Loads the specified font styles | `FontStyle?: string`, `Font?: string` | `HTMLLinkElement \| undefined` |

## Accessibility (a11y)

To ensure your icons are accessible, consider the following best practices:

- Use `aria-label` for icon buttons to provide a text alternative
- Add `title` attributes to icons that convey information
- Avoid using icons alone for critical navigation or actions unless properly labeled

### Example with aria-label

```html
<ct-icon-button icon="menu" aria-label="Open navigation menu"></ct-icon-button>
```

## Event Handling

The icon button supports standard click events and can be used with any framework:

#### Vue Example

```vue
<template>
	<ct-icon-button icon="search" @click="handleClick"></ct-icon-button>
</template>

<script setup>
import "@conectate/components/ct-icon-button";

const handleClick = () => {
	console.log('Icon button clicked!');
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-icon-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-icon-button icon="search" @click=${this.handleClick}></ct-icon-button>
		`;
	}

	handleClick() {
		console.log("Icon button clicked!");
	}
}
```

#### React Example

```tsx
import React from 'react';
import "@conectate/components/ct-icon-button.js";

function MyComponent() {
	const handleClick = () => {
		console.log('Icon button clicked!');
	};

	return (
		<ct-icon-button icon="search" aria-label="Search" onClick={handleClick} />
	);
}
```

## Icon List

The component provides access to all Material Icons available in the Google Fonts library. You can browse the available icons at:

[https://fonts.google.com/icons](https://fonts.google.com/icons)

When using TypeScript, the package provides type definitions for all available icons, giving you autocomplete support in compatible editors.
