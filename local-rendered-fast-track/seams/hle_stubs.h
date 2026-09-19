#pragma once

// Rendered-fast-track seam shadowing the desktop runtime's hle_stubs.h.
//
// The desktop header drags in the runtime's parallel ABI world
// (abi_bridge/system_bridge/runtime_log/ppc_runtime) whose Memory, GuestFlat,
// dispatch-record and trait declarations collide with the Horizon slice that
// this target intentionally binds to (see include/memory.h). Nothing compiled
// into the rendered Switch executable uses the VI/Audio/OS HLE entry points
// or the translator override macros from that header: the GX FIFO decoder
// (HleFifoWrite) and the HleGxState it fills live in gx_internal.h, which is
// unaffected by this seam.
