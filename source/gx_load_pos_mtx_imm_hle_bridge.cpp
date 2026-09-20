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

float ReadGuestFloat32(std::uint32_t address) {
    const std::uint32_t bits = Memory::Read32(address);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

[[noreturn]] void AbortInvalidPositionMatrix(
    CpuContext* cpu,
    std::uint32_t address) noexcept {
    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_POS_MTX_IMM_INVALID");
    mkw_switch_report_unsupported_translated_dispatch(
        "GX_LOAD_POS_MTX_IMM_INVALID_MATRIX",
        address,
        cpu);
    std::abort();
}

} // namespace

extern "C" void mkw_switch_hle_gx_load_pos_mtx_imm(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t matrixAddress = cpu->gpr[3];
    const std::uint32_t matrixId = cpu->gpr[4];

    if (!Memory::IsInitialized() || matrixAddress == 0u ||
        !Memory::Contains(matrixAddress, 12u * sizeof(std::uint32_t))) {
        AbortInvalidPositionMatrix(cpu, matrixAddress);
    }

    float matrix[12]{};
    try {
        for (std::uint32_t i = 0; i < 12u; ++i) {
            matrix[i] = ReadGuestFloat32(matrixAddress + i * 4u);
        }
    } catch (...) {
        AbortInvalidPositionMatrix(cpu, matrixAddress);
    }

    mkw_switch_set_fast_track_stage("RMCP01_GX_LOAD_POS_MTX_IMM");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    // Pinned WiiCompiled converts the guest's big-endian 3x4 position matrix
    // and forwards it unchanged to Aurora GX with the PPC matrix id in r4.
    GXLoadPosMtxImm(reinterpret_cast<float(*)[4]>(matrix), matrixId);
#else
    (void)matrixId;
#endif
}

#endif
