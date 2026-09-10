#pragma once

// Minimal Horizon ABI seam for translated-function checkpoints.
#include "ppc_runtime.h"
#include "isa/ppc_isa_cr.h"

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

    // The generated aggregate shard includes a sibling trait header containing
    // every translated direct-call dependency visible to that shard. Use it.
    // This keeps the fast-track on the real translated call graph without
    // requiring registration/static constructors or one-off address patches.
    if constexpr (KnownTranslatedCpuCall<Target>::kAvailable) {
        ApplyRuntimeCallOptions(Target, cpu);
        DispatchKnownTranslatedCpuTargetStatic<Target>(cpu);
        return;
    }

    // A target not represented by a translated trait is a genuine boundary for
    // the current Switch port (HLE/native/dynamic/etc.). Record it durably.
    mkw_switch_report_unsupported_translated_dispatch("DIRECT", Target, cpu);
    std::abort();
}

[[noreturn]] inline void InvokeIndirectCpu(std::uint32_t target, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch("INDIRECT_CALL", target, cpu);
    std::abort();
}

[[noreturn]] inline void InvokeIndirectJump(std::uint32_t target, CpuContext* cpu) {
    mkw_switch_report_unsupported_translated_dispatch("INDIRECT_JUMP", target, cpu);
    std::abort();
}

inline CpuContext& GetPersistentCpuContext() noexcept {
    static CpuContext context{};
    return context;
}
inline void InitializePersistentCpuContext() noexcept {}
