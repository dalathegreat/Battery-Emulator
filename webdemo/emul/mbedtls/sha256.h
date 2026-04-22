#include <stddef.h>

typedef struct {

} mbedtls_sha256_context;

void mbedtls_sha256_init(mbedtls_sha256_context* ctx);
void mbedtls_sha256_starts(mbedtls_sha256_context* ctx, int is224);
void mbedtls_sha256_update(mbedtls_sha256_context* ctx, const unsigned char* input, size_t ilen);
void mbedtls_sha256_finish(mbedtls_sha256_context* ctx, unsigned char output[32]);
