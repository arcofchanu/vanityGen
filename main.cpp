#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>

#include "cli/args.hpp"
#include "wallet/wallet.hpp"
#include "wallet/eth_wallet.hpp"
#include "wallet/btc_wallet.hpp"
#include "wallet/sol_wallet.hpp"
#include "vanity/search.hpp"
#include "output/exporter.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// wallet-gen — Multi-chain bulk wallet generator & vanity address finder
//
// Entry point: parse args → validate → dispatch bulk or vanity mode
// ─────────────────────────────────────────────────────────────────────────────

/// Generate a single wallet for the given chain.
static Wallet generate_wallet_for_chain(Chain chain) {
    switch (chain) {
        case Chain::ETH: return generate_eth_wallet();
        case Chain::BSC: return generate_bsc_wallet();
        case Chain::BTC: return generate_btc_wallet();
        case Chain::SOL: return generate_sol_wallet();
    }
    return generate_eth_wallet();
}

/// Format a large number with commas: 1234567 → "1,234,567"
static std::string format_num(uint64_t n) {
    std::string s = std::to_string(n);
    int pos = static_cast<int>(s.length()) - 3;
    while (pos > 0) {
        s.insert(pos, ",");
        pos -= 3;
    }
    return s;
}

/// Bulk generation: multi-threaded wallet generation with per-thread local vectors.
static std::vector<Wallet> run_bulk_generation(const Config& cfg) {
    uint64_t total = cfg.bulk_count;
    int threads_count = cfg.thread_count;

    std::cout << "[*] Generating " << format_num(total) << " "
              << chain_to_string(cfg.chain) << " wallets using "
              << threads_count << " threads...\n\n";

    auto start_time = std::chrono::steady_clock::now();

    // Per-thread wallet storage
    std::vector<std::vector<Wallet>> thread_results(threads_count);
    std::vector<std::thread> threads;
    threads.reserve(threads_count);

    // Divide work across threads
    uint64_t per_thread = total / threads_count;
    uint64_t remainder = total % threads_count;

    for (int t = 0; t < threads_count; ++t) {
        uint64_t count = per_thread + (static_cast<uint64_t>(t) < remainder ? 1 : 0);
        threads.emplace_back([&cfg, &thread_results, t, count]() {
            thread_results[t].reserve(count);
            for (uint64_t i = 0; i < count; ++i) {
                thread_results[t].push_back(generate_wallet_for_chain(cfg.chain));
            }
        });
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();

    // Merge results
    std::vector<Wallet> all_wallets;
    all_wallets.reserve(total);
    for (auto& tv : thread_results) {
        all_wallets.insert(all_wallets.end(),
                          std::make_move_iterator(tv.begin()),
                          std::make_move_iterator(tv.end()));
    }

    double speed = total / elapsed;
    std::cout << "[+] Generated " << format_num(total) << " wallets in "
              << std::fixed << std::setprecision(2) << elapsed << "s"
              << " (~" << format_num(static_cast<uint64_t>(speed)) << " wallets/sec)\n";

    return all_wallets;
}

/// Print a wallet to stdout in a formatted box.
static void print_wallet(const Wallet& w) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✓ VANITY MATCH FOUND!                                         ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";

    std::cout << "║  Chain:       " << w.chain;
    std::cout << std::string(52 - w.chain.size(), ' ') << "║\n";

    std::cout << "║  Address:     " << w.address;
    if (w.address.size() < 52)
        std::cout << std::string(52 - w.address.size(), ' ');
    std::cout << "║\n";

    std::cout << "║  Private Key: " << w.private_key_hex.substr(0, 52);
    if (w.private_key_hex.size() > 52) {
        std::cout << "║\n";
        std::cout << "║               " << w.private_key_hex.substr(52);
        size_t rem = w.private_key_hex.size() - 52;
        if (rem < 52) std::cout << std::string(52 - rem, ' ');
    } else {
        std::cout << std::string(52 - w.private_key_hex.size(), ' ');
    }
    std::cout << "║\n";

    std::cout << "║  Public Key:  " << w.public_key_hex.substr(0, 52);
    if (w.public_key_hex.size() > 52) {
        std::cout << "║\n";
        std::cout << "║               " << w.public_key_hex.substr(52);
        size_t rem = w.public_key_hex.size() - 52;
        if (rem < 52) std::cout << std::string(52 - rem, ' ');
    } else {
        std::cout << std::string(52 - w.public_key_hex.size(), ' ');
    }
    std::cout << "║\n";

    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main(int argc, char** argv) {
    // Parse command-line arguments
    Config cfg = parse_args(argc, argv);

    // Print configuration summary
    print_config(cfg);

    if (cfg.run_mode == RunMode::BULK) {
        // ── Bulk Mode ──────────────────────────────────────────────────
        auto wallets = run_bulk_generation(cfg);
        output::export_wallets(cfg, wallets);

        std::cout << "\n[✓] Done! Generated " << format_num(cfg.bulk_count)
                  << " " << chain_to_string(cfg.chain) << " wallets → "
                  << cfg.output_file << ".*\n";

    } else {
        // ── Vanity Mode ────────────────────────────────────────────────
        std::cout << "[*] Searching for " << cfg.vanity_count
                  << " vanity address" << (cfg.vanity_count > 1 ? "es" : "") << "...\n";

        std::vector<Wallet> results = vanity::run_vanity_search(cfg);

        // Print all results
        for (const auto& w : results) {
            print_wallet(w);
        }

        // Save to file
        output::export_wallets(cfg, results);

        std::cout << "[✓] " << results.size() << " vanity wallet"
                  << (results.size() > 1 ? "s" : "") << " saved to "
                  << cfg.output_file << ".*\n";
    }

    return 0;
}
