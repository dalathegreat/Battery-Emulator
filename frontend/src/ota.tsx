import { useEffect, useState } from "preact/hooks";

// @ts-ignore
const md5 = function(h){function i(c,d){return((c>>1)+(d>>1)<<1)+(c&1)+(d&1)}for(var m=[],l=0;64>l;)m[l]=0|4294967296*Math.abs(Math.sin(++l));return function(c){for(var d,g,f,a,j=[],c=unescape(encodeURI(c)),b=c.length,k=[d=1732584193,g=-271733879,~d,~g],e=0;e<=b;)j[e>>2]|=(c.charCodeAt(e)||128)<<8*(e++%4);j[c=(b+8>>6)*h+14]=8*b;for(e=0;e<c;e+=h){b=k;for(a=0;64>a;)b=[f=b[3],i(d=b[1],(f=i(i(b[0],[d&(g=b[2])|~d&f,f&d|~f&g,d^g^f,g^(d|~f)][b=a>>4]),i(m[a],j[[a,5*a+1,3*a+5,7*a][b]%h+e])))<<(b=[7,12,17,22,5,9,14,20,4,11,h,23,6,10,15,21][4*b+a++%4])|f>>>32-b),d,g];for(a=4;a;)k[--a]=i(k[a],b[a])}for(c="";32>a;)c+=(k[a>>3]>>4*(1^a++&7)&15).toString(h);return c}}(16);

function calculateMD5(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = function(e) {
            if (!e.target) {
                reject('File read error');
                return;
            }
            const content = e.target.result as string;
            const hash = md5(content);
            resolve(hash);
        };
        reader.onerror = function() {
            reject('File read error');
        };
        reader.readAsBinaryString(file);
    });
}

async function flash(file: File, cb: (ratio: number) => void) {
    try {
        cb(0);
        
        const md5Hash = await calculateMD5(file);

        await fetch(import.meta.env.VITE_API_BASE + '/ota/start?mode=fr&hash=' + md5Hash);

        const fd = new FormData();
        fd.append('file', file);

        const xhr = new XMLHttpRequest();
        xhr.upload.addEventListener("progress", (event) => {
            if (event.lengthComputable) {
                cb(Math.min(event.loaded / event.total, 0.999));
            }
        });
        xhr.addEventListener("load", () => {
            if( xhr.status === 200 ) cb(1);
        });
        xhr.open("POST", import.meta.env.VITE_API_BASE + "/ota/upload");
        xhr.send(fd);
    } catch (error) {
        alert('Update failed.');
    }
    
}

export function Ota() {
    const [progress, setProgress] = useState(-1);
    
    useEffect(() => {
        console.log('progress', progress);
        if(progress===1) {
            fetch(import.meta.env.VITE_API_BASE + "/api/reboot", { method: 'POST' });
            setTimeout(() => {
                window.location.href = "../";
            }, 6000);
        }
    }, [progress]);

    return <div>
        <h2>OTA Update</h2>
        <div style="text-align: center; margin: 5rem 0;">
        <input type="file" style="font-size: 1.25rem; border: 2px solid #333; border-radius: 0.5rem;" onChange={ (ev) => {
            const input = ev.target as HTMLInputElement;
            if (input?.files?.length) {
                flash(input.files[0], setProgress);
                input.value = '';
            }
        } } />
        </div>
        <div style="margin: 0 0 5rem; font-size: 1.25rem">
        { progress===1 ? <div>
            Update complete. Rebooting...
        </div> : progress>=0 ? <div>
            Progress: { (progress*100).toFixed(0) } %
        </div> : null }
        </div>
    </div>;
}