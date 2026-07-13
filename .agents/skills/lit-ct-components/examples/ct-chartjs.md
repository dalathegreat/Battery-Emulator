---
outline: deep
---

<script setup>
import { ref, onMounted } from 'vue'
import '@conectate/components/ct-chartjs.js'

const chartData = {
  labels: ['January', 'February', 'March', 'April', 'May', 'June'],
  datasets: [{
    label: 'Sales',
    data: [12, 19, 3, 5, 2, 3],
    backgroundColor: 'rgba(0, 174, 255, 0.2)',
    borderColor: 'rgba(0, 174, 255, 1)',
    borderWidth: 1
  }]
}
</script>

# ct-chartjs

A web component wrapper for the popular [Chart.js](https://www.chartjs.org/) library.

## Installation

Install via npm or pnpm:

```sh
pnpm i @conectate/components chart.js
# or
npm i @conectate/components chart.js
```

## Basic Usage

Import the component and ensure `chart.js` is available in your project.

```ts
import "@conectate/components/ct-chartjs.js";
```

### Example with Vue

<div style="margin: 1rem 0; max-width: 600px;">
  <ct-chartjs type="bar" :data="chartData" autopaint></ct-chartjs>
</div>

```vue
<template>
	<ct-chartjs type="bar" :data="chartData" autopaint></ct-chartjs>
</template>

<script setup>
import "@conectate/components/ct-chartjs.js";

const chartData = {
  labels: ['Red', 'Blue', 'Yellow', 'Green', 'Purple', 'Orange'],
  datasets: [{
    label: '# of Votes',
    data: [12, 19, 3, 5, 2, 3],
    backgroundColor: [
      'rgba(255, 99, 132, 0.2)',
      'rgba(54, 162, 235, 0.2)',
      'rgba(255, 206, 86, 0.2)',
      'rgba(75, 192, 192, 0.2)',
      'rgba(153, 102, 255, 0.2)',
      'rgba(255, 159, 64, 0.2)'
    ],
    borderColor: [
      'rgba(255, 99, 132, 1)',
      'rgba(54, 162, 235, 1)',
      'rgba(255, 206, 86, 1)',
      'rgba(75, 192, 192, 1)',
      'rgba(153, 102, 255, 1)',
      'rgba(255, 159, 64, 1)'
    ],
    borderWidth: 1
  }]
};
</script>
```

### Example with LitElement (TypeScript)

```ts
import { LitElement, customElement, html } from "lit";
import "@conectate/components/ct-chartjs.js";

@customElement("my-element")
class MyElement extends LitElement {
	chartData = {
		labels: ['A', 'B', 'C'],
		datasets: [{ data: [10, 20, 30] }]
	};

	render() {
		return html`
			<ct-chartjs type="pie" .data=${this.chartData} autopaint></ct-chartjs>
		`;
	}
}
```

### Example with React (TypeScript)

```tsx
import React from 'react';
import "@conectate/components/ct-chartjs.js";

declare module "react/jsx-runtime" {  // preact/jsx-runtime
    namespace JSX {
        interface IntrinsicElements {
			'ct-chartjs': React.DetailedHTMLProps<
				React.HTMLAttributes<HTMLElement> & {
					type?: string;
					data?: any;
					options?: any;
					autopaint?: boolean;
					delay?: number;
					autoadjust?: boolean;
					x_?: number;
					y_?: number;
				},
				HTMLElement
			>;
		}
	}
}

export function MyChart() {
	const data = {
		labels: ['Jan', 'Feb', 'Mar'],
		datasets: [{ label: 'Data', data: [1, 2, 3] }]
	};

	return (
		<div style={{ maxWidth: '500px' }}>
			<ct-chartjs type="line" data={data} autopaint />
		</div>
	);
}
```

## Properties

| Property     | Attribute    | Type      | Default | Description                                   |
| ------------ | ------------ | --------- | ------- | --------------------------------------------- |
| `type`       | `type`       | `string`  | -       | Chart type (bar, line, pie, etc.)             |
| `data`       | -            | `object`  | -       | Chart.js data object                          |
| `options`    | -            | `object`  | `{}`    | Chart.js options object                       |
| `delay`      | `delay`      | `number`  | `0`     | Delay in ms before initializing (if > 0)      |
| `autopaint`  | `autopaint`  | `boolean` | `false` | Automatically initialize and paint on load    |
| `autoadjust` | `autoadjust` | `boolean` | `false` | Adjust height based on label count            |
| `x_`         | `x_`         | `number`  | `400`   | Initial width (max-width)                     |
| `y_`         | `y_`         | `number`  | `400`   | Initial height                                |

## Methods

| Method        | Description                                     |
| ------------- | ----------------------------------------------- |
| `init()`      | Initializes the chart instance                  |
| `paint()`     | Updates the chart or creates it if not exists   |
| `updateChart()` | Manually triggers a chart update              |
| `observe(obj)`| Wraps an object in a Proxy to auto-update chart |

## CSS Variables

The component uses internal styles for the canvas container.

```css
ct-chartjs {
	/* Layout */
	max-width: 900px;
	margin: 0 auto;
}

/* Custom size styling is handled via sizeChart property internally */
```

## Accessibility (a11y)

Chart.js generates a canvas element. It's recommended to provide a text alternative or a data table for users who cannot see the chart. Use `aria-label` on the component for a high-level description.

## Event Handling

The component doesn't emit specific events, but you can access the `chart` property to interact with the Chart.js instance directly.


