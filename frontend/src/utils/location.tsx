import { useEffect, useState } from 'preact/hooks'

export function useLocation() {
    const [location, setLocation] = useState(window.location.pathname);

    function onPopState() {
        setLocation(window.location.pathname);
    }
    useEffect(() => {
        window.addEventListener('popstate', onPopState);
        return () => {
            window.removeEventListener('popstate', onPopState);
        };
    });

    return location;
    //return location.substring(import.meta.env.BASE_URL.length - (import.meta.env.BASE_URL.endsWith('/') ? 1 : 0));
}

export function Link(props: { href: string; children: preact.ComponentChildren; class?: string }) {
    const href = import.meta.env.BASE_URL.replace(/\/$/g, '').replace(/^\./g, '') + props.href;
    const path = window.location.pathname;
    function onClick(e: MouseEvent) {
        e.preventDefault();
        window.history.pushState(null, '', href);
        const navEvent = new PopStateEvent('popstate');
        window.dispatchEvent(navEvent);
    }
    return <a href={href} 
              onClick={onClick}
              class={props.class}
              data-pre={path.startsWith(href) ? 'true' : undefined}
              data-cur={path===href ? 'true' : undefined}     
              >{props.children}</a>;
}
