#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80172888u>::kAvailable);

// Nintendo-data-free compile/link coverage for the hardware-observed PAL
// GXSetPixelFmt boundary. Discovery hardware captured r3=1 / r4=0.
extern "C" __attribute__((used)) bool synthetic_gx_set_pixel_fmt_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 1u;
    cpu.gpr[4] = 0u;
    InvokeDirectCpu<0x80172888u>(&cpu);
    return true;
}

#endif
