#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Public CI must not fabricate or execute the
// private RMCP01 StaticR.rel RelProlog body.
static_assert(KnownNativeCpuCall<0x8055531Cu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_staticr_rel_prolog_hle_probe() {
    return KnownNativeCpuCall<0x8055531Cu>::kAvailable;
}

#endif
