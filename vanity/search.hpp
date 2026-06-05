#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <functional>
#include <cmath>

#include "cli/args.hpp"
#include "wallet/wallet.hpp"
#include "wallet/eth_wallet.hpp"
#include "wallet/btc_wallet.hpp"
#include "wallet/sol_wallet.hpp"
#include "vanity/matcher.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Vanity Search Engine
//
// Multi-threaded search using std::thread with atomic counters.
// Each thread generates wallets independently (no shared mutable state on hot path).
// Supports finding 1 or more matches via --vcount flag.
// ─────────────────────────────────────────────────────────────────────────────

namespace vanity {

/// Generate a wallet for the given chain.
inline Wallet generate_wallet(Chain chain) {
    switch (chain) {
        case Chain::ETH: return generate_eth_wallet();
        case Chain::BSC: return generate_bsc_wallet();
        case Chain::BTC: return generate_btc_wallet();
        case Chain::SOL: return generate_sol_wallet();
    }
    return generate_eth_wallet(); // fallback
}

/// Estimate expected attempts for a vanity pattern.
/// ETH/BSC: 16^N per hex character
/// BTC/SOL: 58^N per Base58 character
inline double estimate_attempts(Chain chain, const std::string& pattern, const std::string& suffix, VanityMode mode) {
    int base = 16;
    if (chain == Chain::BTC || chain == Chain::SOL) {
        base = 58;
    }

    double total = 1.0;

    switch (mode) {
        case VanityMode::PREFIX:
            total = std::pow(base, pattern.size());
            break;
        case VanityMode::SUFFIX:
            total = std::pow(base, suffix.size());
            break;
        case VanityMode::BOTH:
            // Combined probability — multiply both
            total = std::pow(base, pattern.size()) * std::pow(base, suffix.size());
            break;
    }

    return total;
}

/// Print estimated search time.
inline void print_estimate(const Config& cfg) {
    double expected = estimate_attempts(cfg.chain, cfg.pattern, cfg.suffix_pattern, cfg.vanity_mode);

    // Rough speed estimates per thread
    double speed_per_thread = 200000.0;
    switch (cfg.chain) {
        case Chain::ETH:
        case Chain::BSC:
            speed_per_thread = 200000.0;
            break;
        case Chain::BTC:
            speed_per_thread = 150000.0;
            break;
        case Chain::SOL:
            speed_per_thread = 300000.0;
            break;
    }

    double total_speed = speed_per_thread * cfg.thread_count;
    double seconds = expected / total_speed;

    std::cout << std::fixed << std::setprecision(0);
    std::cout << "[*] Expected ~" << expected << " attempts per match";
    if (cfg.vanity_count > 1) {
        std::cout << " (~" << (expected * cfg.vanity_count) << " total for " << cfg.vanity_count << " matches)";
    }
    std::cout << "\n";

    seconds *= cfg.vanity_count;
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "[*] At ~" << total_speed << " addr/sec (" << cfg.thread_count << " threads): ";

    if (seconds < 1.0) {
        std::cout << std::setprecision(2) << seconds << " seconds estimated\n";
    } else if (seconds < 60.0) {
        std::cout << std::setprecision(1) << seconds << " seconds estimated\n";
    } else if (seconds < 3600.0) {
        std::cout << std::setprecision(2) << (seconds / 60.0) << " minutes estimated\n";
    } else if (seconds < 86400.0) {
        std::cout << std::setprecision(2) << (seconds / 3600.0) << " hours estimated\n";
    } else {
        std::cout << std::setprecision(1) << (seconds / 86400.0) << " days estimated\n";
    }
    std::cout << "\n";
}

/// Format a large number with commas: 1234567 → "1,234,567"
inline std::string format_number(uint64_t n) {
    std::string s = std::to_string(n);
    int insert_pos = static_cast<int>(s.length()) - 3;
    while (insert_pos > 0) {
        s.insert(insert_pos, ",");
        insert_pos -= 3;
    }
    return s;
}

/// Worker function for vanity search — runs on each thread.
/// Supports finding multiple matches via target_count.
inline void vanity_worker(
    Chain chain,
    const std::string& pattern,
    const std::string& suffix,
    VanityMode mode,
    bool case_sensitive,
    uint64_t target_count,
    std::atomic<bool>& done,
    std::atomic<uint64_t>& found_count,
    std::atomic<uint64_t>& attempts,
    std::mutex& result_mutex,
    std::vector<Wallet>& results
) {
    while (!done.load(std::memory_order_relaxed)) {
        Wallet w = generate_wallet(chain);
        attempts.fetch_add(1, std::memory_order_relaxed);

        if (matches(w.address, pattern, suffix, mode, chain, case_sensitive)) {
            std::lock_guard<std::mutex> lock(result_mutex);
            if (found_count.load() < target_count) {
                results.push_back(std::move(w));
                uint64_t new_count = found_count.fetch_add(1) + 1;
                if (new_count >= target_count) {
                    done.store(true);
                }
            }
            if (done.load()) return;
        }
    }
}

/// Run the vanity search with the given configuration.
/// Returns all matching wallets (1 or more based on vanity_count).
inline std::vector<Wallet> run_vanity_search(const Config& cfg) {
    print_estimate(cfg);

    uint64_t target = cfg.vanity_count;
    std::atomic<bool> done{false};
    std::atomic<uint64_t> found_count{0};
    std::atomic<uint64_t> attempts{0};
    std::mutex result_mutex;
    std::vector<Wallet> results;
    results.reserve(target);

    auto start_time = std::chrono::steady_clock::now();

    // Spawn worker threads
    std::vector<std::thread> threads;
    threads.reserve(cfg.thread_count);

    for (int i = 0; i < cfg.thread_count; ++i) {
        threads.emplace_back(
            vanity_worker,
            cfg.chain,
            std::cref(cfg.pattern),
            std::cref(cfg.suffix_pattern),
            cfg.vanity_mode,
            cfg.case_sensitive,
            target,
            std::ref(done),
            std::ref(found_count),
            std::ref(attempts),
            std::ref(result_mutex),
            std::ref(results)
        );
    }

    // Progress reporting on main thread
    uint64_t last_attempts = 0;
    while (!done.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (done.load(std::memory_order_relaxed)) break;

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        uint64_t current = attempts.load(std::memory_order_relaxed);
        uint64_t speed = current - last_attempts;
        last_attempts = current;
        uint64_t fc = found_count.load(std::memory_order_relaxed);

        std::cout << "\r[+] Attempts: " << format_number(current)
                  << " | Speed: ~" << format_number(speed) << "/sec"
                  << " | Found: " << fc << "/" << target
                  << " | Elapsed: " << std::fixed << std::setprecision(1) << elapsed << "s"
                  << "        " << std::flush;
    }

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    double total_elapsed = std::chrono::duration<double>(end_time - start_time).count();
    uint64_t total_attempts = attempts.load();

    std::cout << "\r[+] Attempts: " << format_number(total_attempts)
              << " | Speed: ~" << format_number(static_cast<uint64_t>(total_attempts / total_elapsed)) << "/sec"
              << " | Found: " << results.size() << "/" << target
              << " | Elapsed: " << std::fixed << std::setprecision(2) << total_elapsed << "s"
              << "        \n";

    return results;
}

} // namespace vanity
