#include "translated_execution_handoff.hpp"

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" __attribute__((weak))
const MkwSwitchTranslatedExecutionHandoffApi*
mkw_switch_get_translated_execution_handoff_api() noexcept {
    // Public/default builds expose no translated-function execution provider.
    return nullptr;
}

namespace mkw::translated_execution_handoff {

Status inspect() noexcept {
    mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_HANDOFF_INSPECT");
    Status result{};

    const auto* api = mkw_switch_get_translated_execution_handoff_api();
    if (!api) {
        mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_HANDOFF_NOT_LINKED");
        return result;
    }

    result.linked = true;
    result.reported_abi = api->abi_version;
    result.abi_compatible = api->abi_version == kAbiVersion;
    result.runner_available = result.abi_compatible &&
        api->run_first_translated_function != nullptr;
    mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_HANDOFF_INSPECTED");
    return result;
}

bool run_first_translated_function(MkwSwitchTranslatedExecutionProbeResult& result) noexcept {
    result = {};
    const auto* api = mkw_switch_get_translated_execution_handoff_api();
    if (!api || api->abi_version != kAbiVersion ||
        api->run_first_translated_function == nullptr) {
        mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_UNAVAILABLE");
        return false;
    }

    mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_ENTER");
    const bool passed = api->run_first_translated_function(&result);
    mkw_switch_set_fast_track_stage("TRANSLATED_EXEC_RETURNED");
    return passed;
}

} // namespace mkw::translated_execution_handoff
