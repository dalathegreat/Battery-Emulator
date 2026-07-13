---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-input-wrapper.js'
import '@conectate/components/ct-button.js'

const handleFiles = (e) => {
  console.log('Files selected:', e.detail.files)
}
</script>

# ct-input-wrapper

A wrapper component that overlays an invisible input (usually for files) over any content, making the entire content area act as a trigger.

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
import "@conectate/components/ct-input-wrapper.js";
```

### Example with Vue

<div style="margin: 1rem 0;">
  <ct-input-wrapper type="file" @files="handleFiles">
    <ct-button raised>Click to Upload File</ct-button>
  </ct-input-wrapper>
</div>

```vue
<template>
	<ct-input-wrapper type="file" @files="handleFiles">
		<ct-button raised>Upload Profile Picture</ct-button>
	</ct-input-wrapper>
</template>

<script setup>
import "@conectate/components/ct-input-wrapper.js";
import "@conectate/components/ct-button.js";

const handleFiles = (e) => {
	const files = e.detail.files;
	console.log('Selected files:', files);
};
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-input-wrapper.js";
import "@conectate/components/ct-button.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-input-wrapper type="file" @files=${this.onFiles}>
				<ct-button raised>Choose File</ct-button>
			</ct-input-wrapper>
		`;
	}

	onFiles(e: CustomEvent) {
		console.log(e.detail.files);
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-input-wrapper.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-input-wrapper': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					type?: string;
					accept?: string;
					multiple?: boolean;
					onFiles?: (e: CustomEvent<{files: FileList}>) => void;
				},
				HTMLElement
			>;
		}
	}
}

export function FileUpload() {
	return (
		<ct-input-wrapper 
			type="file" 
			accept="image/*" 
			onFiles={(e) => console.log(e.detail.files)}
		>
			<div style={{ padding: '20px', border: '2px dashed #ccc' }}>
				Drop files or click here
			</div>
		</ct-input-wrapper>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `type` | `type` | `string` | `"file"` | The type of the internal input (file, text, color, etc.) |
| `accept` | `accept` | `string` | `"text"` | Accepted file types or input values |
| `multiple` | `multiple` | `boolean` | `false` | Allow multiple file selection |

## Methods

| Method | Description |
| ------ | ----------- |
| `clear()` | Resets the internal input value |

## Slots

- **Default slot**: The content that will trigger the input. The invisible input will cover this entire area.

## Event Handling

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `files` | `{ files: FileList }` | Fired when files are selected via the input |

#### Vue Example
```vue
<ct-input-wrapper @files="onFiles">
	<span>Click me</span>
</ct-input-wrapper>
```


