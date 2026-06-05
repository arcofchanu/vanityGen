#pragma once

#include <cstdint>
#include <cstddef>

namespace crypto {

/// Compute Keccak-256 hash (original Keccak, NOT NIST SHA3-256).
/// Ethereum uses Keccak-256 with 0x01 padding, not SHA3's 0x06.
void keccak256(const uint8_t* input, size_t len, uint8_t output[32]);

} // namespace crypto
