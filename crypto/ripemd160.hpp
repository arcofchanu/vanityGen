#pragma once

#include <cstdint>
#include <cstddef>
#include <openssl/evp.h>
#include <stdexcept>

namespace crypto {

/// Compute RIPEMD-160 hash using OpenSSL EVP API.
/// Used in Bitcoin address derivation: RIPEMD160(SHA256(pubkey)).
inline void ripemd160(const uint8_t* input, size_t len, uint8_t output[20]) {
    unsigned int out_len = 20;
    if (!EVP_Digest(input, len, output, &out_len, EVP_ripemd160(), nullptr)) {
        throw std::runtime_error("RIPEMD-160 failed");
    }
}

/// Compute Hash160: RIPEMD160(SHA256(input)).
/// Standard Bitcoin public key hash.
inline void hash160(const uint8_t* input, size_t len, uint8_t output[20]) {
    uint8_t sha_hash[32];
    unsigned int sha_len = 32;
    if (!EVP_Digest(input, len, sha_hash, &sha_len, EVP_sha256(), nullptr)) {
        throw std::runtime_error("SHA-256 failed in hash160");
    }
    ripemd160(sha_hash, 32, output);
}

} // namespace crypto
