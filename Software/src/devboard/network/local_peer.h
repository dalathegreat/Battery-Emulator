#pragma once

#include <stdint.h>

// Tells whether an IPv4 peer is on the local network, so the web interface can refuse peers that
// arrive through a port forward while password protection is off. A router port forward rewrites
// only the destination, so such a peer keeps its public source address and lands outside every
// range below. A reverse proxy or NAT on the LAN rewrites the source too and looks local: that is
// a setup the owner built on purpose, and it is out of reach for this check anyway.
//
// 100.64.0.0/10 (carrier-grade NAT) is deliberately not local: with the router's WAN address in an
// ISP's CGNAT range, other customers of the same ISP arrive from it.
//
// All addresses in host byte order (10.0.0.1 is 0x0A000001). own_ip/own_mask describe the station
// interface; 0 when it has no address.
inline bool ipv4_is_local_peer(uint32_t peer, uint32_t own_ip, uint32_t own_mask) {
  if ((peer & 0xFF000000u) == 0x0A000000u ||  // 10.0.0.0/8
      (peer & 0xFFF00000u) == 0xAC100000u ||  // 172.16.0.0/12
      (peer & 0xFFFF0000u) == 0xC0A80000u ||  // 192.168.0.0/16
      (peer & 0xFFFF0000u) == 0xA9FE0000u ||  // 169.254.0.0/16 link-local
      (peer & 0xFF000000u) == 0x7F000000u) {  // 127.0.0.0/8
    return true;
  }
  // A LAN on public addresses: same subnet as the station interface. Masks wider than /8 are a
  // misconfiguration that would let a large part of the internet in, so they do not count.
  if (own_ip == 0 || (own_mask & 0xFF000000u) != 0xFF000000u) {
    return false;
  }
  return (peer & own_mask) == (own_ip & own_mask);
}
