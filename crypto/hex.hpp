#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace crypto {

/// Convert a byte array to a lowercase hex string.
inline std::string to_hex(const uint8_t* data, size_t len) {
    static constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(hex_chars[(data[i] >> 4) & 0x0F]);
        result.push_back(hex_chars[data[i] & 0x0F]);
    }
    return result;
}

/// Convert a byte array to a hex string with "0x" prefix.
inline std::string to_hex_prefixed(const uint8_t* data, size_t len) {
    return "0x" + to_hex(data, len);
}

} // namespace crypto
