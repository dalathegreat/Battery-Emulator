import { useGetApi } from './utils/api.tsx'

export function Log() {
    const data = useGetApi("/api/log");
    return (
        <>
            <h2>Log</h2>
            <div class="panel">
                <div class="log">
                    <div>{ data?.b && data.b.substring(data.b.indexOf('\n') + 1) }</div>
                    <div>{ data?.a }</div>
                </div>
            </div>
        </>
    );
}