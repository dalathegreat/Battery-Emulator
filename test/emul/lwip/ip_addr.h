#pragma once

#include <stdint.h>
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ip_addr {
  union {
    ip4_addr_t ip4;
  } u_addr;
  /** @ref lwip_ip_addr_type */
  uint8_t type;
} ip_addr_t;

#ifdef __cplusplus
}
#endif
