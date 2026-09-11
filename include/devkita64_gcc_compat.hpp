#pragma once

// Compatibility surface for WiiCompiled translated shards when they are built
// with devkitA64 GCC instead of the pinned runtime's primary Clang toolchain.
// This file is preincluded only in translated-link modes.

#include "memory.h"

#include <cstdint>

#if defined(__GNUC__) && !defined(__clang__)

// WiiCompiled's pinned AArch64 ISA config spells the two-word state-free result
// with Clang's ext_vector_type. GCC's equivalent attribute is vector_size and
// expects the vector size in bytes.
#ifndef ext_vector_type
#define ext_vector_type(N) vector_size(sizeof(std::uint64_t) * (N))
#endif

// Clang exposes __builtin_rotateleft32; the devkitA64 GCC revision used by
// libnx does not. PPC rotate counts are modulo 32, including shift == 0.
inline std::uint32_t mkw_gcc_rotateleft32(std::uint32_t value,
                                          std::uint32_t shift) noexcept {
    shift &= 31u;
    return (value << shift) | (value >> ((32u - shift) & 31u));
}
#ifndef __builtin_rotateleft32
#define __builtin_rotateleft32 mkw_gcc_rotateleft32
#endif

#endif

// The desktop memory_access.h has optimized stack helpers. The Horizon port is
// deliberately correctness-first and uses heap-backed guest regions with
// checked Memory::* accesses, so stack-specialized lowering must stay on that
// same validated path for this checkpoint.
namespace MemoryInline {
inline std::uint8_t ReadStack8(std::uint32_t address) {
    return Memory::Read8(address);
}
inline std::uint16_t ReadStack16(std::uint32_t address) {
    return Memory::Read16(address);
}
inline std::uint32_t ReadStack32(std::uint32_t address) {
    return Memory::Read32(address);
}
inline std::uint64_t ReadStack64(std::uint32_t address) {
    return Memory::Read64(address);
}

inline void WriteStack8(std::uint32_t address, std::uint8_t value) {
    Memory::Write8(address, value);
}
inline void WriteStack16(std::uint32_t address, std::uint16_t value) {
    Memory::Write16(address, value);
}
inline void WriteStack32(std::uint32_t address, std::uint32_t value) {
    Memory::Write32(address, value);
}
inline void WriteStack64(std::uint32_t address, std::uint64_t value) {
    Memory::Write64(address, value);
}
} // namespace MemoryInline

// Only real translated execution builds have WiiCompiled's runtime ABI include
// paths and need the blocker-driven native/HLE trait extension catalogue. Keep
// generic GCC compatibility probes independent of ppc_runtime.h.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "switch_native_hle_traits.hpp"
#endif
