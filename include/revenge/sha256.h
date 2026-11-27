#ifndef REVENGE_SHA256_H
#define REVENGE_SHA256_H

#include <stddef.h>
#include <stdint.h>

enum
{
    SHA256_DIGEST_BYTES = 32,
    SHA256_BLOCK_BYTES = 64
};

typedef struct Sha256
{
    uint32_t state[8];
    uint64_t byteCount;
    uint8_t block[SHA256_BLOCK_BYTES];
    size_t blockFill;
} Sha256;

void Sha256_Begin(Sha256 *hash);
void Sha256_Add(Sha256 *hash, const void *data, size_t length);
void Sha256_End(Sha256 *hash, uint8_t digest[SHA256_DIGEST_BYTES]);

/* HMAC-SHA-256 (RFC 2104) of data under key. */
typedef struct HmacSha256
{
    Sha256 inner;
    uint8_t outerKey[SHA256_BLOCK_BYTES];
} HmacSha256;

void HmacSha256_Begin(HmacSha256 *mac, const void *key, size_t keyLength);
void HmacSha256_Add(HmacSha256 *mac, const void *data, size_t length);
void HmacSha256_End(HmacSha256 *mac, uint8_t digest[SHA256_DIGEST_BYTES]);

#endif
