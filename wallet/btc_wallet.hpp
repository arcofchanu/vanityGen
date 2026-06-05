#pragma once

#include "wallet.hpp"

/// Generate a single Bitcoin wallet.
/// Pipeline: random 32B → secp256k1 compressed pubkey → SHA256 → RIPEMD160 → Base58Check
/// Address format: P2PKH legacy (starts with "1")
/// Private key format: WIF compressed (starts with "K" or "L")
Wallet generate_btc_wallet();
