#include "switch_ios_kd_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Hardware proves the exact PAL IOS_Open request,
// first IOS_Ioctl KD command-2 boundary, fd-2001 command-1 suspend, fd-2002
// command-0x0F generated-user-id boundary, fd-2003 command-3 resume boundary,
// and the hardware-proven fd-2000/fd-2001/fd-2002 IOS_Close mappings; public
// CI deliberately does not fabricate guest buffers or a game call.
static_assert(KnownNativeCpuCall<0x801938F8u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80194290u>::kAvailable);
static_assert(KnownNativeCpuCall<0x80193AD8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_ios_open_kd_request_hle_probe() {
    return KnownNativeCpuCall<0x801938F8u>::kAvailable &&
           KnownNativeCpuCall<0x80194290u>::kAvailable &&
           KnownNativeCpuCall<0x80193AD8u>::kAvailable;
}

#endif
