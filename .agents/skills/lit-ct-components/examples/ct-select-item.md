---
outline: deep
---

# ct-select-item

A helper component for displaying individual items within a selection list, commonly used with `ct-select` and `ct-select-dialog`.

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
import "@conectate/components/ct-select-item.js";
```

### Example with LitElement

```ts
import { html } from 'lit';
import "@conectate/components/ct-select-item.js";

render() {
  return html`
    <ct-select-item ?selected=${this.isSelected} ?multi=${this.isMulti}>
      Option Label
    </ct-select-item>
  `;
}
```

## Properties

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `selected` | `selected` | `boolean` | `false` | Whether the item is currently selected |
| `multi` | `multi` | `boolean` | `false` | If true, shows a selection indicator (check icon) even when not selected (if logic allows) |
| `value` | `value` | `number` | `0` | Internal value for the item |

## Slots

| Name | Description |
| ---- | ----------- |
| `default` | The content/label to display for the item |

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--color-surface` | `#fff` | Background color |
| `--color-outline` | `#89898936` | Bottom border color |
| `--color-on-surface` | `#535353` | Text color |
| `--color-primary` | `#2cb5e8` | Selection indicator background |
| `--color-on-primary` | `#fff` | Selection indicator icon color |
| `--color-primary-light` | `#2cb5e82b` | Selection indicator background (deselected multi) |

## Accessibility (a11y)

The component uses a `<button>` internally to ensure focusability and standard interaction patterns.


