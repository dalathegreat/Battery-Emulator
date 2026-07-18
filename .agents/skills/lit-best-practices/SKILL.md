---
name: lit-best-practices
description: Lit web components best practices and performance optimization guidelines. Use when writing, reviewing, or refactoring Lit web components. Triggers on tasks involving Lit components, custom elements, shadow DOM, reactive properties, or web component performance.
license: MIT
author: community
version: 1.0.0
---

# Lit Web Components Best Practices

Best practices for building Lit web components, optimized for AI-assisted code generation and review.

## When to Use

Reference these guidelines when:

- Writing new Lit web components
- Implementing reactive properties and state
- Reviewing code for performance or accessibility issues
- Refactoring existing Lit components
- Optimizing rendering and update cycles

## Rule Categories

| Category | Rules | Focus |
|----------|-------|-------|
| 1. Component Structure | 4 rules | Properties, state, TypeScript |
| 2. Rendering | 5 rules | Templates, directives, derived state |
| 3. Styling | 4 rules | Static styles, theming, CSS parts |
| 4. Events | 3 rules | Custom events, naming, cleanup |
| 5. Lifecycle | 4 rules | Callbacks, timing, async |
| 6. Accessibility | 3 rules | ARIA, focus, forms |
| 7. Performance | 4 rules | Updates, caching, lazy loading |

## Priority Levels

| Priority | Description | Action |
|----------|-------------|--------|
| **CRITICAL** | Major correctness or accessibility issues | Fix immediately |
| **HIGH** | Significant maintainability/performance impact | Address in current PR |
| **MEDIUM** | Best practice violations | Address when touching related code |
| **LOW** | Style preferences, micro-optimizations | Consider during refactoring |

## Rules Index

### 1. Component Structure
- `rules/1-1-use-decorators.md` - Use TypeScript Decorators (HIGH)
- `rules/1-2-separate-state.md` - Separate Public Properties from Internal State (HIGH)
- `rules/1-3-reflect-sparingly.md` - Reflect Properties Sparingly (MEDIUM)
- `rules/1-4-default-values.md` - Always Provide Default Values (HIGH)

### 2. Rendering
- `rules/2-1-pure-render.md` - Keep render() Pure (CRITICAL)
- `rules/2-2-use-nothing.md` - Use nothing for Empty Content (MEDIUM)
- `rules/2-3-use-repeat.md` - Use repeat() for Keyed Lists (HIGH)
- `rules/2-4-use-cache.md` - Use cache() for Conditional Subtrees (MEDIUM)
- `rules/2-5-derived-state.md` - Compute Derived State in willUpdate() (HIGH)

### 3. Styling
- `rules/3-1-static-styles.md` - Always Use Static Styles (CRITICAL)
- `rules/3-2-host-styling.md` - Style the Host Element Properly (HIGH)
- `rules/3-3-css-custom-properties.md` - CSS Custom Properties for Theming (MEDIUM)
- `rules/3-4-css-parts.md` - CSS Parts for Deep Styling (MEDIUM)

### 4. Events
- `rules/4-1-composed-events.md` - Dispatch Composed Events (CRITICAL)
- `rules/4-2-event-naming.md` - Event Naming Conventions (MEDIUM)
- `rules/4-3-cleanup-listeners.md` - Clean Up Event Listeners (HIGH)

### 5. Lifecycle
- `rules/5-1-super-call-order.md` - Correct super() Call Order (CRITICAL)
- `rules/5-2-first-updated.md` - Use firstUpdated for DOM Operations (HIGH)
- `rules/5-3-will-update.md` - Use willUpdate for Derived State (HIGH)
- `rules/5-4-update-complete.md` - Async Operations with updateComplete (MEDIUM)

### 6. Accessibility
- `rules/6-1-delegates-focus.md` - delegatesFocus for Interactive Components (HIGH)
- `rules/6-2-aria-attributes.md` - ARIA for Custom Interactive Components (CRITICAL)
- `rules/6-3-form-associated.md` - Form-Associated Custom Elements (HIGH)

### 7. Performance
- `rules/7-1-has-changed.md` - Custom hasChanged for Complex Types (HIGH)
- `rules/7-2-batch-updates.md` - Batch Property Updates (MEDIUM)
- `rules/7-3-lazy-loading.md` - Lazy Load Heavy Dependencies (HIGH)
- `rules/7-4-memoization.md` - Memoize Expensive Computations (MEDIUM)

## Quick Reference

### Essential Imports

```typescript
// Core
import { LitElement, html, css, nothing }  from "@conectate/components/ct-lit";;
import { customElement, property, state, query } from "lit/decorators.js";

// Common Directives
import { repeat } from 'lit/directives/repeat.js';
import { cache } from 'lit/directives/cache.js';
import { classMap } from 'lit/directives/class-map.js';
import { until } from 'lit/directives/until.js';
```

### Component Skeleton

```typescript
@customElement('my-component')
export class MyComponent extends LitElement {
  static styles = css`
    :host { display: block; }
    :host([hidden]) { display: none; }
  `;

  @property({ type: String }) value = '';
  @property({ type: Boolean, reflect: true }) disabled = false;
  @state() private _internal = '';

  render() {
    return html``;
  }
}
```

### Object and array reactive updates

Lit schedules an update when a reactive property is **assigned** a new value. By default it compares old and new with **`===`**. Mutating a nested field (`obj.x = 1` or `arr.push(x)`) does **not** assign the property, so the component may not re-render.

Use **spread** (or another expression that produces a **new** reference) when updating `@property` / `@state` that hold objects or arrays:

```typescript
// Objects — shallow merge, new reference
this.user = { ...this.user, name: nextName }

// Arrays — append
this.items = [...this.items, newItem]

// Arrays — replace one index (immutable)
this.items = this.items.map((item, i) => (i === index ? newItem : item))

// Arrays — filter / remove
this.items = this.items.filter((item) => item.id !== id)

// Only when object or array is so deep and children web components are not reactive, use `this.deepClone` to create a new reference:
// Inefficient, but necessary when children web components are not reactive.
this.user = this.deepClone({...this.user, name: nextName})
this.items = this.deepClone([...this.items, newItem])
```

If you must mutate in place, call `this.requestUpdate()` afterward. Prefer immutable updates for predictable rendering and simpler mental models. See `rules/7-1-has-changed.md` for custom equality when you need finer control.
