import { useGetApi } from './utils/api.tsx'

import { useState } from 'preact/hooks'

export function CellMonitor() {
    const data = useGetApi("/api/cells", 5000);
    const [selectedCell, setSelectedCell] = useState<number | null>(null);

    return (
      <>
        <h2>Cell monitor</h2>

        { (data?.battery ?? []).map((battery: any, idx: number) => {
            const max = Math.max(...battery.voltages, 0);
            const min = Math.min(...battery.voltages, 0);
            const deviation = max - min;
        
            return (
                <>
                    <div class="panel">
                        { data?.battery.length > 1 ? <h3>Battery {idx+1}</h3> : '' }
                        <div class="stats">
                            <div class="stat">
                                <span>COUNT</span>
                                { battery.voltages.length }
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
                        { battery.voltages.map((v: number, k: number) => (
                            v > 0 &&
                            <div class="cell-bars__bar" 
                                key={k} 
                                data-sel={ selectedCell===(idx*1000 + k) || undefined }
                                data-min={ v===min || undefined }
                                data-max={ v===max || undefined }
                                onMouseEnter={ (_ev)=>{ setSelectedCell(idx*1000 + k) } }
                                onMouseLeave={ (_ev)=>{ setSelectedCell(null) } }
                                style={ {height: (v-(min-20))*(200/(deviation+40))} }
                                ></div>
                        ))}
                    </div>

                    <div class="cell-grid">
                        { battery.voltages.map((v: number, k: number) => (
                            <div class="cell-grid__cell" 
                                key={k} 
                                data-sel={ selectedCell===(idx*1000 + k) || undefined }
                                data-min={ v===min || undefined }
                                data-max={ v===max || undefined }
                                onMouseEnter={ (_ev)=>{ setSelectedCell(idx*1000 + k) } }
                                onMouseLeave={ (_ev)=>{ setSelectedCell(null) } }
                                >
                                Cell {k+1}<br />
                                { v || '-' } mV
                            </div>
                        ))}
                    </div>
                </>
            );
        }) }
      </>
    );
}
