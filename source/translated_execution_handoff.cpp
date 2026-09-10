#include "translated_execution_handoff.hpp"

extern "C" __attribute__((weak))
const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    // Public/default builds expose no translated-function execution provider.
    return nullptr;
}

namespace mkw::translated_execution_handoff {

Status inspect() noexcept {
    Status result{};

    const auto* api = mkw_switch_get_translated_execution_handoff_api();
    if (!api) {
        return result;
    }

    result.linked = true;
    result.reported_abi = api->abi_version;
    result.abi_compatible = api->abi_version == kAbiVersion;
    result.runner_available = result.abi_compatible &&
        api->run_first_translated_function != nullptr;
    return result;
}

bool run_first_translated_function(MkwSwitchTranslatedExecutionProbeResult& result) noexcept {
    result = {};
    const auto* api = mkw_switch_get_translated_execution_handoff_api();
    if (!api || api->abi_version != kAbiVersion ||
        api->run_first_translated_function == nullptr) {
        return false;
    }

    return api->run_first_translated_function(&result);
}

} // namespace mkw::translated_execution_handoff
