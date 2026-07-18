---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-card.js'
</script>

# ct-card

A stylized container component that provides a surface for displaying content.

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
import "@conectate/components/ct-card.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
	<ct-card padding>Basic card with padding</ct-card>
	<ct-card decorator>Card with decorator</ct-card>
	<ct-card withborder>Card with border</ct-card>
	<ct-card primary>Primary themed card</ct-card>
</div>

```vue
<template>
	<ct-card padding>Basic card with padding</ct-card>
	<ct-card decorator>Card with decorator</ct-card>
	<ct-card withborder>Card with border</ct-card>
	<ct-card primary>Primary themed card</ct-card>
</template>

<script setup>
import "@conectate/components/ct-card";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-card.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-card padding>
				<h2>Card Title</h2>
				<p>Card content goes here</p>
			</ct-card>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-card.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-card': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					decorator?: boolean;
					withborder?: boolean;
					primary?: boolean;
					secondary?: boolean;
					tertiary?: boolean;
					error?: boolean;
					shadow?: boolean;
					padding?: boolean;
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	return (
		<ct-card padding>
			<h2>Card Title</h2>
			<p>Card content goes here</p>
		</ct-card>
	);
}
```

## Variants

The card component supports several variants:

- **Default**: Standard card with transparent background
- **Primary**: Card with primary theme colors
- **Secondary**: Card with secondary theme colors
- **Tertiary**: Card with tertiary theme colors
- **Error**: Card with error theme colors
- **Decorator**: Card with colored top border
- **With Border**: Card with border outline
- **Padding**: Card with internal padding
- **Shadow**: Card with box shadow (deprecated)

## Properties

| Property     | Attribute    | Type      | Default | Description                   |
| ------------ | ------------ | --------- | ------- | ----------------------------- |
| `decorator`  | `decorator`  | `boolean` | `false` | Add border-top with color-app |
| `withborder` | `withborder` | `boolean` | `false` | Add border around the card    |
| `primary`    | `primary`    | `boolean` | `false` | Apply primary theme colors    |
| `secondary`  | `secondary`  | `boolean` | `false` | Apply secondary theme colors  |
| `tertiary`   | `tertiary`   | `boolean` | `false` | Apply tertiary theme colors   |
| `error`      | `error`      | `boolean` | `false` | Apply error theme colors      |
| `shadow`     | `shadow`     | `boolean` | `false` | Add box-shadow (deprecated)   |
| `padding`    | `padding`    | `boolean` | `false` | Add 16px padding to the card  |

## Slots

- **Default slot**: Main content area of the card

### Example with Card Content and Actions

<div style="margin: 1rem 0;">
	<ct-card>
		<div class="card-content">This is the main content of the card.</div>
		<div class="card-actions">
			<ct-button>Action 1</ct-button>
			<ct-button>Action 2</ct-button>
		</div>
	</ct-card>
</div>

```html
<ct-card>
	<div class="card-content">This is the main content of the card.</div>
	<div class="card-actions">
		<ct-button>Action 1</ct-button>
		<ct-button>Action 2</ct-button>
	</div>
</ct-card>
```

## CSS Variables

You can customize the component using the following CSS variables:

```css
ct-card {
	/* Layout */
	--border-radius: 16px; /* Border radius of the card */

	/* Colors */
	--color-surface: #fff; /* Background color */
	--color-on-surface: inherit; /* Text color */
	--color-outline: #8d8d8d38; /* Border color when using withborder attribute */
	--color-app: linear-gradient(90deg, #0fb8ad 0%, #1fc8db 51%, #2cb5e8 75%); /* Color for the decorator (top border) */
	--color-primary: inherit; /* Fallback for decorator */

	/* Theme Variants */
	--color-primary-container: var(--color-surface, #fff); /* Background for primary variant */
	--color-on-primary-container: var(--color-primary, var(--color-on-surface, #000)); /* Text color for primary variant */
	--color-secondary-container: var(--color-surface, #fff); /* Background for secondary variant */
	--color-on-secondary-container: var(--color-secondary, var(--color-on-surface, #000)); /* Text color for secondary variant */
	--color-tertiary-container: var(--color-surface, #fff); /* Background for tertiary variant */
	--color-on-tertiary-container: var(--color-tertiary, var(--color-on-surface, #000)); /* Text color for tertiary variant */
	--color-error-container: var(--color-surface, #fff); /* Background for error variant */
	--color-on-error-container: var(--color-error, var(--color-on-surface, #000)); /* Text color for error variant */

	/* Effects */
	--ct-card-box-shadow: 0 8px 16px 0 rgba(10, 14, 29, 0.02), 0 8px 40px 0 rgba(10, 14, 29, 0.06); /* Custom box-shadow when using shadow attribute */
}
```

