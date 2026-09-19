#pragma once

// Probe-local seam shadowing the runtime's header-only guest-write header.
// The probe has no guest RAM and no write-generation tracking, but the GX
// HLE headers need address canonicalization, so provide exactly that piece
// here, mirroring the runtime's aliasing rules against our Memory layout.

#include "memory.h"

#include <cstdint>

inline uint32_t CanonicalizeGxMainRamAddress(uint32_t addr) noexcept {
    if (addr < 0x01800000u) {
        return addr;
    }
    if (addr >= Memory::kMem2PhysicalBase && addr < Memory::kMem2PhysicalEnd) {
        return addr;
    }
    if (addr >= 0x80000000u && addr < 0x81800000u) {
        return addr - 0x80000000u;
    }
    if (addr >= Memory::kMem2CachedBase && addr < Memory::kMem2CachedEnd) {
        return addr - 0x80000000u;
    }
    if (addr >= 0xC0000000u && addr < 0xC1800000u) {
        return addr - 0xC0000000u;
    }
    if (addr >= Memory::kMem2UncachedBase && addr < Memory::kMem2UncachedEnd) {
        return addr - 0xC0000000u;
    }
    return addr;
}
