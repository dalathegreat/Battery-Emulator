---
outline: deep
---

<script setup>
import { ref } from 'vue'
import '@conectate/components/ct-snackbar.js'
import { showSnackBar } from '@conectate/components/ct-snackbar.js'

const triggerMsg = () => {
  showSnackBar("This is a notification message!")
}

const triggerQueue = () => {
  showSnackBar("First message")
  showSnackBar("Second message in queue")
}
</script>

# ct-snackbar

A notification component that displays brief messages at the bottom of the screen.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components
# or
npm i @conectate/components
```

## Basic Usage

Import the helper function or the component:

```ts
import { showSnackBar } from "@conectate/components/ct-snackbar.js";
```

### Using Helper Function

The easiest way to show a message is using the global `showSnackBar` function.

```ts
showSnackBar("Operation successful!");
```

### Example with Vue

<div style="margin: 1rem 0; display: flex; gap: 1rem;">
  <button @click="triggerMsg" style="padding: 8px 16px; cursor: pointer; background: #3c3f41; color: white; border: none; border-radius: 4px;">
    Show Snackbar
  </button>
  <button @click="triggerQueue" style="padding: 8px 16px; cursor: pointer; background: #3c3f41; color: white; border: none; border-radius: 4px;">
    Trigger Queue
  </button>
</div>

```vue
<script setup>
import { showSnackBar } from "@conectate/components/ct-snackbar.js";

const notify = () => {
	showSnackBar("You have a new notification");
};
</script>

<template>
	<button @click="notify">Notify Me</button>
</template>
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import { showSnackBar } from "@conectate/components/ct-snackbar.js";

export function NotifyButton() {
	return (
		<button onClick={() => showSnackBar("Action completed")}>
			Save Changes
		</button>
	);
}
```

## API Reference

### `showSnackBar(msg: string)`

Utility function that creates (if not exists) and opens a `ct-snackbar` instance.

### Properties (`CtSnackbar`)

| Property | Attribute | Type | Default | Description |
| -------- | --------- | ---- | ------- | ----------- |
| `msg` | `msg` | `string` | `""` | Message to display |

### Methods (`CtSnackbar`)

| Method | Description |
| ------ | ----------- |
| `open(msg)`| Opens the snackbar with the given message. Handles queuing if already open. |
| `close()` | Closes the current snackbar and shows the next one in queue (if any). |

## Behavior

- **Queuing**: If `showSnackBar` is called while a message is already visible, the new message is added to a queue and displayed after the current one closes.
- **Auto-close**: Messages automatically disappear after 4 seconds.
- **Manual close**: Clicking the snackbar message dismisses it immediately.

## CSS Variables

| Variable | Default | Description |
| -------- | ------- | ----------- |
| `--color-on-background`| `#3c3f41`| Background color |
| `--color-background` | `#e9e9e9`| Text color |

## Accessibility (a11y)

The snackbar uses a high `z-index` and is positioned at the bottom of the viewport. It's recommended to use snackbars for non-critical information that doesn't require immediate user action.


