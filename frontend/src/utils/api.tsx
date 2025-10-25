import { useEffect, useState } from 'preact/hooks'

function patch(resp: any) {
    if (resp && typeof resp === 'object') {
        // Crudely mark the time we received this response
        resp._now = Date.now();
    }
    return resp;
}

export function useGetApi(url: string, period: number=0) {
    const [response, setResponse] = useState<any>(null);

    const ctx = {t:0};
    function call() {
        fetch(import.meta.env.VITE_API_BASE + url).then(
            r => (r.headers.get('Content-Type')?.includes('application/json')) ? r.json().then(patch) : r.text()
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
