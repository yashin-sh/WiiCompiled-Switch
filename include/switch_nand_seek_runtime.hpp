#pragma once

#include <cstdint>

namespace mkw::switch_nand_runtime {
// Source-owned frontier: ordinary owned write handle, offset 0, origin 0.
struct RewindResult {
    bool admitted = false;
    std::int32_t result = -8;
    std::int32_t fd = 0;
    std::int32_t mode = 0;
    std::uint8_t accessType = 0;
    std::uint8_t openFlag = 0;
};
RewindResult RewindOwnedSync(std::uint32_t fileInfoPtr,
                             std::uint32_t offset,
                             std::uint32_t whence) noexcept;
} // namespace mkw::switch_nand_runtime
