#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"
#include "gx_stream_common.h"
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_begin(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t primitive = cpu->gpr[3];
    const std::uint32_t vtx_fmt = cpu->gpr[4];
    const std::uint32_t vertex_count = cpu->gpr[5];

    mkw_switch_set_fast_track_stage("RMCP01_GX_BEGIN");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    if (IsDisplayListActive()) {
        WriteDisplayListData(static_cast<u8>(primitive | vtx_fmt), 1);
        WriteDisplayListData(static_cast<u16>(vertex_count), 2);
        return;
    }

    const auto fmt = static_cast<GXVtxFmt>(vtx_fmt);

    // Match the pinned immediate-mode publication contract. Indexed attributes
    // are expanded to GX_DIRECT by PublishAuroraVtxState, and immediate mode
    // republishes VAT rows only from POS through TEX7.
    GxStream::PublishAuroraVtxState(
        fmt,
        GxStream::AuroraVtxPublishOptions{
            /*includeNbt=*/false,
            /*fmtLoopFirst=*/static_cast<int>(GX_VA_POS),
            /*fmtLoopLast=*/static_cast<int>(GX_VA_TEX7)});

    // Mirror pinned GX__Begin_8016f0f0 state exactly. The Switch runtime does
    // not duplicate WiiCompiled's desktop deferred timing shim here; timing and
    // guest scheduling remain owned by the already hardware-proven Switch
    // runtime, avoiding nested fiber/timer work inside this native boundary.
    g_hleGxState.currentVtxFmt = fmt;
    g_hleGxState.currentPrim = static_cast<GXPrimitive>(primitive);
    g_hleGxState.vertsRemaining = vertex_count;
    g_hleGxState.inBegin = true;
    g_hleGxState.auroraBeginCalled = false;
    g_hleGxState.fifoReadOffset = 0;
    g_hleGxState.fifoByteCount = 0;
    g_hleGxState.ResetVertex();
#else
    (void)primitive;
    (void)vtx_fmt;
    (void)vertex_count;
#endif
}

#endif
