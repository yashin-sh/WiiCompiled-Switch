#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"
#include "RuntimeConfig.h"

#include <cstdint>

extern "C" void func_80006090(CpuContext* ctx);
extern "C" void func_8000609C(CpuContext* ctx);
#if defined(MKW_LOCAL_BOOTSTRAP_PRELUDE) && MKW_LOCAL_BOOTSTRAP_PRELUDE
extern "C" void func_80006210(CpuContext* ctx);
#endif

namespace {

constexpr std::uint32_t kGetterGuestAddress = 0x8000609Cu;
constexpr std::uint32_t kSetterGuestAddress = 0x80006090u;
constexpr std::uint32_t kBootstrapRegistersGuestAddress = 0x80006210u;
constexpr std::uint32_t kGuestStack = 0x81700000u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;
constexpr std::uint32_t kRegisterSentinel = 0xCDCDCDCDu;

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

#if defined(MKW_LOCAL_BOOTSTRAP_PRELUDE) && MKW_LOCAL_BOOTSTRAP_PRELUDE
bool run_local_bootstrap_register_prelude(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    // This is the first operation performed by PAL RMCP01 __start. Seed every
    // GPR with a visible non-zero value so the translated routine must actually
    // establish the PPC startup register contract. Do not execute
    // __init_hardware or any later startup code at this checkpoint.
    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kBootstrapRegistersGuestAddress;
    for (auto& gpr : cpu.gpr) {
        gpr = kRegisterSentinel;
    }

    out->guest_address = kBootstrapRegistersGuestAddress;
    out->r3_before = cpu.gpr[3];

    try {
        CpuContextScope scope(&cpu);
        func_80006210(&cpu);
    } catch (...) {
        out->r1 = cpu.gpr[1];
        out->r2 = cpu.gpr[2];
        out->r13 = cpu.gpr[13];
        out->r3_after = cpu.gpr[3];
        return false;
    }

    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_after = cpu.gpr[3];

    bool cleared = cpu.gpr[0] == 0u;
    for (std::uint32_t i = 3; i <= 12; ++i) {
        cleared = cleared && cpu.gpr[i] == 0u;
    }
    for (std::uint32_t i = 14; i <= 31; ++i) {
        cleared = cleared && cpu.gpr[i] == 0u;
    }

    // SDA bases are generated from the user's local PAL RMCP01 translation and
    // are therefore safe exact postconditions. The original stack symbol is not
    // part of RuntimeConfig, so require the translated routine to replace the
    // sentinel with an aligned MEM1 address and expose the exact value in the
    // runtime report for hardware validation.
    const bool stack_looks_valid =
        cpu.gpr[1] >= 0x80000000u && cpu.gpr[1] < 0x81800000u &&
        (cpu.gpr[1] & 0x7u) == 0u && cpu.gpr[1] != kRegisterSentinel;
    const bool sda_matches =
        cpu.gpr[2] == RuntimeConfig::SDA2_BASE &&
        cpu.gpr[13] == RuntimeConfig::SDA1_BASE;

    return cleared && stack_looks_valid && sda_matches &&
           TryGetCpuContext() == nullptr;
}
#endif

#if defined(MKW_LOCAL_BOOTSTRAP_PRELUDE) && MKW_LOCAL_BOOTSTRAP_PRELUDE
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_local_bootstrap_register_prelude,
};
#elif defined(MKW_LOCAL_FUNCTION_SEQUENCE) && MKW_LOCAL_FUNCTION_SEQUENCE
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
