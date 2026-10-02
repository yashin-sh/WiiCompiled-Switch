#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8017039Cu>::kAvailable);

// Headless dispatch/link coverage; no guest color content is inferred from
// the hardware-captured pointer. Executable decoding coverage is separate.
extern "C" __attribute__((used)) bool synthetic_gx_set_chan_amb_color_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 4u;
    cpu.gpr[4] = 0u;
    InvokeDirectCpu<0x8017039Cu>(&cpu);
    return cpu.gpr[3] == 4u && cpu.gpr[4] == 0u;
}

#endif
