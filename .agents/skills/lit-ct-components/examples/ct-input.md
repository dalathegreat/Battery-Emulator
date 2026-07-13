---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-input.js'

const inputValue = ref('')
const emailValue = ref('')
const passwordValue = ref('')
const message = ref('')
</script>

# ct-input

A collection of customizable input components including text input, textarea, and autocomplete functionality.

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
import "@conectate/components/ct-input.js";
```

### Example with Vue

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
	<ct-input label="Username" v-model="inputValue"></ct-input>
	<ct-input type="password" label="Password" v-model="passwordValue"></ct-input>
	<ct-input type="email" label="Email" v-model="emailValue" errorMessage="Please enter a valid email"></ct-input>
	<ct-input label="Disabled Input" disabled value="Cannot edit"></ct-input>
</div>

```vue
<template>
	<ct-input label="Username" v-model="inputValue"></ct-input>
	<ct-input type="password" label="Password" v-model="passwordValue"></ct-input>
	<ct-input type="email" label="Email" v-model="emailValue" errorMessage="Please enter a valid email"></ct-input>
	<ct-input label="Disabled Input" disabled value="Cannot edit"></ct-input>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-input";
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-input.js";

@customElement("my-form")
class MyForm extends LitElement {
	render() {
		return html`
			<ct-input
				label="Username"
				@input=${this.handleInput}
				required
			></ct-input>
		`;
	}

	handleInput(e) {
		console.log("Input value:", e.target.value);
	}
}
```

### Example with React (TypeScript)

```tsx
import React, { useState } from 'react';
import "@conectate/components/ct-input.js";

// Declare the custom element for TypeScript
declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-input': React.DetailedHTMLProps<
				React.InputHTMLAttributes<HTMLInputElement> & {
					label?: string;
					errorMessage?: string;
					charCounter?: boolean;
					invalid?: boolean;
					required?: boolean;
					disabled?: boolean;
					readonly?: boolean;
					dark?: boolean;
					block?: boolean;
					noHover?: boolean;
					raiseOnActive?: boolean;
					raiseOnValue?: boolean;
					raiseForced?: boolean;
					onValue?: (event: CustomEvent) => void;
				},
				HTMLElement
			>;
		}
	}
}

export function MyComponent() {
	const [value, setValue] = useState('');

	const handleInput = (e: React.ChangeEvent<HTMLInputElement>) => {
		setValue(e.target.value);
		console.log('Input value:', e.target.value);
	};

	return (
		<ct-input
			label="Username"
			value={value}
			onChange={handleInput}
			required
		/>
	);
}
```

## Input Types

The input component supports various HTML input types:

<div style="display: flex; flex-direction: column; gap: 1rem; margin: 1rem 0;">
	<ct-input type="text" label="Text Input"></ct-input>
	<ct-input type="email" label="Email Input"></ct-input>
	<ct-input type="password" label="Password Input"></ct-input>
	<ct-input type="number" label="Number Input"></ct-input>
	<ct-input type="tel" label="Phone Input"></ct-input>
	<ct-input type="url" label="URL Input"></ct-input>
	<ct-input type="search" label="Search Input"></ct-input>
</div>

## Components

This package includes multiple input components:

- `ct-input`: Standard text input component
- `ct-textarea`: Multiline text input
- `ct-textarea-autogrow`: Textarea that grows with content
- `ct-input-autocomplete`: Input with autocomplete suggestions
- `ct-input-container`: Container component for inputs
- `ct-input-phone`: Phone number input
- `ct-autocomplete-suggestions`: Suggestions dropdown for autocomplete

## Properties

| Property       | Attribute      | Type               | Default                   | Description                             |
| -------------- | -------------- | ------------------ | ------------------------- | --------------------------------------- |
| `type`         | `type`         | `CtInputType`      | `"text"`                  | Input type (text, password, email, etc) |
| `value`        | `value`        | `string`           | `""`                      | Input value                             |
| `label`        | `label`        | `string`           | `""`                      | Label text                              |
| `placeholder`  | `placeholder`  | `string`           | `""`                      | Placeholder text                        |
| `errorMessage` | `errorMessage` | `string`           | `""`                      | Error message to display                |
| `disabled`     | `disabled`     | `boolean`          | `false`                   | Disables the input                      |
| `required`     | `required`     | `boolean`          | `false`                   | Makes the input required                |
| `pattern`      | `pattern`      | `string \| RegExp` | `""`                      | Validation pattern                      |
| `charCounter`  | `charCounter`  | `boolean`          | `false`                   | Show character counter                  |
| `maxlength`    | `maxlength`    | `number`           | `Number.MAX_SAFE_INTEGER` | Maximum input length                    |
| `invalid`      | `invalid`      | `boolean`          | `false`                   | Whether the input is invalid            |
| `readonly`     | `readonly`     | `boolean`          | `false`                   | Makes the input readonly                |
| `dark`         | `dark`         | `boolean`          | `false`                   | Dark theme variant                      |
| `block`        | `block`        | `boolean`          | `false`                   | Display as block element                |
| `autofocus`    | `autofocus`    | `boolean`          | `false`                   | Automatically focus on mount            |
| `inputmode`    | `inputmode`    | `CtInputMode`      | `undefined`               | Hint for virtual keyboard               |
| `minlength`    | `minlength`    | `number`           | `0`                       | Minimum input length                    |
| `min`          | `min`          | `number`           | `undefined`               | Minimum value (for number/date types)   |
| `max`          | `max`          | `number`           | `undefined`               | Maximum value (for number/date types)   |
| `step`         | `step`         | `number`           | `undefined`               | Step value (for number/date types)      |
| `name`         | `name`         | `string`           | `undefined`               | Input name for forms                    |
| `autocomplete` | `autocomplete` | `string`           | `"off"`                   | Autocomplete attribute                  |
| `size`         | `size`         | `number`           | `24`                      | Input size attribute                    |

## Slots

- **Default slot**: Content inside the input (rarely used)
- **prefix**: Content placed at the start of the input
- **suffix**: Content placed at the end of the input

### Example with Slots

<div style="margin: 1rem 0;">
	<ct-input label="Search">
		<span slot="prefix">🔍</span>
		<span slot="suffix">✓</span>
	</ct-input>
</div>

```html
<ct-input label="Search">
	<span slot="prefix">🔍</span>
	<span slot="suffix">✓</span>
