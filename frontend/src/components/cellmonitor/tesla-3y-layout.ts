/** Tesla Model 3/Y pack brick layout (ScanMyTesla-style 96-brick view). */
export const TESLA_MODEL_3Y_TYPE = 32;

/** Brick counts per module, top row then bottom row. Total 96. */
export const TESLA_3Y_MODULES = [
	{ id: 1, label: "Module 1", rows: [14, 9] as const },
	{ id: 2, label: "Module 2", rows: [14, 11] as const },
	{ id: 3, label: "Module 3", rows: [14, 11] as const },
	{ id: 4, label: "Module 4", rows: [14, 9] as const }
] as const;

export function isTeslaModel3Y(battType: number, cellCount = 0): boolean {
	return battType === TESLA_MODEL_3Y_TYPE || cellCount === 96;
}

export function moduleBrickCount(rows: readonly number[]): number {
	return rows.reduce((a, b) => a + b, 0);
}

/** 1-based module id for a 0-based brick index, or 0 if out of range. */
export function moduleIdForBrick(index: number): number {
	let offset = 0;
	for (const mod of TESLA_3Y_MODULES) {
		const n = moduleBrickCount(mod.rows);
		if (index >= offset && index < offset + n) return mod.id;
		offset += n;
	}
	return 0;
}

export function splitIntoModules<T>(cells: T[]): T[][] {
	const modules: T[][] = [];
	let offset = 0;
	for (const mod of TESLA_3Y_MODULES) {
		const n = moduleBrickCount(mod.rows);
		modules.push(cells.slice(offset, offset + n));
		offset += n;
	}
	return modules;
}

/** Most frequent voltage (rounded to 1 mV), for "Mode" in the stats bar. */
export function modeVoltage_mV(voltages: number[]): number | null {
	if (voltages.length === 0) return null;
	const counts = new Map<number, number>();
	let best = voltages[0];
	let bestN = 0;
	for (const v of voltages) {
		const n = (counts.get(v) ?? 0) + 1;
		counts.set(v, n);
		if (n > bestN) {
			bestN = n;
			best = v;
		}
	}
	return best;
}
