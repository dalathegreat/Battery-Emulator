import { useState } from "preact/hooks";

export default function Button({children, confirm, disabled, onClick}: {children: preact.ComponentChildren, confirm?: string, disabled?: boolean, onClick?: (ev: preact.JSX.TargetedMouseEvent<HTMLButtonElement>) => void}) {
    const [spinning, setSpinning] = useState(false);
    const handleClick = (ev: preact.JSX.TargetedMouseEvent<HTMLButtonElement>) => {
        if(onClick && !disabled) {
            if(spinning) {
                return;
            }
            if(confirm && !window.confirm(confirm)) return;
            var ret: any = onClick(ev);
            if(ret instanceof Promise) {
                setSpinning(true);
                ret.finally(() => {
                    setSpinning(false);
                });
            }
        }
    };
    return <button class="button" onClick={handleClick} data-spin={spinning||undefined} disabled={disabled||undefined}>
        {children}
    </button>;
};
