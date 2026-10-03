#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {
[[noreturn]] void AbortInvalidMatrix(CpuContext* cpu) noexcept {
    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_TEX_MTX_IMM_INVALID_MATRIX");
    mkw_switch_report_unsupported_translated_dispatch(
        "GX_LOAD_TEX_MTX_IMM_INVALID_MATRIX", cpu->gpr[3], cpu);
    std::abort();
}
} // namespace

extern "C" void mkw_switch_hle_gx_load_tex_mtx_imm(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixAddress = cpu->gpr[3];
    const std::uint32_t matrixId = cpu->gpr[4];
    const std::uint32_t matrixType = cpu->gpr[5];
    // Match the pinned wrapper: type 0 consumes a 3x4 matrix; every other
    // type consumes eight coefficients and leaves the last four as zero.
    const std::uint32_t count = matrixType == 0u ? 12u : 8u;
    if (!Memory::IsInitialized() || matrixAddress == 0u ||
        !Memory::Contains(matrixAddress, count * sizeof(std::uint32_t))) {
        AbortInvalidMatrix(cpu);
    }

    float matrix[12]{};
    try {
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t bits = Memory::Read32(matrixAddress + i * 4u);
            std::memcpy(&matrix[i], &bits, sizeof(bits));
        }
    } catch (...) {
        AbortInvalidMatrix(cpu);
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_TEX_MTX_IMM");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Aurora validates the id/type and emits the real XF FIFO commands.
    // The pinned texture-matrix wrapper does not begin or present a frame.
    GXLoadTexMtxImm(matrix, matrixId, static_cast<GXTexMtxType>(matrixType));
#else
    (void)matrixId;
#endif
}

#endif
