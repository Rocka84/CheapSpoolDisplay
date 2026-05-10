#include "HKDF.h"
#include <mbedtls/md.h>
#include <cstring>

bool HKDF::deriveSHA256(const uint8_t* salt, size_t salt_len,
                        const uint8_t* ikm, size_t ikm_len,
                        const uint8_t* info, size_t info_len,
                        uint8_t* okm, size_t okm_len) {
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md_info) return false;

    // 1. Extract: PRK = HMAC-SHA256(salt, IKM)
    uint8_t prk[32];
    if (mbedtls_md_hmac(md_info, salt, salt_len, ikm, ikm_len, prk) != 0) {
        return false;
    }

    // 2. Expand: T(i) = HMAC-SHA256(PRK, T(i-1) | info | i)
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, md_info, 1) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }

    uint8_t t[32];
    size_t generated = 0;
    uint8_t counter = 1;

    while (generated < okm_len) {
        mbedtls_md_hmac_starts(&ctx, prk, 32);
        if (generated > 0) {
            mbedtls_md_hmac_update(&ctx, t, 32);
        }
        mbedtls_md_hmac_update(&ctx, info, info_len);
        mbedtls_md_hmac_update(&ctx, &counter, 1);
        mbedtls_md_hmac_finish(&ctx, t);

        size_t to_copy = (okm_len - generated < 32) ? (okm_len - generated) : 32;
        memcpy(okm + generated, t, to_copy);
        generated += to_copy;
        counter++;
    }

    mbedtls_md_free(&ctx);
    return true;
}
