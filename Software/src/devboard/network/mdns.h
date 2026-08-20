#pragma once

// Interface-agnostic mDNS responder

#ifndef SMALL_FLASH_DEVICE
// Start the mDNS responder (advertises http/tcp/80) bound to the active
// interface's hostname. Self-guarding: starts at most once.
void init_mDNS();
#endif
