#pragma once

#include "abi_bridge.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

namespace mkw::switch_nand_hle {

constexpr std::uint32_t kTitleIdHi = 0x00010004u;
constexpr std::uint32_t kPalTitleIdLo = 0x524D4350u; // "RMCP" fallback used by pinned WiiCompiled.
constexpr std::uint32_t kNandHomeDirAddr = 0x80346D20u;
constexpr std::uint32_t kNandInitializedAddr = 0x80386848u;
constexpr std::uint32_t kNandInitializedValue = 2u;
constexpr std::uint32_t kNandResultOk = 0u;
constexpr std::uint32_t kNandResultInvalid = static_cast<std::uint32_t>(-8);

inline bool IsValidGameCode(std::uint32_t code) noexcept {
    for (int shift = 24; shift >= 0; shift -= 8) {
        const unsigned char ch = static_cast<unsigned char>((code >> shift) & 0xFFu);
        if (!std::isalnum(ch)) {
            return false;
        }
    }
    return true;
}

inline std::uint32_t CurrentGameCode() noexcept {
    if (Memory::Contains(0x80000000u, 4u)) {
        const std::uint32_t code = Memory::Read32(0x80000000u);
        if (IsValidGameCode(code)) {
            return code;
        }
    }
    return kPalTitleIdLo;
}

inline std::string HexWord(std::uint32_t value) {
    char text[9] = {};
    std::snprintf(text, sizeof(text), "%08x", value);
    return text;
}

inline std::string CurrentNandDataDir(std::uint32_t gameCode) {
    char path[64] = {};
    std::snprintf(path,
                  sizeof(path),
                  "/title/%08x/%08x/data",
                  kTitleIdHi,
                  gameCode);
    return path;
}

inline bool WriteGuestCString(std::uint32_t address, const std::string& value) noexcept {
    if (!Memory::Contains(address, value.size() + 1u)) {
        return false;
    }

    for (std::size_t i = 0; i < value.size(); ++i) {
        Memory::Write8(address + static_cast<std::uint32_t>(i),
                       static_cast<std::uint8_t>(value[i]));
    }
    Memory::Write8(address + static_cast<std::uint32_t>(value.size()), 0u);
    return true;
}

inline void EnsureHostTitleDataDir(std::uint32_t gameCode) noexcept {
    try {
        std::error_code ec;
        const std::filesystem::path dataDir =
            mkw::horizon_runtime_services::nand_root() /
            "title" /
            HexWord(kTitleIdHi) /
            HexWord(gameCode) /
            "data";
        std::filesystem::create_directories(dataDir, ec);
    } catch (...) {
        // Pinned WiiCompiled treats host directory creation as bootstrap support,
        // not as a guest-visible NANDInit failure. Keep it best-effort here too.
    }
}

} // namespace mkw::switch_nand_hle

// NANDInit (PAL 0x8019E18C). Mirror pinned WiiCompiled's synchronous NAND
// bootstrap: initialize the host-side ISFS data root, publish the title's NAND
// data directory into the fixed guest NANDHomeDir buffer, mark NAND initialized,
// and return NAND_RESULT_OK. Real IOS /dev/fs is deliberately not opened on
// Horizon; later NAND/ISFS entry points can share the same SD-backed nand_root.
template <>
struct KnownNativeCpuCall<0x8019E18Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        const std::uint32_t gameCode = mkw::switch_nand_hle::CurrentGameCode();
        const std::string dataDir = mkw::switch_nand_hle::CurrentNandDataDir(gameCode);

        mkw::switch_nand_hle::EnsureHostTitleDataDir(gameCode);

        if (!mkw::switch_nand_hle::WriteGuestCString(
                mkw::switch_nand_hle::kNandHomeDirAddr, dataDir) ||
            !Memory::Contains(mkw::switch_nand_hle::kNandInitializedAddr, 4u)) {
            cpu->gpr[3] = mkw::switch_nand_hle::kNandResultInvalid;
            return;
        }

        Memory::Write32(mkw::switch_nand_hle::kNandInitializedAddr,
                        mkw::switch_nand_hle::kNandInitializedValue);
        cpu->gpr[3] = mkw::switch_nand_hle::kNandResultOk;
    }
};
