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
constexpr std::uint32_t kGxDrawDoneFlagAddr = 0x803867D8u;

void PublishFinishInterruptState() noexcept {
    if (!Memory::IsInitialized()) {
        return;
    }

    try {
        if (Memory::Contains(kGxDataPtrAddr, 4u)) {
            const std::uint32_t gxData = Memory::Read32(kGxDataPtrAddr);
            if (gxData != 0u && Memory::Contains(gxData + 0x0Au, 2u)) {
                Memory::Write16(
                    gxData + 0x0Au,
                    static_cast<std::uint16_t>(
                        Memory::Read16(gxData + 0x0Au) | 0x0008u));
            }
        }

        if (Memory::Contains(kGxDrawDoneFlagAddr, 1u)) {
            Memory::Write8(kGxDrawDoneFlagAddr, 1u);
        }
    } catch (...) {
        // Match pinned WiiCompiled's best-effort guest bookkeeping.
    }
}

} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_draw_done(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    // Pinned WiiCompiled clears the guest done flag before draining GX, then
    // publishes the PE-finish bookkeeping after the drain completes.
    if (Memory::IsInitialized()) {
        try {
            if (Memory::Contains(kGxDrawDoneFlagAddr, 1u)) {
                Memory::Write8(kGxDrawDoneFlagAddr, 0u);
            }
        } catch (...) {
        }
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_DRAW_DONE");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Aurora's GXDrawDone drains the real pinned FIFO and invokes its host-side
    // draw-done callback when one is installed. Do not fabricate a present:
    // GXCopyDisp remains the separate frame/present boundary.
    GXDrawDone();
#endif

    PublishFinishInterruptState();

    // The pinned override is void. Preserve the translated caller's GPRs.
}

#endif
