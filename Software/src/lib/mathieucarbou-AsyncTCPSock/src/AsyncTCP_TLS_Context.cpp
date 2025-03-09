#include <Arduino.h>
#include <esp32-hal-log.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <mbedtls/sha256.h>
#include <mbedtls/oid.h>


#include "AsyncTCP_TLS_Context.h"

#if ASYNC_TCP_SSL_ENABLED
#if !defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED) && !defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
#  warning "Please configure IDF framework to include mbedTLS -> Enable pre-shared-key ciphersuites and activate at least one cipher"
#else

static const char *pers = "esp32-tls";

static int _handle_error(int err, const char * function, int line)
{
    if(err == -30848){
        return err;
    }
#ifdef MBEDTLS_ERROR_C
    char error_buf[100];
    mbedtls_strerror(err, error_buf, 100);
    log_e("[%s():%d]: (%d) %s", function, line, err, error_buf);
#else
    log_e("[%s():%d]: code %d", function, line, err);
#endif
    return err;
}

#define handle_error(e) _handle_error(e, __FUNCTION__, __LINE__)

AsyncTCP_TLS_Context::AsyncTCP_TLS_Context(void)
{
    mbedtls_ssl_init(&ssl_ctx);
    mbedtls_ssl_config_init(&ssl_conf);
    mbedtls_ctr_drbg_init(&drbg_ctx);
    _socket = -1;
    _have_ca_cert = false;
    _have_client_cert = false;
    _have_client_key = false;
    handshake_timeout = 120000;
}

int AsyncTCP_TLS_Context::startSSLClientInsecure(int sck, const char * host_or_ip)
{
    return _startSSLClient(sck, host_or_ip,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        NULL, NULL,
        true);
}

int AsyncTCP_TLS_Context::startSSLClient(int sck, const char * host_or_ip,
        const char *pskIdent, const char *psKey)
{
    return _startSSLClient(sck, host_or_ip,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        pskIdent, psKey,
        false);
}

int AsyncTCP_TLS_Context::startSSLClient(int sck, const char * host_or_ip,
        const char *rootCABuff,
        const char *cli_cert,
        const char *cli_key)
{
    return startSSLClient(sck, host_or_ip,
        (const unsigned char *)rootCABuff, (rootCABuff != NULL) ? strlen(rootCABuff) + 1 : 0,
        (const unsigned char *)cli_cert, (cli_cert != NULL) ? strlen(cli_cert) + 1 : 0,
        (const unsigned char *)cli_key, (cli_key != NULL) ? strlen(cli_key) + 1 : 0);
}

int AsyncTCP_TLS_Context::startSSLClient(int sck, const char * host_or_ip,
        const unsigned char *rootCABuff, const size_t rootCABuff_len,
        const unsigned char *cli_cert, const size_t cli_cert_len,
        const unsigned char *cli_key, const size_t cli_key_len)
{
    return _startSSLClient(sck, host_or_ip,
        rootCABuff, rootCABuff_len,
        cli_cert, cli_cert_len,
        cli_key, cli_key_len,
        NULL, NULL,
        false);
}

