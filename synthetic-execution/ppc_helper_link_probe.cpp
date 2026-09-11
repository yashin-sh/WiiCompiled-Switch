#include "abi_bridge.h"
#include "isa/ppc_isa_int.h"

#include <cstdint>

#if defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" void synthetic_translated_fast_track_start(CpuContext* ctx);

// Nintendo-data-free stand-in for the translated PAL OSInitAlarm body. Real
// local fast-track builds resolve this symbol from the user's generated shards.
extern "C" __attribute__((noinline, used)) void func_801A961C(CpuContext* ctx) {
    (void)ctx;
}

// Link-only coverage for helper families emitted by real translated shards.
// Use WiiCompiled's own ISA declarations so this probe cannot drift from the
// pinned runtime ABI. Retaining this function forces the references through the
// devkitA64 linker while remaining Nintendo-data-free.
extern "C" __attribute__((noinline, used))
void synthetic_ppc_helper_link_probe(CpuContext* ctx) {
    volatile auto timebase = PPC_Mftb() ^ PPC_Mftbu();
    (void)timebase;

    const std::uint32_t ctr = ctx ? ctx->ctr : 0u;
    PPC_WriteSpr(9u, ctr);
    (void)PPC_ReadSpr(9u);

    // OS::Init emits mtfsb1 while configuring FPSCR. Force the exact helper
    // through the public AArch64 link so a missing Switch bridge is caught by CI.
    PPC_Mtfsb1(31u);

    // Exercise the same static native-dispatch path that the real translated
    // __start graph uses for PAL __OSGetSystemTime (0x801AAD7C).
    if (ctx) {
        InvokeDirectCpu<0x801AAD7Cu>(ctx);

        // REGISTER_NATIVE_FUNCTION_AS makes OSInitAlarm a native winner while
        // retaining its original translated body. Verify that the Switch HLE
        // catalogue forwards that boundary back into translated code.
        InvokeDirectCpu<0x801A961Cu>(ctx);

        // OSClearContext is a real guest-memory bookkeeping HLE in the pinned
        // runtime. Exercise its null-context path here so public CI validates
        // compile/link/dispatch without assuming a synthetic guest allocation.
        const std::uint32_t savedClearContextR3 = ctx->gpr[3];
        ctx->gpr[3] = 0u;
        InvokeDirectCpu<0x801A2098u>(ctx);
        ctx->gpr[3] = savedClearContextR3;

        // OSSetCurrentContext follows OSClearContext during early thread setup.
        // The null-context path still proves the native dispatch is compiled and
        // linked while keeping the public probe independent of Nintendo data.
        const std::uint32_t savedSetCurrentContextR3 = ctx->gpr[3];
        ctx->gpr[3] = 0u;
        InvokeDirectCpu<0x801A1E70u>(ctx);
        ctx->gpr[3] = savedSetCurrentContextR3;

        // Cover the full early interrupt-state trio in one pass so a real
        // startup run cannot immediately fall from Disable into an uncovered
        // Enable/Restore boundary on the next hardware iteration.
        const std::uint32_t savedR3 = ctx->gpr[3];
        InvokeDirectCpu<0x801A65ACu>(ctx);
        InvokeDirectCpu<0x801A65C0u>(ctx);
        ctx->gpr[3] = 1u;
        InvokeDirectCpu<0x801A65D4u>(ctx);
        ctx->gpr[3] = savedR3;

        // OS::Init also enters the native exception/interrupt initialization
        // pair. The pinned HLE skips Wii exception vectors and keeps only safe
        // guest interrupt bookkeeping, so force both through the static catalog.
        InvokeDirectCpu<0x801A00E0u>(ctx);
        InvokeDirectCpu<0x801A661Cu>(ctx);
        ctx->gpr[3] = 1u;
        InvokeDirectCpu<0x801A65D4u>(ctx);
        ctx->gpr[3] = savedR3;

        // WiiCompiled native-overrides the whole early data-cache range family.
        // The Switch port has no GX RAM tracker yet, so these preserve the guest
        // CPU context while host cache coherency is handled by Horizon/AArch64.
        InvokeDirectCpu<0x801A1600u>(ctx); // DCInvalidateRange
        InvokeDirectCpu<0x801A162Cu>(ctx); // DCFlushRange
        InvokeDirectCpu<0x801A165Cu>(ctx); // DCStoreRange
        InvokeDirectCpu<0x801A168Cu>(ctx); // DCFlushRangeNoSync
        InvokeDirectCpu<0x801A16B8u>(ctx); // DCStoreRangeNoSync

        // Pinned WiiCompiled routes these cache-control entry points through one
        // shared no-op host stub. Cover the whole true no-op family together;
        // memory-mutating DCZeroRange and LC transfer helpers stay excluded.
        InvokeDirectCpu<0x801A15ECu>(ctx); // DCEnable
        InvokeDirectCpu<0x801A1710u>(ctx); // ICInvalidateRange
        InvokeDirectCpu<0x801A1744u>(ctx); // ICFlashInvalidate
        InvokeDirectCpu<0x801A1754u>(ctx); // ICEnable
        InvokeDirectCpu<0x801A1768u>(ctx); // __LCEnable
        InvokeDirectCpu<0x801A1834u>(ctx); // LCEnable
        InvokeDirectCpu<0x801A186Cu>(ctx); // LCDisable
        InvokeDirectCpu<0x801A1AE4u>(ctx); // OS____CacheInit

        // Early EXI setup is native/HLE in pinned WiiCompiled. Init skips all
        // Hollywood MMIO and returns 0; Select/Deselect are stubbed successful,
        // while EXI interrupt masking is a host no-op.
        InvokeDirectCpu<0x80168FA0u>(ctx); // EXIInit -> 0
        InvokeDirectCpu<0x801689D0u>(ctx); // EXISelect -> 1
        InvokeDirectCpu<0x80168B00u>(ctx); // EXIDeselect -> 1
        InvokeDirectCpu<0x80167E78u>(ctx); // SetExiInterruptMask
        ctx->gpr[3] = savedR3;

        // The next EXI transaction family is also native/HLE upstream. Exercise
        // EXIImm in write mode so the public probe does not depend on a guest
        // buffer address, then cover DMA/sync/unlock success paths.
        const std::uint32_t savedR4 = ctx->gpr[4];
        const std::uint32_t savedR5 = ctx->gpr[5];
        const std::uint32_t savedR6 = ctx->gpr[6];
        const std::uint32_t savedR7 = ctx->gpr[7];
        ctx->gpr[3] = 0u;
        ctx->gpr[4] = 0u;
        ctx->gpr[5] = 4u;
        ctx->gpr[6] = 1u;
        ctx->gpr[7] = 0u;
        InvokeDirectCpu<0x80167F68u>(ctx);
        InvokeDirectCpu<0x80168288u>(ctx);
        InvokeDirectCpu<0x80168380u>(ctx);
        InvokeDirectCpu<0x80169260u>(ctx);
        ctx->gpr[3] = savedR3;
        ctx->gpr[4] = savedR4;
        ctx->gpr[5] = savedR5;
        ctx->gpr[6] = savedR6;
        ctx->gpr[7] = savedR7;

        // SI initialization and sampling-rate setup are host no-ops upstream;
        // they skip Wii controller-port MMIO while preserving guest CPU state.
        InvokeDirectCpu<0x801B2DE0u>(ctx); // SIInit
        InvokeDirectCpu<0x801B3ACCu>(ctx); // SISetSamplingRate
        ctx->gpr[3] = savedR3;

        // PPC SDK startup clears/configures the performance monitor through
        // MMCR0/MMCR1 and PMC1..PMC4. WiiCompiled's pinned HLE treats these
        // writes as no-ops on the host, so compile all six through the same
        // KnownNativeCpuCall path used by the real fast-track graph.
        InvokeDirectCpu<0x8012E5B8u>(ctx);
        InvokeDirectCpu<0x8012E5C0u>(ctx);
        InvokeDirectCpu<0x8012E5C8u>(ctx);
        InvokeDirectCpu<0x8012E5D0u>(ctx);
        InvokeDirectCpu<0x8012E5D8u>(ctx);
        InvokeDirectCpu<0x8012E5E0u>(ctx);

        // These adjacent PPC architecture helpers are also explicit no-op
        // native overrides upstream. Keep them separate from HID2, whose HLE
        // has real CpuContext state semantics.
        InvokeDirectCpu<0x8012E640u>(ctx); // PPCMfwpar
        InvokeDirectCpu<0x8012E64Cu>(ctx); // PPCMtwpar
        InvokeDirectCpu<0x8012E654u>(ctx); // PPCDisableSpeculation
        InvokeDirectCpu<0x8012E684u>(ctx); // PPCMthid4
    }

    // Keep the existing synthetic startup graph auditable in the same ELF.
    if (ctx) {
        synthetic_translated_fast_track_start(ctx);
    }
}
#endif
