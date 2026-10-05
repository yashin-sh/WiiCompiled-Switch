#pragma once

#include "memory.h"

#include <cstdint>

namespace mkw::switch_gx_tex_coord_mirror {
constexpr std::uint32_t kGxDataPtrAddress = 0x803886C8u;

inline bool CheckedAddress(
    std::uint32_t base,
    std::uint32_t offset,
    std::uint32_t bytes,
    std::uint32_t& address) noexcept {
    const std::uint64_t wide = static_cast<std::uint64_t>(base) + offset;
    if (wide > 0xffffffffull) {
        return false;
    }
    address = static_cast<std::uint32_t>(wide);
    return Memory::Contains(address, bytes);
}

inline void Scale(
    std::uint32_t coord,
    std::uint32_t enable,
    std::uint32_t sSize,
    std::uint32_t tSize) noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(kGxDataPtrAddress, 4u)) {
        return;
    }
    try {
        const std::uint32_t gxData = Memory::Read32(kGxDataPtrAddress);
        if (gxData == 0u) {
            return;
        }
        std::uint32_t address = 0u;
        if (!CheckedAddress(gxData, 0x5E4u, 4u, address)) {
            return;
        }
        Memory::Write32(
            address,
            (Memory::Read32(address) & ~(1u << coord)) | ((enable & 1u) << coord));
        if (enable == 0u) {
            return;
        }
        if (!CheckedAddress(gxData, 0x108u + coord * 4u, 4u, address)) {
            return;
        }
        Memory::Write32(address, (Memory::Read32(address) & 0xFFFF0000u) |
                                     ((sSize - 1u) & 0xFFFFu));
        if (!CheckedAddress(gxData, 0x128u + coord * 4u, 4u, address)) {
            return;
        }
        Memory::Write32(address, (Memory::Read32(address) & 0xFFFF0000u) |
                                     ((tSize - 1u) & 0xFFFFu));
        if (!CheckedAddress(gxData, 2u, 2u, address)) {
            return;
        }
        Memory::Write16(address, 0u);
    } catch (...) {
        // Pinned bookkeeping is best effort: retain prior writes and stop.
    }
}

inline void Bias(std::uint32_t coord, std::uint32_t sEnable, std::uint32_t tEnable) noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(kGxDataPtrAddress, 4u)) {
        return;
    }
    try {
        const std::uint32_t gxData = Memory::Read32(kGxDataPtrAddress);
        if (gxData == 0u) {
            return;
        }
        std::uint32_t address = 0u;
        if (!CheckedAddress(gxData, 0x108u + coord * 4u, 4u, address)) {
            return;
        }
        Memory::Write32(address, (Memory::Read32(address) & 0xFFFEFFFFu) |
                                     ((sEnable & 1u) << 16u));
        if (!CheckedAddress(gxData, 0x128u + coord * 4u, 4u, address)) {
            return;
        }
        Memory::Write32(address, (Memory::Read32(address) & 0xFFFEFFFFu) |
                                     ((tEnable & 1u) << 16u));
        if (!CheckedAddress(gxData, 0x5E4u, 4u, address)) {
            return;
        }
        if ((Memory::Read32(address) & (1u << coord)) == 0u) {
            return;
        }
        if (!CheckedAddress(gxData, 2u, 2u, address)) {
            return;
        }
        Memory::Write16(address, 0u);
    } catch (...) {
        // Mirror failures do not undo native GX or completed guest writes.
    }
}
} // namespace mkw::switch_gx_tex_coord_mirror
