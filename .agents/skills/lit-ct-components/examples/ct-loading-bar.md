---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-loading-bar.js'
</script>

# ct-loading-bar

A horizontal progress bar component used to indicate that an operation is in progress when the percentage completion is indeterminate.

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
import "@conectate/components/ct-loading-bar.js";
```

### Example with Vue

<div style="margin: 1rem 0; background: #eee; padding: 2rem; border-radius: 8px;">
  <ct-loading-bar></ct-loading-bar>
</div>

```vue
<template>
	<ct-loading-bar></ct-loading-bar>
</template>

<script setup>
import "@conectate/components/ct-loading-bar.js";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-loading-bar.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-loading-bar></ct-loading-bar>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-loading-bar.js";

export function LoadingComponent() {
	return (
		<div style={{ width: '100%' }}>
			<ct-loading-bar />
		</div>
	);
}
```

## CSS Variables

You can customize the colors of the animated bars using these variables:

```css
ct-loading-bar {
	/* Colors */
	--ct-loading-bar-c1: #4998ff; /* First bar color */
	--ct-loading-bar-c2: #fff;    /* Second bar color */
	--ct-loading-bar-c3: #4998ff; /* Third bar color */
}
```

## Accessibility (a11y)

The component is purely visual. For better accessibility, consider adding `role="progressbar"` and `aria-label` to the container element where you use the loading bar.


