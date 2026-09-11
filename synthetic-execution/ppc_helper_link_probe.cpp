#include "abi_bridge.h"
#include "isa/ppc_isa_int.h"

#include <cstdint>

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" void synthetic_translated_fast_track_start(CpuContext* ctx);

// Link-only coverage for the helper family emitted by real translated shards.
// Use WiiCompiled's own ISA declarations so this probe cannot drift from the
// pinned runtime ABI. Retaining this function forces all four helper references
// through the devkitA64 linker while remaining Nintendo-data-free.
extern "C" __attribute__((noinline, used))
void synthetic_ppc_helper_link_probe(CpuContext* ctx) {
    volatile auto timebase = PPC_Mftb() ^ PPC_Mftbu();
    (void)timebase;

    const std::uint32_t ctr = ctx ? ctx->ctr : 0u;
    PPC_WriteSpr(9u, ctr);
    (void)PPC_ReadSpr(9u);

    // Exercise the same static native-dispatch path that the real translated
    // __start graph uses for PAL __OSGetSystemTime (0x801AAD7C).
    if (ctx) {
        InvokeDirectCpu<0x801AAD7Cu>(ctx);

        // Cover the full early interrupt-state trio in one pass so a real
        // startup run cannot immediately fall from Disable into an uncovered
        // Enable/Restore boundary on the next hardware iteration.
        const std::uint32_t savedR3 = ctx->gpr[3];
        InvokeDirectCpu<0x801A65ACu>(ctx);
        InvokeDirectCpu<0x801A65C0u>(ctx);
        ctx->gpr[3] = 1u;
        InvokeDirectCpu<0x801A65D4u>(ctx);
        ctx->gpr[3] = savedR3;
    }

    // Keep the existing synthetic startup graph auditable in the same ELF.
    if (ctx) {
        synthetic_translated_fast_track_start(ctx);
    }
}
#endif
