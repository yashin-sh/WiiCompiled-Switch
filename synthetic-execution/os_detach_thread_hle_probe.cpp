#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Public CI must not fabricate a private RMCP01
// OSThread lifecycle beyond the exact hardware-owned mapping.
static_assert(KnownNativeCpuCall<0x801AA4ECu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_os_detach_thread_hle_probe() {
    return KnownNativeCpuCall<0x801AA4ECu>::kAvailable;
}

#endif