int AsyncTCP_TLS_Context::_startSSLClient(int sck, const char * host_or_ip,
        const unsigned char *rootCABuff, const size_t rootCABuff_len,
        const unsigned char *cli_cert, const size_t cli_cert_len,
        const unsigned char *cli_key, const size_t cli_key_len,
        const char *pskIdent, const char *psKey,
        bool insecure)
{
    int ret;
    int enable = 1;

    // The insecure flag will skip server certificate validation. Otherwise some
    // certificate is required.
    if (rootCABuff == NULL && pskIdent == NULL && psKey == NULL && !insecure) {
        return -1;
    }

#define ROE(x,msg) { if (((x)<0)) { log_e("LWIP Socket config of " msg " failed."); return -1; }}
//     ROE(lwip_setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)),"SO_RCVTIMEO");
//     ROE(lwip_setsockopt(sck, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)),"SO_SNDTIMEO");

    ROE(lwip_setsockopt(sck, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)),"TCP_NODELAY");
    ROE(lwip_setsockopt(sck, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)),"SO_KEEPALIVE");

    log_v("Seeding the random number generator");
    mbedtls_entropy_init(&entropy_ctx);

    ret = mbedtls_ctr_drbg_seed(&drbg_ctx, mbedtls_entropy_func,
                                &entropy_ctx, (const unsigned char *) pers, strlen(pers));
    if (ret < 0) {
        return handle_error(ret);
    }

    log_v("Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(&ssl_conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        return handle_error(ret);
    }

    if (insecure) {
        mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
        log_i("WARNING: Skipping SSL Verification. INSECURE!");
    } else if (rootCABuff != NULL) {
        log_v("Loading CA cert");
        mbedtls_x509_crt_init(&ca_cert);
        mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        ret = mbedtls_x509_crt_parse(&ca_cert, rootCABuff, rootCABuff_len);
        _have_ca_cert = true;
        mbedtls_ssl_conf_ca_chain(&ssl_conf, &ca_cert, NULL);
        if (ret < 0) {
            // free the ca_cert in the case parse failed, otherwise, the old ca_cert still in the heap memory, that lead to "out of memory" crash.
            _deleteHandshakeCerts();
            return handle_error(ret);
        }
    } else if (pskIdent != NULL && psKey != NULL) {
        log_v("Setting up PSK");
        // convert PSK from hex to binary
        if ((strlen(psKey) & 1) != 0 || strlen(psKey) > 2*MBEDTLS_PSK_MAX_LEN) {
            log_e("pre-shared key not valid hex or too long");
            return -1;
        }
        unsigned char psk[MBEDTLS_PSK_MAX_LEN];
        size_t psk_len = strlen(psKey)/2;
        for (int j=0; j<strlen(psKey); j+= 2) {
            char c = psKey[j];
            if (c >= '0' && c <= '9') c -= '0';
            else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
            else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
            else return -1;
            psk[j/2] = c<<4;
            c = psKey[j+1];
            if (c >= '0' && c <= '9') c -= '0';
            else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
            else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
            else return -1;
            psk[j/2] |= c;
        }
        // set mbedtls config
        ret = mbedtls_ssl_conf_psk(&ssl_conf, psk, psk_len,
                 (const unsigned char *)pskIdent, strlen(pskIdent));
        if (ret != 0) {
            log_e("mbedtls_ssl_conf_psk returned %d", ret);
            return handle_error(ret);
        }
    } else {
        return -1;
    }

    if (!insecure && cli_cert != NULL && cli_key != NULL) {
        mbedtls_x509_crt_init(&client_cert);
        mbedtls_pk_init(&client_key);

        log_v("Loading CRT cert");

        ret = mbedtls_x509_crt_parse(&client_cert, cli_cert, cli_cert_len);
        _have_client_cert = true;
        if (ret < 0) {
            // free the client_cert in the case parse failed, otherwise, the old client_cert still in the heap memory, that lead to "out of memory" crash.
            _deleteHandshakeCerts();
            return handle_error(ret);
        }

        log_v("Loading private key");
#if MBEDTLS_VERSION_NUMBER < 0x03000000
        ret = mbedtls_pk_parse_key(&client_key, cli_key, cli_key_len, NULL, 0);
#else
        ret = mbedtls_pk_parse_key(&client_key, cli_key, cli_key_len, NULL, 0, mbedtls_ctr_drbg_random, &drbg_ctx);
#endif
        _have_client_key = true;

        if (ret != 0) {
            _deleteHandshakeCerts();
            return handle_error(ret);
        }

        mbedtls_ssl_conf_own_cert(&ssl_conf, &client_cert, &client_key);
    }

    log_v("Setting hostname for TLS session...");

    // Hostname set here should match CN in server certificate
    if ((ret = mbedtls_ssl_set_hostname(&ssl_ctx, host_or_ip)) != 0){
        _deleteHandshakeCerts();
        return handle_error(ret);
    }

    mbedtls_ssl_conf_rng(&ssl_conf, mbedtls_ctr_drbg_random, &drbg_ctx);

    if ((ret = mbedtls_ssl_setup(&ssl_ctx, &ssl_conf)) != 0) {
        _deleteHandshakeCerts();
        return handle_error(ret);
    }

    _socket = sck;
    mbedtls_ssl_set_bio(&ssl_ctx, &_socket, mbedtls_net_send, mbedtls_net_recv, NULL );
    handshake_start_time = 0;

    return 0;
}

