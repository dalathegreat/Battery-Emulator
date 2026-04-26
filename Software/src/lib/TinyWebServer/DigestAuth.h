#ifndef DIGEST_AUTH_H
#define DIGEST_AUTH_H

#include <array>
#include <map>
#include "TwsHandler.h"

extern "C" {
#include <mbedtls/md5.h>
#include <mbedtls/sha256.h>
}

class Sha256Hash {
public:
    void init() {
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256
    }

    void update(const char *data, size_t len) {
        mbedtls_sha256_update(&ctx, (const unsigned char *)data, len);
    }

    void toHex(char hash_output_hex[64]) {
        uint8_t hash_output[32];
        mbedtls_sha256_finish(&ctx, hash_output);
        for(int i = 0; i < 32; i++) {
            hash_output_hex[i*2] = "0123456789abcdef"[hash_output[i] >> 4];
            hash_output_hex[i*2 + 1] = "0123456789abcdef"[hash_output[i] & 0x0F];
        }
    }

    int getHashLengthHex() const {
        return 64; // SHA-256 produces a 32-byte hash, which is 64 hex characters
    }

private:
    mbedtls_sha256_context ctx;
};

class Md5Hash {
public:
    void init() {
        mbedtls_md5_init(&ctx);
        mbedtls_md5_starts(&ctx);
    }

    void update(const char *data, size_t len) {
        mbedtls_md5_update(&ctx, (const unsigned char *)data, len);
    }

    void toHex(char hash_output_hex[32]) {
        uint8_t hash_output[16];
        mbedtls_md5_finish(&ctx, hash_output);
        for(int i = 0; i < 16; i++) {
            hash_output_hex[i*2] = "0123456789abcdef"[hash_output[i] >> 4];
            hash_output_hex[i*2 + 1] = "0123456789abcdef"[hash_output[i] & 0x0F];
        }
    }

    int getHashLengthHex() const {
        return 32; // MD5 produces a 16-byte hash, which is 32 hex characters
    }

private:
    mbedtls_md5_context ctx;
};

const int NONCE_LENGTH = 32;
typedef std::array<char, NONCE_LENGTH> Nonce;

template<typename HASH_CONTEXT>
struct DigestAuthState {
    bool authed;
    bool denied;
    uint8_t parse_state;
    uint8_t hash_state;
    HASH_CONTEXT ha2;
    HASH_CONTEXT r;
    char response[64];
    Nonce nonce;
};

class DigestAuthSessionManager {
public:
    DigestAuthSessionManager(uint64_t timeout_ms = 3600 * 1000) : timeout_ms(timeout_ms) {}
    bool check(Nonce nonce, uint32_t nc);
private:
    void clear_old_sessions();

    struct Session {
        uint64_t last_seen;
        uint32_t count;
    };
    std::map<Nonce, Session> sessions;
    uint64_t timeout_ms;
};

typedef int (*GetPasswordHashFunc)(const char* username, char* output);

template<typename HASH_CONTEXT, int HASH_TYPE>
class DigestAuth : public TwsStatefulMiddleware<DigestAuthState<HASH_CONTEXT>> {
public:
    DigestAuth(GetPasswordHashFunc getPasswordHash, DigestAuthSessionManager *sessionManager = nullptr);
    void handleHeader(TwsRequest &request, const char *line, int len) override;
    int handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) override;
    bool denyIfUnauthed(TwsRequest &request);
    int handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) override;
    void handleRequest(TwsRequest &request) override;

    GetPasswordHashFunc getPasswordHash;
    DigestAuthSessionManager *sessionManager = nullptr;
};

using Md5DigestAuth = DigestAuth<Md5Hash, 0>;
using Sha256DigestAuth = DigestAuth<Sha256Hash, 1>;

#endif // DIGEST_AUTH_H
