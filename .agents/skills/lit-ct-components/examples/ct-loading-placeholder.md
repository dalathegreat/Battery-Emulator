---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-loading-placeholder.js'
</script>

# ct-loading-placeholder

A simple skeleton loading component (shimmer effect) used to indicate content is being loaded, similar to placeholder blocks in modern apps.

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
import "@conectate/components/ct-loading-placeholder.js";
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; flex-direction: column; gap: 1rem;">
  <div style="display: flex; gap: 1rem; align-items: center;">
    <ct-loading-placeholder style="width: 50px; height: 50px; border-radius: 50%; flex-shrink: 0;"></ct-loading-placeholder>
    <div style="flex: 1; display: flex; flex-direction: column; gap: 0.5rem;">
      <ct-loading-placeholder style="width: 100%; height: 15px;"></ct-loading-placeholder>
      <ct-loading-placeholder style="width: 60%; height: 15px;"></ct-loading-placeholder>
    </div>
  </div>
  <ct-loading-placeholder style="width: 100%; height: 150px; border-radius: 8px;"></ct-loading-placeholder>
</div>

```vue
<template>
	<div class="skeleton-card">
		<ct-loading-placeholder class="avatar"></ct-loading-placeholder>
		<div class="lines">
			<ct-loading-placeholder class="line"></ct-loading-placeholder>
			<ct-loading-placeholder class="line short"></ct-loading-placeholder>
		</div>
	</div>
</template>

<script setup>
import "@conectate/components/ct-loading-placeholder.js";
</script>

<style scoped>
.avatar { width: 48px; height: 48px; border-radius: 50%; }
.line { width: 100%; height: 12px; margin-bottom: 8px; }
.short { width: 60%; }
</style>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-loading-placeholder.js";

@customElement("my-skeleton")
class MySkeleton extends LitElement {
	render() {
		return html`
			<ct-loading-placeholder 
				style="width: 200px; height: 20px; margin-bottom: 10px;"
			></ct-loading-placeholder>
			<ct-loading-placeholder 
				style="width: 150px; height: 20px;"
			></ct-loading-placeholder>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-loading-placeholder.js";

export function SkeletonLoader() {
	return (
		<div style={{ display: 'flex', gap: '10px' }}>
			<ct-loading-placeholder style={{ width: '40px', height: '40px' }} />
			<div style={{ flex: 1 }}>
				<ct-loading-placeholder style={{ width: '100%', height: '10px', marginBottom: '5px' }} />
				<ct-loading-placeholder style={{ width: '80%', height: '10px' }} />
			</div>
		</div>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `buffering`| `buffering`| `boolean`| `false` | Enables a secondary striped animation style |

## CSS Variables

```css
ct-loading-placeholder {
	/* Colors */
	--loading-placeholder-color-1: #e0e0e0; /* Shimmer base color */
	--loading-placeholder-color-2: #c0c0c0; /* Shimmer highlight color */
}
```

## Accessibility (a11y)

Placeholder components should be hidden from screen readers using `aria-hidden="true"` or by providing a generic "Loading..." text in a parent container with `aria-live="polite"`.


