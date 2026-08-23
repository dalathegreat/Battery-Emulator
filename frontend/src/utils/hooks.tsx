import { useEffect, useState } from 'preact/hooks';

export function useDeferred(value: any) {
  // Defers the value update by one tick. Useful to control rendering order to
  // avoid flickering.
  const [deferredValue, setDeferredValue] = useState(value);
  useEffect(() => {
    requestAnimationFrame(() => {
      setDeferredValue(value);
    });
  }, [value]);
  return deferredValue;
}
