#pragma once

#include <string>

/// Common wallet data structure shared across all chains.
struct Wallet {
    std::string chain;           // "ETH", "BSC", "BTC", "SOL"
    std::string private_key_hex; // Hex-encoded private key (or WIF for BTC, Base58 for SOL)
    std::string public_key_hex;  // Hex-encoded public key (or Base58 for SOL)
    std::string address;         // Chain-specific address string
};
