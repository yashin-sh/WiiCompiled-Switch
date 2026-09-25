#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x800077C8u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_strap_check_input_hle_probe() {
    return KnownNativeCpuCall<0x800077C8u>::kAvailable;
}

#endif
