---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-button.js'

const count = ref(0)
const message = ref('')
</script>

# ct-button

A customizable button web component with various styles and configurations.

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
import "@conectate/components/ct-button.js";
```

### Example with Vue

<div style="display: flex; gap: 1rem; flex-wrap: wrap; margin: 1rem 0;">
  <ct-button>Click me</ct-button>
  <ct-button raised>Raised Button</ct-button>
  <ct-button shadow>Shadow Button</ct-button>
  <ct-button flat>Flat Button</ct-button>
  <ct-button light>Light Button</ct-button>
  <ct-button disabled>Disabled Button</ct-button>
</div>

```vue
<template>
	<ct-button>Click me</ct-button>
	<ct-button raised>Raised Button</ct-button>
	<ct-button shadow>Shadow Button</ct-button>
	<ct-button flat>Flat Button</ct-button>
	<ct-button light>Light Button</ct-button>
	<ct-button disabled>Disabled Button</ct-button>
</template>

<script setup>
import "@conectate/components/ct-button";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-button raised @click=${this.handleClick}>Click Me</ct-button>
		`;
	}

	handleClick() {
		console.log("Button clicked!");
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-button.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-button': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					raised?: boolean;
					shadow?: boolean;
					flat?: boolean;
					light?: boolean;
					disabled?: boolean;
					type?: 'button' | 'submit' | 'reset';
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	const [count, setCount] = useState(0);

	const handleClick = () => {
		setCount(count + 1);
		console.log('Button clicked!');
	};

	return (
		<div>
			<p>Click count: {count}</p>
			<ct-button raised onClick={handleClick}>
				Click Me
			</ct-button>
			<ct-button shadow onClick={() => setCount(0)}>
				Reset
			</ct-button>
			<ct-button flat disabled>
				Disabled Button
			</ct-button>
		</div>
	);
}

export function ButtonWithSlots() {
	return (
		<ct-button raised>
			<span slot="prefix">↑</span>
			Click Me
			<span slot="suffix">↓</span>
		</ct-button>
	);
}

```

## Variants

The button comes in several variants:

- **Default**: Standard button
- **Raised**: Primary colored button
- **Shadow**: Button with opaque black background
- **Flat**: Button with a primary color border
- **Light**: Button with a primary color border (alternative style)

### Interactive Example

<div style="margin: 1rem 0;">
  <p>Click count: {{ count }}</p>
  <div style="display: flex; gap: 1rem; flex-wrap: wrap;">
    <ct-button @click="count++">Default</ct-button>
    <ct-button raised @click="count++">Raised</ct-button>
    <ct-button shadow @click="count++">Shadow</ct-button>
    <ct-button flat @click="count++">Flat</ct-button>
    <ct-button light @click="count++">Light</ct-button>
  </div>
</div>

## Components

This package includes:

- `ct-button`: Standard button component
- `ct-button-menu`: Button with dropdown menu functionality
- `ct-button-split`: Split button with primary action and dropdown

## Properties

| Property   | Attribute  | Type      | Default    | Description                         |
| ---------- | ---------- | --------- | ---------- | ----------------------------------- |
| `raised`   | `raised`   | `boolean` | `false`    | Raised Style (primary color)        |
| `shadow`   | `shadow`   | `boolean` | `false`    | Shown with opaque black background. |
| `flat`     | `flat`     | `boolean` | `false`    | Shown with a primary color border   |
| `light`    | `light`    | `boolean` | `false`    | Shown with a primary color border   |
| `disabled` | `disabled` | `boolean` | `false`    | If `true`, Disable clicks           |
| `type`     | `type`     | `string`  | `"button"` | Button type (button, submit, reset) |
| `role`     | `role`     | `string`  | `"button"` | ARIA role                           |
| `gap`      | `gap`      | `boolean` | `false`    | Adds a gap between slots (1ch)      |

## Slots

- **Default slot**: Main content of the button
- **prefix**: Content placed above the main content
- **suffix**: Content placed below the main content

### Example with Slots

<div style="margin: 1rem 0;">
  <ct-button raised>
    <span slot="prefix">↑</span>
    Click Me
    <span slot="suffix">↓</span>
  </ct-button>
</div>

```html
<ct-button raised>
	<span slot="prefix">↑</span>
	Click Me
	<span slot="suffix">↓</span>
</ct-button>
```

## CSS Variables

You can customize the component using the following CSS variables:

