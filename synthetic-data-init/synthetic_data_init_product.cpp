#include "memory.h"
#include "translated_product.hpp"
#include "translated_product_handoff.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

constexpr std::uint32_t kSyntheticGuestAddress = 0x80004000u;
constexpr std::uint8_t kSyntheticPayload[] = {
    0x53, 0x59, 0x4e, 0x54, 0x48, 0x2d, 0x44, 0x41,
    0x54, 0x41, 0x2d, 0x49, 0x4e, 0x49, 0x54, 0x21,
};

bool initialize_synthetic_data_sections() noexcept {
    if (!Memory::Contains(kSyntheticGuestAddress, sizeof(kSyntheticPayload))) {
        return false;
    }

    auto* destination = Memory::GetPointer(kSyntheticGuestAddress,
                                           sizeof(kSyntheticPayload));
    if (!destination) {
        return false;
    }

    std::memcpy(destination, kSyntheticPayload, sizeof(kSyntheticPayload));
    return std::memcmp(destination, kSyntheticPayload,
                       sizeof(kSyntheticPayload)) == 0;
}

constexpr MkwSwitchTranslatedProductApi kProductApi{
    .abi_version = mkw::translated_product::kAbiVersion,
    .product_id = "synthetic-data-init-product",
    .build_description = "Nintendo-data-free data-init handoff probe",
};

constexpr MkwSwitchTranslatedProductHandoffApi kHandoffApi{
    .abi_version = mkw::translated_product_handoff::kAbiVersion,
    .initialize_data_sections = initialize_synthetic_data_sections,
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
