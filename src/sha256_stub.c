/**
 *  @file sha256_stub.c
 *  @author TheSomeMan
 *  @date 2025-12-29
 *  @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 *  @details SHA-256 RAM stub for nRF52 (works on nRF52811/nRF52832/nRF52840)
 *
 *  Assumptions:
 *  - Host resets + halts target
 *  - Host uploads this blob to RAM
 *  - Host sets PC to stub_entry 0x20001001 (Thumb) (See STUBRAM/ORIGIN in linker script)
 *  - Host fills g_stub_params at 0x20000000 (See '.stub_params' in linker script):
 *     - num_segments: number of segments (1..8)
 *     - seg[]: array of (flash_offset, length) pairs
 *  - Host starts execution
 *  - Stub hashes up to 8 segments described by (flash_offset, length).
 *  - Stub writes result to g_stub_result at 0x20000080 (See '.stub_result' in linker script):
 *  - Host reads g_stub_result.status:
 *      - 0 = running
 *      - 1 = finished
 *      - 2 = error
 *  - On finished, host reads g_stub_result.digest (32 bytes)
 */

#include <stdint.h>
#include <stddef.h>

#define FLASH_BASE_ADDR (0x00000000U)

#define SHA256_STUB_MAX_SEGMENTS (8U)
#define SHA256_STUB_DIGEST_SIZE_BYTES (32U)

#define SHA256_STUB_STATUS_RUNNING  (0U)
#define SHA256_STUB_STATUS_FINISHED (1U)
#define SHA256_STUB_STATUS_ERROR    (2U)

typedef struct
{
    uint32_t flash_offset;  // Offset from FLASH_BASE_ADDR
    uint32_t length;        // Segment length in bytes
} segment_t;

typedef struct
{
    uint32_t num_segments;  // 1..SHA256_STUB_MAX_SEGMENTS
    segment_t seg[SHA256_STUB_MAX_SEGMENTS];
} stub_params_t;

typedef struct
{
    volatile uint32_t status;
    uint8_t digest[SHA256_STUB_DIGEST_SIZE_BYTES];
} stub_result_t;

typedef struct
{
    uint32_t h[8];
    uint64_t total_len;
    uint8_t  buf[64];
    uint32_t buf_len;
} sha256_ctx_t;

// ---------------------- Global variables ------------------------

__attribute__ ((used, section (".stub_params"))) /* Placed by linker script */
volatile stub_params_t g_stub_params;

__attribute__ ((used, section (".stub_result"))) /* Placed by linker script */
stub_result_t g_stub_result;

// ------------------------- tiny SHA-256 -------------------------

static inline uint32_t rotr32 (const uint32_t x, const uint32_t n)
{
    return (x >> n) | (x << (32u - n));
}

