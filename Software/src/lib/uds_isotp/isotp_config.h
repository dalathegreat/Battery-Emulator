#if !defined(ISOTP_CONFIG_H)
#define ISOTP_CONFIG_H

#ifndef CONFIG_ISOTP_MAX_MSG_LENGTH
#define CONFIG_ISOTP_MAX_MSG_LENGTH 512
#endif
/* Receive of flow control timeout */
#ifndef CONFIG_ISOTP_BS_TIMEOUT
#define CONFIG_ISOTP_BS_TIMEOUT 100
#endif
/* Consecutive Frame timeout */
#ifndef CONFIG_ISOTP_CR_TIMEOUT
#define CONFIG_ISOTP_CR_TIMEOUT 100
#endif
/* Block Size, define number of consecutive frames before sending flow control */
#ifndef CONFIG_ISOTP_BS
#define CONFIG_ISOTP_BS 8
#endif
/* Separation Time minimum */
#ifndef CONFIG_ISOTP_STMIN
#define CONFIG_ISOTP_STMIN 1
#endif
/* Maximum number of wait frame transmission when waiting for flow control frame */
#ifndef CONFIG_ISOTP_WFTMAX
#define CONFIG_ISOTP_WFTMAX 1
#endif
/* Padding byte used in Single Frame and Consecutive Frame when data len is less than 7 bytes. */
#ifndef CONFIG_ISOTP_TX_PADDING_BYTE
#define CONFIG_ISOTP_TX_PADDING_BYTE 0x55
#endif

#endif // ISOTP_CONFIG_H
