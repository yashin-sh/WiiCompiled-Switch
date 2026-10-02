#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {
[[noreturn]] void AbortInvalidMatrix(
    CpuContext* cpu,
    const char* stage,
    const char* reason) noexcept {
    mkw_switch_set_fast_track_stage(stage);
    mkw_switch_report_unsupported_translated_dispatch(reason, cpu->gpr[4], cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_gx_set_ind_tex_mtx(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixId = cpu->gpr[3];
    const std::uint32_t matrixAddress = cpu->gpr[4];
    const std::uint32_t scaleExponent = cpu->gpr[5];
    if (!Memory::IsInitialized() || matrixAddress == 0u ||
        !Memory::Contains(matrixAddress, 6u * sizeof(std::uint32_t))) {
        AbortInvalidMatrix(
            cpu,
            "RMCP01_GX_SET_IND_TEX_MTX_INVALID_MATRIX",
            "GX_SET_IND_TEX_MTX_INVALID_MATRIX");
    }

    float matrix[6]{};
    try {
        for (std::uint32_t i = 0; i < 6u; ++i) {
            const std::uint32_t bits = Memory::Read32(matrixAddress + i * 4u);
            std::memcpy(&matrix[i], &bits, sizeof(bits));
        }
    } catch (...) {
        AbortInvalidMatrix(
            cpu,
            "RMCP01_GX_SET_IND_TEX_MTX_INVALID_MATRIX",
            "GX_SET_IND_TEX_MTX_INVALID_MATRIX");
    }

    // Aurora casts 1024.f * each coefficient to s32. Preserve every defined
    // conversion; hard-stop before nonfinite/out-of-range conversion, without
    // clamping coefficients or forwarding the guest address as a host pointer.
    for (const float value : matrix) {
        const float scaled = 1024.0f * value;
        if (!std::isfinite(scaled) || static_cast<double>(scaled) < -2147483648.0 ||
            static_cast<double>(scaled) >= 2147483648.0) {
            AbortInvalidMatrix(
                cpu,
                "RMCP01_GX_SET_IND_TEX_MTX_INVALID_COEFFICIENT",
                "GX_SET_IND_TEX_MTX_INVALID_COEFFICIENT");
        }
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_IND_TEX_MTX");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetIndTexMtx(static_cast<GXIndTexMtxID>(matrixId), matrix, static_cast<s8>(scaleExponent));
#else
    (void)matrixId;
    (void)scaleExponent;
#endif
}

#endif
