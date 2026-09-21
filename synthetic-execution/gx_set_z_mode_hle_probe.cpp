#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80172824u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetZMode boundary. The current durable blocker captured r3 only; r4/r5
// were not present in that diagnostic, so this probe deliberately does not
// fabricate synthetic argument values for them.
extern "C" __attribute__((used)) bool synthetic_gx_set_z_mode_hle_probe() {
    auto* invoke = &KnownNativeCpuCall<0x80172824u>::Invoke;
    return invoke != nullptr;
}

#endif