### CSS Variables Reference Table

| Variable                         | Default                                                                    | Description                                   |
| -------------------------------- | -------------------------------------------------------------------------- | --------------------------------------------- |
| `--border-radius`                | `16px`                                                                     | Border radius of the card                     |
| `--color-surface`                | `#fff`                                                                     | Background color                              |
| `--color-on-surface`             | `inherit`                                                                  | Text color                                    |
| `--color-outline`                | `#8d8d8d38`                                                                | Border color when using withborder attribute  |
| `--color-app`                    | `linear-gradient(90deg, #0fb8ad 0%, #1fc8db 51%, #2cb5e8 75%)`             | Color for the decorator (top border)          |
| `--color-primary`                | `inherit`                                                                  | Fallback for decorator                        |
| `--color-primary-container`      | `var(--color-surface, #fff)`                                               | Background for primary variant                |
| `--color-on-primary-container`   | `var(--color-primary, var(--color-on-surface, #000))`                      | Text color for primary variant                |
| `--color-secondary-container`    | `var(--color-surface, #fff)`                                               | Background for secondary variant              |
| `--color-on-secondary-container` | `var(--color-secondary, var(--color-on-surface, #000))`                    | Text color for secondary variant              |
| `--color-tertiary-container`     | `var(--color-surface, #fff)`                                               | Background for tertiary variant               |
| `--color-on-tertiary-container`  | `var(--color-tertiary, var(--color-on-surface, #000))`                     | Text color for tertiary variant               |
| `--color-error-container`        | `var(--color-surface, #fff)`                                               | Background for error variant                  |
| `--color-on-error-container`     | `var(--color-error, var(--color-on-surface, #000))`                        | Text color for error variant                  |
| `--ct-card-box-shadow`           | `0 8px 16px 0 rgba(10, 14, 29, 0.02), 0 8px 40px 0 rgba(10, 14, 29, 0.06)` | Custom box-shadow when using shadow attribute |

### Custom Styling Examples

<div style="margin: 1rem 0;">
	<div style="display: flex; flex-direction: column; gap: 1rem;">
		<ct-card style="--border-radius: 8px; --color-surface: #f0f0f0;">Custom Border Radius</ct-card>
		<ct-card style="--color-primary-container: #e3f2fd; --color-on-primary-container: #1976d2;">Custom Primary Colors</ct-card>
	</div>
</div>

```html
<!-- Custom border radius -->
<ct-card style="--border-radius: 8px; --color-surface: #f0f0f0;">Custom Border Radius</ct-card>

<!-- Custom primary colors -->
<ct-card primary style="--color-primary-container: #e3f2fd; --color-on-primary-container: #1976d2;">Custom Primary Colors</ct-card>
```

## CSS Classes

You can use these classes inside the card to get predefined styles:

- **`.card-actions`**: For action buttons (adds top border)
- **`.card-content`**: For content (adds padding)

## Accessibility (a11y)

The component is a semantic container. Use appropriate heading levels and semantic HTML within the card for better accessibility.

## Event Handling

The card component doesn't emit events directly, but you can handle events from elements inside it:

#### Vue Example

```vue
<template>
	<ct-card padding @click="handleCardClick"> Card content </ct-card>
</template>

<script setup>
import "@conectate/components/ct-card";

const handleCardClick = () => {
	console.log('Card clicked!');
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-card.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-card padding @click=${this.handleClick}>
				Card content
			</ct-card>
		`;
	}

	handleClick() {
		console.log("Card clicked!");
	}
}
```

#### React Example

```tsx
import React from 'react';
import "@conectate/components/ct-card.js";

function MyComponent() {
	const handleClick = () => {
		console.log('Card clicked!');
	};

	return (
		<ct-card padding onClick={handleClick}>
			Card content
		</ct-card>
	);
}
```
