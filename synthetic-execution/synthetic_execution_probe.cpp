#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kSyntheticGuestAddress = 0x7F000100u;
constexpr std::uint32_t kSyntheticStack = 0x81700000u;
constexpr std::uint32_t kSyntheticSda2 = 0x81234560u;
constexpr std::uint32_t kSyntheticSda1 = 0x87654320u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;

} // namespace

extern "C" __attribute__((noinline, used))
void synthetic_translated_execution_leaf(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    // Model the same observable contract as the first local translated probe:
    // a CpuContext is selected, ABI environment registers are seeded, the
    // translated-style function returns through r3, and control returns to the
    // Horizon caller.
    ctx->gpr[3] =
        TryGetCpuContext() == ctx &&
        ctx->gpr[1] == kSyntheticStack &&
        ctx->gpr[2] == kSyntheticSda2 &&
        ctx->gpr[13] == kSyntheticSda1
            ? 0u
            : 1u;
}

namespace {

bool run_synthetic_first_function(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kSyntheticGuestAddress;
    cpu.gpr[1] = kSyntheticStack;
    cpu.gpr[2] = kSyntheticSda2;
    cpu.gpr[13] = kSyntheticSda1;
    cpu.gpr[3] = kR3Sentinel;

    out->guest_address = kSyntheticGuestAddress;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_before = cpu.gpr[3];

    {
        CpuContextScope scope(&cpu);
        synthetic_translated_execution_leaf(&cpu);
    }

    out->r3_after = cpu.gpr[3];
    return cpu.gpr[3] == 0u && TryGetCpuContext() == nullptr;
}

constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_synthetic_first_function,
};

} // namespace

extern "C" const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    return &kExecutionApi;
}
