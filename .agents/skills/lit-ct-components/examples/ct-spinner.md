---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-spinner.js'

const isActive = ref(true)
</script>

# ct-spinner

A clean, animated loading spinner web component.

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
import "@conectate/components/ct-spinner.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; align-items: center; gap: 1rem; margin: 1rem 0;">
  <ct-spinner :active="isActive"></ct-spinner>
  <button @click="isActive = !isActive" style="padding: 4px 12px; cursor: pointer;">
    Toggle Active
  </button>
</div>

```vue
<template>
	<ct-spinner v-if="loading" active></ct-spinner>
</template>

<script setup>
import "@conectate/components/ct-spinner.js";
const loading = ref(true);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { html } from 'lit';
import "@conectate/components/ct-spinner.js";

render() {
	return html`
		<div class="loader">
			<ct-spinner ?active=${this.loading}></ct-spinner>
		</div>
	`;
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `active` | `active` | `boolean` | `true` | Whether the spinner is visible and animating |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--ct-spinner-1` | `--color-primary` | Main color for the spin animation |
| `--ct-spinner-2` | `--color-secondary`| Secondary color used during the animation pulse |
| `--color-primary` | `#2cb5e8` | Fallback for primary color |
| `--color-secondary`| `#0fb8ad` | Fallback for secondary color |

## Accessibility (a11y)

The spinner should ideally be accompanied by an `aria-busy="true"` or `role="status"` on a parent container to inform screen readers that content is loading.


