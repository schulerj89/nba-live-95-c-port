/* SHA256 implementation copied unchanged from nba_rom.c; standalone bootstrap
 * validates the same complete immutable ROM before interpreting its data. */
#include "nba_bootstrap_internal.h"
#define NBA_ROM_EXPECTED_SHA256 "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t block[64];
    size_t block_size;
} NbaSha256;

static uint32_t sha_rotr(uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32u - bits));
}

static void sha_transform(NbaSha256 *sha, const uint8_t block[64]) {
    static const uint32_t k[64] = {
        0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u,
        0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
        0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u,
        0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
        0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu,
        0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
        0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u,
        0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
        0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u,
        0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
        0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u,
        0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
        0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u,
        0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
        0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
        0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u
    };
    uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               (uint32_t)block[i * 4 + 3];
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = sha_rotr(w[i - 15], 7) ^ sha_rotr(w[i - 15], 18) ^
                      (w[i - 15] >> 3);
        uint32_t s1 = sha_rotr(w[i - 2], 17) ^ sha_rotr(w[i - 2], 19) ^
                      (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = sha->state[0], b = sha->state[1], c = sha->state[2];
    uint32_t d = sha->state[3], e = sha->state[4], f = sha->state[5];
    uint32_t g = sha->state[6], h = sha->state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t sum1 = sha_rotr(e, 6) ^ sha_rotr(e, 11) ^ sha_rotr(e, 25);
        uint32_t choose = (e & f) ^ (~e & g);
        uint32_t temp1 = h + sum1 + choose + k[i] + w[i];
        uint32_t sum0 = sha_rotr(a, 2) ^ sha_rotr(a, 13) ^ sha_rotr(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = sum0 + majority;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    sha->state[0] += a; sha->state[1] += b; sha->state[2] += c;
    sha->state[3] += d; sha->state[4] += e; sha->state[5] += f;
    sha->state[6] += g; sha->state[7] += h;
}

static void sha_init(NbaSha256 *sha) {
    static const uint32_t initial[8] = {
        0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
        0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u
    };
    memcpy(sha->state, initial, sizeof(initial));
    sha->bit_count = 0;
    sha->block_size = 0;
}

static void sha_update(NbaSha256 *sha, const uint8_t *data, size_t size) {
    sha->bit_count += (uint64_t)size * 8u;
    while (size > 0) {
        size_t available = sizeof(sha->block) - sha->block_size;
        size_t chunk = size < available ? size : available;
        memcpy(sha->block + sha->block_size, data, chunk);
        sha->block_size += chunk;
        data += chunk;
        size -= chunk;
        if (sha->block_size == sizeof(sha->block)) {
            sha_transform(sha, sha->block);
            sha->block_size = 0;
        }
    }
}

static void sha_final(NbaSha256 *sha, uint8_t digest[32]) {
    uint64_t original_bits = sha->bit_count;
    uint8_t padding[64] = {0x80};
    size_t padding_size = sha->block_size < 56 ?
                          56 - sha->block_size : 120 - sha->block_size;
    sha_update(sha, padding, padding_size);
    uint8_t length[8];
    for (int i = 0; i < 8; ++i) {
        length[7 - i] = (uint8_t)(original_bits >> (i * 8));
    }
    sha_update(sha, length, sizeof(length));
    for (int i = 0; i < 8; ++i) {
        digest[i * 4] = (uint8_t)(sha->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(sha->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(sha->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)sha->state[i];
    }
}

bool nba_bootstrap_rom_valid(const uint8_t *data, size_t size) {
    static const char hex[] = "0123456789abcdef";
    uint8_t digest[32];
    char actual[65];
    NbaSha256 sha;
    sha_init(&sha);
    sha_update(&sha, data, size);
    sha_final(&sha, digest);
    for (int i = 0; i < 32; ++i) {
        actual[i * 2] = hex[digest[i] >> 4];
        actual[i * 2 + 1] = hex[digest[i] & 15];
    }
    actual[64] = '\0';
    if (strcmp(actual, NBA_ROM_EXPECTED_SHA256) != 0) {
        fprintf(stderr, "[ROM] SHA-256 mismatch. Expected %s, got %s\n",
                NBA_ROM_EXPECTED_SHA256, actual);
        return false;
    }
    return true;
}

