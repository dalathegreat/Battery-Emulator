import { useEffect, useState } from 'preact/hooks'

import { useMockGetApi } from './mock_api.tsx';

export function useGetApi(url: string, period: number=0) {
    if (import.meta.env.VITE_DEMO_MODE === 'true') {
        return useMockGetApi(url, period);
    }
    const [response, setResponse] = useState<any>(null);

    function patch(resp: any) {
        if (resp !== null) {
            // Crudely mark the time we received this response
            resp._now = Date.now();
        }
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
            clearTimeout(ctx.t);
            ctx.t = setTimeout(call, period) as unknown as number;
            return () => {
                // Destructor
                clearTimeout(ctx.t);
            }
        }
    }
    useEffect(() => {
        const h = () => call();
        window.addEventListener('api-invalidate', h);
        const cleanup = call();
        return () => {
            window.removeEventListener('api-invalidate', h);
            cleanup?.();
        };
    }, [url, period]);

    return response;
}

export function refreshApi() {
    window.dispatchEvent(new Event('api-invalidate'));
}

export async function apiPost(url: string, data: any) {
    if (import.meta.env.VITE_DEMO_MODE === 'true') {
        const { apiPostMock } = await import('./mock_api.tsx');
        return apiPostMock(url, data);
    }
    const response = await fetch(import.meta.env.VITE_API_BASE + url, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    });
    if (response.status >= 400) {
        const errorData = await response.json();
        throw new Error('Failed to save settings: ' + JSON.stringify(errorData));
    }
    return response.json();
}
