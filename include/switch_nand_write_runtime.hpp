#pragma once

#include <cstdint>

namespace mkw::switch_nand_runtime {
// Source-owned interface: only the observed ordinary temporary banner write.
// Keep this out of the shared translated ABI headers.
struct BannerWriteResult {
    bool admitted = false;
    std::int32_t result = -8;
    std::int32_t fd = 0;
    std::int32_t mode = 0;
    std::uint8_t openFlag = 0;
};
BannerWriteResult WriteBannerSync(std::uint32_t fileInfoPtr,
                                  std::uint32_t bufferPtr,
                                  std::uint32_t length) noexcept;
} // namespace mkw::switch_nand_runtime
