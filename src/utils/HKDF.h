#ifndef HKDF_H
#define HKDF_H

#include <cstdint>
#include <cstddef>

class HKDF {
public:
    /**
     * @brief HMAC-based Extract-and-Expand Key Derivation Function (HKDF) using SHA-256.
     * 
     * @param salt Input salt.
     * @param salt_len Length of the salt.
     * @param ikm Input keying material.
     * @param ikm_len Length of the IKM.
     * @param info Optional context and application specific information.
     * @param info_len Length of the info.
     * @param okm Output keying material.
     * @param okm_len Length of the OKM.
     * @return true if successful.
     */
    static bool deriveSHA256(const uint8_t* salt, size_t salt_len,
                             const uint8_t* ikm, size_t ikm_len,
                             const uint8_t* info, size_t info_len,
                             uint8_t* okm, size_t okm_len);
};

#endif // HKDF_H
