---
outline: deep
---

# ct-textarea-autogrow

A low-level textarea component that automatically expands its height as the user types, using a "mirror" div technique or modern CSS `field-sizing`.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

This component is often used internally by `ct-textarea`, but can be used standalone for custom implementations.

### Example with LitElement

```ts
import { html } from 'lit';
import "@conectate/components/ct-textarea-autogrow.js";

render() {
	return html`
		<ct-textarea-autogrow 
			placeholder="Type something..." 
			rows="2" 
			@value=${(e: any) => console.log(e.detail.value)}>
		</ct-textarea-autogrow>
	`;
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `value` | - | `string` | `""` | Current text value |
| `name` | `name` | `string` | `""` | Name of the textarea |
| `rows` | `rows` | `number` | `1` | Initial number of rows |
| `maxRows` | `maxRows` | `number` | `0` | Max rows before scrolling (0 = no limit) |
| `placeholder`| `placeholder`| `string`| `""` | Placeholder text |
| `readonly` | `readonly` | `boolean` | `false` | If true, user cannot edit |
| `disabled` | `disabled` | `boolean` | `false` | If true, input is disabled |
| `required` | `required` | `boolean` | `false` | If true, field is required |
| `autofocus` | `autofocus`| `boolean`| `false` | Auto focus on mount |
| `minlength` | `minlength`| `number` | - | Minimum character length |
| `maxlength` | `maxlength`| `number` | - | Maximum character length |
| `label` | `label` | `string` | `""` | ARIA label |

## Methods

| Method | Description |
| ------ | ----------- |
| `focus()` | Moves focus to the internal textarea |

## Events

| Event Name | Detail | Description |
| ---------- | ------ | ----------- |
| `value` | `{ value: string }`| Fired when the text content changes |

## Accessibility (a11y)

The component ensures accessibility by mapping properties to the native `<textarea>` and providing a mirror div for layout that is hidden from screen readers via `aria-hidden="true"`.


