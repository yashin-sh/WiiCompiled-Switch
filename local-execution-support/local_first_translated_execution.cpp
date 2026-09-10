#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"
#include "RuntimeConfig.h"

#include <cstdint>

extern "C" void func_8000609C(CpuContext* ctx);

namespace {

constexpr std::uint32_t kGuestAddress = 0x8000609Cu;
constexpr std::uint32_t kGuestStack = 0x81700000u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;

bool run_local_first_function(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    // This is intentionally the smallest possible translated-execution
    // boundary. Do not register/dispatch functions and do not run constructors.
    // The persistent context exists only to match the pinned WiiCompiled ABI
    // contract used by later execution stages.
    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kGuestAddress;
    cpu.gpr[1] = kGuestStack;
    cpu.gpr[2] = RuntimeConfig::SDA2_BASE;
    cpu.gpr[13] = RuntimeConfig::SDA1_BASE;
    cpu.gpr[3] = kR3Sentinel;

    out->guest_address = kGuestAddress;
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

    // PAL RMCP01 0x8000609C is __get_debug_bba. At this checkpoint no startup
    // code has called __set_debug_bba, and GuestFlat backing is zero-created, so
    // its BSS byte must still be zero. A zero r3 therefore proves the translated
    // load executed and returned through the PPC ABI context.
    return cpu.gpr[3] == 0u && TryGetCpuContext() == nullptr;
}

constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_local_first_function,
};

} // namespace

extern "C" const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    return &kExecutionApi;
}
