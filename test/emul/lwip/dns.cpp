#include <cstring>
#include "sockets.h"

extern "C" {

err_t dns_gethostbyname(const char* hostname, ip_addr_t* addr, dns_found_callback found, void* callback_arg) {

  // Simulate a DNS lookup
  if (strcmp(hostname, "example.com") == 0) {
    addr->u_addr.ip4.addr = 0;
    if (found) {
      found(hostname, addr, callback_arg);
    }
    return ERR_OK;
  }

  return ERR_VAL;
}
}
