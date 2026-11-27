/* SHA-256 (FIPS 180-4) and HMAC-SHA-256 (RFC 2104). */
#include "revenge/sha256.h"

#include <string.h>

static const uint32_t RoundConstants[64] = {
    0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u, 0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
    0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u, 0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
    0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu, 0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
    0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u, 0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
    0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u, 0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
    0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u, 0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
    0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u, 0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
    0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u, 0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u
};

static uint32_t RotateRight(uint32_t value, unsigned bits)
{
    return (value >> bits) | (value << (32u - bits));
}

static void CompressBlock(Sha256 *hash, const uint8_t block[SHA256_BLOCK_BYTES])
{
    uint32_t schedule[64];
    uint32_t a, b, c, d, e, f, g, h;
    unsigned round;

    for (round = 0; round < 16; ++round)
    {
        schedule[round] = (uint32_t) block[round * 4] << 24 | (uint32_t) block[round * 4 + 1] << 16 |
            (uint32_t) block[round * 4 + 2] << 8 | block[round * 4 + 3];
    }
    for (round = 16; round < 64; ++round)
    {
        uint32_t s0 = RotateRight(schedule[round - 15], 7) ^ RotateRight(schedule[round - 15], 18) ^
            (schedule[round - 15] >> 3);
        uint32_t s1 = RotateRight(schedule[round - 2], 17) ^ RotateRight(schedule[round - 2], 19) ^
            (schedule[round - 2] >> 10);
        schedule[round] = schedule[round - 16] + s0 + schedule[round - 7] + s1;
    }
    a = hash->state[0];
    b = hash->state[1];
    c = hash->state[2];
    d = hash->state[3];
    e = hash->state[4];
    f = hash->state[5];
    g = hash->state[6];
    h = hash->state[7];
    for (round = 0; round < 64; ++round)
    {
        uint32_t sum1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
        uint32_t choose = (e & f) ^ (~e & g);
        uint32_t first = h + sum1 + choose + RoundConstants[round] + schedule[round];
        uint32_t sum0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t second = sum0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    hash->state[0] += a;
    hash->state[1] += b;
    hash->state[2] += c;
    hash->state[3] += d;
    hash->state[4] += e;
    hash->state[5] += f;
    hash->state[6] += g;
    hash->state[7] += h;
}

void Sha256_Begin(Sha256 *hash)
{
    static const uint32_t initialState[8] = {
        0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au, 0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
    };

    memcpy(hash->state, initialState, sizeof(initialState));
    hash->byteCount = 0;
    hash->blockFill = 0;
}

void Sha256_Add(Sha256 *hash, const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *) data;

    hash->byteCount += length;
    while (length > 0)
    {
        size_t take = SHA256_BLOCK_BYTES - hash->blockFill;

        if (take > length)
            take = length;
        memcpy(hash->block + hash->blockFill, bytes, take);
        hash->blockFill += take;
        bytes += take;
        length -= take;
        if (hash->blockFill == SHA256_BLOCK_BYTES)
        {
            CompressBlock(hash, hash->block);
            hash->blockFill = 0;
        }
    }
}

void Sha256_End(Sha256 *hash, uint8_t digest[SHA256_DIGEST_BYTES])
{
    uint64_t bitCount = hash->byteCount * 8;
    uint8_t lengthBytes[8];
    static const uint8_t padding[SHA256_BLOCK_BYTES] = { 0x80 };
    size_t padLength;
    unsigned index;

    for (index = 0; index < 8; ++index)
        lengthBytes[index] = (uint8_t) (bitCount >> (56 - index * 8));
    padLength = hash->blockFill < 56 ? 56 - hash->blockFill : 120 - hash->blockFill;
    Sha256_Add(hash, padding, padLength);
    Sha256_Add(hash, lengthBytes, sizeof(lengthBytes));
    for (index = 0; index < 8; ++index)
    {
        digest[index * 4] = (uint8_t) (hash->state[index] >> 24);
        digest[index * 4 + 1] = (uint8_t) (hash->state[index] >> 16);
        digest[index * 4 + 2] = (uint8_t) (hash->state[index] >> 8);
        digest[index * 4 + 3] = (uint8_t) hash->state[index];
    }
}

void HmacSha256_Begin(HmacSha256 *mac, const void *key, size_t keyLength)
{
    uint8_t blockKey[SHA256_BLOCK_BYTES] = { 0 };
    uint8_t innerKey[SHA256_BLOCK_BYTES];
    unsigned index;

    if (keyLength > SHA256_BLOCK_BYTES)
    {
        Sha256 keyHash;

        Sha256_Begin(&keyHash);
        Sha256_Add(&keyHash, key, keyLength);
        Sha256_End(&keyHash, blockKey);
    }
    else
    {
        memcpy(blockKey, key, keyLength);
    }
    for (index = 0; index < SHA256_BLOCK_BYTES; ++index)
    {
        innerKey[index] = blockKey[index] ^ 0x36;
        mac->outerKey[index] = blockKey[index] ^ 0x5C;
    }
    Sha256_Begin(&mac->inner);
    Sha256_Add(&mac->inner, innerKey, sizeof(innerKey));
}

void HmacSha256_Add(HmacSha256 *mac, const void *data, size_t length)
{
    Sha256_Add(&mac->inner, data, length);
}

void HmacSha256_End(HmacSha256 *mac, uint8_t digest[SHA256_DIGEST_BYTES])
{
    uint8_t innerDigest[SHA256_DIGEST_BYTES];
    Sha256 outer;

    Sha256_End(&mac->inner, innerDigest);
    Sha256_Begin(&outer);
    Sha256_Add(&outer, mac->outerKey, sizeof(mac->outerKey));
    Sha256_Add(&outer, innerDigest, sizeof(innerDigest));
    Sha256_End(&outer, digest);
}
