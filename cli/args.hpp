#pragma once

#include <string>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <thread>
#include <algorithm>

// -----------------------------------------------------------------------------
// CLI Argument Parser
//
// Parses command-line arguments into a Config struct.
// No external library - manual parsing for zero dependencies.
// -----------------------------------------------------------------------------

enum class Chain { ETH, BSC, BTC, SOL };
enum class VanityMode { PREFIX, SUFFIX, BOTH };
enum class OutputMode { CSV, JSON, BOTH, TRACE, ADDRESSES_ONLY };
enum class RunMode { BULK, VANITY };

struct Config {
    RunMode     run_mode       = RunMode::BULK;
    Chain       chain          = Chain::ETH;
    VanityMode  vanity_mode    = VanityMode::PREFIX;
    std::string pattern        = "";          // Vanity prefix pattern
    std::string suffix_pattern = "";          // Vanity suffix pattern (for BOTH mode)
    uint64_t    bulk_count     = 1000;        // Wallets to generate in bulk mode
    uint64_t    vanity_count   = 1;           // How many vanity matches to find
    int         thread_count   = 0;           // 0 = auto-detect
    OutputMode  output_mode    = OutputMode::CSV;
    std::string output_file    = "wallets";   // Output filename without extension
    bool        case_sensitive = false;       // ETH/BSC default case-insensitive
};

inline const char* chain_to_string(Chain c) {
    switch (c) {
        case Chain::ETH: return "ETH";
        case Chain::BSC: return "BSC";
        case Chain::BTC: return "BTC";
        case Chain::SOL: return "SOL";
    }
    return "UNKNOWN";
}

inline const char* run_mode_to_string(RunMode m) {
    return m == RunMode::BULK ? "BULK" : "VANITY";
}

inline const char* vanity_mode_to_string(VanityMode m) {
    switch (m) {
        case VanityMode::PREFIX: return "PREFIX";
        case VanityMode::SUFFIX: return "SUFFIX";
        case VanityMode::BOTH:   return "BOTH";
    }
    return "UNKNOWN";
}

inline const char* output_mode_to_string(OutputMode m) {
    switch (m) {
        case OutputMode::CSV:   return "CSV";
        case OutputMode::JSON:  return "JSON";
        case OutputMode::BOTH:  return "CSV+JSON";
        case OutputMode::TRACE: return "TRACE";
        case OutputMode::ADDRESSES_ONLY: return "ADDRESSES_ONLY";
    }
    return "UNKNOWN";
}

inline void print_help() {
    std::cout << R"(
wallet-gen - Multi-chain bulk wallet generator & vanity address finder

USAGE:
  wallet-gen [OPTIONS]

OPTIONS:
  --chain     <eth|bsc|btc|sol>      Target chain (default: eth)
  --mode      <bulk|vanity>          Run mode (default: bulk)
  --count     <N>                    Wallets to generate in bulk mode (default: 1000)
  --pattern   <string>               Vanity pattern to search for
  --suffix    <string>               Suffix pattern (for --vanity both)
  --vanity    <prefix|suffix|both>   Vanity match type (default: prefix)
  --vcount    <N>                    Number of vanity matches to find (default: 1)
  --threads   <N>                    Thread count (default: auto-detect)
  --output    <filename>             Output file base name (default: wallets)
  --format    <csv|json|both|trace|x>  Output format (default: csv, x=addresses only)
  --case                             Enable case-sensitive pattern matching
  --help                             Show this help

EXAMPLES:
  wallet-gen --chain eth --mode bulk --count 10000
  wallet-gen --chain eth --mode vanity --pattern dead --vanity prefix
  wallet-gen --chain btc --mode vanity --pattern Chanu --vanity prefix --case
  wallet-gen --chain sol --mode vanity --pattern Sol --suffix ana --vanity both
  wallet-gen --chain eth --mode vanity --pattern cafe --vcount 5
)";
}

