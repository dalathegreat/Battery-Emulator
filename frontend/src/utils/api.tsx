import { useEffect, useState } from 'preact/hooks'

export function useGetApi(url: string, period: number=0) {
    const [response, setResponse] = useState<any>(null);

    function patch(resp: any) {
        if (resp !== null) {
            // Crudely mark the time we received this response
            resp._now = Date.now();
        }
        // TODO - should we clear the timeout if this gets called?
        resp._reload = call;
        return resp;
    }

    const ctx = {t:0};
    function call() {
        // Don't make calls while rebooting
        if((window as unknown as any)._rebooting) return;

        fetch(
            url?.startsWith('https://') ? url : (import.meta.env.VITE_API_BASE + url)
        ).then(
            r => (r.headers.get('Content-Type')?.includes('application/json')) ? r.json().then(patch) : r.text()
        ).then(setResponse);
        if(period>0) {
            ctx.t = setTimeout(call, period) as unknown as number;
            return () => {
                // Destructor
                clearTimeout(ctx.t);
            }
        }
    }
    useEffect(call, [url, period]);

    return response;
}