```css
ct-button {
	/* Primary Colors */
	--color-primary: #00aeff; /* Primary color */
	--dark-primary-color: #00aeff; /* Dark Primary color */
	--color-primary-hover: rgba(173, 195, 222, 0.1); /* Primary color on hover (used in flat variant) */
	--color-primary-active: rgba(173, 195, 222, 0.2); /* Primary color on active state (used in flat variant) */

	/* Text Colors */
	--color-on-primary: #fff; /* Text color on primary background */
	--color-on-background: #535353; /* Text color on background (fallback for flat variant) */
	--color-disable: #a8a8a8; /* Disabled state color */

	/* Shadow & Effects */
	--ct-button-box-shadow: 0 2px 16px 4px rgba(99, 188, 240, 0.45); /* Box-shadow for hover on raised variant */
	--ct-button-shadow-color: rgba(64, 117, 187, 0.1); /* Shadow color (used in shadow variant) */

	/* Layout & Spacing */
	--ct-button-radius: 26px; /* Button border radius */
	--border-radius: 24px; /* Fallback border radius (used if --ct-button-radius is not set) */
	--ct-button-padding: 0.4em 1em; /* Button padding */
	--ct-button-white-space: nowrap; /* White space handling for button content */
}
```

### CSS Variables Reference Table

| Variable                   | Default                                   | Description                                                     |
| -------------------------- | ----------------------------------------- | --------------------------------------------------------------- |
| `--color-primary`          | `#00aeff`                                 | Primary color used for borders, backgrounds, and text           |
| `--dark-primary-color`     | `#00aeff`                                 | Dark variant of primary color                                   |
| `--color-primary-hover`    | `rgba(173, 195, 222, 0.1)`                | Background color on hover (used in flat variant)                |
| `--color-primary-active`   | `rgba(173, 195, 222, 0.2)`                | Background color on active/pressed state (used in flat variant) |
| `--color-on-primary`       | `#fff`                                    | Text color when button has primary background (raised variant)  |
| `--color-on-background`    | `#535353`                                 | Text color fallback for flat variant                            |
| `--color-disable`          | `#a8a8a8`                                 | Text and border color when button is disabled                   |
| `--ct-button-box-shadow`   | `0 2px 16px 4px rgba(99, 188, 240, 0.45)` | Box shadow applied on hover for raised variant                  |
| `--ct-button-shadow-color` | `rgba(64, 117, 187, 0.1)`                 | Background color for shadow variant                             |
| `--ct-button-radius`       | `26px`                                    | Border radius of the button                                     |
| `--border-radius`          | `24px`                                    | Fallback border radius if `--ct-button-radius` is not set       |
| `--ct-button-padding`      | `0.4em 1em`                               | Internal padding of the button                                  |
| `--ct-button-white-space`  | `nowrap`                                  | White space handling for button text content                    |

### Custom Styling Examples

<div style="margin: 1rem 0;">
  <div style="display: flex; gap: 1rem; flex-wrap: wrap; margin-bottom: 1rem;">
    <ct-button raised style="--color-primary: #ff6b6b; --ct-button-radius: 8px;">
      Custom Primary & Radius
    </ct-button>
    <ct-button flat style="--color-primary: #51cf66; --color-primary-hover: rgba(81, 207, 102, 0.1);">
      Custom Flat Colors
    </ct-button>
    <ct-button shadow style="--ct-button-shadow-color: rgba(255, 107, 107, 0.2);">
      Custom Shadow
    </ct-button>
  </div>
</div>

```html
<!-- Custom primary color and border radius -->
<ct-button raised style="--color-primary: #ff6b6b; --ct-button-radius: 8px;"> Custom Primary & Radius </ct-button>

<!-- Custom flat variant colors -->
<ct-button flat style="--color-primary: #51cf66; --color-primary-hover: rgba(81, 207, 102, 0.1);"> Custom Flat Colors </ct-button>

<!-- Custom shadow variant background -->
<ct-button shadow style="--ct-button-shadow-color: rgba(255, 107, 107, 0.2);"> Custom Shadow </ct-button>
```

## Accessibility (a11y)

The component has proper focus states and keyboard navigation support. Use `aria-label` for improved accessibility when needed.

### Example with aria-label

```html
<ct-button raised aria-label="Submit form">Submit</ct-button>
```

## Event Handling

The button supports standard click events and can be used with any framework:

<div style="margin: 1rem 0;">
  <p v-if="message" style="color: green; font-weight: bold;">{{ message }}</p>
  <ct-button raised @click="message = 'Button clicked!'">Click to see message</ct-button>
  <ct-button @click="message = ''" style="margin-left: 0.5rem;">Clear</ct-button>
</div>

#### Vue Example

```vue
<template>
	<ct-button raised @click="handleClick">Click Me</ct-button>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-button";

const handleClick = () => {
  console.log('Button clicked!');
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-button raised @click=${this.handleClick}>Click Me</ct-button>
		`;
	}

	handleClick() {
		console.log("Button clicked!");
	}
}
```

#### React Example

```tsx
import React from 'react';
import "@conectate/components/ct-button.js";

function MyComponent() {
	const handleClick = () => {
		console.log('Button clicked!');
	};

	return (
		<ct-button raised onClick={handleClick}>Click Me</ct-button>
	);
}
```
