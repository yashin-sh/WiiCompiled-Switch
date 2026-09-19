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

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixAddress = cpu->gpr[3];
    const std::uint32_t projectionType = cpu->gpr[4];

    if (!Memory::IsInitialized() || matrixAddress == 0u ||
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
    // Pinned WiiCompiled converts the guest's big-endian 4x4 matrix to host
    // floats and forwards it directly to Aurora GX. Keep GXGetProjectionv's
    // cached-vector state as a separate boundary if hardware later reaches it.
    GXSetProjection(matrix, static_cast<GXProjectionType>(projectionType));
#else
    // Headless/synthetic paths keep only the CPU/HLE boundary. They do not pull
    // Aurora into public Nintendo-data-free CI.
    (void)projectionType;
#endif
}

#endif