int AsyncTCP_TLS_Context::runSSLHandshake(void)
{
    int ret, flags;
    if (_socket < 0) return -1;

    if (handshake_start_time == 0) handshake_start_time = millis();
    ret = mbedtls_ssl_handshake(&ssl_ctx);
    if (ret != 0) {
        // Something happened before SSL handshake could be completed

        // Negotiation error, other than socket not readable/writable when required
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            return handle_error(ret);
        }

        // Handshake is taking too long
        if ((millis()-handshake_start_time) > handshake_timeout)
            return -1;

        // Either MBEDTLS_ERR_SSL_WANT_READ or MBEDTLS_ERR_SSL_WANT_WRITE
        return ret;
    }

    // Handshake completed, validate remote side if required...

    if (_have_client_cert && _have_client_key) {
        log_d("Protocol is %s Ciphersuite is %s", mbedtls_ssl_get_version(&ssl_ctx), mbedtls_ssl_get_ciphersuite(&ssl_ctx));
        if ((ret = mbedtls_ssl_get_record_expansion(&ssl_ctx)) >= 0) {
            log_d("Record expansion is %d", ret);
        } else {
            log_w("Record expansion is unknown (compression)");
        }
    }

    log_v("Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(&ssl_ctx)) != 0) {
        char buf[512];
        memset(buf, 0, sizeof(buf));
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
        log_e("Failed to verify peer certificate! verification info: %s", buf);
        _deleteHandshakeCerts();
        return handle_error(ret);
    } else {
        log_v("Certificate verified.");
    }

    _deleteHandshakeCerts();

    log_v("Free internal heap after TLS %u", ESP.getFreeHeap());

    return 0;
}

int AsyncTCP_TLS_Context::write(const uint8_t *data, size_t len)
{
    if (_socket < 0) return -1;

    log_v("Writing packet, %d bytes unencrypted...", len);
    int ret = mbedtls_ssl_write(&ssl_ctx, data, len);
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0) {
        log_v("Handling error %d", ret); //for low level debug
        return handle_error(ret);
    }
    return ret;
}

int AsyncTCP_TLS_Context::read(uint8_t * data, size_t len)
{
    int ret = mbedtls_ssl_read(&ssl_ctx, data, len);
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0) {
        log_v("Handling error %d", ret); //for low level debug
        return handle_error(ret);
    }
    if (ret > 0) log_v("Read packet, %d out of %d requested bytes...", ret, len);
    return ret;
}

void AsyncTCP_TLS_Context::_deleteHandshakeCerts(void)
{
    if (_have_ca_cert) {
        log_v("Cleaning CA certificate.");
        mbedtls_x509_crt_free(&ca_cert);
        _have_ca_cert = false;
    }
    if (_have_client_cert) {
        log_v("Cleaning client certificate.");
        mbedtls_x509_crt_free(&client_cert);
        _have_client_cert = false;
    }
    if (_have_client_key) {
        log_v("Cleaning client certificate key.");
        mbedtls_pk_free(&client_key);
        _have_client_key = false;
    }
}

AsyncTCP_TLS_Context::~AsyncTCP_TLS_Context()
{
    _deleteHandshakeCerts();

    log_v("Cleaning SSL connection.");

    mbedtls_ssl_free(&ssl_ctx);
    mbedtls_ssl_config_free(&ssl_conf);
    mbedtls_ctr_drbg_free(&drbg_ctx);
    mbedtls_entropy_free(&entropy_ctx); // <-- Is this OK to free if mbedtls_entropy_init() has not been called on it?
}

#endif
#endif // ASYNC_TCP_SSL_ENABLED