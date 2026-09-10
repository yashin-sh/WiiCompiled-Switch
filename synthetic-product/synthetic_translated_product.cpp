#include "translated_product.hpp"

namespace {

constexpr MkwSwitchTranslatedProductApi kSyntheticProductApi{
    .abi_version = mkw::translated_product::kAbiVersion,
    .product_id = "synthetic-ci-product",
    .build_description = "Nintendo-data-free strong-link probe",
};

} // namespace

// Intentionally strong: when this source is enabled for the synthetic build,
// it must override the weak Nintendo-data-free default from translated_product.cpp.
extern "C" const MkwSwitchTranslatedProductApi*
mkw_switch_get_translated_product_api() noexcept {
    return &kSyntheticProductApi;
}
