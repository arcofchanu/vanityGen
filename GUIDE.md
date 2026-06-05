# 🚀 wallet-gen — Quick Start Guide

## Table of Contents

- [Prerequisites](#prerequisites)
- [Building the Project](#building-the-project)
- [Running wallet-gen](#running-wallet-gen)
  - [Bulk Mode](#bulk-mode)
  - [Vanity Mode](#vanity-mode)
- [CLI Reference](#cli-reference)
- [Output Files](#output-files)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

This project was built using **MSYS2 (UCRT64)** on Windows. You need:

- **MSYS2** installed at `C:\msys64`
- The following MSYS2 packages (already installed if you followed the build):
  - `mingw-w64-ucrt-x86_64-gcc` (GCC 16+)
  - `mingw-w64-ucrt-x86_64-cmake`
  - `mingw-w64-ucrt-x86_64-ninja`
  - `mingw-w64-ucrt-x86_64-openssl`
  - `mingw-w64-ucrt-x86_64-libsodium`
  - `libsecp256k1` (built from source, installed to `/ucrt64`)

---

## Building the Project

### First-time build

Open **PowerShell** and run:

```powershell
C:\msys64\usr\bin\env.exe MSYSTEM=UCRT64 C:\msys64\usr\bin\bash.exe -lc "cd 'e:/crypto vanity address/wallet-gen' && mkdir -p build && cd build && cmake .. -G Ninja && ninja"
```

### Rebuilding after code changes

```powershell
C:\msys64\usr\bin\env.exe MSYSTEM=UCRT64 C:\msys64\usr\bin\bash.exe -lc "cd 'e:/crypto vanity address/wallet-gen/build' && ninja"
```

### Where's the binary?

After building, the executable is at:

```
e:\crypto vanity address\wallet-gen\build\wallet-gen.exe
```

---

## Running wallet-gen

All commands below assume you're running from the **build** directory.  
Use this prefix to run any command:

```powershell
C:\msys64\usr\bin\env.exe MSYSTEM=UCRT64 C:\msys64\usr\bin\bash.exe -lc "cd 'e:/crypto vanity address/wallet-gen/build' && ./wallet-gen.exe <ARGS>"
```

Or open an **MSYS2 UCRT64** terminal directly from the Start Menu, then:

```bash
cd "e:/crypto vanity address/wallet-gen/build"
./wallet-gen.exe <ARGS>
```

---

### Bulk Mode

Generate N wallets as fast as possible and save to file.

#### Generate 1,000 ETH wallets (default)

```bash
./wallet-gen.exe
```

#### Generate 10,000 ETH wallets to CSV

```bash
./wallet-gen.exe --chain eth --mode bulk --count 10000 --output my_wallets
```

> Creates `my_wallets.csv` in the current directory.

#### Generate 500 BTC wallets in JSON format

```bash
./wallet-gen.exe --chain btc --mode bulk --count 500 --format json --output btc_dump
```

> Creates `btc_dump.json`.

#### Generate SOL wallets with both CSV and JSON

```bash
./wallet-gen.exe --chain sol --mode bulk --count 1000 --format both --output sol_wallets
```

> Creates `sol_wallets.csv` AND `sol_wallets.json`.

#### Use a specific number of threads

```bash
./wallet-gen.exe --chain eth --mode bulk --count 5000 --threads 4
```

---

### Vanity Mode

Search for an address that matches a specific pattern.

#### Find ETH address starting with "dead"

```bash
./wallet-gen.exe --chain eth --mode vanity --pattern dead --vanity prefix
```

**Example output:**
```
[*] Expected ~65,536 attempts on average
[*] At ~2,400,000 addr/sec (12 threads): 0.03 seconds estimated

[+] Attempts: 4,423 | Speed: ~4,400/sec | Elapsed: 1.01s

╔══════════════════════════════════════════════════════════════════╗
║  ✓ VANITY MATCH FOUND!                                         ║
╠══════════════════════════════════════════════════════════════════╣
║  Chain:       ETH                                               ║
║  Address:     0xdeadb1ffff2437123dc8094a502e74dd81a59487          ║
║  Private Key: 0x130160f47260dffef5bcaaff...                      ║
╚══════════════════════════════════════════════════════════════════╝
```

#### Find BTC address starting with "Chanu" (case-sensitive)

```bash
./wallet-gen.exe --chain btc --mode vanity --pattern Chanu --vanity prefix --case
```

> ⚠️ BTC/SOL use Base58 (58 possible chars per position), so longer patterns take **much** longer than ETH hex (16 chars per position).

#### Find SOL address starting with "Sol" and ending with "ana"

```bash
./wallet-gen.exe --chain sol --mode vanity --pattern Sol --suffix ana --vanity both --case
```

#### Find ETH address ending with "1337"

```bash
./wallet-gen.exe --chain eth --mode vanity --pattern 1337 --vanity suffix
```

#### Save vanity result to JSON

```bash
./wallet-gen.exe --chain eth --mode vanity --pattern cafe --vanity prefix --format json --output my_vanity
```

#### Find multiple vanity wallets at once

```bash
# Find 5 ETH addresses starting with "aa"
./wallet-gen.exe --chain eth --mode vanity --pattern aa --vcount 5

# Find 3 BTC addresses starting with "A"
./wallet-gen.exe --chain btc --mode vanity --pattern A --vcount 3 --case --format both
```

> The progress bar shows `Found: 2/5` in real-time as matches are discovered.
> All matches are saved to the output file together.

---

### Time Estimates for Vanity Patterns

The tool prints an estimate before searching. Here's a rough guide:

#### ETH / BSC (hex — 16 chars per position)

| Pattern Length | Expected Attempts | Time @ 100k/sec |
|:-:|:-:|:-:|
| 1 char | ~16 | instant |
| 2 chars | ~256 | instant |
| 3 chars | ~4,096 | instant |
| 4 chars | ~65,536 | < 1 sec |
| 5 chars | ~1,048,576 | ~10 sec |
| 6 chars | ~16,777,216 | ~3 min |
| 7 chars | ~268,435,456 | ~45 min |
| 8 chars | ~4,294,967,296 | ~12 hours |

#### BTC / SOL (Base58 — 58 chars per position)

| Pattern Length | Expected Attempts | Time @ 100k/sec |
|:-:|:-:|:-:|
| 1 char | ~58 | instant |
| 2 chars | ~3,364 | instant |
| 3 chars | ~195,112 | ~2 sec |
| 4 chars | ~11,316,496 | ~2 min |
| 5 chars | ~656,356,768 | ~2 hours |
| 6 chars | ~38 billion | ~4 days |

> 💡 **Tip:** Multiply threads to divide time. 12 threads → 12x faster.

---

## CLI Reference

```
wallet-gen [OPTIONS]

OPTIONS:
  --chain     <eth|bsc|btc|sol>      Target chain (default: eth)
  --mode      <bulk|vanity>          Run mode (default: bulk)
  --count     <N>                    Wallets to generate in bulk mode (default: 1000)
  --pattern   <string>               Vanity pattern (prefix by default)
  --suffix    <string>               Suffix pattern (use with --vanity both)
  --vanity    <prefix|suffix|both>   Vanity match type (default: prefix)
  --vcount    <N>                    Number of vanity matches to find (default: 1)
  --threads   <N>                    Thread count (default: auto-detect all cores)
  --output    <filename>             Output file base name (default: wallets)
  --format    <csv|json|both>        Output format (default: csv)
  --case                             Enable case-sensitive matching
  --help                             Show help
```

---

## Where Files Are Saved

By default, output files are saved **in the directory you run the command from** (the current working directory).

### Default location (when running from build/)

```
e:\crypto vanity address\wallet-gen\build\wallets.csv
e:\crypto vanity address\wallet-gen\build\wallets.json
```

### Controlling the filename

Use `--output` to set the base filename (extension is added automatically):

```bash
# Saves to: ./my_eth_wallets.csv
./wallet-gen.exe --chain eth --mode bulk --count 100 --output my_eth_wallets

# Saves to: ./vanity_result.json
./wallet-gen.exe --chain eth --mode vanity --pattern dead --format json --output vanity_result
```

### Saving to a specific folder

Pass a full path (or relative path) to `--output`:

```bash
# Save to Desktop
./wallet-gen.exe --chain eth --mode bulk --count 100 --output "C:/Users/Dell/Desktop/eth_wallets"

# Save to a subfolder
mkdir -p ../exports
./wallet-gen.exe --chain btc --mode bulk --count 500 --output "../exports/btc_wallets"
```

### Controlling the format

| Flag | Creates | Example |
|------|---------|---------|
| `--format csv` (default) | `wallets.csv` | One file, spreadsheet-friendly |
| `--format json` | `wallets.json` | One file, API/script-friendly |
| `--format both` | `wallets.csv` + `wallets.json` | Both files at once |

```bash
# CSV only (default)
./wallet-gen.exe --chain eth --mode bulk --count 100

# JSON only
./wallet-gen.exe --chain eth --mode bulk --count 100 --format json

# Both CSV and JSON
./wallet-gen.exe --chain eth --mode bulk --count 100 --format both --output my_wallets
# Creates: my_wallets.csv AND my_wallets.json
```

---

## Output File Formats

### CSV Format

Opens directly in Excel, Google Sheets, or any spreadsheet app.

```csv
chain,private_key,public_key,address
ETH,0xabc123...,0x04def...,0x742d35Cc...
ETH,0x456789...,0x04abc...,0x1234ab...
```

### JSON Format

Easy to parse with any programming language or script.

```json
[
  {
    "chain": "ETH",
    "private_key": "0xabc123...",
    "public_key": "0x04def...",
    "address": "0x742d35Cc..."
  }
]
```

### Private Key Formats by Chain

| Chain | Private Key Format | Example Start |
|-------|-------------------|--------------|
| ETH / BSC | Hex with `0x` prefix (64 hex chars) | `0x4fff5b35...` |
| BTC | WIF compressed (Base58Check) | `L42HbeMS...` or `Kx...` |
| SOL | Base58 (64-byte secret key) | `5XRq56yd...` |

---

## How to Use / Import Generated Wallets

### ETH / BSC — Import into MetaMask

1. Open MetaMask → click your account icon → **Import Account**
2. Select **Private Key** type
3. Paste the `private_key` from the CSV/JSON (include the `0x` prefix)
4. Click **Import**

### BTC — Import into Electrum

1. Open Electrum → **File → New/Restore**
2. Choose **Import Bitcoin addresses or private keys**
3. Paste the WIF private key (starts with `K` or `L`)
4. The wallet will show the corresponding address

### SOL — Import into Phantom / Solflare

1. Open Phantom → Settings → **Add/Connect Wallet → Import Private Key**
2. Paste the Base58 private key from the output
3. The wallet will derive the matching address

### Programmatic Access (Python example)

```python
import csv
import json

# Read from CSV
with open('wallets.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        print(f"Address: {row['address']}, Key: {row['private_key']}")

# Read from JSON
with open('wallets.json') as f:
    wallets = json.load(f)
    for w in wallets:
        print(f"Address: {w['address']}, Key: {w['private_key']}")
```

> ⚠️ **Security Warning:** Output files contain real private keys. Treat them like passwords.
> - On Linux/macOS: `chmod 600 wallets.csv`
> - On Windows: right-click → Properties → Security → restrict access
> - **Never share** these files or commit them to git
> - **Never paste** private keys into websites you don't trust

---

## Project Structure

```
wallet-gen/
├── CMakeLists.txt          ← Build configuration
├── vcpkg.json              ← Dependency manifest (vcpkg alternative)
├── README.md               ← Project overview
├── GUIDE.md                ← This file
├── main.cpp                ← Entry point (bulk + vanity dispatch)
│
├── cli/
│   └── args.hpp            ← CLI argument parsing & Config struct
│
├── crypto/
│   ├── random.hpp          ← OS-level CSPRNG (BCryptGenRandom / getrandom)
│   ├── hex.hpp             ← Byte → hex string conversion
│   ├── keccak256.hpp/cpp   ← Keccak-256 hash (Ethereum's variant, NOT SHA3)
│   ├── sha256.hpp          ← SHA-256 + double-SHA256 (via OpenSSL)
│   ├── ripemd160.hpp       ← RIPEMD-160 + Hash160 (via OpenSSL)
│   └── base58.hpp          ← Base58 + Base58Check encoding
│
├── wallet/
│   ├── wallet.hpp          ← Common Wallet struct
│   ├── eth_wallet.hpp/cpp  ← ETH/BSC: secp256k1 → Keccak → 0x address
│   ├── btc_wallet.hpp/cpp  ← BTC: secp256k1 → SHA256 → RIPEMD160 → Base58
│   └── sol_wallet.hpp/cpp  ← SOL: ed25519 → Base58 pubkey
│
├── vanity/
│   ├── matcher.hpp         ← Pattern matching (prefix/suffix/both)
│   └── search.hpp          ← Multi-threaded vanity search engine
│
├── output/
│   ├── exporter.hpp        ← Export interface
│   └── exporter.cpp        ← CSV + JSON writer (atomic file writes)
│
└── build/                  ← Build output (created by cmake)
    └── wallet-gen.exe      ← The compiled binary
```

### How the code flows

```
main.cpp
  ├── parse_args()              → cli/args.hpp
  ├── print_config()            → cli/args.hpp
  │
  ├── [BULK MODE]
  │   ├── spawn N threads
  │   │   └── generate_wallet() → wallet/eth_wallet.cpp (or btc/sol)
  │   │       ├── secure_random_bytes()  → crypto/random.hpp
  │   │       ├── secp256k1 / ed25519    → libsecp256k1 / libsodium
  │   │       └── keccak256 / hash160    → crypto/keccak256.cpp, sha256, ripemd160
  │   ├── merge results
  │   └── export_wallets()      → output/exporter.cpp
  │
  └── [VANITY MODE]
      ├── print_estimate()      → vanity/search.hpp
      ├── spawn N threads
      │   └── vanity_worker()   → vanity/search.hpp
      │       ├── generate_wallet()
      │       └── matches()     → vanity/matcher.hpp
      ├── progress reporting (main thread, 1s interval)
      ├── join all threads
      └── export_wallets()      → output/exporter.cpp
```

---

## Troubleshooting

### "wallet-gen.exe is not recognized"

Make sure you're running from the build directory, or use the full path:

```bash
"e:/crypto vanity address/wallet-gen/build/wallet-gen.exe" --help
```

### "DLL not found" error on Windows

The executable needs MSYS2's UCRT64 DLLs. Either:
1. Run from an MSYS2 UCRT64 terminal, **or**
2. Add `C:\msys64\ucrt64\bin` to your system PATH:
   ```powershell
   $env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
   .\wallet-gen.exe --help
   ```

### Vanity search is taking too long

- Use shorter patterns (each extra char = 16x longer for ETH, 58x for BTC/SOL)
- Check thread count — use all cores: `--threads 0` (auto-detect)
- Press `Ctrl+C` to abort at any time

### Build errors after code changes

Clean rebuild:
```bash
cd "e:/crypto vanity address/wallet-gen/build"
rm -rf *
cmake .. -G Ninja
ninja
```
