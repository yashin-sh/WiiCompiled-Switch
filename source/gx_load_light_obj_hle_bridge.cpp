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
    mkw_switch_report_unsupported_translated_dispatch(reason, 0x80170320u, cpu);
    std::abort();
}
} // namespace
extern "C" void mkw_switch_hle_gx_load_light_obj_imm(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_LIGHT_OBJ_IMM");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto address = cpu->gpr[3];
    const auto id = cpu->gpr[4];
    if (!id || id > 0x80u || (id & (id - 1u)))
        Refuse("GX_LOAD_LIGHT_INVALID_ID", cpu);
    if (!Memory::IsInitialized() || !address || address > UINT32_MAX - 63u ||
        !Memory::Contains(address, 64u))
        Refuse("GX_LOAD_LIGHT_INVALID_OBJECT", cpu);
    // Guest padding occupies the first three words; Aurora starts at color.
    // Convert the complete guest input before any native operation.
    const auto color = Memory::Read32(address + 12u);
    float fields[12];
    for (unsigned i = 0; i < 12; ++i)
        fields[i] = std::bit_cast<float>(Memory::Read32(address + 16u + i * 4u));
    GXLightObj host{};
    GXInitLightColor(&host, {static_cast<u8>(color >> 24), static_cast<u8>(color >> 16),
                             static_cast<u8>(color >> 8), static_cast<u8>(color)});
    GXInitLightAttn(&host, fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]);
    GXInitLightPos(&host, fields[6], fields[7], fields[8]);
    // The guest stores XF direction components; GXInitLightDir negates input.
    GXInitLightDir(&host, -fields[9], -fields[10], -fields[11]);
    GXLoadLightObjImm(&host, static_cast<GXLightID>(id));
#else
    Refuse("GX_LOAD_LIGHT_REQUIRES_RENDERER", cpu);
#endif
}
#endif
