#pragma once
#include <cstdint>
namespace mkw::switch_nand_runtime {
struct LengthResult {
    bool admitted = false;
    std::int32_t result = -8, fd = 0, mode = 0;
    std::uint8_t accessType = 0, openFlag = 0;
    std::uint32_t length = 0;
    long position = 0;
    int endSeek = 0, restoreSeek = 0;
};
LengthResult LengthOwnedReadSync(std::uint32_t fileInfoPtr, std::uint32_t outLengthPtr) noexcept;
} // namespace mkw::switch_nand_runtime
