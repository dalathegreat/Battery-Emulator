---
outline: deep
---

# CtDialogBuilder

A builder class for creating and configuring dialogs with a fluent API, following Material Design patterns.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

`CtDialogBuilder` allows you to construct complex dialogs without manually managing the internal component structure.

```ts
import CtDialogBuilder from "@conectate/components/ct-dialog-builder.js";

const builder = new CtDialogBuilder();

builder
	.title("Confirm Changes")
	.content("Are you sure you want to save the changes made to this document?")
	.positiveButton("Save")
	.negativeButton("Discard")
	.onDismiss(() => console.log("Dialog closed"))
	.show();
```

## API Reference

### Constructor

#### `new CtDialogBuilder(container?, dialog?)`

- `container`: The HTMLElement where the dialog will be attached (defaults to `document.body`).
- `dialog`: Optional existing `CtDialog` instance.

### Configuration Methods

All configuration methods return the builder instance for chaining.

| Method | Description |
| ------ | ----------- |
| `title(text)` | Sets the dialog title |
| `content(text)` | Sets the main content text |
| `icon(svg)` | Sets an icon for the header (as SVG string) |
| `cornerRadius(px)` | Sets the border radius of the dialog |

### Button Methods

| Method | Description |
| ------ | ----------- |
| `positiveButton(text)` | Adds a primary action button |
| `negativeButton(text)` | Adds a secondary action button |
| `neutralButton(text)` | Adds a tertiary action button |

### List Methods

| Method | Description |
| ------ | ----------- |
| `listItems(items)` | Adds a simple list of string items |
| `listItemsSingleChoice(items)` | Adds a single-choice list |
| `listItemsMultiChoice(items)` | Adds a multi-choice list |
| `customListAdapter(items, renderFn)` | Adds a custom list with a render function |

### Event Callbacks

| Method | Description |
| ------ | ----------- |
| `onPreShow(fn)` | Called just before the dialog is displayed |
| `onPostShow(fn)` | Called immediately after the dialog is displayed |
| `onDismiss(fn)` | Called when the dialog is closed or cancelled |

### Control Methods

| Method | Description |
| ------ | ----------- |
| `show()` | Displays the dialog and returns the dialog instance |
| `dismiss()` | Programmatically closes the dialog |

## Example: Custom List

```ts
import { html } from 'lit';
import CtDialogBuilder from "@conectate/components/ct-dialog-builder.js";

const items = [
	{ id: 1, name: 'Apple' },
	{ id: 2, name: 'Banana' },
	{ id: 3, name: 'Cherry' }
];

new CtDialogBuilder()
	.title("Select Fruit")
	.customListAdapter(items, (item, index) => html`
		<div style="padding: 12px; border-bottom: 1px solid #eee;">
			<strong>${item.name}</strong> (ID: ${item.id})
		</div>
	`)
	.positiveButton("Done")
	.show();
```


