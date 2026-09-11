#include "ppc_runtime.h"

#include <cstdint>

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" std::uint32_t PPC_Mftb();
extern "C" std::uint32_t PPC_Mftbu();
extern "C" std::uint32_t PPC_ReadSpr(std::uint32_t spr);
extern "C" void PPC_WriteSpr(std::uint32_t spr, std::uint32_t value);
extern "C" void synthetic_translated_fast_track_start(CpuContext* ctx);

// Link-only coverage for the helper family emitted by real translated shards.
// Retaining this one function forces all four helper references through the
// devkitA64 linker while remaining Nintendo-data-free.
extern "C" __attribute__((noinline, used))
void synthetic_ppc_helper_link_probe(CpuContext* ctx) {
    volatile std::uint32_t timebase = PPC_Mftb() ^ PPC_Mftbu();
    (void)timebase;

    const std::uint32_t ctr = ctx ? ctx->ctr : 0u;
    PPC_WriteSpr(9u, ctr);
    (void)PPC_ReadSpr(9u);

    // Keep the existing synthetic startup graph auditable in the same ELF.
    if (ctx) {
        synthetic_translated_fast_track_start(ctx);
    }
}
#endif
