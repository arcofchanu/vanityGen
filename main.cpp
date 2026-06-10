#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>

#include "cli/args.hpp"
#include "wallet/wallet.hpp"
#include "wallet/eth_wallet.hpp"
#include "wallet/btc_wallet.hpp"
#include "wallet/sol_wallet.hpp"
#include "vanity/search.hpp"
#include "output/exporter.hpp"

// ---------------------------------------------------------------------------
// wallet-gen -- Multi-chain bulk wallet generator & vanity address finder
//
// Entry point: parse args -> validate -> dispatch bulk or vanity mode
// ---------------------------------------------------------------------------

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

/// Format a large number with commas: 1234567 -> "1,234,567"
static std::string format_num(uint64_t n) {
    std::string s = std::to_string(n);
    int pos = static_cast<int>(s.length()) - 3;
    while (pos > 0) {
        s.insert(pos, ",");
        pos -= 3;
    }
    return s;
}

/// Bulk generation: multi-threaded wallet generation with progress bar.
static std::vector<Wallet> run_bulk_generation(const Config& cfg) {
    uint64_t total = cfg.bulk_count;
    int threads_count = cfg.thread_count;

    std::cout << "[*] Generating " << format_num(total) << " "
              << chain_to_string(cfg.chain) << " wallets using "
              << threads_count << " threads...\n\n";

    auto start_time = std::chrono::steady_clock::now();

    // Shared atomic progress counter
    std::atomic<uint64_t> progress{0};

    // Per-thread wallet storage
    std::vector<std::vector<Wallet>> thread_results(threads_count);
    std::vector<std::thread> threads;
    threads.reserve(threads_count);

    // Divide work across threads
    uint64_t per_thread = total / threads_count;
    uint64_t remainder = total % threads_count;

    for (int t = 0; t < threads_count; ++t) {
        uint64_t count = per_thread + (static_cast<uint64_t>(t) < remainder ? 1 : 0);
        threads.emplace_back([&cfg, &thread_results, &progress, t, count]() {
            thread_results[t].reserve(count);
            for (uint64_t i = 0; i < count; ++i) {
                thread_results[t].push_back(generate_wallet_for_chain(cfg.chain));
                progress.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Progress bar display on main thread
    const int bar_width = 30;
    uint64_t current = 0;
    while ((current = progress.load(std::memory_order_relaxed)) < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        current = progress.load(std::memory_order_relaxed);
        if (current > total) current = total;

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        double pct = (static_cast<double>(current) / total) * 100.0;
        uint64_t speed = elapsed > 0.0 ? static_cast<uint64_t>(current / elapsed) : 0;
        int filled = static_cast<int>(pct / 100.0 * bar_width);

        std::cout << "\r  [";
        for (int i = 0; i < bar_width; ++i)
            std::cout << (i < filled ? '#' : '.');
        std::cout << "] " << std::fixed << std::setprecision(0) << pct << "%"
                  << " | " << format_num(current) << "/" << format_num(total)
                  << " | ~" << format_num(speed) << "/sec   " << std::flush;
    }

    // Final progress bar at 100%
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        uint64_t speed = elapsed > 0.0 ? static_cast<uint64_t>(total / elapsed) : 0;
        std::cout << "\r  [";
        for (int i = 0; i < bar_width; ++i) std::cout << '#';
        std::cout << "] 100% | " << format_num(total) << "/" << format_num(total)
                  << " | ~" << format_num(speed) << "/sec   \n";
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
    std::cout << "\n[+] Generated " << format_num(total) << " wallets in "
              << std::fixed << std::setprecision(2) << elapsed << "s"
              << " (~" << format_num(static_cast<uint64_t>(speed)) << " wallets/sec)\n";

    return all_wallets;
}

/// Print a wallet to stdout in a formatted box.
static void print_wallet(const Wallet& w) {
    const int content_width = 67;
    std::string border = "+" + std::string(content_width, '-') + "+\n";
    auto row = [&](const std::string& text) {
        std::string line = text;
        if (line.size() < static_cast<size_t>(content_width))
            line += std::string(content_width - line.size(), ' ');
        std::cout << "|" << line << "|\n";
    };

    std::cout << "\n";
    std::cout << border;
    row("  * VANITY MATCH FOUND!");
    std::cout << border;

    row("  Chain:       " + w.chain + std::string(52 - w.chain.size(), ' '));

    {
        std::string addr_line = "  Address:     " + w.address;
        row(addr_line);
    }

    {
        std::string pk_line = "  Private Key: " + w.private_key_hex.substr(0, 52);
        row(pk_line);
        if (w.private_key_hex.size() > 52) {
            row("               " + w.private_key_hex.substr(52));
        }
    }

    {
        std::string pub_line = "  Public Key:  " + w.public_key_hex.substr(0, 52);
        row(pub_line);
        if (w.public_key_hex.size() > 52) {
            row("               " + w.public_key_hex.substr(52));
        }
    }

    std::cout << border;
    std::cout << "\n";
}

int main(int argc, char** argv) {
    // Parse command-line arguments
    Config cfg = parse_args(argc, argv);

    // Print configuration summary
    print_config(cfg);

    if (cfg.run_mode == RunMode::BULK) {
        // -- Bulk Mode -------------------------------------------------------
        auto wallets = run_bulk_generation(cfg);
        output::export_wallets(cfg, wallets);

        std::cout << "\n[+] Done! Generated " << format_num(cfg.bulk_count)
                  << " " << chain_to_string(cfg.chain) << " wallets -> "
                  << cfg.output_file << ".*\n";

    } else {
        // -- Vanity Mode -----------------------------------------------------
        std::cout << "[*] Searching for " << cfg.vanity_count
                  << " vanity address" << (cfg.vanity_count > 1 ? "es" : "") << "...\n";

        std::vector<Wallet> results = vanity::run_vanity_search(cfg);

        // Print all results
        for (const auto& w : results) {
            print_wallet(w);
        }

        // Save to file
        output::export_wallets(cfg, results);

        std::cout << "[+] " << results.size() << " vanity wallet"
                  << (results.size() > 1 ? "s" : "") << " saved to "
                  << cfg.output_file << ".*\n";
    }

    return 0;
}
