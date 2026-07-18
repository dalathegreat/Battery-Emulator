---
outline: deep
---

# ct-lit

A base wrapper class for `LitElement` that provides additional utility methods for common operations like element selection, event dispatching, and scrolling animations.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Extend `CtLit` instead of `LitElement` to access the utility methods:

```ts
import { CtLit, customElement, html } from "@conectate/components/ct-lit.js";

@customElement("my-component")
class MyComponent extends CtLit {
	render() {
		return html`<div>My component</div>`;
	}
}
```

## API Reference

### Methods

#### `$$(selector)`
Returns the first element that matches the selector within the component's shadow root.
- **Parameters**: `selector: string`
- **Returns**: `HTMLElement | Element | null`

#### `$$$(selector)`
Returns all elements that match the selector within the component's shadow root.
- **Parameters**: `selector: string`
- **Returns**: `NodeListOf<Element> | undefined`

#### `fire(name, detail)`
Dispatches a custom event with the specified name and detail object.
- **Parameters**: 
  - `name: string`: Event name
  - `detail: any`: Data to include in `event.detail`

#### `deepClone(object)`
Creates a deep clone of a native object or array using `structuredClone` (if available) or `JSON.stringify/parse`.
- **Parameters**: `object: T`
- **Returns**: `T`

#### `scrollToY(scrollTargetY, time?, easing?, target?)`
Performs a smooth animated scroll to a specific vertical position.
- **Parameters**:
  - `scrollTargetY: number`: Target position in pixels (default: 0)
  - `time: number`: Animation duration in ms (default: 600)
  - `easing: string`: Easing function (default: "easeInOutCubic")
  - `target: Element`: Element to scroll (default: window.scrollTarget)

### Decorators

`ct-lit` re-exports common Lit decorators for convenience:
- `@customElement` (enhanced with legacy browser support)
- `@property`
- `@state`
- `@query`, `@queryAll`, `@queryAsync`
- `@queryAssignedNodes`

## Helper Functions

### `If(condition, template)`
A template helper for conditional rendering in Lit.

```ts
import { If } from "@conectate/components/ct-lit.js";

render() {
  return html`
    ${If(this.showContent, html`<p>Content visible</p>`)}
  `;
}
```