</ct-input>
```

## CSS Variables

You can customize the component using the following CSS variables:

```css
ct-input {
	/* Primary Colors */
	--color-primary: #2cb5e8; /* Primary color for focus and underline */
	--color-error: #ed4f32; /* Error color for invalid state */
	--default-color-active: #1a396008; /* Background color when active/focused */

	/* Text Colors */
	--color-on-surface: #535353; /* Text color on surface */
	--ct-input-color: inherit; /* Input text color */
	--ct-input-placeholder-color: rgba(0, 0, 0, 0.38); /* Placeholder color */

	/* Background Colors */
	--ct-input-background: transparent; /* Input background */

	/* Layout & Spacing */
	--border-radius: 16px; /* Border radius of the input container */
	--ct-input-height: 3.3em; /* Input height */
	--ct-input-padding: 0em 1em; /* Input padding */

	/* Typography */
	--ct-input-font-family: inherit; /* Input font family */
	--ct-input-font-weight: inherit; /* Input font weight */
	--ct-label-font-size: 12px; /* Label font size */

	/* Indicators */
	--ct-indicator: "*"; /* Required field indicator character */

	/* Search Input */
	--ct-input-search-cancel-button-mask-image: url("data:image/svg+xml..."); /* Search cancel button icon */
	--ct-input-search-cancel-button-background-size: 1.5em 1.5em; /* Search cancel button size */

	/* Readonly */
	--ct-input--readonly-cursor: not-allowed; /* Cursor for readonly inputs */
}
```

### CSS Variables Reference Table

| Variable                                          | Default                        | Description                                   |
| ------------------------------------------------- | ------------------------------ | --------------------------------------------- |
| `--color-primary`                                 | `#2cb5e8`                      | Primary color for focus underline and caret   |
| `--color-error`                                   | `#ed4f32`                      | Error color for invalid state and error text  |
| `--default-color-active`                          | `#1a396008`                    | Background color when input is active/focused |
| `--color-on-surface`                              | `#535353`                      | Text color on surface (label and text)        |
| `--ct-input-color`                                | `inherit`                      | Input text color                              |
| `--ct-input-placeholder-color`                    | `rgba(0, 0, 0, 0.38)`          | Placeholder text color                        |
| `--ct-input-background`                           | `transparent`                  | Input background color                        |
| `--border-radius`                                 | `16px`                         | Border radius of the input container          |
| `--ct-input-height`                               | `3.3em`                        | Height of the input element                   |
| `--ct-input-padding`                              | `0em 1em`                      | Internal padding of the input container       |
| `--ct-input-font-family`                          | `inherit`                      | Font family for the input text                |
| `--ct-input-font-weight`                          | `inherit`                      | Font weight for the input text                |
| `--ct-label-font-size`                            | `12px`                         | Font size for the label                       |
| `--ct-indicator`                                  | `"*"`                          | Character shown for required fields           |
| `--ct-input-search-cancel-button-mask-image`      | `url("data:image/svg+xml...")` | Mask image for search input cancel button     |
| `--ct-input-search-cancel-button-background-size` | `1.5em 1.5em`                  | Size of the search cancel button              |
| `--ct-input--readonly-cursor`                     | `not-allowed`                  | Cursor style for readonly inputs              |

