#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr std::uint32_t kStrapCheckInputAddress = 0x800077C8u;
constexpr std::uint32_t kObservedScenePtr = 0x90112A34u;

[[noreturn]] void AbortUnproven(CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(
        "STRAP_CHECK_INPUT_UNPROVEN_SCENE",
        kStrapCheckInputAddress,
        cpu);
    std::abort();
}

} // namespace

extern "C" void mkw_switch_hle_strap_check_input(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t scenePtr = cpu->gpr[3];
    if (scenePtr != kObservedScenePtr) {
        AbortUnproven(cpu);
    }

    // Pinned WiiCompiled's StrapScene__CheckInput_Skip ignores scenePtr,
    // notifies only its host settings overlay, and returns 1 to the guest.
    // The overlay side effect is desktop-only; the guest-visible contract is r3=1.
    cpu->gpr[3] = 1u;
}

#endif
