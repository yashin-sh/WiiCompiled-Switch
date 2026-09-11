#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t kSyntheticGuestAddress = 0x7F000100u;
constexpr std::uint32_t kSyntheticSequenceSetter = 0x7F000120u;
constexpr std::uint32_t kSyntheticSequenceGetter = 0x7F000140u;
constexpr std::uint32_t kSyntheticBootstrapRegisters = 0x7F000160u;
constexpr std::uint32_t kSyntheticFastTrackStart = 0x7F000180u;
constexpr std::uint32_t kSyntheticFastTrackStage2 = 0x7F0001A0u;
constexpr std::uint32_t kSyntheticFastTrackStage3 = 0x7F0001C0u;
constexpr std::uint32_t kSyntheticStack = 0x81700000u;
constexpr std::uint32_t kSyntheticSda2 = 0x81234560u;
constexpr std::uint32_t kSyntheticSda1 = 0x87654320u;
constexpr std::uint32_t kR3Sentinel = 0xA5A5A5A5u;
constexpr std::uint32_t kRegisterSentinel = 0xCDCDCDCDu;
std::uint32_t gSyntheticGuestState = 0;

} // namespace

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
MKW_TRANSLATED_TRAIT(7F0001A0, synthetic_translated_fast_track_stage2, 0x00000000u);
MKW_TRANSLATED_TRAIT(7F0001C0, synthetic_translated_fast_track_stage3, 0x00000000u);
#endif

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

extern "C" __attribute__((noinline, used))
void synthetic_translated_bootstrap_registers(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }

    ctx->gpr[0] = 0u;
    for (std::uint32_t i = 3; i <= 12; ++i) {
        ctx->gpr[i] = 0u;
    }
    for (std::uint32_t i = 14; i <= 31; ++i) {
        ctx->gpr[i] = 0u;
    }
    ctx->gpr[1] = kSyntheticStack;
    ctx->gpr[2] = kSyntheticSda2;
    ctx->gpr[13] = kSyntheticSda1;
}

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" __attribute__((noinline, used))
void synthetic_translated_fast_track_stage3(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }
    ctx->pc = kSyntheticFastTrackStage3;
    ctx->gpr[3] = 0u;
}

extern "C" __attribute__((noinline, used))
void synthetic_translated_fast_track_stage2(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }
    ctx->pc = kSyntheticFastTrackStage2;
    InvokeDirectCpu<0x7F0001C0u>(ctx);
}
#endif

extern "C" __attribute__((noinline, used))
void synthetic_translated_fast_track_start(CpuContext* ctx) {
    if (!ctx || TryGetCpuContext() != ctx) {
        return;
    }

    // Model the real startup boundary first: __init_hardware is a native HLE
    // override in WiiCompiled and must return without falling into the
    // unsupported-dispatch abort path.
#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
    InvokeDirectCpu<0x80006348u>(ctx);
#endif

    // Then traverse translated direct-call edges through the same generic seam
    // used by aggregate shards in the real fast-track build.
    ctx->gpr[1] = kSyntheticStack;
    ctx->gpr[2] = kSyntheticSda2;
    ctx->gpr[13] = kSyntheticSda1;
#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
    InvokeDirectCpu<0x7F0001A0u>(ctx);
#else
    ctx->pc = kSyntheticFastTrackStage3;
    ctx->gpr[3] = 0u;
#endif
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

bool run_synthetic_bootstrap_register_prelude(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kSyntheticBootstrapRegisters;
    for (auto& gpr : cpu.gpr) {
        gpr = kRegisterSentinel;
    }

    out->guest_address = kSyntheticBootstrapRegisters;
    out->r3_before = cpu.gpr[3];

    {
        CpuContextScope scope(&cpu);
        synthetic_translated_bootstrap_registers(&cpu);
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

    return cleared && cpu.gpr[1] == kSyntheticStack &&
           cpu.gpr[2] == kSyntheticSda2 && cpu.gpr[13] == kSyntheticSda1 &&
           TryGetCpuContext() == nullptr;
}

bool run_synthetic_fast_track(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kSyntheticFastTrackStart;
    for (auto& gpr : cpu.gpr) {
        gpr = kRegisterSentinel;
    }

    out->guest_address = kSyntheticFastTrackStart;
    out->r3_before = cpu.gpr[3];

    {
        CpuContextScope scope(&cpu);
        synthetic_translated_fast_track_start(&cpu);
    }

    out->guest_address = cpu.pc;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_after = cpu.gpr[3];

    return cpu.pc == kSyntheticFastTrackStage3 && cpu.gpr[3] == 0u &&
           cpu.gpr[1] == kSyntheticStack && cpu.gpr[2] == kSyntheticSda2 &&
           cpu.gpr[13] == kSyntheticSda1 && TryGetCpuContext() == nullptr;
}

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_synthetic_fast_track,
};
#elif defined(MKW_SYNTHETIC_BOOTSTRAP_PRELUDE) && MKW_SYNTHETIC_BOOTSTRAP_PRELUDE
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_synthetic_bootstrap_register_prelude,
};
#elif defined(MKW_SYNTHETIC_SEQUENCE) && MKW_SYNTHETIC_SEQUENCE
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
