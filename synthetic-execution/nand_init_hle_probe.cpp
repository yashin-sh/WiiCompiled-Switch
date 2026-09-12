#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free compile coverage for PAL NANDInit (0x8019E18C).
// Pinned WiiCompiled initializes its host-side ISFS root, writes the current
// title data path into guest NANDHomeDir, marks NAND initialized, and returns
// NAND_RESULT_OK. No Nintendo file content is embedded by this probe.
extern "C" __attribute__((used)) void synthetic_nand_init_hle_probe(CpuContext* ctx) {
    if (!ctx) {
        return;
    }
    InvokeDirectCpu<0x8019E18Cu>(ctx);
}

#endif
