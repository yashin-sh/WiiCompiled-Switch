#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Mapping/link-only coverage. Public CI must not fabricate the private
// Home Button texture object or execute the hardware-owned LOD tuple.
static_assert(KnownNativeCpuCall<0x80170A4Cu>::kAvailable);

extern "C" __attribute__((used)) bool synthetic_gx_init_tex_obj_lod_hle_probe() {
    return KnownNativeCpuCall<0x80170A4Cu>::kAvailable;
}

#endif
