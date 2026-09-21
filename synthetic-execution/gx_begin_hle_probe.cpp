#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x8016F0F0u>::kAvailable);

// Nintendo-data-free compile/link coverage using the exact hardware-observed
// scalar tuple: GX_QUADS-like opcode 0x80, GX_VTXFMT0, 4 vertices.
extern "C" __attribute__((used)) bool synthetic_gx_begin_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 0x80u;
    cpu.gpr[4] = 0u;
    cpu.gpr[5] = 4u;
    InvokeDirectCpu<0x8016F0F0u>(&cpu);
    return cpu.gpr[3] == 0x80u && cpu.gpr[4] == 0u && cpu.gpr[5] == 4u;
}

#endif
