#include "abi_bridge.h"
#include "translated_execution_handoff.hpp"
#include "RuntimeConfig.h"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <sys/stat.h>

extern "C" void func_80006090(CpuContext* ctx);
extern "C" void func_8000609C(CpuContext* ctx);
#if defined(MKW_LOCAL_BOOTSTRAP_PRELUDE) && MKW_LOCAL_BOOTSTRAP_PRELUDE
extern "C" void func_80006210(CpuContext* ctx);
#endif
#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
extern "C" void func_800060A4(CpuContext* ctx);
#endif

namespace {

constexpr std::uint32_t kGetterGuestAddress = 0x8000609Cu;
constexpr std::uint32_t kSetterGuestAddress = 0x80006090u;
constexpr std::uint32_t kBootstrapRegistersGuestAddress = 0x80006210u;
constexpr std::uint32_t kFastTrackStartGuestAddress = 0x800060A4u;
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

#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
void write_fast_track_progress(const char* state,
                               const CpuContext& cpu,
                               const char* detail = nullptr) noexcept {
    constexpr const char* kDiagnosticDirectory = "sdmc:/switch/WiiCompiled-Switch";
    constexpr const char* kPrimaryProgressPath =
        "sdmc:/switch/WiiCompiled-Switch/fast-track-progress.txt";
    constexpr const char* kFallbackProgressPath =
        "sdmc:/switch/fast-track-progress.txt";

    // The NRO does not have to be installed inside /switch/WiiCompiled-Switch.
    // Make the diagnostics directory explicitly instead of silently depending
    // on the user's install layout. If that still fails, leave a fallback file
    // directly under /switch so a non-crashing stall remains observable.
    (void)::mkdir(kDiagnosticDirectory, 0777);

    std::FILE* out = std::fopen(kPrimaryProgressPath, "w");
    if (!out) {
        out = std::fopen(kFallbackProgressPath, "w");
    }
    if (!out) {
        return;
    }

    std::fprintf(out, "WiiCompiled-Switch fast-track startup\n");
    std::fprintf(out, "====================================\n");
    std::fprintf(out, "state                 : %s\n", state ? state : "UNKNOWN");
    std::fprintf(out, "start guest address   : 0x%08x\n", kFastTrackStartGuestAddress);
    std::fprintf(out, "current guest pc      : 0x%08x\n", cpu.pc);
    std::fprintf(out, "r1                    : 0x%08x\n", cpu.gpr[1]);
    std::fprintf(out, "r2                    : 0x%08x\n", cpu.gpr[2]);
    std::fprintf(out, "r3                    : 0x%08x\n", cpu.gpr[3]);
    std::fprintf(out, "r13                   : 0x%08x\n", cpu.gpr[13]);
    if (detail && *detail) {
        std::fprintf(out, "detail                : %s\n", detail);
    }
    std::fprintf(out, "policy                : first-blocker fast track; no per-helper checkpoint\n");
    std::fclose(out);
}

bool run_local_fast_track(MkwSwitchTranslatedExecutionProbeResult* out) noexcept {
    if (!out) {
        return false;
    }

    CpuContext& cpu = GetPersistentCpuContext();
    cpu = {};
    cpu.pc = kFastTrackStartGuestAddress;
    for (auto& gpr : cpu.gpr) {
        gpr = kRegisterSentinel;
    }

    out->guest_address = kFastTrackStartGuestAddress;
    out->r3_before = cpu.gpr[3];
    write_fast_track_progress("ENTERING_TRANSLATED_START", cpu);

    try {
        CpuContextScope scope(&cpu);
        func_800060A4(&cpu);
    } catch (const std::exception& ex) {
        out->guest_address = cpu.pc;
        out->r1 = cpu.gpr[1];
        out->r2 = cpu.gpr[2];
        out->r13 = cpu.gpr[13];
        out->r3_after = cpu.gpr[3];
        write_fast_track_progress("BLOCKED_STD_EXCEPTION", cpu, ex.what());
        return false;
    } catch (...) {
        out->guest_address = cpu.pc;
        out->r1 = cpu.gpr[1];
        out->r2 = cpu.gpr[2];
        out->r13 = cpu.gpr[13];
        out->r3_after = cpu.gpr[3];
        write_fast_track_progress("BLOCKED_UNKNOWN_EXCEPTION", cpu);
        return false;
    }

    out->guest_address = cpu.pc;
    out->r1 = cpu.gpr[1];
    out->r2 = cpu.gpr[2];
    out->r13 = cpu.gpr[13];
    out->r3_after = cpu.gpr[3];
    write_fast_track_progress("RETURNED_FROM_TRANSLATED_START", cpu);
    return TryGetCpuContext() == nullptr;
}
#endif

#if defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK
constexpr MkwSwitchTranslatedExecutionHandoffApi kExecutionApi{
    .abi_version = mkw::translated_execution_handoff::kAbiVersion,
    .run_first_translated_function = run_local_fast_track,
};
#elif defined(MKW_LOCAL_BOOTSTRAP_PRELUDE) && MKW_LOCAL_BOOTSTRAP_PRELUDE
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
