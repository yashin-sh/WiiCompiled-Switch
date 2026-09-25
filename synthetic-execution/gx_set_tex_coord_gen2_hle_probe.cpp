#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Public CI must not fabricate a game texture
// coordinate call or exercise Aurora state for private RMCP01 data.
static_assert(KnownNativeCpuCall<0x8016E37Cu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_set_tex_coord_gen2_hle_probe() {
    return KnownNativeCpuCall<0x8016E37Cu>::kAvailable;
}

#endif
