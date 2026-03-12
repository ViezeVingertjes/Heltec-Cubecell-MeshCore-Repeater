#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sha256_context_ {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[64];
} sha256_context;

void sha256_init(sha256_context* ctx);
void sha256_update(sha256_context* ctx, const uint8_t* data, size_t len);
void sha256_final(sha256_context* ctx, uint8_t* hash);
void sha256(const uint8_t* data, size_t len, uint8_t* hash);

#ifdef __cplusplus
}
#endif

#endif
