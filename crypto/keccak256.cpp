#include "keccak256.hpp"
#include <cstring>

namespace crypto {

// ─────────────────────────────────────────────────────────────────────────────
// Standalone Keccak-256 implementation (original Keccak, NOT NIST SHA3-256).
//
// Ethereum uses Keccak-256 with domain-separation byte 0x01 (original Keccak),
// not 0x06 (NIST SHA3). OpenSSL's EVP_sha3_256() uses NIST SHA3 — DO NOT use it.
//
// Based on the compact reference implementation from the Keccak team.
// Rate = 1088 bits (136 bytes), capacity = 512 bits, output = 256 bits.
// ─────────────────────────────────────────────────────────────────────────────

static constexpr size_t KECCAK_RATE = 136;  // r = 1088 bits = 136 bytes
static constexpr size_t KECCAK_STATE_SIZE = 25; // 5x5 uint64_t = 200 bytes

// Round constants for Keccak-f[1600]
static constexpr uint64_t RC[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

// Rotation offsets
static constexpr int ROTATIONS[25] = {
     0,  1, 62, 28, 27,
    36, 44,  6, 55, 20,
     3, 10, 43, 25, 39,
    41, 45, 15, 21,  8,
    18,  2, 61, 56, 14
};

// Pi permutation indices
static constexpr int PI[25] = {
     0, 10, 20,  5, 15,
    16,  1, 11, 21,  6,
     7, 17,  2, 12, 22,
    23,  8, 18,  3, 13,
    14, 24,  9, 19,  4
};

static inline uint64_t rotl64(uint64_t x, int n) {
    return (x << n) | (x >> (64 - n));
}

/// Keccak-f[1600] permutation: 24 rounds
static void keccak_f1600(uint64_t state[25]) {
    for (int round = 0; round < 24; ++round) {
        // θ (theta)
        uint64_t C[5], D[5];
        for (int x = 0; x < 5; ++x) {
            C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
        }
        for (int x = 0; x < 5; ++x) {
            D[x] = C[(x + 4) % 5] ^ rotl64(C[(x + 1) % 5], 1);
        }
        for (int x = 0; x < 5; ++x) {
            for (int y = 0; y < 5; ++y) {
                state[x + 5 * y] ^= D[x];
            }
        }

        // ρ (rho) and π (pi) combined
        uint64_t temp[25];
        for (int i = 0; i < 25; ++i) {
            temp[PI[i]] = rotl64(state[i], ROTATIONS[i]);
        }

        // χ (chi)
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                state[x + 5 * y] = temp[x + 5 * y] ^
                    ((~temp[((x + 1) % 5) + 5 * y]) & temp[((x + 2) % 5) + 5 * y]);
            }
        }

        // ι (iota)
        state[0] ^= RC[round];
    }
}

void keccak256(const uint8_t* input, size_t len, uint8_t output[32]) {
    uint64_t state[KECCAK_STATE_SIZE];
    std::memset(state, 0, sizeof(state));

    auto* state_bytes = reinterpret_cast<uint8_t*>(state);

    // Absorb phase: XOR input blocks into state, then permute
    size_t offset = 0;
    while (offset + KECCAK_RATE <= len) {
        for (size_t i = 0; i < KECCAK_RATE; ++i) {
            state_bytes[i] ^= input[offset + i];
        }
        keccak_f1600(state);
        offset += KECCAK_RATE;
    }

    // Absorb remaining bytes
    size_t remaining = len - offset;
    for (size_t i = 0; i < remaining; ++i) {
        state_bytes[i] ^= input[offset + i];
    }

    // Padding: Keccak uses 0x01, NOT SHA3's 0x06
    // pad10*1: first bit after message = 1, last bit of rate block = 1
    state_bytes[remaining] ^= 0x01;
    state_bytes[KECCAK_RATE - 1] ^= 0x80;

    // Final permutation
    keccak_f1600(state);

    // Squeeze phase: extract 32 bytes (256 bits) of output
    std::memcpy(output, state_bytes, 32);
}

} // namespace crypto
