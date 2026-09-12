#include "translated_product_handoff.hpp"

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" __attribute__((weak))
const MkwSwitchTranslatedProductHandoffApi*
mkw_switch_get_translated_product_handoff_api() noexcept {
    // Public/default builds expose no executable translated-product handoff.
    return nullptr;
}

namespace mkw::translated_product_handoff {

Status inspect() noexcept {
    mkw_switch_set_fast_track_stage("DATA_HANDOFF_INSPECT");
    Status result{};

    const auto* api = mkw_switch_get_translated_product_handoff_api();
    if (!api) {
        mkw_switch_set_fast_track_stage("DATA_HANDOFF_NOT_LINKED");
        return result;
    }

    result.linked = true;
    result.reported_abi = api->abi_version;
    result.abi_compatible = api->abi_version == kAbiVersion;
    result.data_initializer_available =
        result.abi_compatible && api->initialize_data_sections != nullptr;
    mkw_switch_set_fast_track_stage("DATA_HANDOFF_INSPECTED");
    return result;
}

bool run_data_initializer() noexcept {
    const auto* api = mkw_switch_get_translated_product_handoff_api();
    if (!api || api->abi_version != kAbiVersion ||
        api->initialize_data_sections == nullptr) {
        mkw_switch_set_fast_track_stage("DATA_INIT_UNAVAILABLE");
        return false;
    }

    mkw_switch_set_fast_track_stage("DATA_INIT_ENTER");
    const bool result = api->initialize_data_sections();
    mkw_switch_set_fast_track_stage("DATA_INIT_RETURNED");
    return result;
}

} // namespace mkw::translated_product_handoff
