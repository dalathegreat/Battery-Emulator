# Lit Web Components Best Practices - Agent Guide

> **Note:** This document provides high-level guidance for AI agents working with Lit components. Detailed rules with code examples are in the `rules/` directory.

## How to Use This Skill

1. **Read SKILL.md first** for the rule index and quick reference
2. **Load specific rules as needed** based on the task at hand
3. **Prioritize by impact** - CRITICAL and HIGH rules should always be followed

## Rule Selection by Task

### Writing New Components
Load these rules:
- `1-1-use-decorators.md` - Property declarations
- `1-2-separate-state.md` - Public vs internal state
- `3-1-static-styles.md` - Styling approach
- `4-1-composed-events.md` - Event configuration

### Reviewing Components
Check for violations of CRITICAL rules:
- `2-1-pure-render.md` - Side effects in render()
- `3-1-static-styles.md` - Inline styles in templates
- `4-1-composed-events.md` - Events not composed
- `5-1-super-call-order.md` - Wrong super() order
- `6-2-aria-attributes.md` - Missing accessibility

### Performance Optimization
Load these rules:
- `2-3-use-repeat.md` - List rendering
- `2-5-derived-state.md` - Expensive computations
- `7-1-has-changed.md` - Update optimization
- `7-3-lazy-loading.md` - Code splitting

### Accessibility Audit
Load these rules:
- `6-1-delegates-focus.md` - Focus handling
- `6-2-aria-attributes.md` - ARIA implementation
- `6-3-form-associated.md` - Form integration

## Critical Patterns to Always Follow

### 1. Static Styles Only
```typescript
// ✓ Always
static styles = css`...`;

// ✗ Never
render() {
  return html`<style>...</style>...`;
}
```

### 2. Composed Events
```typescript
// ✓ Always
this.dispatchEvent(new CustomEvent('change', {
  bubbles: true,
  composed: true,
  detail: { value }
}));
```

### 3. Pure render()
```typescript
// ✓ Only return templates
render() {
  return html`...`;
}

// ✗ No side effects
render() {
  this.count++; // Bad
  console.log(); // Bad
  return html`...`;
}
```

### 4. Correct Lifecycle Order
```typescript
connectedCallback() {
  super.connectedCallback(); // First
  // Your code
}

disconnectedCallback() {
  // Your code
  super.disconnectedCallback(); // Last
}
```

### 5. State Separation
```typescript
// Public API
@property({ type: String }) value = '';

// Internal only
@state() private _computed = '';
```

## Common Mistakes to Catch

| Mistake | Rule | Fix |
|---------|------|-----|
| Inline `<style>` in template | 3-1 | Use `static styles` |
| Event without `composed: true` | 4-1 | Add to CustomEvent options |
| Side effects in render() | 2-1 | Move to updated() or willUpdate() |
| No default property values | 1-4 | Add `= ''` or appropriate default |
| Reflecting complex types | 1-3 | Remove `reflect: true` for objects/arrays |
| super() in wrong position | 5-1 | First in connectedCallback, last in disconnectedCallback |
| DOM access in connectedCallback | 5-2 | Move to firstUpdated() |
| Setting state in updated() | 5-3 | Use willUpdate() for derived state |
| Missing keyboard handlers | 6-2 | Add @keydown with Space/Enter handling |
| map() for dynamic lists | 2-3 | Use repeat() directive with keys |

## File Structure

```
lit-best-practices/
├── SKILL.md          # Index, quick reference, when to use
├── AGENTS.md         # This file - agent guidance
├── README.md         # Human documentation
└── rules/
    ├── 1-1-use-decorators.md
    ├── 1-2-separate-state.md
    ├── ...
    └── 7-4-memoization.md
```

## Rule File Format

Each rule file contains:
- **Frontmatter** with title, impact, tags
- **Problem description**
- **Incorrect code example**
- **Correct code example**
- **Explanation of why**

Load rules by reading the specific file when that topic is relevant to the current task.
