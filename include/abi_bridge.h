#pragma once

// Minimal Horizon ABI seam for translated-function checkpoints.
#include "ppc_runtime.h"
#include "isa/ppc_isa_cr.h"
#include "switch_indirect_dispatch.hpp"

#include <cstdint>
#include <cstdlib>

extern "C" {
void GX_HLE_FIFO_WriteFloat(float value);
void GX_HLE_FIFO_Write32(std::uint32_t value);
void GX_HLE_FIFO_Write16(std::uint16_t value);
void GX_HLE_FIFO_Write8(std::uint8_t value);
void GX_HLE_FIFO_WriteBurst(const std::uint8_t* data, std::uint32_t sizeBytes);

// Diagnostic seam used by blocker-driven fast-track startup. Non-fast-track
// builds provide a no-op implementation, so generated-code behavior remains
// unchanged except that an attributable record can be emitted before abort.
void mkw_switch_report_unsupported_translated_dispatch(
    const char* kind,
    std::uint32_t target,
    CpuContext* cpu) noexcept;

// Switch-native implementation of Wii SDK __OSGetSystemTime (PAL 0x801AAD7C).
// The pinned WiiCompiled runtime treats this address as a native override and
// publishes the 64-bit result in guest r3:r4.
void mkw_switch_hle_os_get_system_time(CpuContext* cpu) noexcept;

// Switch-native early OS interrupt-state overrides. These mirror the pinned
// WiiCompiled HLE and publish the previous interrupt state in guest r3.
void mkw_switch_hle_os_disable_interrupts(CpuContext* cpu) noexcept;
void mkw_switch_hle_os_enable_interrupts(CpuContext* cpu) noexcept;
void mkw_switch_hle_os_restore_interrupts(CpuContext* cpu) noexcept;

// Switch-native early OS exception/interrupt initialization. WiiCompiled
// replaces both guest entry points with host HLE to avoid installing Wii
// exception vectors or touching the Hollywood interrupt controller.
void mkw_switch_hle_os_exception_init(CpuContext* cpu) noexcept;
void mkw_switch_hle_os_interrupt_init(CpuContext* cpu) noexcept;
}

inline void ApplyRuntimeCallOptions(std::uint32_t, CpuContext*) noexcept {}

inline constexpr std::uint32_t kPpcAllNonvolatileFprMask = 0xFFFFC000u;

template <std::uint32_t Target>
struct KnownTranslatedCpuCall {
    static constexpr bool kAvailable = false;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = kPpcAllNonvolatileFprMask;
    static constexpr bool kMustRemainDynamicallyDispatchable = true;
    static constexpr void (*Entry)(CpuContext*) = nullptr;
};

#define MKW_TRANSLATED_TRAIT(addr, winner, nonvolatile_fpr_write_mask)                    \
    extern "C" void winner(CpuContext* ctx);                                              \
    template <>                                                                           \
    struct KnownTranslatedCpuCall<0x##addr##u> {                                          \
        static constexpr bool kAvailable = true;                                          \
        static constexpr std::uint32_t kNonvolatileFprWriteMask = nonvolatile_fpr_write_mask; \
        static constexpr bool kMustRemainDynamicallyDispatchable = false;                 \
        static constexpr void (*Entry)(CpuContext*) = &winner;                            \
    }

// Minimal native/HLE catalog for guest entry points that WiiCompiled itself
// deliberately excludes from translation. Keep these semantics aligned with
// the pinned upstream runtime instead of attempting to execute raw Wii
// hardware setup on Horizon.
template <std::uint32_t Target>
struct KnownNativeCpuCall {
    static constexpr bool kAvailable = false;
    static inline void Invoke(CpuContext*) noexcept {}
};

// PAL __init_hardware. The pinned WiiCompiled runtime replaces this entire
// routine with a native no-op, avoiding PPC machine-state/cache/FPU hardware
// initialization that has no meaning on Switch. Preserve the guest CpuContext.
template <>
struct KnownNativeCpuCall<0x80006348u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext*) noexcept {}
};

// PAL __OSGetSystemTime. WiiCompiled's pinned runtime supplies a native HLE for
// this address rather than translating the SDK routine. Keep that same boundary
// on Horizon so startup does not fall into the unsupported translated dispatcher.
template <>
struct KnownNativeCpuCall<0x801AAD7Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_get_system_time(cpu);
    }
};

// PAL OSDisableInterrupts/OSEnableInterrupts/OSRestoreInterrupts. WiiCompiled
// provides native overrides for the interrupt-state bookkeeping rather than
// touching Broadway/Hollywood interrupt hardware directly.
template <>
struct KnownNativeCpuCall<0x801A65ACu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_disable_interrupts(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x801A65C0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_enable_interrupts(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x801A65D4u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_restore_interrupts(cpu);
    }
};

