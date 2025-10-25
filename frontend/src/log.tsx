import { useGetApi } from './utils/api.tsx'

export function Log() {
    const data = useGetApi("/api/log") ?? "";

    const first_nul = data.indexOf('\0');
    const newline = first_nul > -1 ? data.indexOf('\n', first_nul+1) : -1;
    const second_nul = first_nul>0 ? data.indexOf('\0', first_nul+1) : -1;

    return (
        <>
            <h2>Log</h2>
            <div class="panel">
                <div class="log">
                    <div>
                        { second_nul > -1 && data.substring(Math.max(first_nul+1, newline+1), second_nul) }
                        { first_nul > -1 && data.substring(0, first_nul) }
                    </div>
                </div>
            </div>
        </>
    );
}