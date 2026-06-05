#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include "cli/args.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Vanity Address Pattern Matcher
//
// Chain-aware matching logic:
//   ETH/BSC: Strip "0x" prefix, case-insensitive by default (hex addresses)
//   BTC:     Skip leading "1", case-sensitive (Base58 is case-sensitive)
//   SOL:     Full Base58 address, case-sensitive
// ─────────────────────────────────────────────────────────────────────────────

namespace vanity {

inline std::string str_to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/// Extract the matchable portion of an address based on chain.
/// ETH/BSC: strip "0x" prefix
/// BTC: skip leading "1" (P2PKH version character)
/// SOL: full address
inline std::string get_matchable_address(const std::string& address, Chain chain) {
    switch (chain) {
        case Chain::ETH:
        case Chain::BSC:
            // Strip "0x" prefix
            if (address.size() > 2 && address[0] == '0' && address[1] == 'x') {
                return address.substr(2);
            }
            return address;

        case Chain::BTC:
            // Skip leading "1" for P2PKH addresses
            if (!address.empty() && address[0] == '1') {
                return address.substr(1);
            }
            return address;

        case Chain::SOL:
            return address;
    }
    return address;
}

/// Check if an address matches the given vanity pattern(s).
///
/// @param address       Full address string from wallet generation
/// @param pattern       Prefix pattern (for PREFIX and BOTH modes)
/// @param suffix        Suffix pattern (for SUFFIX and BOTH modes)
/// @param mode          PREFIX, SUFFIX, or BOTH
/// @param chain         Target chain (affects address preprocessing)
/// @param case_sensitive Whether matching is case-sensitive
/// @return true if the address matches the pattern criteria
inline bool matches(
    const std::string& address,
    const std::string& pattern,
    const std::string& suffix,
    VanityMode mode,
    Chain chain,
    bool case_sensitive
) {
    std::string addr = get_matchable_address(address, chain);

    std::string pat = pattern;
    std::string suf = suffix;

    // ETH/BSC: default case-insensitive (hex addresses)
    if (!case_sensitive && (chain == Chain::ETH || chain == Chain::BSC)) {
        addr = str_to_lower(addr);
        pat  = str_to_lower(pat);
        suf  = str_to_lower(suf);
    }

    // BTC and SOL: always case-sensitive by nature (Base58)

    switch (mode) {
        case VanityMode::PREFIX:
            if (pat.empty()) return false;
            if (addr.size() < pat.size()) return false;
            return addr.substr(0, pat.size()) == pat;

        case VanityMode::SUFFIX:
            if (suf.empty()) return false;
            if (addr.size() < suf.size()) return false;
            return addr.substr(addr.size() - suf.size()) == suf;

        case VanityMode::BOTH: {
            if (pat.empty() && suf.empty()) return false;
            bool prefix_ok = pat.empty() || 
                (addr.size() >= pat.size() && addr.substr(0, pat.size()) == pat);
            bool suffix_ok = suf.empty() || 
                (addr.size() >= suf.size() && addr.substr(addr.size() - suf.size()) == suf);
            return prefix_ok && suffix_ok;
        }
    }

    return false;
}

} // namespace vanity
