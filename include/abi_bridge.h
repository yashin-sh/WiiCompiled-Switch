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

template <std::uint32_t Target>
struct KnownTranslatedCpuCall {
    static constexpr bool kAvailable = false;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = 0xFFFFC000u;
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

template <std::uint32_t Target>
inline bool IsBaseTranslatedCpuTargetActive() noexcept {
    (void)Target;
    return false;
}

template <std::uint32_t Target>
[[noreturn]] inline void InvokeDirectCpu(CpuContext* cpu) {
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
