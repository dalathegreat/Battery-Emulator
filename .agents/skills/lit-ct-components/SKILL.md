---
name: lit-ct-components
description: Use Conectate Components (ct-*) when building UIs with Lit, Vue, or React. Covers @conectate/components web components (ct-button, ct-input, ct-dialog, ct-select, etc.), CtLit base class, imports, slots, CSS variables, and events. Use when the user mentions ct-components, Conectate Components, or building interfaces with this library.
---

# Conectate Components (ct-components)

Conectate Components is a library of web components published as `@conectate/components`. Use this skill when building UIs with Lit, Vue, or React and the project uses or should use these components.

## Installation

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Base class CtLit

For Lit-based components, extend `CtLit` instead of `LitElement` to get utilities:

```ts
import { CtLit, customElement, html } from "@conectate/components/ct-lit.js";

@customElement("my-component")
class MyComponent extends CtLit {
  render() {
    return html`<div>My component</div>`;
  }
}
```

- **`$$(selector)`** — first match in shadow root. **`$$$(selector)`** — all matches.
- **`fire(name, detail)`** — dispatch custom event with `detail`.
- **`scrollToY(scrollTargetY, time?, easing?, target?)`** — animated scroll.
- **`deepClone(object)`** — deep clone (structuredClone or JSON fallback).

Full API: [examples/ct-lit.md](examples/ct-lit.md).

## Imports

Always import the component script before use. One import per component:

```ts
import "@conectate/components/ct-button";
import "@conectate/components/ct-input";
import "@conectate/components/ct-dialog";
```

Path pattern: `@conectate/components/<component-name>`.

## Components (overview)

Use each tag only after importing its script. For full props, slots, and CSS variables, see the linked docs.

