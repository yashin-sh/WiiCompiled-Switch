// Narrow Switch bridge for WiiCompiled floating-point helpers that translated
// shards can call before the full desktop runtime is available.
//
// Keep this file CPU-only. Importing runtime/src/fpu_helpers.cpp wholesale would
// also pull the desktop guest-memory implementation through memory.h, which is
// intentionally not part of the Horizon fast-track slice.
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)

#include "ppc_runtime.h"

#include <cstdint>

// Exact pinned WiiCompiled semantics for mtfsb1 (a135beb...):
// - PowerPC bit numbering is MSB-first, hence shift = 31 - bit.
// - FEX (bit 1) and VX (bit 2) are read-only summary bits and cannot be set.
// - FPSCR[NI] changes are mirrored into the host FP environment. On AArch64,
//   MkwApplyHostNiMode maps NI to FPCR.FZ, which the pinned ISA layer supports.
extern "C" void PPC_Mtfsb1(std::uint32_t bit) {
    CpuContext* cpu = CurrentCpuContext();

    bit &= 31u;
    if (bit == 1u || bit == 2u) {
        return;
    }

    const std::uint32_t shift = 31u - bit;
    cpu->fpscr |= (1u << shift);
    MkwApplyHostNiMode(cpu->fpscr);
}

#endif
