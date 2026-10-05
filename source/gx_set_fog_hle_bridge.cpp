#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_fog(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t type = cpu->gpr[3];
    const std::uint32_t colorAddress = cpu->gpr[4];
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_FOG");

    // Bound this bridge to the two captured GX_FOG_NONE tuples. Check
    // f64 representations before any narrowing, guest lookup or native work:
    // Aurora still normalizes coefficients for NONE, including a float->u32
    // conversion that is unsafe for some non-finite or negative parameters.
    constexpr std::array<std::array<std::uint64_t, 4>, 2> capturedBits{{
        {0x0000000000000000ull, 0x3ff0000000000000ull,
         0x3fb99999a0000000ull, 0x3ff0000000000000ull},
        {0x3ff0000000000000ull, 0x3ff0000000000000ull,
         0x0000000000000000ull, 0x0000000000000000ull},
    }};
    std::array<std::uint64_t, 4> argumentBits{};
    for (std::size_t i = 0; i < argumentBits.size(); ++i) {
        std::uint64_t bits;
        static_assert(sizeof(bits) == sizeof(cpu->fpr[1].d));
        std::memcpy(&bits, &cpu->fpr[1u + i].d, sizeof(bits));
        argumentBits[i] = bits;
    }
    const bool proven = type == 0u &&
                        (argumentBits == capturedBits[0] || argumentBits == capturedBits[1]);
    if (!proven) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_FOG_UNPROVEN_ARGS", 0x801722CCu, cpu);
        std::abort();
    }

    const auto* bytes = Memory::GetPointer(colorAddress, 4u);
    if (!bytes) {
        mkw_switch_report_unsupported_translated_dispatch(
            "GX_SET_FOG_UNREADABLE_COLOR", 0x801722CCu, cpu);
        std::abort();
    }

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const GXColor color{bytes[0], bytes[1], bytes[2], bytes[3]};
    GXSetFog(static_cast<GXFogType>(type),
             static_cast<float>(cpu->fpr[1].d),
             static_cast<float>(cpu->fpr[2].d),
             static_cast<float>(cpu->fpr[3].d),
             static_cast<float>(cpu->fpr[4].d), color);
#endif
}

#endif