| Category          | Component                                          | Doc                                                                                                                                                       | Typical use                                                                              |
| ----------------- | -------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| Base              | CtLit                                              | [ct-lit.md](examples/ct-lit.md)                                                                                                                           | Base class for Lit components                                                            |
| Buttons           | ct-button                                          | [ct-button.md](examples/ct-button.md)                                                                                                                     | `raised`, `flat`, `shadow`, `light`; slots `prefix`/`suffix`                             |
|                   | ct-button-helpers                                  | [ct-button-helpers.md](examples/ct-button-helpers.md)                                                                                                     | Button helpers                                                                           |
|                   | ct-button-menu                                     | [ct-button-menu.md](examples/ct-button-menu.md)                                                                                                           | Button with dropdown menu                                                                |
|                   | ct-button-split                                    | [ct-button-split.md](examples/ct-button-split.md)                                                                                                         | Split button + menu                                                                      |
|                   | ct-icon-button                                     | [ct-icon-button.md](examples/ct-icon-button.md)                                                                                                           | Icon-only button                                                                         |
| Inputs            | ct-input                                           | [ct-input.md](examples/ct-input.md)                                                                                                                       | Text input with label, `errorMessage`, `charCounter`                                     |
|                   | ct-input-container, ct-input-wrapper               | [ct-input-container.md](examples/ct-input-container.md), [ct-input-wrapper.md](examples/ct-input-wrapper.md)                                              | Input layout / wrapper                                                                   |
|                   | ct-textarea                                        | [ct-textarea.md](examples/ct-textarea.md)                                                                                                                 | Multiline input                                                                          |
|                   | ct-textarea-autogrow                               | [ct-textarea-autogrow.md](examples/ct-textarea-autogrow.md)                                                                                               | Auto-growing textarea                                                                    |
|                   | ct-input-autocomplete, ct-autocomplete-suggestions | [ct-input-autocomplete.md](examples/ct-input-autocomplete.md), [ct-autocomplete-suggestions.md](examples/ct-autocomplete-suggestions.md)                  | Input with suggestions                                                                   |
|                   | ct-input-phone, ct-phone-input                     | [ct-input-phone.md](examples/ct-input-phone.md), [ct-phone-input.md](examples/ct-phone-input.md)                                                          | Phone number input                                                                       |
|                   | ct-date                                            | [ct-date.md](examples/ct-date.md)                                                                                                                         | Date picker                                                                              |
|                   | ct-checkbox                                        | [ct-checkbox.md](examples/ct-checkbox.md)                                                                                                                 | Checkbox                                                                                 |
|                   | ct-radio                                           | [ct-radio.md](examples/ct-radio.md)                                                                                                                       | Radio group                                                                              |
| Dialogs & overlay | ct-dialog                                          | [ct-dialog.md](examples/ct-dialog.md)                                                                                                                     | Modal via `showCtDialog(content)`; animations `alert`, `slide-right`, `bottom-sheet`     |
|                   | ct-dialog-builder                                  | [ct-dialog-builder.md](examples/ct-dialog-builder.md)                                                                                                     | Dialog builder                                                                           |
|                   | ct-card-dialog                                     | [ct-card-dialog.md](examples/ct-card-dialog.md)                                                                                                           | Card-style dialog                                                                        |
|                   | ct-bottom-sheet                                    | [ct-bottom-sheet.md](examples/ct-bottom-sheet.md)                                                                                                         | Bottom sheet                                                                             |
|                   | ct-confirm                                         | [ct-confirm.md](examples/ct-confirm.md)                                                                                                                   | Confirmation dialog                                                                      |
| Selection         | ct-select                                          | [ct-select.md](examples/ct-select.md)                                                                                                                     | Single/multi select; `items`, `value`, `searchable`; event `value` with `e.detail.value` |
|                   | ct-select-dialog, ct-select-item                   | [ct-select-dialog.md](examples/ct-select-dialog.md), [ct-select-item.md](examples/ct-select-item.md)                                                      | Building blocks for select UIs                                                           |
| Feedback          | ct-loading, ct-loading-bar, ct-loading-placeholder | [ct-loading.md](examples/ct-loading.md), [ct-loading-bar.md](examples/ct-loading-bar.md), [ct-loading-placeholder.md](examples/ct-loading-placeholder.md) | Loading states                                                                           |
|                   | ct-spinner                                         | [ct-spinner.md](examples/ct-spinner.md)                                                                                                                   | Spinner                                                                                  |
|                   | ct-snackbar                                        | [ct-snackbar.md](examples/ct-snackbar.md)                                                                                                                 | Toast/snackbar                                                                           |
| Layout & content  | ct-card                                            | [ct-card.md](examples/ct-card.md)                                                                                                                         | Card container                                                                           |
|                   | ct-menu                                            | [ct-menu.md](examples/ct-menu.md)                                                                                                                         | Menu / dropdown                                                                          |
|                   | ct-tabs, ct-tab                                    | [ct-tabs.md](examples/ct-tabs.md), [ct-tab.md](examples/ct-tab.md)                                                                                        | Tabs                                                                                     |
|                   | ct-collapse                                        | [ct-collapse.md](examples/ct-collapse.md)                                                                                                                 | Collapsible section                                                                      |
|                   | ct-list-item                                       | [ct-list-item.md](examples/ct-list-item.md)                                                                                                               | List item                                                                                |
|                   | ct-icon                                            | [ct-icon.md](examples/ct-icon.md)                                                                                                                         | Icon                                                                                     |
|                   | ct-img                                             | [ct-img.md](examples/ct-img.md)                                                                                                                           | Image                                                                                    |
|                   | ct-tooltip                                         | [ct-tooltip.md](examples/ct-tooltip.md)                                                                                                                   | Tooltip                                                                                  |
| Utilities         | ct-helpers                                         | [ct-helpers.md](examples/ct-helpers.md)                                                                                                                   | `getClient`, `getGeoLocation`, `sleep`, `PushID`                                         |
|                   | ct-scroll-threshold                                | [ct-scroll-threshold.md](examples/ct-scroll-threshold.md)                                                                                                 | Infinite scroll trigger                                                                  |
|                   | ct-chartjs, ct-qr-tools, ct-promp                  | [ct-chartjs.md](examples/ct-chartjs.md), [ct-qr-tools.md](examples/ct-qr-tools.md), [ct-promp.md](examples/ct-promp.md)                                   | Charts, QR, prompt                                                                       |

