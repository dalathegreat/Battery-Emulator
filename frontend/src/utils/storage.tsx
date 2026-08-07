import { useState } from "preact/hooks";

export const CANSENDER_SELECTED_IDS_KEY = 1;

export const useStored = <T,>(k: any, d: T): [T, (v: T) => void] => {
    const [s, set] = useState<T>(() => {
        const r = localStorage.getItem(k);
        return r ? JSON.parse(r) : d;
    });
    return [s, (v: T) => {
        set(v);
        localStorage.setItem(k, JSON.stringify(v));
    }];
};
