#pragma once

#include "abi_bridge.h"
#include "memory.h"

#include <cstdint>
#include <cstring>

namespace mkw::switch_sc_hle {

constexpr std::uint32_t kProductAreaTable = 0x8029CEB0u;
constexpr std::uint32_t kProductAreaStride = 5u;
constexpr std::uint32_t kProductAreaCount = 13u;
constexpr char kDefaultPalArea[] = "EUR";

// Match pinned WiiCompiled's SC product-region lookup against the SDK-owned
// PAL table already present in the locally translated guest data. The public
// runtime never embeds that table or any Nintendo data; it only reads it when
// the user's local product mapped the address.
inline std::uint32_t LookupProductArea(const char* value, std::uint32_t length) noexcept {
    if (!value || length >= (kProductAreaStride - 1u)) {
        return 0xFFFFFFFFu;
    }

    for (std::uint32_t index = 0; index < kProductAreaCount; ++index) {
        const std::uint32_t entry = kProductAreaTable + index * kProductAreaStride;
        if (!Memory::Contains(entry, kProductAreaStride)) {
            break;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(
            Memory::GetPointer(entry, kProductAreaStride));
        if (!bytes || bytes[0] == 0xFFu) {
            break;
        }

        if (std::memcmp(bytes + 1u, value, length) == 0 &&
            bytes[1u + length] == 0u) {
            return bytes[0];
        }
    }

    return 0xFFFFFFFFu;
}

} // namespace mkw::switch_sc_hle

// SCGetProductArea (PAL 0x801B23A0). Pinned WiiCompiled resolves the AREA
// string from the emulated NAND identity through the SDK table at 0x8029CEB0.
// Its PAL first-boot NAND defaults are AREA=EUR / CODE=LEH / GAME=EU; the
// Switch fast-track does not yet expose a full console-identity surface, so
// mirror that exact fresh-PAL default for this hardware-proven boundary only.
// Adjacent SC identity APIs remain unsupported until hardware reaches them.
template <>
struct KnownNativeCpuCall<0x801B23A0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        if (!cpu) {
            return;
        }

        cpu->gpr[3] = mkw::switch_sc_hle::LookupProductArea(
            mkw::switch_sc_hle::kDefaultPalArea, 3u);
    }
};
