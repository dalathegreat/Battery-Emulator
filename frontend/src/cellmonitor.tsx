import { useGetApi } from './utils/api.tsx'

import { useState } from 'preact/hooks'

export function CellMonitor() {
    const data = useGetApi("/api/cells", 5000);
    const [selectedCell, setSelectedCell] = useState<number | null>(null);

    return (
      <>
        <h2>Cell monitor</h2>

        { (data?.battery ?? []).map((battery: any, idx: number) => {
            const cells = battery.voltages
                .map((v: number, i: number) => ({v, i}))
                .filter((c: any) => c.v > 0);
            const max = Math.max(...cells.map((c: any) => c.v), 0);
            const min = Math.min(...cells.map((c: any) => c.v), 9999);
            const deviation = max - min;

            return (
                <>
                    <div class="panel">
                        { data?.battery.length > 1 ? <h3>Battery {idx+1}</h3> : '' }
                        <div class="stats">
                            <div class="stat">
                                <span>COUNT</span>
                                { cells.length }
                            </div>
                            <div class="stat">
                                <span>MINIMUM</span>
                                { min }<em>mV</em>
                            </div>
                            <div class="stat">
                                <span>MAXIMUM</span>
                                { max }<em>mV</em>
                            </div>
                            <div class="stat">
                                <span>DEVIATION</span>
                                { deviation }<em>mV</em>
                            </div>
                            <div class="stat">
                                <span>TEMP RANGE</span>
                                { battery.temp_min }<em>-</em> { battery.temp_max }<em>°C</em>
                            </div>
                        </div>
                    </div>

                    <div class="cell-bars">
                        { cells.map((c: any) => (
                            <div class="cell-bars__bar"
                                key={c.i}
                                data-sel={ selectedCell===(idx*1000 + c.i) || undefined }
                                data-min={ c.v===min || undefined }
                                data-max={ c.v===max || undefined }
                                onMouseEnter={ ()=>{ setSelectedCell(idx*1000 + c.i) } }
                                onMouseLeave={ ()=>{ setSelectedCell(null) } }
                                style={ {height: (c.v-(min-20))*(200/(deviation+40))} }
                                ></div>
                        ))}
                    </div>

                    <div class="cell-grid">
                        { cells.map((c: any, k: number) => (
                            <div class="cell-grid__cell"
                                key={c.i}
                                data-sel={ selectedCell===(idx*1000 + c.i) || undefined }
                                data-min={ c.v===min || undefined }
                                data-max={ c.v===max || undefined }
                                onMouseEnter={ ()=>{ setSelectedCell(idx*1000 + c.i) } }
                                onMouseLeave={ ()=>{ setSelectedCell(null) } }
                                >
                                Cell {k+1}<br />
                                { c.v } mV
                            </div>
                        ))}
                    </div>
                </>
            );
        }) }
      </>
    );
}
