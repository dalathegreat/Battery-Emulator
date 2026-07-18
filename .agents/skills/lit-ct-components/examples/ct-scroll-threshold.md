---
outline: deep
---

# ct-scroll-threshold

A component that triggers an event when the user scrolls past a specified threshold, perfect for implementing "infinite scroll" or "load more" patterns.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Wrap your list content with `ct-scroll-threshold` and listen for the `lower-threshold` event.

```ts
import "@conectate/components/ct-scroll-threshold.js";

render() {
	return html`
		<ct-scroll-threshold @lower-threshold=${this.loadMoreData}>
			<div class="list">
				${this.items.map(item => html`<div>${item.name}</div>`)}
			</div>
		</ct-scroll-threshold>
	`;
}
```

### Example with Vue

```vue
<template>
	<ct-scroll-threshold @lower-threshold="fetchNextPage" :threshold="0.8">
		<div class="product-grid">
			<div v-for="p in products" :key="p.id" class="product-card">
				{{ p.name }}
			</div>
		</div>
		<div v-if="loading" class="loading-spinner">Loading more...</div>
	</ct-scroll-threshold>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-scroll-threshold.js";

const products = ref([...])
const loading = ref(false)

const fetchNextPage = async () => {
	if (loading.value) return;
	loading.value = true;
	// API call logic...
	loading.value = false;
}
</script>
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `threshold`| `threshold`| `number` | `0.9` | Intersection ratio (0.0 to 1.0) to trigger event |
| `scrollTarget`| - | `HTMLElement`| `document.body`| The scrollable element to observe |

## Methods

| Method | Description |
| ------ | ----------- |
| `toggleScrollListener(bool)`| Manually enable or disable the observer |
| `clearTriggers()`| Re-activates the threshold observer after a trigger |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `lower-threshold`| `{}` | Fired when the scroll position reaches the threshold |

## Performance

The component uses the `IntersectionObserver` API, which is highly efficient and doesn't rely on expensive `scroll` event listeners. For older browsers, a polyfill is automatically loaded if needed.