// PAL OS__ExceptionInit/OS____InterruptInit. The pinned runtime replaces both
// with host HLE: exception-vector setup is skipped, while interrupt init keeps
// only the guest-visible handler table/mask state and avoids Wii MMIO.
template <>
struct KnownNativeCpuCall<0x801A00E0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_exception_init(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x801A661Cu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_os_interrupt_init(cpu);
    }
};

// Broadway hardware-register helpers that the pinned WiiCompiled runtime
// intentionally replaces with host no-ops. Keep the catalogue narrowly scoped
// to helpers whose upstream HLE has no guest-visible state change.
#define MKW_NATIVE_NOOP_TRAIT(addr)            \
    template <>                                \
    struct KnownNativeCpuCall<0x##addr##u> {   \
        static constexpr bool kAvailable = true; \
        static inline void Invoke(CpuContext*) noexcept {} \
    }

// Performance-monitor writes.
MKW_NATIVE_NOOP_TRAIT(8012E5B8); // PPCMtmmcr0
MKW_NATIVE_NOOP_TRAIT(8012E5C0); // PPCMtmmcr1
MKW_NATIVE_NOOP_TRAIT(8012E5C8); // PPCMtpmc1
MKW_NATIVE_NOOP_TRAIT(8012E5D0); // PPCMtpmc2
MKW_NATIVE_NOOP_TRAIT(8012E5D8); // PPCMtpmc3
MKW_NATIVE_NOOP_TRAIT(8012E5E0); // PPCMtpmc4

// Write-pipe/speculation/HID4 helpers. The pinned runtime stubs these exact
// entry points; do not fold HID2 here because its HLE updates CpuContext::hid2.
MKW_NATIVE_NOOP_TRAIT(8012E640); // PPCMfwpar
MKW_NATIVE_NOOP_TRAIT(8012E64C); // PPCMtwpar
MKW_NATIVE_NOOP_TRAIT(8012E654); // PPCDisableSpeculation
MKW_NATIVE_NOOP_TRAIT(8012E684); // PPCMthid4

// PAL data-cache range maintenance. WiiCompiled native-overrides this whole
// five-function family because host CPUs own cache coherency. Upstream also
// validates the guest range and notifies its GX RAM tracker; the Switch port
// does not have that GX tracker yet, so preserving CpuContext is the complete
// guest-visible CPU behavior for this stage of startup.
MKW_NATIVE_NOOP_TRAIT(801A1600); // DCInvalidateRange
MKW_NATIVE_NOOP_TRAIT(801A162C); // DCFlushRange
MKW_NATIVE_NOOP_TRAIT(801A165C); // DCStoreRange
MKW_NATIVE_NOOP_TRAIT(801A168C); // DCFlushRangeNoSync
MKW_NATIVE_NOOP_TRAIT(801A16B8); // DCStoreRangeNoSync

// PAL cache-control entry points whose pinned WiiCompiled HLE is the shared
// Cache_Maintenance_Stub. These have no guest-visible state change on the host.
// Keep DCZeroRange and LC transfer/queue helpers out of this list: those have
// real guest-memory or return-value semantics and must be ported faithfully.
MKW_NATIVE_NOOP_TRAIT(801A15EC); // DCEnable
MKW_NATIVE_NOOP_TRAIT(801A1710); // ICInvalidateRange
MKW_NATIVE_NOOP_TRAIT(801A1744); // ICFlashInvalidate
MKW_NATIVE_NOOP_TRAIT(801A1754); // ICEnable
MKW_NATIVE_NOOP_TRAIT(801A1768); // __LCEnable
MKW_NATIVE_NOOP_TRAIT(801A1834); // LCEnable
MKW_NATIVE_NOOP_TRAIT(801A186C); // LCDisable
MKW_NATIVE_NOOP_TRAIT(801A1AE4); // OS____CacheInit

// EXI interrupt masking has no host-side effect in the pinned HLE. Real EXI
// transactions are deliberately not folded into this no-op family.
MKW_NATIVE_NOOP_TRAIT(80167E78); // SetExiInterruptMask

// Serial Interface initialization/poll timing are host-side no-ops in pinned
// WiiCompiled. They only avoid Wii SI MMIO/controller-port setup during boot.
MKW_NATIVE_NOOP_TRAIT(801B2DE0); // SIInit
MKW_NATIVE_NOOP_TRAIT(801B3ACC); // SISetSamplingRate

#undef MKW_NATIVE_NOOP_TRAIT

