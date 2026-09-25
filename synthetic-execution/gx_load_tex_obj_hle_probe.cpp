#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. The exact descriptor is hardware-owned and the
// public probe must not fabricate texture bytes or invoke GXLoadTexObj.
static_assert(KnownNativeCpuCall<0x80170F2Cu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_load_tex_obj_hle_probe() {
    return KnownNativeCpuCall<0x80170F2Cu>::kAvailable;
}

#endif
