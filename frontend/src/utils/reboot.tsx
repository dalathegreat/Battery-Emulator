function reboot() {
    (window as unknown as any)._rebooting = true;

    fetch(import.meta.env.VITE_API_BASE + "/api/reboot", { method: 'POST' })
        .then(r => r.json())
        .then(({ uptime: preRebootUptime }) => {
            const check = () => {
                fetch(
                    import.meta.env.VITE_API_BASE + "/api/status",
                    { signal: AbortSignal.timeout(1000) }
                ).then(r => r.json()).then(({ uptime }) => {
                    // The reboot API returns the uptime before the reboot. If
                    // the current uptime is less than that, we know the reboot
                    // has completed.
                    if (uptime < preRebootUptime) {
                        window.location.href = "../";
                    } else {
                        setTimeout(check, 1000);
                    }
                }).catch(() => {
                    setTimeout(check, 1000);
                });
            };
            // Wait long enough for the reboot to start
            setTimeout(check, 1000);
        });
    // Promise that never resolves
    return new Promise<void>((_res, _rej) => {});
}

export { reboot };
