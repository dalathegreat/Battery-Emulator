#include <stddef.h>

typedef struct {

} mbedtls_md5_context;

void mbedtls_md5_init(mbedtls_md5_context* ctx);
void mbedtls_md5_starts(mbedtls_md5_context* ctx);
void mbedtls_md5_update(mbedtls_md5_context* ctx, const unsigned char* input, size_t ilen);
void mbedtls_md5_finish(mbedtls_md5_context* ctx, unsigned char output[16]);
