#pragma once

#include <cstdint>

// Independent execution handoff ABI layered on top of the already hardware-
// validated translated-product metadata ABI. Keeping this separate lets the
// runtime discover a product without automatically executing any generated
// guest-derived initialization code.
extern "C" {

struct MkwSwitchTranslatedProductHandoffApi {
    std::uint32_t abi_version;
    bool (*initialize_data_sections)() noexcept;
};

const MkwSwitchTranslatedProductHandoffApi*
mkw_switch_get_translated_product_handoff_api() noexcept;

} // extern "C"

namespace mkw::translated_product_handoff {

inline constexpr std::uint32_t kAbiVersion = 1;

struct Status {
    bool linked = false;
    bool abi_compatible = false;
    bool data_initializer_available = false;
    std::uint32_t reported_abi = 0;
};

// Inspection never executes product code.
Status inspect() noexcept;

// Executes only the explicit data-section callback exposed by the strong local
// or synthetic handoff provider. Returns false for missing/incompatible APIs or
// if the provider cannot verify initialization.
bool run_data_initializer() noexcept;

} // namespace mkw::translated_product_handoff
