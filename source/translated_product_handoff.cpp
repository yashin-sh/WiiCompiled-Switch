#include "translated_product_handoff.hpp"

extern "C" __attribute__((weak))
const MkwSwitchTranslatedProductHandoffApi*
mkw_switch_get_translated_product_handoff_api() noexcept {
    // Public/default builds expose no executable translated-product handoff.
    return nullptr;
}

namespace mkw::translated_product_handoff {

Status inspect() noexcept {
    Status result{};

    const auto* api = mkw_switch_get_translated_product_handoff_api();
    if (!api) {
        return result;
    }

    result.linked = true;
    result.reported_abi = api->abi_version;
    result.abi_compatible = api->abi_version == kAbiVersion;
    result.data_initializer_available =
        result.abi_compatible && api->initialize_data_sections != nullptr;
    return result;
}

bool run_data_initializer() noexcept {
    const auto* api = mkw_switch_get_translated_product_handoff_api();
    if (!api || api->abi_version != kAbiVersion ||
        api->initialize_data_sections == nullptr) {
        return false;
    }

    return api->initialize_data_sections();
}

} // namespace mkw::translated_product_handoff
