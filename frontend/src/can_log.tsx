
//import { useCallback, useEffect, useRef } from "preact/hooks";

/*
function fetchInChunks(url: string, callback: (chunk: Uint8Array) => boolean) {
    fetch(url)
    .then(response => response.body)
    .then(rs => {
        if(!rs) {
            throw new Error("No response body");
        }
        const reader = rs.getReader();

        return new ReadableStream({
            async start(_controller) {
                while (true) {
                    const { done, value } = await reader.read();

                    // When no more data needs to be consumed, break the reading
                    if (done) {
                        break;
                    }

                    if(callback(value)) {
                        // If callback returns true, stop reading
                        break;
                    }

                    // Optionally append the value if you need the full blob later.
                    //controller.enqueue(value);
                }

                // Close the stream
                console.log('Closing stream');
                // controller.close();
                // reader.releaseLock();
                reader.cancel();
            }
        })
    })
    // Create a new response out of the stream (can be avoided?)
    //.then(rs => new Response(rs))
    // Create an object URL for the response
    //.then(response => response.blob())
    //.then(blob => { console.log("Do something with full blob") }
    .catch(console.error);
}
*/

export function CanLog() {
    /*
    const log = useRef(null);
    const running = useRef(false);

    const handleClick = useCallback((event: Event) => {
        event.preventDefault();

        if(running.current) {
            (event.target as HTMLAnchorElement).innerText = "Start logging";
            running.current = false;
            return;
        }
        (event.target as HTMLAnchorElement).innerText = "Stop logging";
        running.current = true;

        // const tick = () => {
        //     if(!running.current) {
        //         return;
        //     }
        //     const el = log.current as HTMLDivElement;
        //     const nel = document.createElement("div");
        //     nel.style.whiteSpace = "pre";
        //     nel.style.font = "14px monospace";
        //     let msg = "";
        //     for(let i = 0; i < 50; i++) {
        //         msg += "here is lots of text being added yeah here is lots of text being added yeah\n";
        //     }
        //     nel.innerText = msg;
        //     el.appendChild(nel);
        //     el.scrollTop = el.scrollHeight; // Scroll to bottom
        //     setTimeout(tick, 20);
        // };
        //tick();
        
        fetchInChunks(import.meta.env.VITE_API_BASE + "/dump_can", (chunk: Uint8Array) => {
            if(log.current) {
                const el = log.current as HTMLDivElement;
                const text = new TextDecoder().decode(chunk);
                const nel = document.createElement("div");
                nel.style.whiteSpace = "pre";
                nel.style.font = "14px monospace";
                nel.innerText = text;
                el.appendChild(nel);
                el.scrollTop = el.scrollHeight; // Scroll to bottom
            }
            return !running.current;
        });
    }, [log]);

    useEffect(() => {
        return () => {
            running.current = false;
        }
    }, [log]);
    */

    return (
        <div>
            <h2>CAN Log</h2>
            <a href="/dump_can" class="button" target="_blank">Start logging in new tab</a>
        </div>
    );
}

/*
            <a href="#" class="button" onClick={ handleClick }>Start logging</a> <a href="#" class="button" onClick={ () => { if(log.current) (log.current as HTMLDivElement).innerText = ""; } }>Clear log</a> <a href="/dump_can" class="button" download="can_log.txt">Log to file</a> <a href="/dump_can" class="button" target="_blank">Log in new tab</a>
            <br /><br />
            <div class="panel">
                <div ref={log} style="max-height: 20rem; overflow: auto; width: 100%;"></div>
            </div>

*/