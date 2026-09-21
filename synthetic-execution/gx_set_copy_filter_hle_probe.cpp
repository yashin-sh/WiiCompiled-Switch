#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. The hardware blocker captured r3/r4/r5 but not
// r6, so the Nintendo-data-free probe deliberately does not fabricate a
// vertical-filter pointer or invoke the bridge with invented live arguments.
static_assert(KnownNativeCpuCall<0x8016FA40u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_copy_filter_hle_probe() {
    return KnownNativeCpuCall<0x8016FA40u>::kAvailable;
}

#endif
