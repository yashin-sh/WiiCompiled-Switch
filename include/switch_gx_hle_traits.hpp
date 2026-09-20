#pragma once

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
#endif

extern "C" void mkw_switch_hle_gx_init(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_draw_done(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_viewport(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_scissor(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_load_pos_mtx_imm(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_current_mtx(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_clear_vtx_desc(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_desc(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_attr_fmt(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_chans(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_mat_color(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_ctrl(CpuContext* cpu) noexcept;
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

// GXSetScissor (PAL 0x80173430). Pinned WiiCompiled consumes r3..r6 as
// unsigned left/top/width/height, mirrors the two guest GX scissor BP words
// and dirty flag, then forwards the rectangle to Aurora GX.
template <>
struct KnownNativeCpuCall<0x80173430u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_scissor(cpu);
    }
};

// GXLoadPosMtxImm (PAL 0x8017310C). Pinned WiiCompiled consumes the guest
// 3x4 big-endian position matrix from r3 and the matrix id from r4, converts
// twelve float32 entries to host order, then forwards them to Aurora GX.
template <>
struct KnownNativeCpuCall<0x8017310Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_load_pos_mtx_imm(cpu);
    }
};

// GXSetCurrentMtx (PAL 0x80173214). Pinned WiiCompiled consumes r3 as the
// current position-matrix id and forwards it directly to Aurora GX.
template <>
struct KnownNativeCpuCall<0x80173214u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_current_mtx(cpu);
    }
};

// GXClearVtxDesc (PAL 0x8016DC34). Pinned WiiCompiled clears all tracked
// vertex descriptors, invalidates its cached vertex-layout hash if needed,
// preserves array base/stride state, then forwards GXClearVtxDesc to Aurora.
template <>
struct KnownNativeCpuCall<0x8016DC34u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_clear_vtx_desc(cpu);
    }
};

// GXSetVtxDesc (PAL 0x8016D3A4). Pinned WiiCompiled consumes attr/type from
// r3/r4, canonicalizes NBT to NRM for tracked HLE state, invalidates the
// vertex-layout hash on changes, skips Aurora writes for matrix-index attrs,
// and expands INDEX8/INDEX16 descriptors to GX_DIRECT for Aurora streaming.
template <>
struct KnownNativeCpuCall<0x8016D3A4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_vtx_desc(cpu);
    }
};

// GXSetVtxAttrFmt (PAL 0x8016DC68). Pinned WiiCompiled consumes
// r3..r7 = vtxfmt/attr/count/type/frac, canonicalizes NBT to NRM for tracked
// HLE state, invalidates the vertex-layout hash only when the format changes,
// then forwards valid public attributes to Aurora GX.
template <>
struct KnownNativeCpuCall<0x8016DC68u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_vtx_attr_fmt(cpu);
    }
};

// GXSetNumChans (PAL 0x8017054C). Pinned WiiCompiled consumes r3 as the
// requested channel count, narrows it to u8, and forwards it directly to
// Aurora GXSetNumChans.
template <>
struct KnownNativeCpuCall<0x8017054Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_num_chans(cpu);
    }
};

// GXSetChanMatColor (PAL 0x80170474). Pinned WiiCompiled consumes r3 as
// GXChannelID and r4 as a guest pointer to one packed RGBA word, ensures an
// Aurora frame is active, decodes the guest color, then forwards it to Aurora.
template <>
struct KnownNativeCpuCall<0x80170474u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_chan_mat_color(cpu);
    }
};

// GXSetChanCtrl (PAL 0x80170570). Pinned WiiCompiled consumes
// r3..r9 = channel/enable/ambient-source/material-source/light-mask/
// diffuse-function/attenuation-function and forwards those values directly to
// Aurora GXSetChanCtrl, with enable normalized as non-zero -> true.
template <>
struct KnownNativeCpuCall<0x80170570u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_chan_ctrl(cpu);
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
