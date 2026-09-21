#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_egg_async_display_end_render(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    mkw_switch_set_fast_track_stage("RMCP01_EGG_ASYNC_DISPLAY_END_RENDER");

#if defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION
    // Public Nintendo-data-free probes do not publish the private RMCP01
    // translated dispatch table. The synthetic gate therefore proves only
    // that the indirect native seam resolves this hardware-observed override.
    return;
#else
    // Exact pinned WiiCompiled semantics for PAL 0x8020FF9C:
    // preserve the incoming display pointer in r3, make the guest LR visible
    // as endRender, then execute the two original guest calls through the
    // normal translated indirect dispatcher. Do not bypass copyEFBtoXFB and
    // do not manufacture a GXCopyDisp/present here.
    const std::uint32_t display = cpu->gpr[3];
    cpu->gpr[3] = display;
    cpu->lr = 0x8020FF9Cu;

    CpuContextScope scope(cpu);
    InvokeIndirectCpu(0x80219FB4u, cpu); // EGG::Display::copyEFBtoXFB
    InvokeIndirectCpu(0x8016ED50u, cpu); // GXSetDrawDoneCallback
#endif
}

#endif
