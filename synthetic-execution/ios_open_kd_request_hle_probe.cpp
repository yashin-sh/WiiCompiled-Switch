#include "switch_ios_kd_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Hardware proves the exact PAL IOS_Open address,
// guest path and mode; public CI deliberately does not fabricate a game call.
static_assert(KnownNativeCpuCall<0x801938F8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_ios_open_kd_request_hle_probe() {
    return KnownNativeCpuCall<0x801938F8u>::kAvailable;
}

#endif
