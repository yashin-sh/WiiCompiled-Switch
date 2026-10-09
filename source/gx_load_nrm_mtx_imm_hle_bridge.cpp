#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include "memory.h"
#include <bit>
#include <cstdlib>
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x80173188u, cpu);
    std::abort();
}
} // namespace
extern "C" void mkw_switch_hle_gx_load_nrm_mtx_imm(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_NRM_MTX_IMM");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto address = cpu->gpr[3];
    const auto id = cpu->gpr[4];
    // Preserve Aurora's inclusive ID range, including noncanonical row IDs.
    if (id > static_cast<std::uint32_t>(GX_PNMTX9))
        Refuse("GX_LOAD_NRM_INVALID_ID", cpu);
    if (!Memory::IsInitialized() || !address || address > UINT32_MAX - 47u ||
        !Memory::Contains(address, 48u))
        Refuse("GX_LOAD_NRM_INVALID_MATRIX", cpu);
    // Pinned HLE reads a full guest 3x4 matrix. Native GX emits its upper-left
    // 3x3, excluding the translation column, at XF normal-matrix address 0x400.
    float matrix[12];
    for (unsigned i = 0; i < 12; ++i)
        matrix[i] = std::bit_cast<float>(Memory::Read32(address + i * 4u));
    GXLoadNrmMtxImm(reinterpret_cast<float (*)[4]>(matrix), id);
#else
    Refuse("GX_LOAD_NRM_REQUIRES_RENDERER", cpu);
#endif
}
#endif
