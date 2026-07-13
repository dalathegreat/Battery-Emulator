---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-tabs.js'
import '@conectate/components/ct-tab.js'

const selectedTab = ref('0')
</script>

# ct-tabs

A container component for organizing content into tabbed views.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the components:

```ts
import "@conectate/components/ct-tabs.js";
import "@conectate/components/ct-tab.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-tabs v-model:selected="selectedTab" handletabs>
    <ct-tab>Dashboard</ct-tab>
    <ct-tab>Profile</ct-tab>
    <ct-tab>Settings</ct-tab>
  </ct-tabs>
  <div style="padding: 1rem; border: 1px solid #eee; border-top: none; min-height: 100px;">
    <div v-if="selectedTab === '0'">Dashboard Content</div>
    <div v-if="selectedTab === '1'">Profile Content</div>
    <div v-if="selectedTab === '2'">Settings Content</div>
  </div>
</div>

```vue
<template>
	<ct-tabs v-model:selected="activeTab" handletabs>
		<ct-tab>Tab 1</ct-tab>
		<ct-tab>Tab 2</ct-tab>
	</ct-tabs>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-tabs.js";
import "@conectate/components/ct-tab.js";

const activeTab = ref('0')
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-tabs.js";
import "@conectate/components/ct-tab.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private selected = "1";

	render() {
		return html`
			<ct-tabs 
				.selected=${this.selected} 
				handletabs
				@selected=${(e: any) => this.selected = e.detail.value}>
				<ct-tab>Item One</ct-tab>
				<ct-tab>Item Two</ct-tab>
			</ct-tabs>
		`;
	}
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `selected` | `selected` | `string` | - | Index of the selected tab (as string) |
| `handletabs` | `handletabs`| `boolean`| `false` | If true, clicking a tab automatically updates `selected` |
| `overflow` | `overflow` | `boolean` | `false` | Read-only. True if tabs overflow the container |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `selected` | `{ value: string }`| Fired when the selected tab changes |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--ct-tabs-background`| - | Background color for the tabs container |
| `--ct-tabs-box-shadow`| - | Box shadow for the tabs container |
| `--ct-tabs-border-color`| - | Border color for selected tab indicator |

## Accessibility (a11y)

The component uses a `<nav>` element internally for better navigation semantics. When using `handletabs`, keyboard navigation should be considered for switching between tab panels.


