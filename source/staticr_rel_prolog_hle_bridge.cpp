#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>
#include <cstdlib>

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

#if defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION
extern "C" void func_8055531C(CpuContext* ctx);
#endif

namespace {

constexpr std::uint32_t kStaticRRelPrologAddress = 0x8055531Cu;
constexpr std::uint32_t kObservedStaticRBase = 0x805102E0u;

[[noreturn]] void AbortUnprovenModule(CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(
        "STATICR_REL_PROLOG_UNPROVEN_MODULE",
        kStaticRRelPrologAddress,
        cpu);
    std::abort();
}

} // namespace

extern "C" void mkw_switch_hle_staticr_rel_prolog(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    mkw_switch_set_fast_track_stage("RMCP01_STATICR_REL_PROLOG");

    // The first hardware-observed RelProlog call is for the exact StaticR.rel
    // image pinned by recomp.local.yml. Keep the native seam scoped to that
    // module instance; a different module/base remains a fresh frontier.
    if (cpu->gpr[3] != kObservedStaticRBase) {
        AbortUnprovenModule(cpu);
    }

#if defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION
    // Pinned WiiCompiled wraps the original translated RelProlog with host
    // RecompMod initializer phases. This Switch base-RMCP01 product links no
    // mod data-patch registrants, so those host-only phases are empty here.
    // Execute the original translated RelProlog exactly rather than replacing
    // it with a no-op or fabricating REL state.
    func_8055531C(cpu);
#else
    // Public Nintendo-data-free CI proves only that this native seam is mapped
    // and linkable. It does not fabricate a private StaticR.rel body.
    mkw_switch_report_unsupported_translated_dispatch(
        "STATICR_REL_PROLOG_PRIVATE_BODY_UNAVAILABLE",
        kStaticRRelPrologAddress,
        cpu);
    std::abort();
#endif
}

#endif
