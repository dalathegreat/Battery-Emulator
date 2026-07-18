---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-collapse.js'
import '@conectate/components/ct-button.js'

const isOpened = ref(false)
</script>

# ct-collapse

A collapsible content component that can smoothly expand and collapse with animation.

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
import "@conectate/components/ct-collapse.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-button raised @click="isOpened = !isOpened">
    {{ isOpened ? 'Collapse' : 'Expand' }} Content
  </ct-button>
  <ct-collapse :opened="isOpened" style="margin-top: 1rem; border: 1px solid #ddd; border-radius: 8px;">
    <div style="padding: 16px;">
      <h3 style="margin-top:0">Collapsible Content</h3>
      <p>This content can be toggled using the button above. The expansion and collapse are smoothly animated.</p>
      <p>Remember that <code>ct-collapse</code> works best with a single child element. If you need multiple elements, wrap them in a <code>div</code>.</p>
    </div>
  </ct-collapse>
</div>

```vue
<template>
	<ct-button raised @click="opened = !opened">Toggle</ct-button>
	
	<ct-collapse :opened="opened">
		<div class="content">
			<h3>Collapsible Content</h3>
			<p>This content expands and collapses smoothly.</p>
		</div>
	</ct-collapse>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-collapse.js";
import "@conectate/components/ct-button.js";

const opened = ref(false)
</script>

<style scoped>
.content {
	padding: 16px;
	border: 1px solid #ccc;
}
</style>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-collapse.js";
import "@conectate/components/ct-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private opened = false;

	render() {
		return html`
			<ct-button raised @click=${() => this.opened = !this.opened}>
				Toggle
			</ct-button>
			<ct-collapse .opened=${this.opened}>
				<div style="padding: 1rem; background: #f5f5f5;">
					Hidden content here
				</div>
			</ct-collapse>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-collapse.js";
import "@conectate/components/ct-button.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-collapse': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					opened?: boolean;
				},
				HTMLElement
			>;
		}
	}
}

export function MyCollapse() {
	const [opened, setOpened] = useState(false);

	return (
		<div>
			<ct-button raised onClick={() => setOpened(!opened)}>
				{opened ? 'Close' : 'Open'}
			</ct-button>
			<ct-collapse opened={opened}>
				<div style={{ padding: '20px', border: '1px solid #eee' }}>
					This content is collapsible.
				</div>
			</ct-collapse>
		</div>
	);
}
```

## Properties

| Property | Attribute | Type      | Default | Description                                            |
| -------- | --------- | --------- | ------- | ------------------------------------------------------ |
| `opened` | `opened`  | `boolean` | `false` | Controls whether the content is expanded or collapsed |

## Methods

| Method     | Description                                       |
| ---------- | ------------------------------------------------- |
| `toggle()` | Toggles the opened state of the collapse component |

## Slots

- **Default slot**: The content to be collapsed. **Note**: Only one child element is supported. Wrap multiple elements in a container (e.g., `<div>`).

## CSS Variables

The component uses internal styles for the transition.

```css
ct-collapse {
	/* Transition timing */
	transition: all 250ms;
}
```

## Accessibility (a11y)

The component handles the `max-height` and `overflow` properties to ensure the content is hidden from both visual users and assistive technologies when collapsed.

## Event Handling

The component doesn't emit specific events when opening or closing, but you can listen for property changes if needed.


