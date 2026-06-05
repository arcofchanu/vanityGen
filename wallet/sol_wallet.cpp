#include "sol_wallet.hpp"
#include "crypto/random.hpp"
#include "crypto/base58.hpp"
#include "crypto/hex.hpp"

#include <sodium.h>
#include <cstring>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Solana Wallet Generation
//
// Pipeline:
//   1. Generate 32 random bytes → ed25519 seed
//   2. crypto_sign_seed_keypair(pk, sk, seed)
//      → pk = 32-byte public key
//      → sk = 64-byte extended secret key (seed + pubkey concatenated)
//   3. Base58 encode the 32-byte public key → Solana address
//   4. Base58 encode the 64-byte secret key → private key representation
//
// Solana addresses ARE the public key in Base58. No hashing step.
// ─────────────────────────────────────────────────────────────────────────────

/// Ensure libsodium is initialized (thread-safe, idempotent).
static bool sodium_initialized = [] {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
    return true;
}();

Wallet generate_sol_wallet() {
    (void)sodium_initialized; // ensure static init runs

    Wallet w;
    w.chain = "SOL";

    // 1. Generate seed
    uint8_t seed[32];
    crypto::secure_random_bytes(seed, 32);

    // 2. Derive keypair
    uint8_t pk[crypto_sign_PUBLICKEYBYTES];  // 32 bytes
    uint8_t sk[crypto_sign_SECRETKEYBYTES];  // 64 bytes

    if (crypto_sign_seed_keypair(pk, sk, seed) != 0) {
        throw std::runtime_error("Failed to generate ed25519 keypair");
    }

    // 3. Address = Base58(public key)
    w.address = crypto::base58_encode(pk, crypto_sign_PUBLICKEYBYTES);

    // 4. Private key = Base58(secret key)
    w.private_key_hex = crypto::base58_encode(sk, crypto_sign_SECRETKEYBYTES);

    // Public key hex for reference
    w.public_key_hex = crypto::to_hex(pk, crypto_sign_PUBLICKEYBYTES);

    // Clear sensitive data
    sodium_memzero(seed, 32);
    sodium_memzero(sk, crypto_sign_SECRETKEYBYTES);

    return w;
}
