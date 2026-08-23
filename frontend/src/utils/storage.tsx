import { useState } from "preact/hooks";

export const CANSENDER_SELECTED_IDS_KEY = 1;

// localStorage can throw (private browsing, storage disabled) and may contain
// corrupt JSON; degrade gracefully instead of crashing the app.
export const useStored = <T,>(k: any, d: T): [T, (v: T) => void] => {
    const [s, set] = useState<T>(() => {
        try {
            const r = localStorage.getItem(k);
            return r ? JSON.parse(r) : d;
        } catch {
            return d;
        }
    });
    return [s, (v: T) => {
        set(v);
        try {
            localStorage.setItem(k, JSON.stringify(v));
        } catch {
            // ignore: preference simply won't persist
        }
    }];
};
