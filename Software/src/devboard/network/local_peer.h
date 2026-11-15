#pragma once

#include <stdint.h>
#include <string.h>
#include <strings.h>

// Tells whether a request comes from the local network, so the web interface can refuse requests
// arriving through a port forward while password protection is off. No check on addresses or
// headers can be airtight: anything that rewrites both the source address and the Host header looks
// local. These two keep the device from answering the internet by accident and off mass scans;
// only password protection keeps out someone who targets it.

// The peer address. A plain port forward changes only the destination, so the peer keeps its public
// address and lands outside every range below. Some routers also masquerade forwarded traffic,
// so every internet client arrives from the gateway's LAN address: the gateway itself is therefore
// never local (nobody browses from the router; a hairpin connection to the public address from the
// LAN is refused too). Sources rewritten to another local address (a reverse proxy or tunnel on the
// LAN, an upstream box in a double NAT that masquerades) still pass here; http_host_is_local()
// catches them as long as they keep the public name or address the client used.
//
// 100.64.0.0/10 (carrier-grade NAT) is deliberately not local: with the router's WAN address in an
// ISP's CGNAT range, other customers of the same ISP arrive from it.
//
// All addresses in host byte order (10.0.0.1 is 0x0A000001). own_ip/own_mask/own_gateway describe
// the station interface; 0 when it has no address.
inline bool ipv4_is_local_peer(uint32_t peer, uint32_t own_ip, uint32_t own_mask, uint32_t own_gateway) {
  if (own_gateway != 0 && peer == own_gateway) {
    return false;
  }
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

// Parses a dotted-quad IPv4 literal of exactly len characters into host byte order.
inline bool parse_ipv4_literal(const char* s, size_t len, uint32_t* out) {
  uint32_t ip = 0;
  size_t i = 0;
  for (int part = 0; part < 4; part++) {
    if (part > 0) {
      if (i >= len || s[i] != '.') {
        return false;
      }
      i++;
    }
    uint32_t value = 0;
    size_t digits = 0;
    while (i < len && s[i] >= '0' && s[i] <= '9' && digits < 3) {
      value = value * 10 + (uint32_t)(s[i] - '0');
      i++;
      digits++;
    }
    if (digits == 0 || value > 255) {
      return false;
    }
    ip = (ip << 8) | value;
  }
  if (i != len) {
    return false;
  }
  *out = ip;
  return true;
}

// The Host header: the name or address the client typed. Through a port forward that is the public
// address or a dynamic DNS name, which mass scanners send too, whatever the router does to the source.
// Local means:
//   - an IPv4 literal that is one of the device's own addresses (own_ip, ap_ip; 0 when absent),
//   - a single-label name (only a local resolver can answer for one),
//   - a name whose first label is the device's hostname, so router DNS names work under any domain
//     (battery-emulator-ab12.fritz.box),
//   - a name under a suffix that public DNS never resolves.
// A missing Host header (HTTP/1.0) tells nothing and passes. Port and trailing dot are ignored.
inline bool http_host_is_local(const char* host, const char* hostname, uint32_t own_ip, uint32_t ap_ip) {
  size_t len = strlen(host);
  if (len == 0) {
    return true;
  }
  if (host[0] == '[') {
    return false;  // IPv6 literal: the server listens on IPv4 only, so the client used some other address
  }
  const char* colon = (const char*)memchr(host, ':', len);
  if (colon != nullptr) {
    len = (size_t)(colon - host);
  }
  if (len > 0 && host[len - 1] == '.') {
    len--;
  }
  if (len == 0) {
    return false;
  }

  uint32_t literal;
  if (parse_ipv4_literal(host, len, &literal)) {
    return (own_ip != 0 && literal == own_ip) || (ap_ip != 0 && literal == ap_ip);
  }

  const char* dot = (const char*)memchr(host, '.', len);
  if (dot == nullptr) {
    return true;
  }
  const size_t first_label = (size_t)(dot - host);
  const size_t hostname_len = strlen(hostname);
  if (hostname_len > 0 && first_label == hostname_len && strncasecmp(host, hostname, hostname_len) == 0) {
    return true;
  }
  static const char* const local_suffixes[] = {".local", ".lan", ".home", ".home.arpa", ".internal", ".localdomain"};
  for (const char* suffix : local_suffixes) {
    const size_t suffix_len = strlen(suffix);
    if (len > suffix_len && strncasecmp(host + len - suffix_len, suffix, suffix_len) == 0) {
      return true;
    }
  }
  return false;
}
