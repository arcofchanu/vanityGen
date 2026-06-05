# wallet-gen

Multi-chain bulk wallet generator and vanity address finder.  
Supports **Ethereum**, **BSC**, **Bitcoin**, and **Solana**.  
Fully offline. Multi-threaded. C++17.

---

## Features

- **Bulk Mode** — Generate N wallets and export to CSV/JSON
- **Vanity Mode** — Find addresses matching a prefix, suffix, or both
- **Multi-threaded** — Uses all available CPU cores for maximum speed
- **Cross-platform** — Windows, Linux, macOS

## Supported Chains

| Chain | Curve | Address Format |
|-------|-------|---------------|
| ETH | secp256k1 | `0x` + 40 hex chars |
| BSC | secp256k1 | `0x` + 40 hex chars (EVM-compatible) |
| BTC | secp256k1 | Base58Check, starts with `1` |
| SOL | ed25519 | Base58, 32-44 chars |

## Dependencies

| Library | Purpose |
|---------|---------|
| libsecp256k1 | ETH/BSC/BTC key generation |
| libsodium | Solana ed25519 keypairs |
| OpenSSL | SHA-256, RIPEMD-160 for BTC |

## Build

### Windows (vcpkg)

```powershell
# Install vcpkg if you don't have it
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat && cd ..

# Build
cd wallet-gen
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Linux (apt)

```bash
sudo apt install -y build-essential cmake libssl-dev libsecp256k1-dev libsodium-dev
cd wallet-gen
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS (Homebrew)

```bash
brew install cmake openssl libsodium secp256k1
cd wallet-gen
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Usage

```
wallet-gen [OPTIONS]

OPTIONS:
  --chain     <eth|bsc|btc|sol>      Target chain (default: eth)
  --mode      <bulk|vanity>          Run mode (default: bulk)
  --count     <N>                    Wallets to generate in bulk mode (default: 1000)
  --pattern   <string>               Vanity pattern to search for
  --suffix    <string>               Suffix pattern (for --vanity both)
  --vanity    <prefix|suffix|both>   Vanity match type (default: prefix)
  --threads   <N>                    Thread count (default: auto-detect)
  --output    <filename>             Output file base name (default: wallets)
  --format    <csv|json|both>        Output format (default: csv)
  --case                             Enable case-sensitive matching
  --help                             Show help
```

## Examples

```bash
# Generate 10,000 ETH wallets to CSV
./wallet-gen --chain eth --mode bulk --count 10000 --output eth_wallets

# Find ETH vanity address starting with "dead"
./wallet-gen --chain eth --mode vanity --pattern dead --vanity prefix

# Find BTC address starting with "1Chanu"
./wallet-gen --chain btc --mode vanity --pattern Chanu --vanity prefix --case

# Find SOL address starting with "Sol" and ending with "ana"
./wallet-gen --chain sol --mode vanity --pattern Sol --suffix ana --vanity both

# Bulk generate 500 BTC wallets, JSON output, 8 threads
./wallet-gen --chain btc --mode bulk --count 500 --threads 8 --format json
```

## Security

- Private keys are generated using OS-level CSPRNG (BCryptGenRandom / /dev/urandom)
- No network calls — fully offline
- Output files contain real private keys — treat them as sensitive
- Consider `chmod 600 wallets.csv` (Linux/macOS) after generation

## Performance

| Chain | Speed (per core) |
|-------|-----------------|
| ETH/BSC | ~150k–300k addr/sec |
| BTC | ~100k–200k addr/sec |
| SOL | ~200k–400k addr/sec |

Scales linearly with core count (lock-free hot path).

## License

MIT
