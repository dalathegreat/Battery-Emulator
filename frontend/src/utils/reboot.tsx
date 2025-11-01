function reboot() {
    (window as unknown as any)._rebooting = true;
    fetch(import.meta.env.VITE_API_BASE + "/api/reboot", { method: 'POST' });
    const check = ()=>{
        fetch(
            import.meta.env.VITE_API_BASE + "/api/status",
            {signal: AbortSignal.timeout(2000)}
        ).then(r=>r.json()).then(()=>{
            window.location.href = "../";
        }).catch(()=>{
            setTimeout(check, 1000);
        });
    };
    // Wait long enough for the reboot to start
    setTimeout(check, 3000);
    // Promise that never resolves
    return new Promise<void>((_res, _rej) => {});
}

export { reboot };
