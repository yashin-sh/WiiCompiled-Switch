#pragma once

#include <cstdint>

// Separate first-execution ABI layered after translated-product discovery and
// generated data-section initialization. Merely inspecting this API must never
// execute translated guest code.
extern "C" {

struct MkwSwitchTranslatedExecutionProbeResult {
    std::uint32_t guest_address;
    std::uint32_t r1;
    std::uint32_t r2;
    std::uint32_t r13;
    std::uint32_t r3_before;
    std::uint32_t r3_after;
};

struct MkwSwitchTranslatedExecutionHandoffApi {
    std::uint32_t abi_version;
    bool (*run_first_translated_function)(MkwSwitchTranslatedExecutionProbeResult* result) noexcept;
};

const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept;

} // extern "C"

namespace mkw::translated_execution_handoff {

inline constexpr std::uint32_t kAbiVersion = 1;

struct Status {
    bool linked = false;
    bool abi_compatible = false;
    bool runner_available = false;
    std::uint32_t reported_abi = 0;
};

// Side-effect free: never runs translated code.
Status inspect() noexcept;

// Runs exactly the provider-selected first translated-function probe.
bool run_first_translated_function(MkwSwitchTranslatedExecutionProbeResult& result) noexcept;

} // namespace mkw::translated_execution_handoff
