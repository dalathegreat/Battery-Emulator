#ifndef LWIP_HDR_IP4_ADDR_H
#define LWIP_HDR_IP4_ADDR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ip4_addr {
  uint32_t addr;
};

typedef struct ip4_addr ip4_addr_t;

/** 255.255.255.255 */
#define IPADDR_NONE ((u32_t)0xffffffffUL)
/** 127.0.0.1 */
#define IPADDR_LOOPBACK ((u32_t)0x7f000001UL)
/** 0.0.0.0 */
#define IPADDR_ANY ((u32_t)0x00000000UL)
/** 255.255.255.255 */
#define IPADDR_BROADCAST ((u32_t)0xffffffffUL)

#ifdef __cplusplus
}
#endif

#endif
