#pragma once

#include <cstdint>
#include <cstddef>
#include <openssl/evp.h>
#include <stdexcept>

namespace crypto {

/// Compute SHA-256 hash using OpenSSL EVP API.
inline void sha256(const uint8_t* input, size_t len, uint8_t output[32]) {
    unsigned int out_len = 32;
    if (!EVP_Digest(input, len, output, &out_len, EVP_sha256(), nullptr)) {
        throw std::runtime_error("SHA-256 failed");
    }
}

/// Compute double SHA-256: SHA256(SHA256(input)).
/// Used in Bitcoin address and WIF checksum computation.
inline void double_sha256(const uint8_t* input, size_t len, uint8_t output[32]) {
    uint8_t intermediate[32];
    sha256(input, len, intermediate);
    sha256(intermediate, 32, output);
}

} // namespace crypto
