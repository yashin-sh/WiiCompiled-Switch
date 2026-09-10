#include "translated_product.hpp"
#include "translated_product_handoff.hpp"

extern "C" void InitializeDataSections();
extern "C" bool IsDataSectionsInitialized();

namespace {

bool initialize_generated_data_sections() noexcept {
    InitializeDataSections();
    return IsDataSectionsInitialized();
}

constexpr MkwSwitchTranslatedProductApi kProductApi{
    .abi_version = mkw::translated_product::kAbiVersion,
    .product_id = "local-wiicompiled-product",
    .build_description = "user-owned WiiCompiled generated data initializer",
};

constexpr MkwSwitchTranslatedProductHandoffApi kHandoffApi{
    .abi_version = mkw::translated_product_handoff::kAbiVersion,
    .initialize_data_sections = initialize_generated_data_sections,
};

} // namespace

extern "C" const MkwSwitchTranslatedProductApi*
mkw_switch_get_translated_product_api() noexcept {
    return &kProductApi;
}

extern "C" const MkwSwitchTranslatedProductHandoffApi*
mkw_switch_get_translated_product_handoff_api() noexcept {
    return &kHandoffApi;
}
