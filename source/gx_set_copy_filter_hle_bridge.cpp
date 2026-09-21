#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "gx_internal.h"

#include <dolphin/gx.h>

#include <cstring>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_copy_filter(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t antialias = cpu->gpr[3];
    const std::uint32_t sample_pattern_addr = cpu->gpr[4];
    const std::uint32_t vfilter_enable = cpu->gpr[5];
    const std::uint32_t vfilter_addr = cpu->gpr[6];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_COPY_FILTER");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    std::uint8_t sample_pattern[12][2]{};
    std::uint8_t vfilter[7]{};

    // Exact pinned WiiCompiled data contract: copy 24 sample-pattern bytes and
    // seven vertical-filter bytes from guest RAM only when the guest pointer is
    // non-zero, then forward the two GXBool values and local arrays to Aurora.
    if (sample_pattern_addr != 0u) {
        std::memcpy(sample_pattern, GuestToHostPtr(sample_pattern_addr, 24u), 24u);
    }
    if (vfilter_addr != 0u) {
        std::memcpy(vfilter, GuestToHostPtr(vfilter_addr, 7u), 7u);
    }

    GXSetCopyFilter(
        static_cast<GXBool>(antialias),
        sample_pattern,
        static_cast<GXBool>(vfilter_enable),
        vfilter);
#else
    (void)antialias;
    (void)sample_pattern_addr;
    (void)vfilter_enable;
    (void)vfilter_addr;
#endif
}

#endif
