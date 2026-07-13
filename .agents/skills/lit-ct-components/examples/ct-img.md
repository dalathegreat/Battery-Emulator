---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-img.js'
</script>

# ct-img

An advanced image loader component that provides lazy loading functionality with various display options.

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
import "@conectate/components/ct-img.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
  <div style="width: 300px; height: 200px; border: 1px solid #ddd;">
    <ct-img src="https://picsum.photos/300/200" alt="Random image"></ct-img>
  </div>
  <div style="width: 100px; height: 100px;">
    <ct-img src="https://picsum.photos/100/100" round alt="Profile picture"></ct-img>
  </div>
</div>

```vue
<template>
	<!-- Basic usage -->
	<ct-img src="image.jpg" alt="Description"></ct-img>

	<!-- Circular image -->
	<ct-img src="profile.jpg" round alt="Profile picture"></ct-img>

	<!-- Lazy loading -->
	<ct-img srcset="image.jpg" lazy alt="Description"></ct-img>
</template>

<script setup>
import "@conectate/components/ct-img";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-img.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-img
				src="image.jpg"
				lazy
				alt="Description"
				.viewport=${document.body}
			></ct-img>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-img.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-img': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					src?: string;
					srcset?: string;
					alt?: string;
					lazy?: boolean;
					round?: boolean;
					contain?: boolean;
					placeholderImg?: string;
					onErrorSrc?: string;
					viewport?: HTMLElement;
					'background-position'?: 'left' | 'right' | 'center';
				},
				HTMLElement
			>;
		}
	}
}

export function ImageComponent() {
	return (
		<ct-img 
			src="image.jpg" 
			alt="Description" 
			lazy 
			round 
		/>
	);
}
```

## Variants

- **Default**: Background cover (crops to fill container)
- **Contain**: Background contain (shows entire image)
- **Round**: Circular image (50% border radius)

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `src` | `src` | `string` | `""` | Image source URL |
| `srcset` | `srcset` | `string` | - | Image source set (use for lazy loading) |
| `alt` | `alt` | `string` | `""` | Alternative text |
| `lazy` | `lazy` | `boolean` | `false` | Enables lazy loading |
| `round` | `round` | `boolean` | `false` | Makes the image circular |
| `contain` | `contain` | `boolean` | `false` | Uses background-size: contain |
| `placeholderImg`| `placeholder-img`| `string`| `""`| Image to show while loading |
| `onErrorSrc` | `on-error-src` | `string` | `[SVG data URI]`| Image to show on error |
| `disable_anim` | `disable_anim` | `boolean` | `false` | Disables fade-in animation |
| `viewport` | - | `HTMLElement`| `document.body`| Scroll container for lazy loading |
| `background-position`| `background-position`| `string`| `"center"`| Position (left, right, center) |

## CSS Variables

```css
ct-img {
	/* Layout & Spacing */
	--ct-img-border-radius: 0px; /* Border radius of the image */
}
```

## Accessibility (a11y)

Always provide a descriptive `alt` text for images. If an image is purely decorative, you can use an empty string `alt=""`.

## Event Handling

| Event Name | Description |
| ---------- | ----------- |
| `loaded-changed` | Fired when the image has finished loading |

#### Vue Example
```vue
<ct-img src="image.jpg" @loaded-changed="onLoad"></ct-img>
```


