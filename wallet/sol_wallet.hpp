#pragma once

#include "wallet.hpp"

/// Generate a single Solana wallet.
/// Pipeline: random 32B seed → ed25519 keypair → Base58 pubkey (address) + Base58 secret key
Wallet generate_sol_wallet();
