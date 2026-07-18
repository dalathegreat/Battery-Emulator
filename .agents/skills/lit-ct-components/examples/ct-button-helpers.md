---
outline: deep
---

# ct-button-helpers

A collection of utility functions to enhance button behavior and accessibility, particularly for keyboard navigation.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the helper functions:

```ts
import { rovingIndex } from "@conectate/components/ct-button-helpers.js";
```

## Roving Index Helper

The `rovingIndex` function implements the "Roving tabindex" pattern, which is commonly used in menus and tabs to manage focus using arrow keys.

### Usage Example

```ts
import { rovingIndex } from "@conectate/components/ct-button-helpers.js";

const menu = document.querySelector('.my-menu');
const items = menu.querySelectorAll('button');

rovingIndex({
	element: menu,    // The container element
	targets: items    // The focusable children
});
```

### Parameters

The `rovingIndex` function accepts an object with the following properties:

| Property  | Type                          | Description                                                                 |
| --------- | ----------------------------- | --------------------------------------------------------------------------- |
| `element` | `HTMLElement`                 | The container element that will listen for focus and keydown events         |
| `target`  | `string` (Optional)           | A CSS selector to find target elements if `targets` is not provided         |
| `targets` | `HTMLElement[]` (Optional)    | An array or NodeList of elements that should be part of the focus rotation |

## How it Works

1. **Initialization**: It sets `tabindex="-1"` on the container and all children, except for the first child which gets `tabindex="0"`.
2. **Keyboard Navigation**:
   - `ArrowRight` or `ArrowDown`: Moves focus to the next item.
   - `ArrowLeft` or `ArrowUp`: Moves focus to the previous item.
3. **Focus Management**: Only the currently active item has `tabindex="0"`, making the entire group behave as a single tab stop in the document flow.

## Use Cases

- **Menus**: Use it in dropdown menus like `ct-button-menu` to allow users to navigate options with arrow keys.
- **Tabs**: Use it in tab lists to switch between tabs using the keyboard.
- **Button Groups**: Use it in a group of related buttons for improved accessibility.

## Internal State

The helper maintains an internal state using a `Map` that tracks the active item and index for each container element.

```ts
interface RoverOptions {
	targets: NodeListOf<HTMLElement> | HTMLElement[];
	active: HTMLElement;
	index: number;
}
```

## Cleanup

The helper automatically cleans up its event listeners when the container element is removed from the document using the `DOMNodeRemovedFromDocument` event.


