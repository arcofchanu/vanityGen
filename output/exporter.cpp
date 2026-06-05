#include "exporter.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
// Atomic file writing: buffer all → write to .tmp → rename to final path.
// This ensures the output file is never in a partial/corrupt state.
// ─────────────────────────────────────────────────────────────────────────────

namespace output {

/// Escape a string for JSON output (handle quotes and backslashes).
static std::string json_escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else result += c;
    }
    return result;
}

/// Write content to a temp file, then rename atomically.
static void atomic_write(const std::string& filename, const std::string& content) {
    std::string tmp_file = filename + ".tmp";

    {
        std::ofstream out(tmp_file, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "[!] Error: Could not open " << tmp_file << " for writing\n";
            return;
        }
        out << content;
        out.flush();
        if (!out.good()) {
            std::cerr << "[!] Error: Failed to write to " << tmp_file << "\n";
            return;
        }
    }

    // Remove existing file if present, then rename
    std::remove(filename.c_str());
    if (std::rename(tmp_file.c_str(), filename.c_str()) != 0) {
        std::cerr << "[!] Error: Failed to rename " << tmp_file << " to " << filename << "\n";
        // Try direct write as fallback
        std::ofstream out(filename, std::ios::binary);
        if (out.is_open()) {
            out << content;
        }
    }
}

void write_csv(const std::string& filename, const std::vector<Wallet>& wallets) {
    std::ostringstream ss;

    // Header
    ss << "chain,private_key,public_key,address\n";

    // Data rows
    for (const auto& w : wallets) {
        ss << w.chain << ","
           << w.private_key_hex << ","
           << w.public_key_hex << ","
           << w.address << "\n";
    }

    atomic_write(filename, ss.str());
    std::cout << "[+] Written " << wallets.size() << " wallets to " << filename << "\n";
}

void write_json(const std::string& filename, const std::vector<Wallet>& wallets) {
    std::ostringstream ss;

    ss << "[\n";
    for (size_t i = 0; i < wallets.size(); ++i) {
        const auto& w = wallets[i];
        ss << "  {\n"
           << "    \"chain\": \"" << json_escape(w.chain) << "\",\n"
           << "    \"private_key\": \"" << json_escape(w.private_key_hex) << "\",\n"
           << "    \"public_key\": \"" << json_escape(w.public_key_hex) << "\",\n"
           << "    \"address\": \"" << json_escape(w.address) << "\"\n"
           << "  }";
        if (i + 1 < wallets.size()) ss << ",";
        ss << "\n";
    }
    ss << "]\n";

    atomic_write(filename, ss.str());
    std::cout << "[+] Written " << wallets.size() << " wallets to " << filename << "\n";
}

void write_trace(const std::string& filename, const std::vector<Wallet>& wallets) {
    // Write JSON file (same as normal JSON)
    write_json(filename + ".json", wallets);

    // Write addresses file
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < wallets.size(); ++i) {
        ss << "    \"" << json_escape(wallets[i].address) << "\"";
        if (i + 1 < wallets.size()) ss << ",";
        ss << "\n";
    }
    ss << "]\n";

    atomic_write(filename + ".addresses.json", ss.str());
    std::cout << "[+] Written " << wallets.size() << " addresses to " << filename << ".addresses.json\n";
}

void export_wallets(const Config& cfg, const std::vector<Wallet>& wallets) {
    if (wallets.empty()) {
        std::cout << "[!] No wallets to export\n";
        return;
    }

    switch (cfg.output_mode) {
        case OutputMode::CSV:
            write_csv(cfg.output_file + ".csv", wallets);
            break;
        case OutputMode::JSON:
            write_json(cfg.output_file + ".json", wallets);
            break;
        case OutputMode::BOTH:
            write_csv(cfg.output_file + ".csv", wallets);
            write_json(cfg.output_file + ".json", wallets);
            break;
        case OutputMode::TRACE:
            write_trace(cfg.output_file, wallets);
            break;
    }
}

} // namespace output
