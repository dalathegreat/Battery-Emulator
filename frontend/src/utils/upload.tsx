export function upload(url: string, body: any, cb: (progress: number, rate: number) => void): { abort: () => void, promise: Promise<any> } {
    const xhr = new XMLHttpRequest();
    const promise = new Promise((resolve, reject)=>{
        const ctx = { l: performance.now(), n: 0 };
        //xhr.responseType = "json";
        xhr.open("POST", url);
        xhr.upload.onprogress = (event) => {
            if(event.lengthComputable) {
                cb(
                    // Proportion of file uploaded (between 0 and 1)
                    Math.min(event.loaded / event.total, 0.999),
                    // Current upload speed in bytes/second
                    (event.loaded - ctx.n) / ((performance.now() - ctx.l)/1000),
                );
                ctx.l = performance.now();
                ctx.n = event.loaded;
            }
        };
        const resp = () => xhr.response ?? xhr.responseText;
        xhr.onload = () => {
            if(xhr.status === 200) {
                cb(1, 0);
                resolve(resp());
            } else {
                reject([xhr.status, resp()]);
            }
        };
        xhr.onerror = () => {
            reject([xhr.status, resp() || "Network error"]);
        };
        xhr.onabort = () => {
            reject([0, "Aborted"]);
        };
        xhr.send(body);
    });

    return {
        abort: () => xhr.abort(),
        promise
    };
}

