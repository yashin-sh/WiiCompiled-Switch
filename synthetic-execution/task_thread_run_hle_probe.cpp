#include "abi_bridge.h"

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK

// Nintendo-data-free coverage for the hardware-proven virtual
// EGG::TaskThread::run target. r3=0 makes the bridge return before touching a
// guest task queue while still proving the indirect native resolver wins.
extern "C" __attribute__((used)) void synthetic_task_thread_run_hle_probe(CpuContext* ctx) {
    if (!ctx || ctx->pc != 0xFFFFFFFCu) {
        return;
    }

    ctx->gpr[3] = 0u;
    InvokeIndirectJump(0x80242D7Cu, ctx);
}

#endif
