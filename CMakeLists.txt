cmake_minimum_required(VERSION 3.16)
project(wallet-gen CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimized release build by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler-specific optimization flags
if(MSVC)
    add_compile_options(/O2 /W3 /EHsc)
else()
    add_compile_options(-O3 -Wall -Wextra -Wpedantic)
endif()

# ── Dependencies ─────────────────────────────────────────────────────────────

find_package(OpenSSL REQUIRED)
find_package(libsecp256k1 CONFIG REQUIRED)

# libsodium: try vcpkg name first, then pkg-config fallback (MSYS2)
find_package(unofficial-sodium CONFIG QUIET)
if(unofficial-sodium_FOUND)
    set(SODIUM_TARGET unofficial-sodium::sodium)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
    set(SODIUM_TARGET PkgConfig::SODIUM)
endif()

# ── Sources ──────────────────────────────────────────────────────────────────

set(SOURCES
    main.cpp
    crypto/keccak256.cpp
    wallet/eth_wallet.cpp
    wallet/btc_wallet.cpp
    wallet/sol_wallet.cpp
    output/exporter.cpp
)

add_executable(wallet-gen ${SOURCES})

target_include_directories(wallet-gen PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(wallet-gen PRIVATE
    OpenSSL::Crypto
    libsecp256k1::secp256k1
    ${SODIUM_TARGET}
)

# Windows: link bcrypt for CSPRNG
if(WIN32)
    target_link_libraries(wallet-gen PRIVATE bcrypt)
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(wallet-gen PRIVATE Threads::Threads)