### Custom Styling Examples

<div style="margin: 1rem 0;">
	<div style="display: flex; flex-direction: column; gap: 1rem; margin-bottom: 1rem;">
		<ct-input label="Custom Primary Color" style="--color-primary: #ff6b6b; --border-radius: 8px;"></ct-input>
		<ct-input label="Custom Height" style="--ct-input-height: 4em; --ct-input-padding: 0.5em 1em;"></ct-input>
		<ct-input label="Custom Error Color" style="--color-error: #ff9800;" required errorMessage="Custom error color"></ct-input>
	</div>
</div>

```html
<!-- Custom primary color and border radius -->
<ct-input label="Custom Primary Color" style="--color-primary: #ff6b6b; --border-radius: 8px;"></ct-input>

<!-- Custom height and padding -->
<ct-input label="Custom Height" style="--ct-input-height: 4em; --ct-input-padding: 0.5em 1em;"></ct-input>

<!-- Custom error color -->
<ct-input label="Custom Error Color" style="--color-error: #ff9800;" errorMessage="Custom error color"></ct-input>
```

## Validation

The component includes built-in validation support:

<div style="margin: 1rem 0;">
	<ct-input label="Email" type="email" required pattern="[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$" errorMessage="Please enter a valid email address"></ct-input>
</div>

```html
<ct-input label="Email" type="email" required pattern="[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$" errorMessage="Please enter a valid email address"></ct-input>
```

You can also validate programmatically:

```js
const input = document.querySelector('ct-input');
const isValid = input.validate();
```

## Character Counter

Enable character counter to show remaining characters:

<div style="margin: 1rem 0;">
	<ct-input label="Message" charCounter maxlength="100" placeholder="Type your message"></ct-input>
</div>

```html
<ct-input label="Message" charCounter maxlength="100" placeholder="Type your message"></ct-input>
```

## Accessibility (a11y)

The component supports proper focus states, keyboard navigation, and aria attributes for accessibility. Use `aria-label` for improved accessibility when needed.

### Example with aria-label

```html
<ct-input label="Username" aria-label="Enter your username"></ct-input>
```

## Event Handling

The input supports standard input events and can be used with any framework:

<div style="margin: 1rem 0;">
	<p v-if="message" style="color: green; font-weight: bold;">{{ message }}</p>
	<ct-input label="Type something" @input="message = 'Input changed!'" @blur="message = 'Input blurred!'"></ct-input>
</div>

#### Vue Example

```vue
<template>
	<ct-input label="Username" @input="handleInput"></ct-input>
</template>

<script setup>
import { ref } from 'vue'
import "@conectate/components/ct-input";

const handleInput = (e) => {
	console.log('Input value:', e.target.value);
}
</script>
```

#### LitElement Example

```ts
import { LitElement, html } from "lit";
import { customElement } from "lit/decorators.js";
import "@conectate/components/ct-input.js";

@customElement("my-element")
class MyElement extends LitElement {
	render() {
		return html`
			<ct-input label="Username" @input=${this.handleInput}></ct-input>
		`;
	}

	handleInput(e) {
		console.log("Input value:", e.target.value);
	}
}
```

#### React Example

```tsx
import React from 'react';
import "@conectate/components/ct-input.js";

function MyComponent() {
	const handleInput = (e: React.ChangeEvent<HTMLInputElement>) => {
		console.log('Input value:', e.target.value);
	};

	return (
		<ct-input label="Username" onChange={handleInput} />
	);
}
```

## Form Integration

The component integrates with HTML forms and supports form validation:

```html
<form @submit.prevent="handleSubmit">
	<ct-input name="username" label="Username" required></ct-input>
	<ct-input name="email" type="email" label="Email" required></ct-input>
	<ct-button type="submit">Submit</ct-button>
</form>
```