More components and containers: [reference.md](reference.md).

## Uso con Lit / Vue / React

- **Lit**: Import the component `.js`, then use the tag. Bind with `.property=${value}`, events with `@eventname=${handler}`. For custom events, `e.detail` holds the payload.
- **Vue**: Import the component `.js`. Use `v-model` where supported (e.g. ct-input, ct-select). Events: `@value`, `@input`, etc.
- **React**: Import the component `.js`. Use `onClick`, `onChange`, `onValue` (for custom events, `e.detail`). Declare JSX types for `ct-*` if needed (see docs examples).

### Minimal examples

**Lit (ct-button):**

```ts
import { LitElement, html } from "lit";
import "@conectate/components/ct-button.js";

class MyEl extends LitElement {
  render() {
    return html`<ct-button raised @click=${() => console.log("ok")}>Click</ct-button>`;
  }
}
```

**Vue (ct-input):**

```vue
<template>
	<ct-input label="Name" v-model="name" />
</template>
<script setup>
import "@conectate/components/ct-input.js";
import { ref } from "vue";
const name = ref("");
</script>
```

**React (ct-select):**

```tsx
import "@conectate/components/ct-select.js";

<ct-select label="Choice" items={items} value={val} onValue={(e) => setVal(e.detail.value)} />
```

For full examples per component, see [examples/](examples/) (e.g. [ct-button.md](examples/ct-button.md), [ct-input.md](examples/ct-input.md)).

## Slots

Many components use **default**, **prefix**, and **suffix** slots. Check each component’s doc for exact slot names and behavior.

```html
<ct-button raised>
	<span slot="prefix">↑</span>
	Label
	<span slot="suffix">↓</span>
</ct-button>
```

## CSS variables

Components are themed via CSS custom properties (e.g. `--color-primary`, `--border-radius`, `--ct-button-radius`). Set them on the host, a wrapper, or `:root`. See each component’s doc for the full list.

```css
ct-button {
	--color-primary: #00aeff;
	--ct-button-radius: 26px;
}
```

## Eventos

- Native events: `click`, `input`, `change`, `blur`, etc. Use as in standard HTML.
- Custom events carry data in `event.detail`. Example: ct-select fires `value` with `detail.value` (and `detail.old`). In Lit: `@value=${(e) => this.value = e.detail.value}`.

For event names and payloads per component, see the corresponding doc in [examples/](examples/) or [reference.md](reference.md).

## Resources

Index of components and their documentation. All paths are relative to this skill (e.g. `examples/ct-button.md`).

