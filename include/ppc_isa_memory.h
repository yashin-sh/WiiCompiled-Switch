#pragma once

// Correctness-first Horizon memory seam for WiiCompiled translated code.
//
// The pinned desktop runtime's ppc_isa_memory.h chains its full memory_access.h
// and assumes a directly accessible/protectable flat guest mapping. The Switch
// port intentionally uses heap-backed MEM1/MEM2 with checked access, so every
// helper below stays on the already hardware-validated Memory::* path.
//
// This is deliberately conservative. It exists so translated function shards
// can be compiled/linked without silently selecting the desktop memory model.

#include "guest_flat_memory.h"
#include "memory.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace MemoryInline {

inline constexpr bool IsMmioAddress(std::uint32_t addr) noexcept {
    return addr >= 0xCC000000u && addr < 0xCE000000u;
}

inline constexpr bool IsGpuFifoAddress(std::uint32_t addr) noexcept {
    return addr >= 0xCC008000u && addr < 0xCC008100u;
}

inline constexpr bool FlatWriteNeedsPolicy(std::uint32_t address) noexcept {
    return (address & 0xFE000000u) == 0xCC000000u;
}

inline std::uint16_t ByteSwap16(std::uint16_t value) noexcept {
    return __builtin_bswap16(value);
}
inline std::uint32_t ByteSwap32(std::uint32_t value) noexcept {
    return __builtin_bswap32(value);
}
inline std::uint64_t ByteSwap64(std::uint64_t value) noexcept {
    return __builtin_bswap64(value);
}

template <typename T>
inline T MaybeByteSwap(T value) noexcept {
    if constexpr (sizeof(T) == 1) return value;
    if constexpr (sizeof(T) == 2) return static_cast<T>(ByteSwap16(static_cast<std::uint16_t>(value)));
    if constexpr (sizeof(T) == 4) return static_cast<T>(ByteSwap32(static_cast<std::uint32_t>(value)));
    if constexpr (sizeof(T) == 8) return static_cast<T>(ByteSwap64(static_cast<std::uint64_t>(value)));
    return value;
}

// Matches the pinned runtime's Broadway/Gekko double->single narrowing used by
// stfs/psq stores. Keeping it here avoids introducing host-cast differences
// when translated execution is enabled in a later checkpoint.
inline std::uint32_t ConvertPpcDoubleToSingleBits(double value) noexcept {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const std::uint32_t exponent = static_cast<std::uint32_t>((bits >> 52) & 0x7FFu);
    if (exponent - 874u <= 22u) {
        std::uint32_t narrowed = static_cast<std::uint32_t>(
            0x80000000ULL | ((bits & 0x000FFFFFFFFFFFFFULL) >> 21));
        narrowed >>= (905u - exponent);
        narrowed |= static_cast<std::uint32_t>((bits >> 32) & 0x80000000ULL);
        return narrowed;
    }
    return static_cast<std::uint32_t>(
        ((bits >> 32) & 0xC0000000ULL) | ((bits >> 29) & 0x3FFFFFFFULL));
}

inline std::uint8_t Read8Slow(std::uint32_t addr) { return Memory::Read8(addr); }
inline std::uint16_t Read16Slow(std::uint32_t addr) { return Memory::Read16(addr); }
inline std::uint32_t Read32Slow(std::uint32_t addr) { return Memory::Read32(addr); }
inline std::uint64_t Read64Slow(std::uint32_t addr) { return Memory::Read64(addr); }

