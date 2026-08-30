#ifndef FAVICON_H
#define FAVICON_H

// Battery glyph served from /favicon.svg. Stored once as raw SVG; the pages
// only carry a link tag to it (see INDEX_HTML_FAVICON_LINK in index_html.h).
#define FAVICON_SVG \
  R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24"><rect x="2" y="6" width="17" height="12" rx="2.5" fill="#ffffff" stroke="#2e9e5b" stroke-width="2"/><rect x="19.4" y="9.4" width="2.6" height="5.2" rx="1" fill="#2e9e5b"/><path d="M6.5,12 L14.5,12" fill="none" stroke="#2563eb" stroke-width="2" stroke-linecap="round"/><path d="M8.5,9.8 L6.2,12 L8.5,14.2" fill="none" stroke="#2563eb" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/><path d="M12.5,9.8 L14.8,12 L12.5,14.2" fill="none" stroke="#2563eb" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>)rawliteral"

#endif  // FAVICON_H
