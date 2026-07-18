import { css } from "lit";

/**
 * Shared component styles: cards, kv grids, buttons, chips, status text and
 * console panes. Import into each view's `static styles` so every screen has
 * a consistent look driven by the theme variables from theme.ts.
 */
export const sharedStyles = css`
	:host {
		display: block;
		color: var(--color-on-background);
		font-family: var(--font-body);
	}

	h2 {
		margin: 0 0 4px;
		font-size: 1.35rem;
		font-weight: 700;
		color: var(--high-emphasis);
		letter-spacing: -0.01em;
	}
	h3 {
		margin: 0 0 12px;
		font-size: 1rem;
		font-weight: 600;
		color: var(--high-emphasis);
	}
	h4 {
		margin: 16px 0 8px;
		font-size: 0.85rem;
		font-weight: 600;
		color: var(--medium-emphasis);
		text-transform: uppercase;
		letter-spacing: 0.05em;
	}

	.page-header {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 10px;
		margin-bottom: 16px;
	}
	.page-header .subtitle {
		width: 100%;
		margin: 0;
		font-size: 0.85rem;
		color: var(--medium-emphasis);
	}
	.spacer {
		flex: 1;
	}

	ct-card {
		display: block;
		margin: 0 0 16px;
		padding: 16px;
		--border-radius: var(--radius-card);
		box-shadow: 0 1px 2px rgb(0 0 0 / 0.14);
	}

	/* Key/value grid */
	.grid {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
		gap: 10px;
	}
	.kv {
		background: var(--color-background);

		padding: 8px 12px;
		border-radius: var(--radius-control);
		min-width: 0;
	}
	.kv .k {
		font-size: 0.72rem;
		color: var(--medium-emphasis);
		text-transform: uppercase;
		letter-spacing: 0.04em;
		margin-bottom: 2px;
	}
	.kv .v {
		font-weight: 650;
		font-size: 0.95rem;
		color: var(--high-emphasis);
		overflow-wrap: anywhere;
	}

	/* Buttons */
	button,
	.btn {
		appearance: none;
		display: inline-flex;
		align-items: center;
		justify-content: center;
		gap: 6px;
		background: var(--color-surface-bright);
		color: var(--high-emphasis);

		padding: 8px 16px;
		border-radius: var(--radius-control);
		cursor: pointer;
		font-size: 0.85rem;
		font-weight: 600;
		font-family: inherit;
		text-decoration: none;
		transition:
			background 120ms ease,
			border-color 120ms ease,
			transform 80ms ease;
	}
	button:hover,
	.btn:hover {
		background: var(--color-surface-dim);
		border-color: var(--color-primary-medium);
	}
	button:active,
	.btn:active {
		transform: scale(0.98);
	}
	button:disabled {
		opacity: 0.5;
		cursor: default;
		pointer-events: none;
	}
	button.primary,
	.btn.primary {
		background: var(--color-primary);
		border-color: transparent;
		color: var(--color-on-primary);
	}
	button.primary:hover,
	.btn.primary:hover {
		background: var(--color-primary-medium);
	}
	button.danger,
	.btn.danger {
		background: var(--color-error-container);
		border-color: var(--color-error);
		color: var(--color-error);
	}
	button.warn,
	.btn.warn {
		background: var(--color-warning-container);
		border-color: var(--color-warning);
		color: var(--color-warning);
	}

	/* ct-components */
	ct-button {
		--ct-button-radius: var(--radius-control);
		font-size: 0.85rem;
		font-weight: 600;
		--ct-icon-size: 18px;
	}
	ct-spinner {
		color: var(--color-primary);
	}

	/* Inputs */
	input[type="text"],
	input[type="number"],
	input[type="password"],
	select {
		font-family: inherit;
		font-size: 0.85rem;
		padding: 7px 10px;
		border-radius: var(--radius-control);

		background: var(--color-surface-dim);
		color: var(--high-emphasis);
		outline: none;
		transition: border-color 120ms ease;
		box-sizing: border-box;
		border: none;
	}
	input:focus,
	select:focus {
		border-color: var(--color-primary);
	}
	input[type="checkbox"] {
		width: 16px;
		height: 16px;
		accent-color: var(--color-primary);
	}

	/* Status chips */
	.chip {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		padding: 3px 10px;
		border-radius: 999px;
		font-size: 0.75rem;
		font-weight: 600;

		background: var(--color-surface-bright);
		color: var(--medium-emphasis);
	}
	.chip.ok {
		background: var(--color-success-container);
		border-color: transparent;
		color: var(--color-success);
	}
	.chip.warn {
		background: var(--color-warning-container);
		border-color: transparent;
		color: var(--color-warning);
	}
	.chip.error {
		background: var(--color-error-container);
		border-color: transparent;
		color: var(--color-error);
	}

	/* Inline status text */
	.err {
		color: var(--color-error);
		font-size: 0.85rem;
	}
	.ok {
		color: var(--color-success);
		font-size: 0.85rem;
	}
	.hint {
		font-size: 0.78rem;
		color: var(--medium-emphasis);
	}
	.empty {
		color: var(--color-disable);
		font-size: 0.85rem;
	}

	a {
		color: var(--color-accent);
	}

	/* Console / monospace panes */
	pre.console {
		font-family: var(--font-mono);
		font-size: 11.5px;
		line-height: 1.5;
		white-space: pre-wrap;
		word-break: break-word;
		margin: 0;
		max-height: 60vh;
		overflow: auto;
		background: var(--color-surface-dim);

		padding: 12px;
		border-radius: var(--radius-control);
		color: var(--color-on-surface);
	}

	.row {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 8px;
		margin: 6px 0;
	}

	/* Loading & error states */
	.state {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 12px;
		padding: 48px 16px;
		color: var(--medium-emphasis);
	}
	.spinner {
		width: 28px;
		height: 28px;
		border-radius: 50%;
		border: 3px solid var(--color-outline);
		border-top-color: var(--color-primary);
		animation: be-spin 0.8s linear infinite;
	}
	@keyframes be-spin {
		to {
			transform: rotate(360deg);
		}
	}

	/* Progress bars (SOC, etc.) */
	.bar {
		position: relative;
		height: 8px;
		border-radius: 999px;
		background: var(--color-surface-dim);

		overflow: hidden;
	}
	.bar > i {
		position: absolute;
		inset: 0 auto 0 0;
		border-radius: inherit;
		background: var(--color-primary);
		transition: width 300ms ease;
	}

	::-webkit-scrollbar {
		width: 8px;
		height: 8px;
	}
	::-webkit-scrollbar-thumb {
		background: var(--color-outline);
		border-radius: 8px;
	}
`;
