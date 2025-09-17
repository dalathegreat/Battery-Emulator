import { useEffect, useState } from 'preact/hooks'

export function useGetApi(url: string, period: number=0) {
    const [response, setResponse] = useState<any>(null);

    const ctx = {t:0};
    function call() {
        fetch(import.meta.env.VITE_API_BASE + url).then(r=>r.json()).then(r=>setResponse(r));
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
