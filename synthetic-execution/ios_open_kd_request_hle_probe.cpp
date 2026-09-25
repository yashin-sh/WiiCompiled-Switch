#include "switch_ios_kd_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Hardware proves the exact PAL IOS_Open request,
// first IOS_Ioctl KD command-2 boundary, and fd-2000 IOS_Close; public CI
// deliberately does not fabricate guest buffers or a game call.
static_assert(KnownNativeCpuCall<0x801938F8u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80194290u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80193AD8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_ios_open_kd_request_hle_probe() {
    return KnownNativeCpuCall<0x801938F8u>::kAvailable &&
           KnownNativeCpuCall<0x80194290u>::kAvailable &&
           KnownNativeCpuCall<0x80193AD8u>::kAvailable;
}

#endif
