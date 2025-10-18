import { useEffect, useState } from 'preact/hooks'

export function useGetApi(url: string, period: number=0) {
    const [response, setResponse] = useState<any>(null);

    const ctx = {t:0};
    function call() {
        fetch(new URL(url, import.meta.env.VITE_API_BASE)).then(
            r => (r.headers.get('Content-Type')?.includes('application/json')) ? r.json() : r.text()
        ).then(setResponse);
        if(period>0) {
            ctx.t = setTimeout(call, period) as unknown as number;
            return () => {
                clearTimeout(ctx.t);
            }
        }
    }
    useEffect(call, [url, period]);

    return response;
}
