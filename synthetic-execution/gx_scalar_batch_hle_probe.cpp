#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8017351Cu>::kAvailable);
static_assert(KnownNativeCpuCall<0x80172930u>::kAvailable);
static_assert(KnownNativeCpuCall<0x8017295Cu>::kAvailable);

// Nintendo-data-free dispatch/link coverage for the audited scalar batch.
// Only GXSetClipMode has a hardware-observed tuple at this revision.
extern "C" __attribute__((used)) bool synthetic_gx_scalar_batch_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0u;
    InvokeDirectCpu<0x8017351Cu>(&cpu);
    cpu.gpr[3] = 1u;
    InvokeDirectCpu<0x80172930u>(&cpu);
    cpu.gpr[4] = 0xffu;
    InvokeDirectCpu<0x8017295Cu>(&cpu);
    return cpu.gpr[3] == 1u && cpu.gpr[4] == 0xffu;
}

#endif
