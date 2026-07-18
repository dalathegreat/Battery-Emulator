---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-date.js'

const dateVal = ref(Math.floor(Date.now() / 1000))
const dateTimeVal = ref(Math.floor(Date.now() / 1000))

const handleDateChange = (e) => {
  dateVal.value = e.detail
}
</script>

# ct-date

A flexible date and time input component that provides a consistent cross-platform experience.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the component and use it in your templates:

```ts
import "@conectate/components/ct-date.js";
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; flex-direction: column; gap: 1rem;">
  <ct-date label="Basic Date" @value="handleDateChange"></ct-date>
  <ct-date label="Date with Time" showhour @value="dateTimeVal = $event.detail"></ct-date>
  <ct-date label="Month/Year Only" nodd></ct-date>
</div>
<p style="color: #666;">
  Date Value: {{ dateVal }} (Unix Timestamp)<br>
  Date/Time Value: {{ dateTimeVal }}
</p>

```vue
<template>
	<ct-date 
		label="Select Date" 
		:value="myDate"
		@value="myDate = $event.detail">
	</ct-date>

	<ct-date 
		label="Select Date and Time" 
		showhour
		@value="handleDateTime">
	</ct-date>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-date.js";

const myDate = ref(1607731200); // Unix timestamp in seconds
const handleDateTime = (e) => console.log("New timestamp:", e.detail);
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html, state } from "lit";
import "@conectate/components/ct-date.js";

@customElement("my-element")
class MyElement extends LitElement {
	@state() private timestamp = Math.floor(Date.now() / 1000);

	render() {
		return html`
			<ct-date 
				label="Event Date" 
				.value=${this.timestamp}
				@value=${(e: CustomEvent) => this.timestamp = e.detail}>
			</ct-date>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-date.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-date': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					label?: string;
					value?: number | string;
					showhour?: boolean;
					nodd?: boolean;
					required?: boolean;
					placeholder?: string;
					usetimezone?: boolean;
					onValue?: (e: CustomEvent<number>) => void;
				},
				HTMLElement
			>;
		}
	}
}

export function DatePicker() {
	const [value, setValue] = useState<number>();

	return (
		<ct-date 
			label="Pick a date" 
			value={value} 
			onValue={(e) => setValue(e.detail)} 
		/>
	);
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `label` | `label` | `string` | `""` | Label text |
| `value` | `value` | `number \| string` | - | Current timestamp (seconds) or date string |
| `showhour` | `showhour` | `boolean` | `false` | Enable time selection (HH:mm) |
| `nodd` | `nodd` | `boolean` | `false` | Hide the day input (month/year only) |
| `required` | `required` | `boolean` | `false` | Mark as required for validation |
| `usetimezone`| `usetimezone`| `boolean` | `false` | Use local timezone for parsing/output |
| `minYYYY` | `minYYYY` | `number` | `1800` | Minimum allowed year |
| `maxYYYY` | `maxYYYY` | `number` | `2300` | Maximum allowed year |
| `placeholder`| `placeholder`| `string` | `""` | Input placeholder |
| `invalid` | `invalid` | `boolean` | `false` | Current validation state |

## Methods

| Method | Description |
| ------ | ----------- |
| `validate()` | Performs validation and returns `true` if valid |
| `loadValue(val)` | Manually load a value into the component |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `value` | `number` | Fired when a valid date is selected, returns timestamp in seconds |

## Features

- **Pasting Support**: You can paste date strings like "12/12/2023" or "2023-12-12 14:30" and the component will automatically parse and fill the fields.
- **Auto-Focus**: Moves focus automatically from Day to Month and Month to Year as you type.
- **Validation**: Includes built-in range validation for months (1-12), days (1-31), and years.

## CSS Variables

The component uses internal styles and relies on `ct-input-container` for the outer structure.

```css
ct-date {
	/* Standard text color */
	color: rgba(0, 0, 0, 0.54);
}
```

## Accessibility (a11y)

The component uses standard `<input type="tel">` for easy number entry on mobile devices and provides proper labels for screen readers.