// Early EXI control calls are explicit native overrides in pinned WiiCompiled.
// They skip Hollywood MMIO and publish the same constant results in guest r3.
template <>
struct KnownNativeCpuCall<0x80168FA0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 0u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x801689D0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

template <>
struct KnownNativeCpuCall<0x80168B00u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        if (cpu) {
            cpu->gpr[3] = 1u;
        }
    }
};

// Keep the translated PPC ABI rule used by WiiCompiled: a callee may write
// f14..f31 internally, but those registers are nonvolatile to its caller.
// Generated trait headers provide the exact write mask for each direct target.
class PpcNonvolatileFprGuard {
public:
    explicit PpcNonvolatileFprGuard(
        CpuContext* cpu,
        std::uint32_t mask = kPpcAllNonvolatileFprMask) noexcept
        : cpu_(cpu), mask_(mask & kPpcAllNonvolatileFprMask) {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14; reg <= 31; ++reg) {
            if ((mask_ & (1u << reg)) != 0u) {
                saved_[reg - 14] = cpu_->fpr[reg];
            }
        }
    }

    ~PpcNonvolatileFprGuard() noexcept {
        if (!cpu_) {
            return;
        }
        for (std::uint32_t reg = 14; reg <= 31; ++reg) {
            if ((mask_ & (1u << reg)) != 0u) {
                cpu_->fpr[reg] = saved_[reg - 14];
            }
        }
    }

    PpcNonvolatileFprGuard(const PpcNonvolatileFprGuard&) = delete;
    PpcNonvolatileFprGuard& operator=(const PpcNonvolatileFprGuard&) = delete;

private:
    CpuContext* cpu_ = nullptr;
    std::uint32_t mask_ = 0u;
    PPC_FPR saved_[18]{};
};

template <std::uint32_t Target>
inline bool IsBaseTranslatedCpuTargetActive() noexcept {
    (void)Target;
    return false;
}

template <std::uint32_t Target>
inline void DispatchKnownTranslatedCpuTargetStatic(CpuContext* cpu) {
    static_assert(KnownTranslatedCpuCall<Target>::kAvailable);
    if constexpr (KnownTranslatedCpuCall<Target>::kNonvolatileFprWriteMask == 0u) {
        KnownTranslatedCpuCall<Target>::Entry(cpu);
    } else {
        PpcNonvolatileFprGuard fpr_guard(
            cpu, KnownTranslatedCpuCall<Target>::kNonvolatileFprWriteMask);
        KnownTranslatedCpuCall<Target>::Entry(cpu);
    }
}

template <std::uint32_t Target>
inline void InvokeDirectCpu(CpuContext* cpu) {
    static_assert(Target != 0u, "InvokeDirectCpu cannot target address 0");

    // Native/HLE overrides win first because their guest entry points are
    // intentionally excluded from the translated graph by WiiCompiled.
    if constexpr (KnownNativeCpuCall<Target>::kAvailable) {
        ApplyRuntimeCallOptions(Target, cpu);
        KnownNativeCpuCall<Target>::Invoke(cpu);
        return;
    }

    // The generated aggregate shard includes a sibling trait header containing
    // every translated direct-call dependency visible to that shard. Use it.
    // This keeps the fast-track on the real translated call graph without
    // requiring registration/static constructors or one-off address patches.
    if constexpr (KnownTranslatedCpuCall<Target>::kAvailable) {
        ApplyRuntimeCallOptions(Target, cpu);
        DispatchKnownTranslatedCpuTargetStatic<Target>(cpu);
        return;
    }

    // A target not represented by a translated trait or native HLE is a genuine
    // boundary for the current Switch port. Record it durably before stopping.
    mkw_switch_report_unsupported_translated_dispatch("DIRECT", Target, cpu);
    std::abort();
}

inline void InvokeIndirectCpu(std::uint32_t target, CpuContext* cpu) {
    ApplyRuntimeCallOptions(target, cpu);
    if (mkw_switch_try_dispatch_indirect(target, cpu)) {
        return;
    }
    mkw_switch_report_unsupported_translated_dispatch("INDIRECT_CALL_MISS", target, cpu);
    std::abort();
}

inline void InvokeIndirectJump(std::uint32_t target, CpuContext* cpu) {
    ApplyRuntimeCallOptions(target, cpu);
    if (mkw_switch_try_dispatch_indirect(target, cpu)) {
        return;
    }
    mkw_switch_report_unsupported_translated_dispatch("INDIRECT_JUMP_MISS", target, cpu);
    std::abort();
}

inline CpuContext& GetPersistentCpuContext() noexcept {
    static CpuContext context{};
    return context;
}
inline void InitializePersistentCpuContext() noexcept {}
