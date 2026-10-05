#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
void Report(std::uint32_t mask) {
    static bool reported = false;
    static std::uint32_t previousMask = 0;
    if (reported && previousMask == mask) {
        return;
    }
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-pad-reset.txt", "w")) {
        std::fprintf(out, "status=pinned-reset-pass\nmask=0x%08x\nreturn_value=1\ncontroller_reset=none\n", mask);
        std::fclose(out);
        reported = true;
        previousMask = mask;
    }
}
} // namespace

extern "C" void mkw_switch_hle_pad_reset(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }
    const std::uint32_t mask = cpu->gpr[3];
    mkw_switch_set_fast_track_stage("RMCP01_PAD_RESET");
    // Pinned PAD__Reset_HLE returns PADReset(mask) ? 1 : 0. Aurora's
    // PADReset ignores the full-width mask and returns true, without polling,
    // initialization, controller/library mutation, callbacks or guest memory.
    Report(mask);
    cpu->gpr[3] = 1u;
}

#endif
