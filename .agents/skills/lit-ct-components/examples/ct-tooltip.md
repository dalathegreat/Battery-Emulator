---
outline: deep
---

# ct-tooltip

A flexible tooltip component that can be used via a custom element or simple `data-tooltip` attributes.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

There are two ways to use tooltips: using the `<ct-tooltip>` element or using the `tooltipStyles` with data attributes.

### Method 1: `<ct-tooltip>` Element

This is the recommended way for most use cases. The tooltip will automatically attach to the element specified by the `for` selector.

```ts
import "@conectate/components/ct-tooltip.js";

render() {
	return html`
		<div style="position: relative; display: inline-block;">
			<button id="myBtn">Hover me</button>
			<ct-tooltip for="#myBtn">This is a tooltip message</ct-tooltip>
		</div>
	`;
}
```

### Method 2: Data Attributes (CSS only)

If you prefer a lighter weight approach, you can import and apply `tooltipStyles` to your component's CSS.

```ts
import { tooltipStyles } from "@conectate/components/ct-tooltip.js";

// In your Lit styles
static styles = [
	tooltipStyles,
	css`...`
];

render() {
	return html`
		<button data-tooltip="Quick message">Help</button>
		<button data-tooltip="Align left" data-toleft>Left</button>
		<button data-tooltip="Show below" data-tobottom>Bottom</button>
	`;
}
```

## Properties (`<ct-tooltip>`)

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `for` | `for` | `string` | `""` | CSS selector for the target element |
| `open` | `open` | `boolean` | `false` | Reflects whether the tooltip is currently visible |
| `placement`| `placement`| `string` | `"top"` | Tooltip placement (currently defaults to top in implementation) |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--color-surface` | - | Background color of the tooltip |
| `--color-disable` | - | Box shadow color |
| `--border-radius` | - | Tooltip corner radius |
| `--font-family` | - | Font family (for data-tooltip method) |

## Accessibility (a11y)

Tooltips are triggered on `mouseenter` and hidden on `mouseleave` or `blur`. The `for` element automatically receives `tabindex="-1"` if it doesn't already have one to ensure it's detectable, although tooltips usually should be paired with focusable elements like buttons for keyboard accessibility.


