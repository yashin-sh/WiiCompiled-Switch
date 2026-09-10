#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kSyntheticGuestAddress = 0x7F000100u;
constexpr std::uint32_t kSyntheticSequenceSetter = 0x7F000120u;
constexpr std::uint32_t kSyntheticSequenceGetter = 0x7F000140u;
constexpr std::uint32_t kSyntheticStack = 0x81700000u;
constexpr std::uint32_t kSyntheticSda2 = 0x81234560u;
constexpr std::uint32_t kSyntheticSda1 = 0x87654320u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;
std::uint32_t gSyntheticGuestState = 0;

} // namespace

extern "C" __attribute__((noinline, used))
void synthetic_translated_execution_leaf(CpuContext* ctx) {
    if (!ctx) {
        return;
    }

    ctx->gpr[3] =
        TryGetCpuContext() == ctx &&
        ctx->gpr[1] == kSyntheticStack &&
        ctx->gpr[2] == kSyntheticSda2 &&
        ctx->gpr[13] == kSyntheticSda1
            ? 0u
            : 1u;
}

extern "C" __attribute__((noinline, used))
void synthetic_translated_sequence_set(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }
    gSyntheticGuestState = 1u;
}

extern "C" __attribute__((noinline, used))
void synthetic_translated_sequence_get(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }
    ctx->gpr[3] = gSyntheticGuestState;
}

namespace {

void seed_context(CpuContext& cpu, std::uint32_t guest_address) {
    cpu = {};
    cpu.pc = guest_address;
    cpu.gpr[1] = kSyntheticStack;
    cpu.gpr[2] = kSyntheticSda2;
    cpu.gpr[13] = kSyntheticSda1;
    cpu.gpr[3] = kR3Sentinel;
}

bool run_synthetic_first_function(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    seed_context(cpu, kSyntheticGuestAddress);

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

bool run_synthetic_stateful_sequence(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    gSyntheticGuestState = 0u;
    CpuContext& cpu = GetPersistentCpuContext();
    seed_context(cpu, kSyntheticSequenceSetter);

    out->guest_address = kSyntheticSequenceSetter;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_before = cpu.gpr[3];

    {
        CpuContextScope scope(&cpu);
        synthetic_translated_sequence_set(&cpu);
        cpu.pc = kSyntheticSequenceGetter;
        synthetic_translated_sequence_get(&cpu);
    }

    out->r3_after = cpu.gpr[3];
    return gSyntheticGuestState == 1u && cpu.gpr[3] == 1u &&
           TryGetCpuContext() == nullptr;
}

#if defined(MKW_SYNTHETIC_SEQUENCE) && MKW_SYNTHETIC_SEQUENCE
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_synthetic_stateful_sequence,
};
#else
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_synthetic_first_function,
};
#endif

} // namespace

extern "C" const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    return &kExecutionApi;
}