inline float ReadFloat32Slow(std::uint32_t addr) {
    const std::uint32_t bits = Memory::Read32(addr);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
inline double ReadFloat64Slow(std::uint32_t addr) {
    const std::uint64_t bits = Memory::Read64(addr);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

inline void Write8Slow(std::uint32_t addr, std::uint8_t value) { Memory::Write8(addr, value); }
inline void Write16Slow(std::uint32_t addr, std::uint16_t value) { Memory::Write16(addr, value); }
inline void Write32Slow(std::uint32_t addr, std::uint32_t value) { Memory::Write32(addr, value); }
inline void Write64Slow(std::uint32_t addr, std::uint64_t value) { Memory::Write64(addr, value); }
inline void WriteFloat32Slow(std::uint32_t addr, double value) {
    Memory::Write32(addr, ConvertPpcDoubleToSingleBits(value));
}
inline void WriteFloat64Slow(std::uint32_t addr, double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    Memory::Write64(addr, bits);
}

template <typename T>
inline T ReadResolvedFallback(std::uint32_t addr) {
    static_assert(std::is_integral_v<T> && sizeof(T) <= 8);
    if constexpr (sizeof(T) == 1) return static_cast<T>(Memory::Read8(addr));
    if constexpr (sizeof(T) == 2) return static_cast<T>(Memory::Read16(addr));
    if constexpr (sizeof(T) == 4) return static_cast<T>(Memory::Read32(addr));
    return static_cast<T>(Memory::Read64(addr));
}
inline float ReadResolvedFallbackFloat32(std::uint32_t addr) { return ReadFloat32Slow(addr); }
inline double ReadResolvedFallbackFloat64(std::uint32_t addr) { return ReadFloat64Slow(addr); }

template <typename T>
inline void WriteResolvedFallback(std::uint32_t addr, T value) {
    static_assert(std::is_integral_v<T> && sizeof(T) <= 8);
    if constexpr (sizeof(T) == 1) Memory::Write8(addr, static_cast<std::uint8_t>(value));
    else if constexpr (sizeof(T) == 2) Memory::Write16(addr, static_cast<std::uint16_t>(value));
    else if constexpr (sizeof(T) == 4) Memory::Write32(addr, static_cast<std::uint32_t>(value));
    else Memory::Write64(addr, static_cast<std::uint64_t>(value));
}
inline void WriteResolvedFallbackFloat32(std::uint32_t addr, double value) { WriteFloat32Slow(addr, value); }
inline void WriteResolvedFallbackFloat64(std::uint32_t addr, double value) { WriteFloat64Slow(addr, value); }

// Checked Horizon mode never hands translated code a raw range pointer. This
// guarantees resolved-range lowering falls back to the guest address carried
// beside the pointer rather than bypassing the Switch mapping policy.
inline std::uint8_t* ResolveRangeHost(std::uint32_t, std::int32_t, std::uint32_t,
                                      bool, bool) noexcept {
    return nullptr;
}
inline bool TryGetPointerFast(std::uint32_t, std::size_t, std::uint8_t*& pointer) noexcept {
    pointer = nullptr;
    return false;
}
inline std::uint8_t* GetPointerFast(std::uint32_t, std::size_t) noexcept { return nullptr; }
inline bool TryGetWritablePointerFast(std::uint32_t, std::size_t, std::uint8_t*& pointer) noexcept {
    pointer = nullptr;
    return false;
}

struct ResolvedLoadPair {
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    bool valid = false;
};

inline ResolvedLoadPair ReadResolvedPair16(std::uint8_t* host, std::uint32_t offset) noexcept {
    if (!host) return {};
    std::uint32_t packed = 0;
    std::memcpy(&packed, host + offset, sizeof(packed));
    packed = ByteSwap32(packed);
    return {packed >> 16, packed & 0xFFFFu, true};
}
inline ResolvedLoadPair ReadResolvedPair32(std::uint8_t* host, std::uint32_t offset) noexcept {
    if (!host) return {};
    std::uint64_t packed = 0;
    std::memcpy(&packed, host + offset, sizeof(packed));
    packed = ByteSwap64(packed);
    return {static_cast<std::uint32_t>(packed >> 32), static_cast<std::uint32_t>(packed), true};
}
inline bool WriteResolvedPair16(std::uint8_t* host, std::uint32_t offset, std::uint32_t packed) noexcept {
    if (!host) return false;
    const std::uint32_t swapped = ByteSwap32(packed);
    std::memcpy(host + offset, &swapped, sizeof(swapped));
    return true;
}
inline bool WriteResolvedPair32(std::uint8_t* host, std::uint32_t offset, std::uint64_t packed) noexcept {
    if (!host) return false;
    const std::uint64_t swapped = ByteSwap64(packed);
    std::memcpy(host + offset, &swapped, sizeof(swapped));
    return true;
}

inline std::uint8_t ReadResolved8(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return Memory::Read8(addr); }
inline std::uint16_t ReadResolved16(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return Memory::Read16(addr); }
inline std::uint32_t ReadResolved32(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return Memory::Read32(addr); }
inline std::uint64_t ReadResolved64(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return Memory::Read64(addr); }
inline float ReadResolvedFloat32(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return ReadFloat32Slow(addr); }
inline double ReadResolvedFloat64(std::uint8_t*, std::uint32_t, std::uint32_t addr) { return ReadFloat64Slow(addr); }

inline void WriteResolved8(std::uint8_t*, std::uint32_t, std::uint32_t addr, std::uint8_t value) { Memory::Write8(addr, value); }
inline void WriteResolved16(std::uint8_t*, std::uint32_t, std::uint32_t addr, std::uint16_t value) { Memory::Write16(addr, value); }
inline void WriteResolved32(std::uint8_t*, std::uint32_t, std::uint32_t addr, std::uint32_t value) { Memory::Write32(addr, value); }
inline void WriteResolved64(std::uint8_t*, std::uint32_t, std::uint32_t addr, std::uint64_t value) { Memory::Write64(addr, value); }
inline void WriteResolvedFloat32(std::uint8_t*, std::uint32_t, std::uint32_t addr, double value) { WriteFloat32Slow(addr, value); }
inline void WriteResolvedFloat64(std::uint8_t*, std::uint32_t, std::uint32_t addr, double value) { WriteFloat64Slow(addr, value); }

// The generator names a complete width matrix, although normal PPC floating
// loads/stores use 32/64-bit forms. Define 8/16-bit entries conservatively so
// the Switch seam remains closed under generated helper selection.
inline float FlatReadFloat8(std::uint32_t addr) { return static_cast<float>(Memory::Read8(addr)); }
inline float FlatReadFloat16(std::uint32_t addr) { return static_cast<float>(Memory::Read16(addr)); }
inline float FlatReadFloat32(std::uint32_t addr) { return ReadFloat32Slow(addr); }
inline double FlatReadFloat64(std::uint32_t addr) { return ReadFloat64Slow(addr); }
inline std::uint8_t FlatRead8(std::uint32_t addr) { return Memory::Read8(addr); }
inline std::uint16_t FlatRead16(std::uint32_t addr) { return Memory::Read16(addr); }
inline std::uint32_t FlatRead32(std::uint32_t addr) { return Memory::Read32(addr); }
inline std::uint64_t FlatRead64(std::uint32_t addr) { return Memory::Read64(addr); }

inline void FlatWrite8(std::uint32_t addr, std::uint8_t value) { Memory::Write8(addr, value); }
inline void FlatWrite16(std::uint32_t addr, std::uint16_t value) { Memory::Write16(addr, value); }
inline void FlatWrite32(std::uint32_t addr, std::uint32_t value) { Memory::Write32(addr, value); }
inline void FlatWrite64(std::uint32_t addr, std::uint64_t value) { Memory::Write64(addr, value); }
inline void FlatWriteFloat8(std::uint32_t addr, double value) { Memory::Write8(addr, static_cast<std::uint8_t>(value)); }
inline void FlatWriteFloat16(std::uint32_t addr, double value) { Memory::Write16(addr, static_cast<std::uint16_t>(value)); }
inline void FlatWriteFloat32(std::uint32_t addr, double value) { WriteFloat32Slow(addr, value); }
inline void FlatWriteFloat64(std::uint32_t addr, double value) { WriteFloat64Slow(addr, value); }

// Even translator-proven RAM stores stay checked on Horizon until a writable
// direct-map policy is validated on hardware.
inline void FlatWriteRam8(std::uint32_t addr, std::uint8_t value) { FlatWrite8(addr, value); }
inline void FlatWriteRam16(std::uint32_t addr, std::uint16_t value) { FlatWrite16(addr, value); }
inline void FlatWriteRam32(std::uint32_t addr, std::uint32_t value) { FlatWrite32(addr, value); }
inline void FlatWriteRam64(std::uint32_t addr, std::uint64_t value) { FlatWrite64(addr, value); }
inline void FlatWriteRamFloat8(std::uint32_t addr, double value) { FlatWriteFloat8(addr, value); }
inline void FlatWriteRamFloat16(std::uint32_t addr, double value) { FlatWriteFloat16(addr, value); }
inline void FlatWriteRamFloat32(std::uint32_t addr, double value) { FlatWriteFloat32(addr, value); }
inline void FlatWriteRamFloat64(std::uint32_t addr, double value) { FlatWriteFloat64(addr, value); }

} // namespace MemoryInline