static inline uint32_t ch (const uint32_t x, const uint32_t y, const uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static inline uint32_t maj (const uint32_t x, const uint32_t y, const uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t big0 (const uint32_t x)
{
    return rotr32 (x, 2) ^ rotr32 (x, 13) ^ rotr32 (x, 22);
}

static inline uint32_t big1 (const uint32_t x)
{
    return rotr32 (x, 6) ^ rotr32 (x, 11) ^ rotr32 (x, 25);
}

static inline uint32_t sml0 (const uint32_t x)
{
    return rotr32 (x, 7) ^ rotr32 (x, 18) ^ (x >> 3);
}

static inline uint32_t sml1 (const uint32_t x)
{
    return rotr32 (x, 17) ^ rotr32 (x, 19) ^ (x >> 10);
}

static uint32_t load_be32 (const uint8_t * const p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static void store_be32 (uint8_t * const p, const uint32_t v)
{
    p[0] = (uint8_t) (v >> 24);
    p[1] = (uint8_t) (v >> 16);
    p[2] = (uint8_t) (v >> 8);
    p[3] = (uint8_t) (v);
}

static const uint32_t K[64] =
{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static void sha256_init (sha256_ctx_t * p_ctx)
{
    p_ctx->h[0] = 0x6a09e667u;
    p_ctx->h[1] = 0xbb67ae85u;
    p_ctx->h[2] = 0x3c6ef372u;
    p_ctx->h[3] = 0xa54ff53au;
    p_ctx->h[4] = 0x510e527fu;
    p_ctx->h[5] = 0x9b05688cu;
    p_ctx->h[6] = 0x1f83d9abu;
    p_ctx->h[7] = 0x5be0cd19u;
    p_ctx->total_len = 0;
    p_ctx->buf_len = 0;
}

static void sha256_compress (sha256_ctx_t * const p_ctx, const uint8_t blk[64])
{
    uint32_t w[64]; // 256 bytes on stack

    for (uint32_t i = 0; i < 16; i++)
    {
        w[i] = load_be32 (&blk[i * 4]);
    }

    for (uint32_t i = 16; i < 64; i++)
    {
        w[i] = sml1 (w[i - 2]) + w[i - 7] + sml0 (w[i - 15]) + w[i - 16];
    }

    uint32_t a = p_ctx->h[0];
    uint32_t b = p_ctx->h[1];
    uint32_t c0 = p_ctx->h[2];
    uint32_t d = p_ctx->h[3];
    uint32_t e = p_ctx->h[4];
    uint32_t f = p_ctx->h[5];
    uint32_t g = p_ctx->h[6];
    uint32_t h = p_ctx->h[7];

    for (uint32_t i = 0; i < 64; i++)
    {
        const uint32_t t1 = h + big1 (e) + ch (e, f, g) + K[i] + w[i];
        const uint32_t t2 = big0 (a) + maj (a, b, c0);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c0;
        c0 = b;
        b = a;
        a = t1 + t2;
    }

    p_ctx->h[0] += a;
    p_ctx->h[1] += b;
    p_ctx->h[2] += c0;
    p_ctx->h[3] += d;
    p_ctx->h[4] += e;
    p_ctx->h[5] += f;
    p_ctx->h[6] += g;
    p_ctx->h[7] += h;
}

static void sha256_update (sha256_ctx_t * const p_ctx, const uint8_t * p_buf,
                           uint32_t len)
{
    p_ctx->total_len += len;

    while (len > 0)
    {
        const uint32_t space = 64u - p_ctx->buf_len;
        const uint32_t take = (len < space) ? len : space;

        for (uint32_t i = 0; i < take; i++)
        {
            p_ctx->buf[p_ctx->buf_len + i] = p_buf[i];
        }

        p_ctx->buf_len += take;
        p_buf += take;
        len -= take;

        if (p_ctx->buf_len == 64u)
        {
            sha256_compress (p_ctx, p_ctx->buf);
            p_ctx->buf_len = 0;
        }
    }
}

static void sha256_final (sha256_ctx_t * const p_ctx,
                          uint8_t out[SHA256_STUB_DIGEST_SIZE_BYTES])
{
    const uint64_t bit_len = p_ctx->total_len * 8u;
    p_ctx->buf[p_ctx->buf_len++] = 0x80u;

    while (p_ctx->buf_len != 56u)
    {
        if (p_ctx->buf_len == 64u)
        {
            sha256_compress (p_ctx, p_ctx->buf);
            p_ctx->buf_len = 0;
        }

        p_ctx->buf[p_ctx->buf_len++] = 0;
    }

    p_ctx->buf[56] = (uint8_t) (bit_len >> 56);
    p_ctx->buf[57] = (uint8_t) (bit_len >> 48);
    p_ctx->buf[58] = (uint8_t) (bit_len >> 40);
    p_ctx->buf[59] = (uint8_t) (bit_len >> 32);
    p_ctx->buf[60] = (uint8_t) (bit_len >> 24);
    p_ctx->buf[61] = (uint8_t) (bit_len >> 16);
    p_ctx->buf[62] = (uint8_t) (bit_len >> 8);
    p_ctx->buf[63] = (uint8_t) (bit_len >> 0);
    sha256_compress (p_ctx, p_ctx->buf);

    for (uint32_t i = 0; i < 8; i++)
    {
        store_be32 (&out[i * 4], p_ctx->h[i]);
    }
}

// ------------------------- entrypoint -------------------------

__attribute__ ((naked, used, section (".text.stub_reset")))
void stub_reset (void)
{
    __asm volatile (
        "ldr r0, =__stack_top__ \n"
        "msr msp, r0            \n"
        "bl  stub_entry         \n"
        "b .                    \n"
    );
}

__attribute__ ((used, section (".text.stub_entry")))
void stub_entry (void)
{
    // If stub_reset is not used, then host MUST set MSP to a safe address before running this function.
    // For worst-case nRF52811 (24 KiB RAM): top of RAM is 0x20006000.
    // A good fixed MSP is e.g. 0x20006000 (or 0x20005C00 for 1 KiB margin).
    g_stub_result.status = SHA256_STUB_STATUS_RUNNING;

    if ((0U == g_stub_params.num_segments)
            || g_stub_params.num_segments > SHA256_STUB_MAX_SEGMENTS)
    {
        g_stub_result.status = SHA256_STUB_STATUS_ERROR; // bad segment count

        while (1)
        {
        }
    }

    sha256_ctx_t ctx;
    sha256_init (&ctx);

    for (uint32_t i = 0; i < g_stub_params.num_segments; ++i)
    {
        const uint32_t off = g_stub_params.seg[i].flash_offset;
        const uint32_t len = g_stub_params.seg[i].length;

        if (0U == len)
        {
            continue;
        }

        const uint8_t * src = (const uint8_t *) (uintptr_t) (FLASH_BASE_ADDR + off);
        sha256_update (&ctx, src, len);
    }

    sha256_final (&ctx, (uint8_t *)&g_stub_result.digest[0]);
    g_stub_result.status = SHA256_STUB_STATUS_FINISHED;

    while (1)
    {
    }
}
