#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>

namespace {

constexpr std::uint32_t kGxDataPtrAddr = 0x803886C8u;

void PublishGuestScissorState(
    std::uint32_t left,
    std::uint32_t top,
    std::uint32_t width,
    std::uint32_t height) noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(kGxDataPtrAddr, 4u)) {
        return;
    }

    try {
        const std::uint32_t gxData = Memory::Read32(kGxDataPtrAddr);
        if (gxData == 0u || !Memory::Contains(gxData + 0x14Cu, 4u) ||
            !Memory::Contains(gxData + 2u, 2u)) {
            return;
        }

        const std::uint32_t oldStart = Memory::Read32(gxData + 0x148u);
        const std::uint32_t oldEnd = Memory::Read32(gxData + 0x14Cu);

        const std::uint32_t sx = left + 0x156u;
        const std::uint32_t sy = top + 0x156u;
        const std::uint32_t ex = sx + width - 1u;
        const std::uint32_t ey = sy + height - 1u;

        const std::uint32_t startWord =
            ((sx << 12u) & 0x007FF000u) |
            (sy & 0x000007FFu) |
            (oldStart & 0xFF800800u);
        const std::uint32_t endWord =
            ((ex << 12u) & 0x007FF000u) |
            (ey & 0x000007FFu) |
            (oldEnd & 0xFF800800u);

        Memory::Write32(gxData + 0x148u, startWord);
        Memory::Write32(gxData + 0x14Cu, endWord);
        Memory::Write16(gxData + 2u, 0u);
    } catch (...) {
        // Match pinned WiiCompiled's best-effort guest GX bookkeeping.
    }
}

} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_scissor(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t left = cpu->gpr[3];
    const std::uint32_t top = cpu->gpr[4];
    const std::uint32_t width = cpu->gpr[5];
    const std::uint32_t height = cpu->gpr[6];

    PublishGuestScissorState(left, top, width, height);
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_SCISSOR");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetScissor(left, top, width, height);
#else
    (void)left;
    (void)top;
    (void)width;
    (void)height;
#endif
}

#endif
