#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>

// Platform-specific CSPRNG headers
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <bcrypt.h>
    #ifndef NT_SUCCESS
        #define NT_SUCCESS(Status) (((long)(Status)) >= 0)
    #endif
#else
    #include <fstream>
    #include <cerrno>
    #if __has_include(<sys/random.h>)
        #include <sys/random.h>
        #define HAS_GETRANDOM 1
    #endif
#endif

// Fallback: OpenSSL
#include <openssl/rand.h>

namespace crypto {

/// Generate cryptographically secure random bytes.
/// Uses OS-level entropy: BCryptGenRandom (Windows), getrandom (Linux), /dev/urandom (fallback).
/// Thread-safe — no shared state.
inline void secure_random_bytes(uint8_t* buf, size_t len) {
#ifdef _WIN32
    NTSTATUS status = BCryptGenRandom(
        NULL,
        buf,
        static_cast<ULONG>(len),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    if (NT_SUCCESS(status)) return;
    // Fallback to OpenSSL
    if (RAND_bytes(buf, static_cast<int>(len)) == 1) return;
    throw std::runtime_error("Failed to generate secure random bytes");
#else
    #ifdef HAS_GETRANDOM
    ssize_t result = getrandom(buf, len, 0);
    if (result == static_cast<ssize_t>(len)) return;
    #endif
    // Fallback: /dev/urandom
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom.good()) {
        urandom.read(reinterpret_cast<char*>(buf), len);
        if (urandom.good()) return;
    }
    // Final fallback: OpenSSL
    if (RAND_bytes(buf, static_cast<int>(len)) == 1) return;
    throw std::runtime_error("Failed to generate secure random bytes");
#endif
}

} // namespace crypto
