import { useEffect, useState } from 'preact/hooks'

function getApplicationPath() {
    if (import.meta.env.VITE_DEMO_MODE === 'true') {
        const basePath = new URL(import.meta.env.BASE_URL, document.baseURI).pathname;
        const path = window.location.pathname;
        const normalizedBase = basePath.endsWith('/') ? basePath : `${basePath}/`;

        if (path.startsWith(normalizedBase)) {
            return `/${path.slice(normalizedBase.length)}`;
        }
        return path;
    } else {
        // Keep it simple
        return window.location.pathname;
    }
}

export function useLocation() {
    const [location, setLocation] = useState(getApplicationPath());

    function onPopState() {
        setLocation(getApplicationPath());
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
    const base = new URL(import.meta.env.BASE_URL, document.baseURI);
    const target = new URL(props.href.replace(/^\/+/, ''), base);
    const href = target.pathname + target.search + target.hash;
    const path = window.location.pathname;

    function onClick(e: MouseEvent) {
        e.preventDefault();
        window.history.pushState(null, '', href);
        const navEvent = new PopStateEvent('popstate');
        window.dispatchEvent(navEvent);
    }

    //        data-pre={path.startsWith(target.pathname) ? 'true' : undefined}

    return <a href={href} 
              onClick={onClick}
              class={props.class}
              data-cur={path===target.pathname ? 'true' : undefined}
              >{props.children}</a>;
}
