#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Public CI does not fabricate the private
// TaskThread queue/list/fiber lifecycle used by the real RMCP01 run.
static_assert(KnownNativeCpuCall<0x801AA1D4u>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_os_cancel_thread_hle_probe() {
    return KnownNativeCpuCall<0x801AA1D4u>::kAvailable;
}

#endif
