#include "translated_product.hpp"

extern "C" __attribute__((weak))
const MkwSwitchTranslatedProductApi* mkw_switch_get_translated_product_api() noexcept {
    // Public CI/repository builds never carry game-derived translated output.
    // A local build may override this weak symbol with a strong adapter that
    // describes the WiiCompiled product linked into the same NRO.
    return nullptr;
}

namespace mkw::translated_product {
namespace {

const char* safe_string(const char* value, const char* fallback) noexcept {
    return value && value[0] != '\0' ? value : fallback;
}

} // namespace

Status inspect() noexcept {
    Status result{};

    const MkwSwitchTranslatedProductApi* api = mkw_switch_get_translated_product_api();
    if (!api) {
        return result;
    }

    result.linked = true;
    result.reported_abi = api->abi_version;
    result.abi_compatible = api->abi_version == kAbiVersion;
    result.product_id = safe_string(api->product_id, "<unspecified>");
    result.build_description = safe_string(api->build_description, "<unspecified>");
    return result;
}

} // namespace mkw::translated_product
