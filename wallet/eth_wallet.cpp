#include "eth_wallet.hpp"
#include "crypto/random.hpp"
#include "crypto/keccak256.hpp"
#include "crypto/hex.hpp"

#include <secp256k1.h>
#include <cstring>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// ETH / BSC Wallet Generation
//
// Pipeline:
//   1. Generate 32 random bytes → private key
//   2. secp256k1_ec_pubkey_create() → secp256k1_pubkey
//   3. Serialize as uncompressed (65 bytes, starts with 0x04)
//   4. Strip the 0x04 prefix → 64-byte payload
//   5. Keccak-256(64 bytes) → 32-byte hash
//   6. Take last 20 bytes → raw address
//   7. Hex-encode with "0x" prefix → final address string
//
// BSC uses identical derivation (EVM-compatible).
// ─────────────────────────────────────────────────────────────────────────────

/// Thread-local secp256k1 context — created once per thread, reused for performance.
static thread_local secp256k1_context* ctx = [] {
    auto* c = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    if (!c) throw std::runtime_error("Failed to create secp256k1 context");
    return c;
}();

static Wallet generate_evm_wallet(const char* chain_name) {
    Wallet w;
    w.chain = chain_name;

    // 1. Generate private key
    uint8_t privkey[32];
    crypto::secure_random_bytes(privkey, 32);

    // Verify the private key is valid for secp256k1
    while (!secp256k1_ec_seckey_verify(ctx, privkey)) {
        crypto::secure_random_bytes(privkey, 32);
    }

    w.private_key_hex = crypto::to_hex_prefixed(privkey, 32);

    // 2. Create public key
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privkey)) {
        throw std::runtime_error("Failed to create public key");
    }

    // 3. Serialize as uncompressed (65 bytes)
    uint8_t pubkey_serialized[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(ctx, pubkey_serialized, &pubkey_len,
                                  &pubkey, SECP256K1_EC_UNCOMPRESSED);

    w.public_key_hex = crypto::to_hex_prefixed(pubkey_serialized, 65);

    // 4. Strip 0x04 prefix → 64-byte payload
    // 5. Keccak-256 hash
    uint8_t hash[32];
    crypto::keccak256(pubkey_serialized + 1, 64, hash);

    // 6. Last 20 bytes = address
    w.address = crypto::to_hex_prefixed(hash + 12, 20);

    // Clear sensitive data
    std::memset(privkey, 0, 32);

    return w;
}

Wallet generate_eth_wallet() {
    return generate_evm_wallet("ETH");
}

Wallet generate_bsc_wallet() {
    return generate_evm_wallet("BSC");
}
