#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "../src/sha256_stub.c"

void setUp (void)
{
}

void tearDown (void)
{
}

static void assert_sha256 (
    const uint8_t * const p_message,
    const uint32_t message_len,
    const uint8_t expected_digest[SHA256_STUB_DIGEST_SIZE_BYTES])
{
    sha256_ctx_t ctx;
    uint8_t digest[SHA256_STUB_DIGEST_SIZE_BYTES];
    sha256_init (&ctx);
    sha256_update (&ctx, p_message, message_len);
    sha256_final (&ctx, digest);
    TEST_ASSERT_EQUAL_HEX8_ARRAY (expected_digest, digest, sizeof (digest));
}

void test_sha256_empty_message_matches_fips_180_4 (void)
{
    static const uint8_t expected_digest[SHA256_STUB_DIGEST_SIZE_BYTES] =
    {
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55,
    };
    assert_sha256 (NULL, 0, expected_digest);
}

void test_sha256_abc_matches_fips_180_4 (void)
{
    static const uint8_t message[] = "abc";
    static const uint8_t expected_digest[SHA256_STUB_DIGEST_SIZE_BYTES] =
    {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    };
    assert_sha256 (message, sizeof (message) - 1U, expected_digest);
}

void test_sha256_multiblock_message_matches_fips_180_4 (void)
{
    static const uint8_t message[] =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    static const uint8_t expected_digest[SHA256_STUB_DIGEST_SIZE_BYTES] =
    {
        0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
        0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
        0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
        0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1,
    };
    assert_sha256 (message, sizeof (message) - 1U, expected_digest);
}
