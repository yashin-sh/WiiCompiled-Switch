#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
#include "abi_bridge.h"
#include "memory.h"
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif
#include <algorithm>
#include <cstdint>
#include <cstdlib>
extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;
namespace {
[[noreturn]] void Refuse(CpuContext* cpu, const char* reason) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x801717ACu, cpu);
    std::abort();
}
std::uint32_t ReadBE32(const std::uint8_t* bytes) noexcept {
    return (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) |
           (std::uint32_t(bytes[2]) << 8) | std::uint32_t(bytes[3]);
}
} // namespace
extern "C" void mkw_switch_hle_gx_set_tev_indirect(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_TEV_INDIRECT");
    const std::uint32_t format = cpu->gpr[5], bias = cpu->gpr[6], matrix = cpu->gpr[7];
    const std::uint32_t wrapS = cpu->gpr[8], wrapT = cpu->gpr[9];
    // Pinned SET_REG_FIELD does not mask an oversized enum before shifting it.
    // Reject SDK sentinels, holes and wide values before any native BP output.
    const bool validMatrix = matrix <= 3u || (matrix >= 5u && matrix <= 7u) ||
                             (matrix >= 9u && matrix <= 11u);
    if (format >= 4u || bias >= 8u || !validMatrix || wrapS >= 7u || wrapT >= 7u)
        Refuse(cpu, "GX_SET_TEV_INDIRECT_UNPROVEN_ARGS");
    // The ninth/tenth u32 arguments occupy one complete BE spill. Validate the
    // 64-bit end before narrowing an address; never accept a wrapped stack.
    const std::uint64_t spill = std::uint64_t(cpu->gpr[1]) + 8u;
    if (spill + 8u > (std::uint64_t{1} << 32))
        Refuse(cpu, "GX_SET_TEV_INDIRECT_STACK_OVERFLOW");
    const auto address = static_cast<std::uint32_t>(spill);
    if (!Memory::IsInitialized() || !Memory::Contains(address, 8u))
        Refuse(cpu, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    const auto* bytes = Memory::GetPointer(address, 8u);
    if (!bytes)
        Refuse(cpu, "GX_SET_TEV_INDIRECT_STACK_UNREADABLE");
    const std::uint32_t originalLOD = ReadBE32(bytes), alpha = ReadBE32(bytes + 4u);
    if (alpha >= 4u)
        Refuse(cpu, "GX_SET_TEV_INDIRECT_UNPROVEN_ARGS");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Preserve the exact pinned wrapper clamps and Aurora's full-u32 bool
    // conversion (0x100 is true). No guest GXData mirror is modified here.
    GXSetTevIndirect(static_cast<GXTevStageID>(std::min(cpu->gpr[3], 15u)),
                     static_cast<GXIndTexStageID>(std::min(cpu->gpr[4], 3u)),
                     static_cast<GXIndTexFormat>(format), static_cast<GXIndTexBiasSel>(bias),
                     static_cast<GXIndTexMtxID>(matrix), static_cast<GXIndTexWrap>(wrapS),
                     static_cast<GXIndTexWrap>(wrapT), static_cast<GXBool>(cpu->gpr[10] != 0u),
                     static_cast<GXBool>(originalLOD != 0u), static_cast<GXIndTexAlphaSel>(alpha));
#else
    (void)originalLOD;
#endif
}
#endif
