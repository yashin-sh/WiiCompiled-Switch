#pragma once

// Minimal Horizon ABI seam for the translated-function *link-only* checkpoint.
//
// The pinned desktop abi_bridge.h pulls the full desktop runtime/registry,
// graphics options and crash UI. None of those are required merely to prove
// devkitA64 can compile and link the translator-owned base function shards.
// Registration and indirect-dispatch shards are intentionally excluded from
// this checkpoint, and the runtime never calls a translated function yet.

#include "ppc_runtime.h"
// Pinned upstream abi_bridge.h exposes the CR-resident helpers separately from
// ppc_runtime.h. Generated function shards call SetCRResident directly, so the
// Switch link-only seam must preserve that same header contract.
#include "isa/ppc_isa_cr.h"

#include <cstdint>
#include <cstdlib>

// Pinned upstream memory_access.h normally exposes these GX FIFO hooks to
// generated stores. The Switch link-only build intentionally does not include
// memory_access.h because it would select the desktop flat-memory model, so
// preserve just the declaration contract here. Implementations remain a later
// graphics/HLE checkpoint; unreferenced translated sections are linker-GC'd.
extern "C" {
void GX_HLE_FIFO_WriteFloat(float value);
void GX_HLE_FIFO_Write32(std::uint32_t value);
void GX_HLE_FIFO_Write16(std::uint16_t value);
void GX_HLE_FIFO_Write8(std::uint8_t value);
void GX_HLE_FIFO_WriteBurst(const std::uint8_t* data, std::uint32_t sizeBytes);
}

inline void ApplyRuntimeCallOptions(std::uint32_t, CpuContext*) noexcept {
    // No game-facing runtime options are active in the link-only checkpoint.
}

template <std::uint32_t Target>
struct KnownTranslatedCpuCall {
    static constexpr bool kAvailable = false;
    static constexpr std::uint32_t kNonvolatileFprWriteMask = 0xFFFFC000u;
    static constexpr bool kMustRemainDynamicallyDispatchable = true;
    static constexpr void (*Entry)(CpuContext*) = nullptr;
};

// Same generated-traits surface as upstream. Aggregate shards include a
// sibling traits header before their function bodies, so statically resolvable
// generated-to-generated calls still type-check and link normally.
#define MKW_TRANSLATED_TRAIT(addr, winner, nonvolatile_fpr_write_mask)                    \
    extern "C" void winner(CpuContext* ctx);                                              \
    template <>                                                                           \
    struct KnownTranslatedCpuCall<0x##addr##u> {                                          \
        static constexpr bool kAvailable = true;                                          \
        static constexpr std::uint32_t kNonvolatileFprWriteMask = nonvolatile_fpr_write_mask; \
        static constexpr bool kMustRemainDynamicallyDispatchable = false;                 \
        static constexpr void (*Entry)(CpuContext*) = &winner;                            \
    }

// State-free optimized regions are deliberately disabled until the real
// translated dispatch/registry lifecycle is brought across to Horizon.
template <std::uint32_t Target>
inline bool IsBaseTranslatedCpuTargetActive() noexcept {
    (void)Target;
    return false;
}

// Generic dispatch remains a loud stop in this checkpoint. Stable translated
// calls lowered by the shard emitter use MKW_STATIC_TRANSLATED_CALL and do not
// pass through these functions. Nothing in the Switch bootstrap invokes them.
template <std::uint32_t Target>
[[noreturn]] inline void InvokeDirectCpu(CpuContext*) {
    (void)Target;
    std::abort();
}

[[noreturn]] inline void InvokeIndirectCpu(std::uint32_t, CpuContext*) {
    std::abort();
}

[[noreturn]] inline void InvokeIndirectJump(std::uint32_t, CpuContext*) {
    std::abort();
}

// Keep the declaration surface available for generated code that refers to a
// context fallback, but do not initialize or run it during this link-only step.
inline CpuContext& GetPersistentCpuContext() noexcept {
    static CpuContext context{};
    return context;
}
inline void InitializePersistentCpuContext() noexcept {}
