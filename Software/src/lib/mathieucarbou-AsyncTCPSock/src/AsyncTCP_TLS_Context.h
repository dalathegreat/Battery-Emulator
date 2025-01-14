#pragma once

#if ASYNC_TCP_SSL_ENABLED

#include "mbedtls/version.h"
#include "mbedtls/platform.h"
#if MBEDTLS_VERSION_NUMBER < 0x03000000
#include "mbedtls/net.h"
#else
#include "mbedtls/net_sockets.h"
#endif
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#define ASYNCTCP_TLS_CAN_RETRY(r)   (((r) == MBEDTLS_ERR_SSL_WANT_READ) || ((r) == MBEDTLS_ERR_SSL_WANT_WRITE))
#define ASYNCTCP_TLS_EOF(r)         (((r) == MBEDTLS_ERR_SSL_CONN_EOF) || ((r) == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY))

class AsyncTCP_TLS_Context
{
private:
    // These fields must persist for the life of the encrypted connection, destroyed on
    // object destructor.
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_ctr_drbg_context drbg_ctx;
    mbedtls_entropy_context entropy_ctx;

    // These allocate memory during handshake but must be freed on either success or failure
    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt client_cert;
    mbedtls_pk_context client_key;
    bool _have_ca_cert;
    bool _have_client_cert;
    bool _have_client_key;

    unsigned long handshake_timeout;
    unsigned long handshake_start_time;

    int _socket;

    int _startSSLClient(int sck, const char * host_or_ip,
        const unsigned char *rootCABuff, const size_t rootCABuff_len,
        const unsigned char *cli_cert, const size_t cli_cert_len,
        const unsigned char *cli_key, const size_t cli_key_len,
        const char *pskIdent, const char *psKey,
        bool insecure);

    // Delete certificates used in handshake
    void _deleteHandshakeCerts(void);
public:
    AsyncTCP_TLS_Context(void);
    virtual ~AsyncTCP_TLS_Context();

    int startSSLClientInsecure(int sck, const char * host_or_ip);

    int startSSLClient(int sck, const char * host_or_ip,
        const char *pskIdent, const char *psKey);

    int startSSLClient(int sck, const char * host_or_ip,
        const char *rootCABuff,
        const char *cli_cert,
        const char *cli_key);

    int startSSLClient(int sck, const char * host_or_ip,
        const unsigned char *rootCABuff, const size_t rootCABuff_len,
        const unsigned char *cli_cert, const size_t cli_cert_len,
        const unsigned char *cli_key, const size_t cli_key_len);

    int runSSLHandshake(void);

    int write(const uint8_t *data, size_t len);

    int read(uint8_t * data, size_t len);
};

#endif // ASYNC_TCP_SSL_ENABLED