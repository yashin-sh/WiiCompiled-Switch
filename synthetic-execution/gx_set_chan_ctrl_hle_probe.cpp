#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

static_assert(KnownNativeCpuCall<0x80170570u>::kAvailable);

// Nintendo-data-free compile/link coverage for PAL GXSetChanCtrl. Only r3=4
// comes from the hardware blocker; the remaining zero values are synthetic and
// deliberately do not claim to reproduce the game's observed r4..r9 state.
extern "C" __attribute__((used)) bool synthetic_gx_set_chan_ctrl_hle_probe() {
    CpuContext cpu{};
    cpu.gpr[3] = 4u;
    cpu.gpr[4] = 0u;
    cpu.gpr[5] = 0u;
    cpu.gpr[6] = 0u;
    cpu.gpr[7] = 0u;
    cpu.gpr[8] = 0u;
    cpu.gpr[9] = 0u;
    InvokeDirectCpu<0x80170570u>(&cpu);
    return true;
}

#endif