| Component / module          | Documentation                                                                      |
| --------------------------- | ---------------------------------------------------------------------------------- |
| **Base**                    |                                                                                    |
| CtLit                       | [examples/ct-lit.md](examples/ct-lit.md)                                           |
| **Buttons**                 |                                                                                    |
| ct-button                   | [examples/ct-button.md](examples/ct-button.md)                                     |
| ct-button-helpers           | [examples/ct-button-helpers.md](examples/ct-button-helpers.md)                     |
| ct-button-menu              | [examples/ct-button-menu.md](examples/ct-button-menu.md)                           |
| ct-button-split             | [examples/ct-button-split.md](examples/ct-button-split.md)                         |
| ct-icon-button              | [examples/ct-icon-button.md](examples/ct-icon-button.md)                           |
| **Inputs & forms**          |                                                                                    |
| ct-input                    | [examples/ct-input.md](examples/ct-input.md)                                       |
| ct-input-autocomplete       | [examples/ct-input-autocomplete.md](examples/ct-input-autocomplete.md)             |
| ct-input-container          | [examples/ct-input-container.md](examples/ct-input-container.md)                   |
| ct-input-phone              | [examples/ct-input-phone.md](examples/ct-input-phone.md)                           |
| ct-input-wrapper            | [examples/ct-input-wrapper.md](examples/ct-input-wrapper.md)                       |
| ct-phone-input              | [examples/ct-phone-input.md](examples/ct-phone-input.md)                           |
| ct-textarea                 | [examples/ct-textarea.md](examples/ct-textarea.md)                                 |
| ct-textarea-autogrow        | [examples/ct-textarea-autogrow.md](examples/ct-textarea-autogrow.md)               |
| ct-date                     | [examples/ct-date.md](examples/ct-date.md)                                         |
| ct-checkbox                 | [examples/ct-checkbox.md](examples/ct-checkbox.md)                                 |
| ct-radio                    | [examples/ct-radio.md](examples/ct-radio.md)                                       |
| ct-autocomplete-suggestions | [examples/ct-autocomplete-suggestions.md](examples/ct-autocomplete-suggestions.md) |
| **Dialogs & overlay**       |                                                                                    |
| ct-dialog                   | [examples/ct-dialog.md](examples/ct-dialog.md)                                     |
| ct-dialog-builder           | [examples/ct-dialog-builder.md](examples/ct-dialog-builder.md)                     |
| ct-card-dialog              | [examples/ct-card-dialog.md](examples/ct-card-dialog.md)                           |
| ct-bottom-sheet             | [examples/ct-bottom-sheet.md](examples/ct-bottom-sheet.md)                         |
| ct-confirm                  | [examples/ct-confirm.md](examples/ct-confirm.md)                                   |
| **Selection**               |                                                                                    |
| ct-select                   | [examples/ct-select.md](examples/ct-select.md)                                     |
| ct-select-dialog            | [examples/ct-select-dialog.md](examples/ct-select-dialog.md)                       |
| ct-select-item              | [examples/ct-select-item.md](examples/ct-select-item.md)                           |
| **Feedback & loading**      |                                                                                    |
| ct-loading                  | [examples/ct-loading.md](examples/ct-loading.md)                                   |
| ct-loading-bar              | [examples/ct-loading-bar.md](examples/ct-loading-bar.md)                           |
| ct-loading-placeholder      | [examples/ct-loading-placeholder.md](examples/ct-loading-placeholder.md)           |
| ct-spinner                  | [examples/ct-spinner.md](examples/ct-spinner.md)                                   |
| ct-snackbar                 | [examples/ct-snackbar.md](examples/ct-snackbar.md)                                 |
| **Layout & content**        |                                                                                    |
| ct-card                     | [examples/ct-card.md](examples/ct-card.md)                                         |
| ct-menu                     | [examples/ct-menu.md](examples/ct-menu.md)                                         |
| ct-tabs                     | [examples/ct-tabs.md](examples/ct-tabs.md)                                         |
| ct-tab                      | [examples/ct-tab.md](examples/ct-tab.md)                                           |
| ct-collapse                 | [examples/ct-collapse.md](examples/ct-collapse.md)                                 |
| ct-list-item                | [examples/ct-list-item.md](examples/ct-list-item.md)                               |
| ct-icon                     | [examples/ct-icon.md](examples/ct-icon.md)                                         |
| ct-img                      | [examples/ct-img.md](examples/ct-img.md)                                           |
| ct-tooltip                  | [examples/ct-tooltip.md](examples/ct-tooltip.md)                                   |
| **Utilities & other**       |                                                                                    |
| ct-helpers                  | [examples/ct-helpers.md](examples/ct-helpers.md)                                   |
| ct-scroll-threshold         | [examples/ct-scroll-threshold.md](examples/ct-scroll-threshold.md)                 |
| ct-chartjs                  | [examples/ct-chartjs.md](examples/ct-chartjs.md)                                   |
| ct-qr-tools                 | [examples/ct-qr-tools.md](examples/ct-qr-tools.md)                                 |
| ct-promp                    | [examples/ct-promp.md](examples/ct-promp.md)                                       |

For usage patterns, props, slots, CSS variables, and events, open the linked doc for each component.
