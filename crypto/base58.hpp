#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>
#include "sha256.hpp"

namespace crypto {

/// Standard Bitcoin Base58 alphabet.
/// Excludes 0, O, I, l to avoid visual ambiguity.
static constexpr const char* BASE58_ALPHABET =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/// Encode a byte array to Base58 string.
/// Algorithm: repeatedly divide the big integer by 58, collect remainders.
/// Leading zero bytes become '1' characters.
inline std::string base58_encode(const uint8_t* data, size_t len) {
    // Count leading zeros
    size_t leading_zeros = 0;
    while (leading_zeros < len && data[leading_zeros] == 0) {
        ++leading_zeros;
    }

    // Allocate enough space (log(256)/log(58) ≈ 1.366, use 1.38 for safety)
    size_t max_size = static_cast<size_t>((len - leading_zeros) * 138 / 100) + 1;
    std::vector<uint8_t> digits(max_size, 0);

    size_t digits_len = 0;

    for (size_t i = leading_zeros; i < len; ++i) {
        uint32_t carry = data[i];
        for (size_t j = 0; j < digits_len; ++j) {
            carry += static_cast<uint32_t>(digits[j]) << 8;
            digits[j] = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
        while (carry > 0) {
            digits[digits_len++] = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
    }

    // Build result string
    std::string result;
    result.reserve(leading_zeros + digits_len);

    // Leading '1's for each leading zero byte
    result.append(leading_zeros, '1');

    // Append digits in reverse order
    for (size_t i = digits_len; i > 0; --i) {
        result.push_back(BASE58_ALPHABET[digits[i - 1]]);
    }

    return result;
}

/// Encode with Base58Check: prepend version byte, append 4-byte double-SHA256 checksum.
/// Used for Bitcoin addresses (version 0x00) and WIF private keys (version 0x80).
inline std::string base58check_encode(uint8_t version, const uint8_t* payload, size_t payload_len) {
    // Build: [version] + [payload] + [checksum]
    std::vector<uint8_t> data(1 + payload_len + 4);
    data[0] = version;
    std::copy(payload, payload + payload_len, data.begin() + 1);

    // Double SHA-256 checksum
    uint8_t hash[32];
    double_sha256(data.data(), 1 + payload_len, hash);

    // Append first 4 bytes of checksum
    std::copy(hash, hash + 4, data.begin() + 1 + payload_len);

    return base58_encode(data.data(), data.size());
}

} // namespace crypto
