#pragma once

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
#endif

extern "C" void mkw_switch_hle_gx_init(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_draw_done(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_viewport(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_disp_copy_src(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_disp_copy_dst(CpuContext* cpu) noexcept;

template <>
struct KnownNativeCpuCall<0x8016B850u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_init(cpu);
    }
};

// GXDrawDone (PAL 0x8016EAB0). Pinned WiiCompiled clears the guest draw-done
// flag, drains GX, then publishes the PE-finish bit and draw-done flag. The
// rendered fast-track uses Aurora's real GXDrawDone drain; headless/synthetic
// builds retain only the guest-visible bookkeeping contract.
template <>
struct KnownNativeCpuCall<0x8016EAB0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_draw_done(cpu);
    }
};

// GXSetProjection (PAL 0x8017301C). Pinned WiiCompiled reads the guest's
// big-endian 4x4 matrix, converts it to host floats, and forwards the matrix
// plus projection type into Aurora GX. This boundary is reached directly by
// RMCP01 after #192.
template <>
struct KnownNativeCpuCall<0x8017301Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_projection(cpu);
    }
};

// GXSetViewport (PAL 0x801733B4). Pinned WiiCompiled consumes six scalar
// float arguments from PPC f1..f6 and forwards them to Aurora GX. This is the
// first exact blocker exposed after hardware-crossing GXSetProjection.
template <>
struct KnownNativeCpuCall<0x801733B4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_viewport(cpu);
    }
};

// GXSetDispCopySrc (PAL 0x8016F438). Pinned WiiCompiled narrows r3..r6 to
// u16, records the display-copy source rectangle, and emits the two BP/RAS
// register writes through the GX FIFO. The Switch fast-track keeps that state
// and FIFO contract while the FIFO itself remains the deliberate headless sink.
template <>
struct KnownNativeCpuCall<0x8016F438u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_disp_copy_src(cpu);
    }
};

// GXSetDispCopyDst (PAL 0x8016F4B8). Pinned WiiCompiled narrows r3/r4 to
// u16, records destination width/height, and emits the BP 0x4D display-copy
// stride write through the GX FIFO. Preserve the void-call GPR contract.
template <>
struct KnownNativeCpuCall<0x8016F4B8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_disp_copy_dst(cpu);
    }
};

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
// GXCopyDisp (PAL 0x8016FC38). In the rendered fast-track this is the first
// real RMCP01 present boundary: prior translated FIFO traffic has already gone
// through pinned HleFifoWrite into Aurora GX, and this host seam resolves the
// display copy and presents the current Dawn/NVK frame on NWindow.
template <>
struct KnownNativeCpuCall<0x8016FC38u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_copy_disp(cpu);
    }
};
#endif
