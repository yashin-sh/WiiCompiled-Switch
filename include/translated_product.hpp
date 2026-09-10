#pragma once

#include <cstdint>

// Stable, Nintendo-data-free link seam between the common Horizon runtime and
// a locally generated WiiCompiled translated product. The repository/CI build
// provides only the weak default implementation from translated_product.cpp.
// A local product adapter may override this symbol with a strong definition.
extern "C" {

struct MkwSwitchTranslatedProductApi {
    std::uint32_t abi_version;
    const char* product_id;
    const char* build_description;
};

const MkwSwitchTranslatedProductApi* mkw_switch_get_translated_product_api() noexcept;

} // extern "C"

namespace mkw::translated_product {

inline constexpr std::uint32_t kAbiVersion = 1;

struct Status {
    bool linked = false;
    bool abi_compatible = false;
    std::uint32_t reported_abi = 0;
    const char* product_id = "<none>";
    const char* build_description = "Nintendo-data-free stub";
};

// Inspection is deliberately side-effect free: this boundary proves whether a
// translated product is linked, but does not initialize data sections or enter
// translated game code yet.
Status inspect() noexcept;

} // namespace mkw::translated_product
