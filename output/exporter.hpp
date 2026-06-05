#pragma once

#include <string>
#include <vector>
#include "wallet/wallet.hpp"
#include "cli/args.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Output Exporter
//
// Writes wallet data to CSV and/or JSON files.
// Atomic write: buffer → write to .tmp → rename to final filename.
// ─────────────────────────────────────────────────────────────────────────────

namespace output {

/// Write wallets to CSV file.
/// Format: chain,private_key,public_key,address
void write_csv(const std::string& filename, const std::vector<Wallet>& wallets);

/// Write wallets to JSON file.
/// Format: top-level array of wallet objects.
void write_json(const std::string& filename, const std::vector<Wallet>& wallets);

/// Write wallets in trace format (JSON + addresses file).
/// Creates: filename.json (same as normal JSON) and filename.addresses.json (addresses array)
void write_trace(const std::string& filename, const std::vector<Wallet>& wallets);

/// Write wallets to the configured output format(s).
void export_wallets(const Config& cfg, const std::vector<Wallet>& wallets);

} // namespace output
