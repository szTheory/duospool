/* SPDX-License-Identifier: MIT OR Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors */
/*
 * sha256.c - SHA-256 from FIPS 180-4, in-house (D-47). The constants are the first 32 bits of the fractional parts of the square
 * roots (initial value) and cube roots (K table) of the first primes, as published in the standard; no code was copied.
 * D-83: the sanitize preset has no -fwrapv, so every intended wrap here is unsigned arithmetic with explicit casts: the word
 * load widens each byte to uint32_t before it shifts (a promoted int shifted into its sign bit would be undefined), the
 * additions are modulo 2^32 on uint32_t, the rotates use shift counts from 2 to 25, and the length counter is a uint64_t.
 */
#include <stddef.h>
#include <stdint.h>

#include "ds_sha256.h"

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

static uint32_t rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32u - n));
}

/* the block is exactly 64 bytes; each word is built big-endian from bytes widened to uint32_t before any shift */
static void compress(uint32_t h[8], const uint8_t *p)
{
    uint32_t w[64];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    uint32_t g;
    uint32_t hh;
    uint32_t i;

    for (i = 0u; i < 16u; i++) {
        const uint8_t *q = p + (size_t)i * 4u;
        w[i] = ((uint32_t)q[0] << 24) | ((uint32_t)q[1] << 16) | ((uint32_t)q[2] << 8) | (uint32_t)q[3];
    }
    for (i = 16u; i < 64u; i++) {
        uint32_t s0 = rotr(w[i - 15u], 7u) ^ rotr(w[i - 15u], 18u) ^ (w[i - 15u] >> 3);
        uint32_t s1 = rotr(w[i - 2u], 17u) ^ rotr(w[i - 2u], 19u) ^ (w[i - 2u] >> 10);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    a = h[0];
    b = h[1];
    c = h[2];
    d = h[3];
    e = h[4];
    f = h[5];
    g = h[6];
    hh = h[7];
    for (i = 0u; i < 64u; i++) {
        uint32_t big1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = hh + big1 + ch + K[i] + w[i];
        uint32_t big0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = big0 + maj;
        hh = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
}

void ds_sha256_init(struct ds_sha256 *s)
{
    s->h[0] = 0x6a09e667u;
    s->h[1] = 0xbb67ae85u;
    s->h[2] = 0x3c6ef372u;
    s->h[3] = 0xa54ff53au;
    s->h[4] = 0x510e527fu;
    s->h[5] = 0x9b05688cu;
    s->h[6] = 0x1f83d9abu;
    s->h[7] = 0x5be0cd19u;
    s->bits = 0u;
    s->fill = 0u;
    __builtin_memset(s->buf, 0, sizeof s->buf);
}

void ds_sha256_update(struct ds_sha256 *s, const uint8_t *data, size_t len)
{
    if (len == 0u) {
        return;
    }
    s->bits += (uint64_t)len * 8u; /* the length is counted in bits, not bytes (FIPS 180-4, section 5.1.1) */
    if (s->fill != 0u) {
        size_t room = sizeof s->buf - (size_t)s->fill;
        size_t take = len < room ? len : room;
        __builtin_memcpy(s->buf + s->fill, data, take);
        s->fill += (uint32_t)take;
        data += take;
        len -= take;
        if (s->fill < sizeof s->buf) {
            return;
        }
        compress(s->h, s->buf);
        s->fill = 0u;
    }
    while (len >= sizeof s->buf) {
        compress(s->h, data);
        data += sizeof s->buf;
        len -= sizeof s->buf;
    }
    if (len != 0u) {
        __builtin_memcpy(s->buf, data, len);
        s->fill = (uint32_t)len;
    }
}

void ds_sha256_final(struct ds_sha256 *s, uint8_t out[DS_SHA256_BYTES])
{
    uint64_t bits = s->bits;
    uint32_t i;

    /* 0x80, then zeros up to 56 mod 64, then the 64-bit big-endian bit length */
    s->buf[s->fill] = 0x80u;
    s->fill++;
    if (s->fill > 56u) {
        __builtin_memset(s->buf + s->fill, 0, sizeof s->buf - (size_t)s->fill);
        compress(s->h, s->buf);
        s->fill = 0u;
    }
    __builtin_memset(s->buf + s->fill, 0, 56u - (size_t)s->fill);
    for (i = 0u; i < 8u; i++) {
        s->buf[56u + i] = (uint8_t)((bits >> (56u - 8u * i)) & 0xffu);
    }
    compress(s->h, s->buf);
    for (i = 0u; i < 8u; i++) {
        out[4u * i] = (uint8_t)((s->h[i] >> 24) & 0xffu);
        out[4u * i + 1u] = (uint8_t)((s->h[i] >> 16) & 0xffu);
        out[4u * i + 2u] = (uint8_t)((s->h[i] >> 8) & 0xffu);
        out[4u * i + 3u] = (uint8_t)(s->h[i] & 0xffu);
    }
}

void ds_sha256(const uint8_t *data, size_t len, uint8_t out[DS_SHA256_BYTES])
{
    struct ds_sha256 s;

    ds_sha256_init(&s);
    ds_sha256_update(&s, data, len);
    ds_sha256_final(&s, out);
}
