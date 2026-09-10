// Nintendo-data-free compile/link probe shaped like a translated shard.
#include "abi_bridge.h"
#include "memory.h"
#include "ppc_isa_memory.h"
#include "recomp_mod_loader.h"

MKW_TRANSLATED_TRAIT(80001000, synthetic_translated_leaf, 0x00000000u);

// Synthetic-only implementation so the retained probe can exercise the same
// generated GX FIFO call shape without enabling graphics/HLE in the runtime.
extern "C" void GX_HLE_FIFO_Write8(std::uint8_t) {}

extern "C" void synthetic_translated_leaf(CpuContext* ctx) {
    if (!ctx) return;

    // Exercises the Clang->devkitA64 GCC vector-ABI compatibility shim while
    // remaining entirely synthetic and never being called by the app.
    const MkwStateFreeResult2 lanes{0x11u, 0x22u};
    ctx->gpr[3] ^= static_cast<std::uint32_t>(lanes[0]);
    ctx->gpr[4] ^= static_cast<std::uint32_t>(lanes[1]);

    // Real translated shards call these CR-resident helpers directly through
    // abi_bridge.h. Exercise both integer overloads so CI catches a missing
    // ppc_isa_cr.h exposure before a private Mario Kart build does.
    SetCRResident(ctx->cr, ctx->xer, 0,
                  static_cast<std::int32_t>(ctx->gpr[3]),
                  static_cast<std::int32_t>(ctx->gpr[4]));
    SetCRResident(ctx->cr, ctx->xer, 1, ctx->gpr[3], ctx->gpr[4]);

    // Real generated stores can call GX FIFO HLE hooks directly. Keep this in
    // the retained synthetic leaf so public CI proves declaration + linkage.
    GX_HLE_FIFO_Write8(static_cast<std::uint8_t>(ctx->gpr[6]));

    // Exercises the checked translated-memory seam at compile/link time. The
    // function is retained for nm verification but never executed in CI.
    ctx->gpr[5] ^= MemoryInline::FlatRead32(0x80004000u);
}
