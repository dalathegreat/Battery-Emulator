// TODO: could we return a function-that-returns-a-promise, rather than the
// promise directly?
export const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));
