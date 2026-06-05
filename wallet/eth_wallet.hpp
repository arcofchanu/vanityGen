#pragma once

#include "wallet.hpp"

/// Generate a single Ethereum wallet.
/// Pipeline: random 32B → secp256k1 uncompressed pubkey → Keccak-256 → last 20 bytes → 0x hex
Wallet generate_eth_wallet();

/// Generate a single BSC wallet.
/// Identical derivation to ETH (EVM-compatible), only the chain label differs.
Wallet generate_bsc_wallet();
