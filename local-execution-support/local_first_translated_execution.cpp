#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"
#include "RuntimeConfig.h"

#include <cstdint>

extern "C" void func_80006090(CpuContext* ctx);
extern "C" void func_8000609C(CpuContext* ctx);

namespace {

constexpr std::uint32_t kGetterGuestAddress = 0x8000609Cu;
constexpr std::uint32_t kSetterGuestAddress = 0x80006090u;
constexpr std::uint32_t kGuestStack = 0x81700000u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;

bool run_local_first_function(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kGetterGuestAddress;
    cpu.gpr[1] = kGuestStack;
    cpu.gpr[2] = RuntimeConfig::SDA2_BASE;
    cpu.gpr[13] = RuntimeConfig::SDA1_BASE;
    cpu.gpr[3] = kR3Sentinel;

    out->guest_address = kGetterGuestAddress;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_before = cpu.gpr[3];

    try {
        CpuContextScope scope(&cpu);
        func_8000609C(&cpu);
    } catch (...) {
        out->r3_after = cpu.gpr[3];
        return false;
    }

    out->r3_after = cpu.gpr[3];
    return cpu.gpr[3] == 0u && TryGetCpuContext() == nullptr;
}

bool run_local_stateful_sequence(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    // PAL RMCP01 sequence:
    //   0x80006090 / func_80006090 / __set_debug_bba
    //   0x8000609C / func_8000609C / __get_debug_bba
    // Generated data initialization leaves __debug_bba at zero. Execute the
    // setter and getter consecutively against the same guest memory and
    // persistent CpuContext. A final r3 of one proves state flowed through real
    // translated guest memory rather than a host-side synthetic value.
    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kSetterGuestAddress;
    cpu.gpr[1] = kGuestStack;
    cpu.gpr[2] = RuntimeConfig::SDA2_BASE;
    cpu.gpr[13] = RuntimeConfig::SDA1_BASE;
    cpu.gpr[3] = kR3Sentinel;

    out->guest_address = kSetterGuestAddress;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_before = cpu.gpr[3];

    try {
        CpuContextScope scope(&cpu);
        func_80006090(&cpu);
        cpu.pc = kGetterGuestAddress;
        func_8000609C(&cpu);
    } catch (...) {
        out->r3_after = cpu.gpr[3];
        return false;
    }

    out->r3_after = cpu.gpr[3];
    return cpu.gpr[3] == 1u && TryGetCpuContext() == nullptr;
}

#if defined(MKW_LOCAL_FUNCTION_SEQUENCE) && MKW_LOCAL_FUNCTION_SEQUENCE
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_local_stateful_sequence,
};
#else
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_local_first_function,
};
#endif

} // namespace

extern "C" const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    return &kExecutionApi;
}
