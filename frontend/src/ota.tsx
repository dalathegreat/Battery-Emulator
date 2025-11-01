import { useEffect, useState } from "preact/hooks";

import { Button } from "./components/button.tsx";

import { useGetApi } from "./utils/api.tsx";
import { upload } from "./utils/upload.tsx";
import { reboot } from "./utils/reboot.tsx";

// @ts-ignore
const md5 = function(h){function i(c,d){return((c>>1)+(d>>1)<<1)+(c&1)+(d&1)}for(var m=[],l=0;64>l;)m[l]=0|4294967296*Math.abs(Math.sin(++l));return function(c){for(var d,g,f,a,j=[],c=unescape(encodeURI(c)),b=c.length,k=[d=1732584193,g=-271733879,~d,~g],e=0;e<=b;)j[e>>2]|=(c.charCodeAt(e)||128)<<8*(e++%4);j[c=(b+8>>6)*h+14]=8*b;for(e=0;e<c;e+=h){b=k;for(a=0;64>a;)b=[f=b[3],i(d=b[1],(f=i(i(b[0],[d&(g=b[2])|~d&f,f&d|~f&g,d^g^f,g^(d|~f)][b=a>>4]),i(m[a],j[[a,5*a+1,3*a+5,7*a][b]%h+e])))<<(b=[7,12,17,22,5,9,14,20,4,11,h,23,6,10,15,21][4*b+a++%4])|f>>>32-b),d,g];for(a=4;a;)k[--a]=i(k[a],b[a])}for(c="";32>a;)c+=(k[a>>3]>>4*(1^a++&7)&15).toString(h);return c}}(16);

function calculateMD5(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
        // Allow any pending UI updates before we do something heavy
        setTimeout(() => {
        //requestAnimationFrame(() => {
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
        }, 100);
    });
}

async function flash(file: File, cb: (ratio: number) => void) {
    try {
        cb(0);
        
        const md5Hash = await calculateMD5(file);

        await fetch(import.meta.env.VITE_API_BASE + '/ota/start?mode=fr&hash=' + md5Hash);
        
        await upload(import.meta.env.VITE_API_BASE + "/ota/upload", file, cb);

        cb(1);
    } catch (error) {
        alert('Update failed: ' + error);
        cb(-1);
    }
}

function versionSortKey(v: string) {
    return v?.split('.').map(part => 
        part.replace('v', '').replace('RC', '  ').padStart(5, part.indexOf('RC')>=0 ? ' ' : '0')
    ).join('.');
}

async function downloadAndFlash(path: string, cb: (ratio: number) => void) {
    const uri = "https://raw.githubusercontent.com/dalathegreat/BE-Web-Installer/refs/heads/main/" + path;
    console.log(path);
    console.log(uri);

    // return new Promise<void>((resolve, reject) => {
    // });

    return fetch(uri).then(async (resp) => {
        if(!resp.ok) {
            alert('Failed to download file.');
            return;
        }
        const blob = await resp.blob();
        const file = new File([blob], 'update.ota.bin', { type: 'application/octet-stream' });
        return flash(file, cb);
    });
}

export function Ota() {
    const [progress, setProgress] = useState(-1);
    const status = useGetApi('/api/status', 0);
    const blah = useGetApi("https://api.github.com/repos/dalathegreat/BE-Web-Installer/git/trees/main?recursive=1");

    const currentVersionKey = versionSortKey("1.2.3");//versionSortKey(status?.firmware);
    const hardware: string = {
        'LilyGo T_2CAN': 'LilygoT-2CAN',
        'LilyGo T-CAN485': 'LilygoT-CAN485',
        'Stark CMR Module': 'StarkCMR',
    }[''+status?.hardware] || 'invalid';

    const releases: any[] = blah?.tree?.map((item: any) =>({
       ...item,
       k: versionSortKey(item.path.split('/')[1]),
    })).filter((item: any) => item.path.endsWith('_' + hardware + '.ota.bin')) || [];
    releases.sort((a: any, b: any) => b.k.localeCompare(a.k));
    
    useEffect(() => {
        if(progress===1) {
            reboot();
        }
    }, [progress]);

    function formatReleases(releases: any[], level?: string) {
        return releases.map(r=>
            <div class={"alert slim"} data-level={level}>
                { r.path.split('/')[2] }&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
                <Button onClick={ ()=>downloadAndFlash(r.path, setProgress) } 
                        confirm="This will install this version and reboot. Are you sure?"
                        disabled={progress>=0 && progress<1}
                        >Install</Button>
            </div>
        );
    }
    const newReleases = releases.filter((r)=> r.k > currentVersionKey);
    const oldReleases = releases.filter((r)=> r.k <= currentVersionKey);

    return <div>
        <h2>Over-The-Air (OTA) upgrade</h2>
        <p>Upload a new <b>.ota.bin</b> firmware file to update the system. The device will reboot automatically, and your settings will be preserved.</p>
        <div style="margin: 1.5rem 0;">
        <input type="file" style="font-size: 1.25rem; border: 2px solid #333; border-radius: 0.5rem;" onChange={ (ev) => {
            const input = ev.target as HTMLInputElement;
            if (input?.files?.length) {
                flash(input.files[0], setProgress);
                input.value = '';
            }
        } } />
        </div>
        <div style="margin: 0 0 5rem; font-size: 1.25rem"><b>
        { progress===1 ? <div>
            Update complete. Rebooting...
        </div> : progress>=0 ? <div>
            Progress: { (progress*100).toFixed(0) } %
        </div> : null }
        </b></div>
        { !!newReleases.length && <>
            <h3>New releases</h3>
            { formatReleases(releases.filter((r)=> r.k > currentVersionKey), "success") }
        </> }
        { !!oldReleases.length && <>
            <h3>Past releases</h3>
            { formatReleases(releases.filter((r)=> r.k <= currentVersionKey), "info") }
        </> }
        
    </div>;
}
