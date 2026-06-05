#include "btc_wallet.hpp"
#include "crypto/random.hpp"
#include "crypto/sha256.hpp"
#include "crypto/ripemd160.hpp"
#include "crypto/base58.hpp"
#include "crypto/hex.hpp"

#include <secp256k1.h>
#include <cstring>
#include <stdexcept>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Bitcoin Wallet Generation
//
// Address pipeline (P2PKH):
//   1. Generate 32 random bytes → private key
//   2. secp256k1_ec_pubkey_create() → compressed pubkey (33 bytes)
//   3. SHA-256(compressed pubkey) → 32 bytes
//   4. RIPEMD-160(SHA-256 result) → 20 bytes (Hash160)
//   5. Prepend 0x00 version byte → 21 bytes
//   6. Double SHA-256(21 bytes) → take first 4 bytes as checksum
//   7. Append checksum → 25 bytes
//   8. Base58 encode → address (starts with "1")
//
// WIF private key (compressed):
//   1. Prepend 0x80 to private key → 33 bytes
//   2. Append 0x01 compression flag → 34 bytes
//   3. Double SHA-256(34 bytes) → take first 4 bytes checksum
//   4. Append checksum → 38 bytes
//   5. Base58 encode → WIF string (starts with "K" or "L")
// ─────────────────────────────────────────────────────────────────────────────

/// Thread-local secp256k1 context
static thread_local secp256k1_context* ctx = [] {
    auto* c = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    if (!c) throw std::runtime_error("Failed to create secp256k1 context");
    return c;
}();

Wallet generate_btc_wallet() {
    Wallet w;
    w.chain = "BTC";

    // 1. Generate private key
    uint8_t privkey[32];
    crypto::secure_random_bytes(privkey, 32);

    while (!secp256k1_ec_seckey_verify(ctx, privkey)) {
        crypto::secure_random_bytes(privkey, 32);
    }

    // ── WIF encoding ────────────────────────────────────────────────────
    // [0x80] + [32-byte privkey] + [0x01 compression flag]
    uint8_t wif_payload[34];
    wif_payload[0] = 0x80;
    std::memcpy(wif_payload + 1, privkey, 32);
    wif_payload[33] = 0x01; // compressed flag

    // Checksum: first 4 bytes of double SHA-256
    uint8_t wif_checksum[32];
    crypto::double_sha256(wif_payload, 34, wif_checksum);

    // Final WIF: [34 bytes] + [4-byte checksum]
    uint8_t wif_data[38];
    std::memcpy(wif_data, wif_payload, 34);
    std::memcpy(wif_data + 34, wif_checksum, 4);

    w.private_key_hex = crypto::base58_encode(wif_data, 38);

    // 2. Create compressed public key (33 bytes)
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privkey)) {
        throw std::runtime_error("Failed to create BTC public key");
    }

    uint8_t pubkey_compressed[33];
    size_t pubkey_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, pubkey_compressed, &pubkey_len,
                                  &pubkey, SECP256K1_EC_COMPRESSED);

    w.public_key_hex = crypto::to_hex(pubkey_compressed, 33);

    // 3-4. Hash160: RIPEMD160(SHA256(pubkey))
    uint8_t hash160[20];
    crypto::hash160(pubkey_compressed, 33, hash160);

    // 5-8. Base58Check encode with version 0x00
    w.address = crypto::base58check_encode(0x00, hash160, 20);

    // Clear sensitive data
    std::memset(privkey, 0, 32);
    std::memset(wif_payload, 0, 34);
    std::memset(wif_data, 0, 38);

    return w;
}
