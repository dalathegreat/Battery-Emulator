---
outline: deep
---

# ct-tab

An individual tab component designed to be used as a child of `ct-tabs`.

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
import "@conectate/components/ct-tab.js";
```

### Example with LitElement

```ts
import { html } from 'lit';
import "@conectate/components/ct-tab.js";

render() {
	return html`
		<ct-tab ?selected=${this.isActive}>
			General Settings
		</ct-tab>
	`;
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `selected` | `selected` | `boolean` | `false` | Reflects whether the tab is currently active |

## Slots

| Name | Description |
| ---- | ----------- |
| `default` | The label or content of the tab |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--ct-tabs-border-color`| `--color-primary`| Color of the indicator line and text when selected |
| `--color-primary` | `#0e92c1` | Fallback primary color |

## Accessibility (a11y)

When used inside `ct-tabs`, this component helps form a standards-compliant tabbed interface. It's recommended to use semantic labels inside the tab.


