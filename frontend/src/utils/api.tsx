import { useEffect, useState } from 'preact/hooks'

import { useMockGetApi } from './mock_api.tsx';

export function useGetApi(url: string, period: number=0) {
    if (import.meta.env.VITE_DEMO_MODE === 'true') {
        return useMockGetApi(url, period);
    }
    const [response, setResponse] = useState<any>(null);

    useEffect(() => {
        // Abort in-flight requests when the component unmounts (or the URL
        // changes) so we never setState after unmount and don't keep polling
        // pages the user has navigated away from.
        const abort = new AbortController();
        let timer: number | undefined;

        function patch(resp: any) {
            if (resp !== null) {
                // Crudely mark the time we received this response
                resp._now = Date.now();
            }
            return resp;
        }

        function call() {
            if (abort.signal.aborted) return;
            // Don't make calls while rebooting
            if ((window as unknown as any)._rebooting) return;

            if (period > 0) {
                clearTimeout(timer);
                timer = setTimeout(call, period) as unknown as number;
            }

            fetch(
                url?.startsWith('https://') ? url : ((import.meta.env.VITE_API_BASE || '') + url),
                { signal: abort.signal }
            ).then(
                r => {
                    const ct = r.headers.get('Content-Type') || '';
                    if (ct.includes('application/json')) return r.json().then(patch);
                    if (ct.includes('application/octet-stream')) return r.arrayBuffer();
                    return r.text();
                }
            ).then(
                resp => { if (!abort.signal.aborted) setResponse(resp); }
            ).catch(() => {
                // Aborted (unmount) or a transient network error; the next poll
                // will retry - ignore the error.
            });
        }

        const invalidate = () => call();
        window.addEventListener('api-invalidate', invalidate);
        call();
        return () => {
            abort.abort();
            clearTimeout(timer);
            window.removeEventListener('api-invalidate', invalidate);
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
    const response = await fetch((import.meta.env.VITE_API_BASE || '') + url, {
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