inline std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/// Validate that a pattern contains only characters valid for the given chain.
inline bool is_valid_pattern(const std::string& pattern, Chain chain) {
    if (pattern.empty()) return true;  // Empty pattern handled elsewhere
    
    switch (chain) {
        case Chain::ETH:
        case Chain::BSC:
            // Hex characters: 0-9, a-f, A-F
            for (char c : pattern) {
                if (!std::isxdigit(c)) {
                    return false;
                }
            }
            return true;
        
        case Chain::BTC:
        case Chain::SOL:
            // Base58 characters: 123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz
            static constexpr const char* BASE58_ALPHABET =
                "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
            for (char c : pattern) {
                bool found = false;
                for (const char* p = BASE58_ALPHABET; *p; ++p) {
                    if (c == *p) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
    }
    return true;
}

inline Config parse_args(int argc, char** argv) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_help();
            std::exit(0);
        }
        else if (arg == "--chain" && i + 1 < argc) {
            std::string val = to_lower(argv[++i]);
            if      (val == "eth") cfg.chain = Chain::ETH;
            else if (val == "bsc") cfg.chain = Chain::BSC;
            else if (val == "btc") cfg.chain = Chain::BTC;
            else if (val == "sol") cfg.chain = Chain::SOL;
            else {
                std::cerr << "[!] Unknown chain: " << val << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--mode" && i + 1 < argc) {
            std::string val = to_lower(argv[++i]);
            if      (val == "bulk")   cfg.run_mode = RunMode::BULK;
            else if (val == "vanity") cfg.run_mode = RunMode::VANITY;
            else {
                std::cerr << "[!] Unknown mode: " << val << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--count" && i + 1 < argc) {
            cfg.bulk_count = std::stoull(argv[++i]);
        }
        else if (arg == "--pattern" && i + 1 < argc) {
            cfg.pattern = argv[++i];
        }
        else if (arg == "--suffix" && i + 1 < argc) {
            cfg.suffix_pattern = argv[++i];
        }
        else if (arg == "--vanity" && i + 1 < argc) {
            std::string val = to_lower(argv[++i]);
            if      (val == "prefix") cfg.vanity_mode = VanityMode::PREFIX;
            else if (val == "suffix") cfg.vanity_mode = VanityMode::SUFFIX;
            else if (val == "both")   cfg.vanity_mode = VanityMode::BOTH;
            else {
                std::cerr << "[!] Unknown vanity mode: " << val << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--vcount" && i + 1 < argc) {
            cfg.vanity_count = std::stoull(argv[++i]);
            if (cfg.vanity_count == 0) cfg.vanity_count = 1;
        }
        else if (arg == "--threads" && i + 1 < argc) {
            cfg.thread_count = std::stoi(argv[++i]);
        }
        else if (arg == "--output" && i + 1 < argc) {
            cfg.output_file = argv[++i];
        }
        else if (arg == "--format" && i + 1 < argc) {
            std::string val = to_lower(argv[++i]);
            if      (val == "csv")   cfg.output_mode = OutputMode::CSV;
            else if (val == "json")  cfg.output_mode = OutputMode::JSON;
            else if (val == "both")  cfg.output_mode = OutputMode::BOTH;
            else if (val == "trace") cfg.output_mode = OutputMode::TRACE;
            else if (val == "x")     cfg.output_mode = OutputMode::ADDRESSES_ONLY;
            else {
                std::cerr << "[!] Unknown format: " << val << "\n";
                std::exit(1);
            }
        }
        else if (arg == "--case") {
            cfg.case_sensitive = true;
        }
        else {
            std::cerr << "[!] Unknown argument: " << arg << "\n";
            print_help();
            std::exit(1);
        }
    }

    // Auto-detect thread count
    if (cfg.thread_count <= 0) {
        cfg.thread_count = static_cast<int>(std::thread::hardware_concurrency());
        if (cfg.thread_count <= 0) cfg.thread_count = 4; // safe fallback
    }

    // Pattern validation (only in vanity mode)
    if (cfg.run_mode == RunMode::VANITY) {
        if (!cfg.pattern.empty() && !is_valid_pattern(cfg.pattern, cfg.chain)) {
            std::cerr << "[!] Error: Pattern '" << cfg.pattern << "' contains invalid characters for " 
                      << chain_to_string(cfg.chain) << "\n";
            std::exit(1);
        }
        if (!cfg.suffix_pattern.empty() && !is_valid_pattern(cfg.suffix_pattern, cfg.chain)) {
            std::cerr << "[!] Error: Suffix pattern '" << cfg.suffix_pattern << "' contains invalid characters for " 
                      << chain_to_string(cfg.chain) << "\n";
            std::exit(1);
        }
    }

    // Validation
    if (cfg.run_mode == RunMode::VANITY && cfg.pattern.empty()) {
        // For suffix-only mode, pattern can be empty if suffix is set
        if (cfg.vanity_mode == VanityMode::SUFFIX && !cfg.suffix_pattern.empty()) {
            // OK: suffix-only
        } else if (cfg.vanity_mode == VanityMode::BOTH && !cfg.suffix_pattern.empty()) {
            // Need at least one pattern for BOTH mode
            // OK if suffix is provided but prefix is empty - treat as suffix-only
        } else {
            std::cerr << "[!] Error: --pattern is required in vanity mode\n";
            std::exit(1);
        }
    }

    // For BOTH mode with suffix flag, use suffix_pattern for suffix matching
    // For BOTH mode without suffix flag, use pattern for both prefix and suffix
    if (cfg.vanity_mode == VanityMode::BOTH && cfg.suffix_pattern.empty()) {
        cfg.suffix_pattern = cfg.pattern;
    }

    // For SUFFIX mode, if no suffix_pattern, use pattern as suffix
    if (cfg.vanity_mode == VanityMode::SUFFIX && cfg.suffix_pattern.empty()) {
        cfg.suffix_pattern = cfg.pattern;
    }

    return cfg;
}

inline void print_config(const Config& cfg) {
    std::cout << "\n";
    std::cout << "+------------------------------------------+\n";
    std::cout << "|         wallet-gen v1.0                  |\n";
    std::cout << "+------------------------------------------+\n";
    std::cout << "|  Chain:      " << chain_to_string(cfg.chain) << std::string(29 - std::strlen(chain_to_string(cfg.chain)), ' ') << "|\n";
    std::cout << "|  Mode:       " << run_mode_to_string(cfg.run_mode) << std::string(29 - std::strlen(run_mode_to_string(cfg.run_mode)), ' ') << "|\n";

    if (cfg.run_mode == RunMode::BULK) {
        std::string count_str = std::to_string(cfg.bulk_count);
        std::cout << "|  Count:      " << count_str << std::string(29 - count_str.size(), ' ') << "|\n";
    } else {
        std::string pat_str = cfg.pattern;
        if (pat_str.size() > 20) pat_str = pat_str.substr(0, 20) + "...";
        std::cout << "|  Pattern:    " << pat_str << std::string(29 - pat_str.size(), ' ') << "|\n";
        if (cfg.vanity_mode == VanityMode::BOTH || cfg.vanity_mode == VanityMode::SUFFIX) {
            std::string suf_str = cfg.suffix_pattern;
            if (suf_str.size() > 20) suf_str = suf_str.substr(0, 20) + "...";
            std::cout << "|  Suffix:     " << suf_str << std::string(29 - suf_str.size(), ' ') << "|\n";
        }
        std::cout << "|  Match:      " << vanity_mode_to_string(cfg.vanity_mode) << std::string(29 - std::strlen(vanity_mode_to_string(cfg.vanity_mode)), ' ') << "|\n";
        std::string vc_str = std::to_string(cfg.vanity_count);
        std::cout << "|  Find:       " << vc_str << " match" << (cfg.vanity_count > 1 ? "es" : "") << std::string(29 - vc_str.size() - (cfg.vanity_count > 1 ? 8 : 6), ' ') << "|\n";
        std::cout << "|  Case-sens:  " << (cfg.case_sensitive ? "YES" : "NO") << std::string(29 - (cfg.case_sensitive ? 3 : 2), ' ') << "|\n";
    }

    std::string threads_str = std::to_string(cfg.thread_count);
    std::cout << "|  Threads:    " << threads_str << std::string(29 - threads_str.size(), ' ') << "|\n";
    std::cout << "|  Output:     " << output_mode_to_string(cfg.output_mode) << std::string(29 - std::strlen(output_mode_to_string(cfg.output_mode)), ' ') << "|\n";

    std::string file_str = cfg.output_file;
    if (file_str.size() > 20) file_str = file_str.substr(0, 20) + "...";
    std::cout << "|  File:       " << file_str << std::string(29 - file_str.size(), ' ') << "|\n";

    std::cout << "+------------------------------------------+\n";
    std::cout << "\n";
}
