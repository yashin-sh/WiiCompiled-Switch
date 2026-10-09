#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>

extern "C" {
// The rendered slice excludes the pinned gx_utils.cpp provider.
float g_projectionVector[7] = {1.f, 1.f, 0.f, 1.f, 0.f, -1.f, 0.f};
}
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

namespace {

float ReadGuestFloat32(std::uint32_t address) {
    const std::uint32_t bits = Memory::Read32(address);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

[[noreturn]] void AbortInvalidProjectionMatrix(CpuContext* cpu, std::uint32_t address) noexcept {
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_PROJECTION_INVALID");
    mkw_switch_report_unsupported_translated_dispatch(
        "GX_SET_PROJECTION_INVALID_MATRIX",
        address,
        cpu);
    std::abort();
}

} // namespace

extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixAddress = cpu->gpr[3];
    const std::uint32_t projectionType = cpu->gpr[4];

    if (!Memory::IsInitialized() || matrixAddress == 0u || matrixAddress > UINT32_MAX - 63u ||
        !Memory::Contains(matrixAddress, 16u * sizeof(std::uint32_t))) {
        AbortInvalidProjectionMatrix(cpu, matrixAddress);
    }

    float matrix[16]{};
    try {
        for (std::uint32_t i = 0; i < 16u; ++i) {
            matrix[i] = ReadGuestFloat32(matrixAddress + i * 4u);
        }
    } catch (...) {
        AbortInvalidProjectionMatrix(cpu, matrixAddress);
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_PROJECTION");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetProjection(matrix, static_cast<GXProjectionType>(projectionType));
    // Pinned UpdateProjectionVectorFromMatrix, after the native setter.
    const bool perspective = projectionType == GX_PERSPECTIVE;
    g_projectionVector[0] = perspective ? 0.f : 1.f;
    g_projectionVector[1] = matrix[0];
    g_projectionVector[2] = matrix[perspective ? 2 : 3];
    g_projectionVector[3] = matrix[5];
    g_projectionVector[4] = matrix[perspective ? 6 : 7];
    g_projectionVector[5] = matrix[10];
    g_projectionVector[6] = matrix[11];
#else
    // Headless/synthetic paths keep only the CPU/HLE boundary. They do not pull
    // Aurora into public Nintendo-data-free CI.
    (void)projectionType;
#endif
}

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
namespace {
void ValidateVector(CpuContext* cpu, const char* reason, std::uint32_t target) {
    const std::uint32_t address = cpu->gpr[3];
    if (!Memory::IsInitialized() || !address || address > UINT32_MAX - 27u ||
        !Memory::Contains(address, 28u)) {
        mkw_switch_report_unsupported_translated_dispatch(reason, target, cpu);
        std::abort();
    }
}
} // namespace
#endif

extern "C" void mkw_switch_hle_gx_get_projectionv(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_GET_PROJECTIONV");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    ValidateVector(cpu, "GX_GET_PROJECTIONV_INVALID_OUTPUT", 0x801730CCu);
    for (std::uint32_t i = 0; i < 7; ++i) {
        std::uint32_t bits;
        std::memcpy(&bits, &g_projectionVector[i], sizeof(bits));
        Memory::Write32(cpu->gpr[3] + i * 4u, bits);
    }
#else
    mkw_switch_report_unsupported_translated_dispatch("GX_GET_PROJECTIONV_REQUIRES_RENDERER", 0x801730CCu, cpu);
    std::abort();
#endif
}

extern "C" void mkw_switch_hle_gx_set_projectionv(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_PROJECTIONV");
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    ValidateVector(cpu, "GX_SET_PROJECTIONV_INVALID_INPUT", 0x80173080u);
    float v[7];
    for (std::uint32_t i = 0; i < 7; ++i)
        v[i] = ReadGuestFloat32(cpu->gpr[3] + i * 4u);
    const auto type = v[0] != 0.f ? GX_ORTHOGRAPHIC : GX_PERSPECTIVE;
    float matrix[16]{};
    matrix[0] = v[1];
    matrix[5] = v[3];
    matrix[10] = v[5];
    matrix[11] = v[6];
    if (type == GX_PERSPECTIVE) {
        matrix[2] = v[2];
        matrix[6] = v[4];
        matrix[14] = -1.f;
    } else {
        matrix[3] = v[2];
        matrix[7] = v[4];
        matrix[15] = 1.f;
    }
    GXSetProjection(matrix, type);
    std::memcpy(g_projectionVector, v, sizeof(v));
#else
    mkw_switch_report_unsupported_translated_dispatch("GX_SET_PROJECTIONV_REQUIRES_RENDERER", 0x80173080u, cpu);
    std::abort();
#endif
}

#endif
