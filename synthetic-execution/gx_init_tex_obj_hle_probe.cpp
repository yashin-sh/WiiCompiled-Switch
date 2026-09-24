#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Hardware has proven the exact entry address and
// r3..r6, while r7..r10 remain live guest arguments consumed only on hardware.
// Do not fabricate texture format/wrap/mipmap values in public CI.
static_assert(KnownNativeCpuCall<0x801707F8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_init_tex_obj_hle_probe() {
    return KnownNativeCpuCall<0x801707F8u>::kAvailable;
}

#endif
