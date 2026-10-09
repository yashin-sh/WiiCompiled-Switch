#pragma once

#include "abi_bridge.h"

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept;
#endif

extern "C" void mkw_switch_hle_gx_init(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_draw_done(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_pix_mode_sync(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_projection(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_projectionv(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_get_projectionv(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_viewport(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_get_viewport(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_z_scale_offset(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_scissor(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_load_pos_mtx_imm(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_load_tex_mtx_imm(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_current_mtx(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_clear_vtx_desc(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_desc(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_vtx_attr_fmt(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_tex_gens(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tex_coord_gen2(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tex_coord_scale_manually(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tex_coord_bias(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_ind_stages(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_ind_tex_mtx(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_ind_tex_coord_scale(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_tev_stages(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_direct(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_color_in(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_color_op(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_alpha_in(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_alpha_op(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_swap_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_k_color(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_color(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_swap_mode_table(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_op(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_order(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_blend_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_alpha_compare(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_fog(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_z_comp_loc(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_color_update(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_alpha_update(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_z_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_pixel_fmt(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_cull_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_co_planar(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_clip_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_dither(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_dst_alpha(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_begin(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_draw_sphere(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_begin_display_list(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_end_display_list(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_num_chans(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_mat_color(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_amb_color(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_chan_ctrl(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_copy_filter(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_flush(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_invalidate_tex_all(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_disp_copy_src(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_disp_copy_dst(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_copy_tex(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_copy_clamp(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tex_copy_src(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_set_tex_copy_dst(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_init_tex_obj(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_init_tex_obj_lod(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_init_tex_obj_wrap_mode(CpuContext* cpu) noexcept;
extern "C" void mkw_switch_hle_gx_load_tex_obj(CpuContext* cpu) noexcept;

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

// GXPixModeSync (PAL 0x8016EB70): best-effort GXData mirror, then Aurora's
// real pixel-engine control BP command. Reached after the native Mii EFB copy.
template <>
struct KnownNativeCpuCall<0x8016EB70u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_pix_mode_sync(cpu);
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

// Projection-vector save/restore used by the same EGG state wrapper. Both
// boundaries share the matrix setter's pinned seven-float shadow.
template <>
struct KnownNativeCpuCall<0x80173080u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_projectionv(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x801730CCu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_get_projectionv(cpu);
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

// Guest-space viewport snapshot and the audited next Mii depth transform.
template <>
struct KnownNativeCpuCall<0x801733E0u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_get_viewport(cpu);
    }
};

template <>
struct KnownNativeCpuCall<0x80173400u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_z_scale_offset(cpu);
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

// GXLoadTexMtxImm (PAL 0x80173234). Hardware reached r3=0x802581C8,
// r4=30, r5=0 after eight IA8 map loads. Decode the guest big-endian matrix
// (12 entries for type 0, otherwise 8 with zero padding), then forward the
// id/type unchanged to Aurora. Invalid guest backing records a hard stop.
template <>
struct KnownNativeCpuCall<0x80173234u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_load_tex_mtx_imm(cpu);
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

// GXSetTexCoordGen2 (PAL 0x8016E37C). Hardware proves coord 0 with the
// tuple (coord,1,4,60,0,125). The audited local setup loop forecasts coords
// 0..7; accept only that bounded variation and retain all other guards.
template <>
struct KnownNativeCpuCall<0x8016E37Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tex_coord_gen2(cpu);
    }
};

// GXSetTexCoordScaleManually (PAL 0x80171180). Hardware captured
// (coord,enable,S,T)=(0,0,0,0). The audited batch accepts coords 0..7,
// canonical bools and u16 size narrowing, then the pinned guest GX mirror.
template <>
struct KnownNativeCpuCall<0x80171180u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tex_coord_scale_manually(cpu);
    }
};

// GXSetTexCoordBias (PAL 0x801711FC). Audited neighbor in the same local
// loop: coords 0..7, canonical bools, native call then guest S/T bias mirror.
template <>
struct KnownNativeCpuCall<0x801711FCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tex_coord_bias(cpu);
    }
};

// GXSetNumTexGens (PAL 0x8016E5A4). Pinned WiiCompiled consumes r3 as the
// requested texture-generator count, narrows it to u8, and forwards it directly
// to Aurora GXSetNumTexGens.
template <>
struct KnownNativeCpuCall<0x8016E5A4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_num_tex_gens(cpu);
    }
};

// GXSetNumIndStages (PAL 0x80171B38). Pinned WiiCompiled consumes r3 as the
// requested indirect-texture-stage count, narrows it to u8, and forwards it
// directly to Aurora GXSetNumIndStages.
template <>
struct KnownNativeCpuCall<0x80171B38u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_num_ind_stages(cpu);
    }
};

// GXSetNumTevStages (PAL 0x801722A8). Pinned WiiCompiled consumes r3 as the
// requested TEV-stage count, rejects values above GX_MAX_TEVSTAGE, narrows a
// valid count to u8, and forwards it to Aurora GXSetNumTevStages.
template <>
struct KnownNativeCpuCall<0x801722A8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_num_tev_stages(cpu);
    }
};

// Bounded TEV scalar batch. All six bridges preserve CpuContext and accept
// stage IDs 0..15, independently of the active stage count. SDK-domain guards
// are deliberately stricter than the pinned wrappers' unchecked enums;
// unknown arguments emit a durable UNPROVEN_ARGS blocker before native GX.
// Native Aurora owns the shared TEV caches and BP effects; these bridges do
// not add guest mirrors, frame activation or work markers.
// GXSetTevDirect: r3 stage.
template <>
struct KnownNativeCpuCall<0x80171B58u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_direct(cpu);
    }
};

// GXSetTevColorIn: r3 stage, r4..r7 color inputs 0..15.
template <>
struct KnownNativeCpuCall<0x80171CE0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_color_in(cpu);
    }
};

// GXSetTevColorOp: r3 stage, r4 op {0,1,8..15}, r5 bias 0..2,
// r6 scale 0..3, r7 clamp (any u32, nonzero is true), r8 output register 0..3.
template <>
struct KnownNativeCpuCall<0x80171D60u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_color_op(cpu);
    }
};

// GXSetTevAlphaIn: r3 stage, r4..r7 alpha inputs 0..7.
template <>
struct KnownNativeCpuCall<0x80171D20u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_alpha_in(cpu);
    }
};

// GXSetTevAlphaOp: r3 stage, r4 op {0,1,14,15}, r5 bias 0..2,
// r6 scale 0..3, r7 clamp (any u32, nonzero is true), r8 output register 0..3.
template <>
struct KnownNativeCpuCall<0x80171DB8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_alpha_op(cpu);
    }
};

// GXSetTevSwapMode: r3 stage, r4 raster selector and r5 texture selector 0..3.
template <>
struct KnownNativeCpuCall<0x80171FD0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_swap_mode(cpu);
    }
};

// Bounded TEV color family. Pointer setters validate raw ID 0..3 before
// resolving all four RGBA bytes. No CPU/guest writes or frame helpers.
// GXSetTevKColor: r3 ID 0..3, r4 readable four-byte guest RGBA pointer.
template <>
struct KnownNativeCpuCall<0x80171ED4u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_k_color(cpu);
    }
};

// GXSetTevColor: r3 register ID 0..3, r4 guest RGBA pointer.
template <>
struct KnownNativeCpuCall<0x80171E10u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_color(cpu);
    }
};

// GXSetTevSwapModeTable: r3 ID and r4..r7 channel enums, all 0..3.
template <>
struct KnownNativeCpuCall<0x8017200Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_swap_mode_table(cpu);
    }
};

// GXSetAlphaCompare: r3/r6 compare enums 0..7, r5 operator 0..3;
// r4/r7 references use the pinned u8 conversion. Rendered mode publishes
// the existing alpha-compare validity flag before native forwarding.
template <>
struct KnownNativeCpuCall<0x80172088u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_alpha_compare(cpu);
    }
};

// GXSetFog: exact captured type-0 f64 tuple in f1..f4; complete color at r4.
// Other tuples abort before narrowing, memory lookup or native forwarding.
template <>
struct KnownNativeCpuCall<0x801722CCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_fog(cpu);
    }
};

// GXSetZCompLoc: r3 uses the pinned TARGET_PC full-word GXBool conversion.
// Prepared from the checked caller; console return is not yet established.
template <>
struct KnownNativeCpuCall<0x80172858u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_z_comp_loc(cpu);
    }
};

// GXSetTevOp (PAL 0x80171C4C). Pinned WiiCompiled consumes r3/r4 as
// TEV-stage id / TEV mode, rejects a stage outside GX_MAX_TEVSTAGE, then
// forwards the two enum values to Aurora GXSetTevOp.
template <>
struct KnownNativeCpuCall<0x80171C4Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_op(cpu);
    }
};

// GXSetTevOrder (PAL 0x8017214C). Pinned WiiCompiled consumes r3..r6 as
// TEV-stage id / texcoord id / texmap id / channel id, rejects a stage outside
// GX_MAX_TEVSTAGE, then forwards the four enum values to Aurora GXSetTevOrder.
template <>
struct KnownNativeCpuCall<0x8017214Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tev_order(cpu);
    }
};

// GXSetBlendMode (PAL 0x8017277C). Pinned WiiCompiled consumes r3..r6 as
// blend-mode type / source factor / destination factor / logic op and forwards
// the four enum values directly to Aurora GXSetBlendMode.
template <>
struct KnownNativeCpuCall<0x8017277Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_blend_mode(cpu);
    }
};

// GXSetColorUpdate (PAL 0x801727CC). Pinned WiiCompiled consumes r3 as the
// color-update enable value, casts it directly to GXBool, and forwards it to
// Aurora GXSetColorUpdate.
template <>
struct KnownNativeCpuCall<0x801727CCu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_color_update(cpu);
    }
};

// GXSetAlphaUpdate (PAL 0x801727F8). Pinned WiiCompiled consumes r3 as the
// alpha-update enable value, casts it directly to GXBool, and forwards it to
// Aurora GXSetAlphaUpdate.
template <>
struct KnownNativeCpuCall<0x801727F8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_alpha_update(cpu);
    }
};

// GXSetZMode (PAL 0x80172824). Pinned WiiCompiled consumes
// r3/r4/r5 = compare-enable / compare-function / update-enable, casts them
// directly to GXBool/GXCompare/GXBool, and forwards them to Aurora GXSetZMode.
template <>
struct KnownNativeCpuCall<0x80172824u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_z_mode(cpu);
    }
};

// GXSetPixelFmt (PAL 0x80172888). Discovery hardware reaches this exact
// pixel-format boundary after crossing GXInvalidateTexAll, with r3=1/r4=0.
// Pinned WiiCompiled casts r3/r4 directly to GXPixelFmt/GXZFmt16 and forwards
// them to Aurora GXSetPixelFmt.
template <>
struct KnownNativeCpuCall<0x80172888u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_pixel_fmt(cpu);
    }
};

// GXSetCullMode (PAL 0x8016F3B8). Pinned WiiCompiled consumes r3 as the
// cull-mode enum and forwards it directly to Aurora GXSetCullMode.
template <>
struct KnownNativeCpuCall<0x8016F3B8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_cull_mode(cpu);
    }
};

// GXSetCoPlanar (PAL 0x8016F3E0). Discovery hardware reaches this boundary
// after GXSetPixelFmt returns, with r3=0. Pinned WiiCompiled casts r3 directly
// to GXBool and forwards it to Aurora GXSetCoPlanar.
template <>
struct KnownNativeCpuCall<0x8016F3E0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_co_planar(cpu);
    }
};

// GXSetClipMode (PAL 0x8017351C). Hardware reaches this boundary with
// r3=0 after GXSetCoPlanar. Pinned WiiCompiled forwards r3 as GXClipMode.
template <>
struct KnownNativeCpuCall<0x8017351Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_clip_mode(cpu);
    }
};

// GXSetIndTexMtx (PAL 0x80171814). Hardware captured r3=1, r4=0x802581F8,
// r5=1. Pinned WiiCompiled decodes six guest float32 coefficients, forwards
// r3 as GXIndTexMtxID and narrows r5 to s8. Invalid memory/coefficients stop.
template <>
struct KnownNativeCpuCall<0x80171814u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_ind_tex_mtx(cpu);
    }
};

// GXSetIndTexCoordScale (PAL 0x80171968). Audited adjacent scalar setter:
// pinned WiiCompiled forwards r3/r4/r5 as stage/S-scale/T-scale enums.
template <>
struct KnownNativeCpuCall<0x80171968u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_ind_tex_coord_scale(cpu);
    }
};

// GXSetDither (PAL 0x80172930). Pre-ported in the audited scalar batch:
// pinned WiiCompiled forwards r3 as GXBool.
template <>
struct KnownNativeCpuCall<0x80172930u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_dither(cpu);
    }
};

// GXSetDstAlpha (PAL 0x8017295C). Pre-ported in the audited scalar batch:
// pinned WiiCompiled forwards r3 as GXBool and r4 as u8.
template <>
struct KnownNativeCpuCall<0x8017295Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_dst_alpha(cpu);
    }
};

// GXBegin (PAL 0x8016F0F0). Pinned WiiCompiled consumes
// r3/r4/r5 = primitive / vertex-format / vertex-count. The immediate-mode
// path republishes the tracked vertex state, initializes HleFifoWrite's begin
// state, and lets subsequent real FIFO payload produce the Aurora draw.
template <>
struct KnownNativeCpuCall<0x8016F0F0u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_begin(cpu);
    }
};

// GX display-list recording pair. Hardware has returned from Begin and End.
// Sphere is bounded to the two audited constructor variants while recording.
template <>
struct KnownNativeCpuCall<0x80172A30u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_draw_sphere(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x80172E00u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_begin_display_list(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x80172EB4u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_end_display_list(cpu);
    }
};

// GXInitTexObj (PAL 0x801707F8). Pinned WiiCompiled consumes
// r3..r10 = guest GXTexObj / image data / width / height / format / wrapS /
// wrapT / mipmap, constructs the Aurora host texture object, and mirrors the
// 32-byte SDK GXTexObj layout back into guest RAM.
template <>
struct KnownNativeCpuCall<0x801707F8u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_init_tex_obj(cpu);
    }
};

// GXSetChanAmbColor (PAL 0x8017039C). Hardware captured channel 4 in r3 and
// guest color pointer 0x80398FD0 in r4. Preserve frame activation, decode
// the big-endian RGBA word, and forward the unchanged channel to Aurora.
template <>
struct KnownNativeCpuCall<0x8017039Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_chan_amb_color(cpu);
    }
};

// GXInitTexObjLOD (PAL 0x80170A4C). The normal rendered build validates
// the complete tiled descriptor, including the hardware-observed Z24X8 layout.
// An optional legacy strict mode retains its earlier exact tuple allowlist.
template <>
struct KnownNativeCpuCall<0x80170A4Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_init_tex_obj_lod(cpu);
    }
};

// GXInitTexObjWrapMode (PAL 0x80170B50). Hardware has now captured eleven
// exact tuples immediately after proven GXInitTexObjLOD calls:
// obj=0x9018E120, obj=0x9018E460, obj=0x9018E140, obj=0x908FA4E0,
// obj=0x907938A0, obj=0x908FA5C0, obj=0x908FA820, obj=0x909019C0,
// obj=0x908FA840, obj=0x9018E480 and obj=0x908FAE00, all with
// wrapS=GX_CLAMP / wrapT=GX_CLAMP. The bridge requires each exact post-LOD
// descriptor before applying the pinned guest/Aurora mutation.
template <>
struct KnownNativeCpuCall<0x80170B50u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_init_tex_obj_wrap_mode(cpu);
    }
};

// GXInvalidateTexAll (PAL 0x80171110). Discovery hardware reaches this
// no-argument texture-cache invalidation boundary after 604,194 translated
// dispatches. Pinned WiiCompiled forwards it directly to Aurora's
// GXInvalidateTexAll with no guest-register return value.
template <>
struct KnownNativeCpuCall<0x80171110u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_invalidate_tex_all(cpu);
    }
};

// GXLoadTexObj (PAL 0x80170F2C). Hardware captures the first load with
// r3=0x901136B4 / r4=0 and a complete 32-byte non-CI descriptor:
// 832x456, format 4, clamp/clamp, no mipmaps, backing 0x00F103E0.
// The bridge accepts only that exact observed descriptor; any later variation
// remains a fresh hardware-defined blocker.
template <>
struct KnownNativeCpuCall<0x80170F2Cu> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_load_tex_obj(cpu);
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

// GXSetCopyFilter (PAL 0x8016FA40). Pinned WiiCompiled consumes
// r3/r4/r5/r6 = antialias / sample-pattern guest pointer / vertical-filter
// enable / vertical-filter guest pointer. It copies 24 + 7 bytes from guest
// RAM when the respective pointer is non-zero, then forwards the local arrays
// and GXBool values to Aurora GXSetCopyFilter.
template <>
struct KnownNativeCpuCall<0x8016FA40u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_copy_filter(cpu);
    }
};

// GXFlush (PAL 0x8016E654). Pinned WiiCompiled has no PPC arguments and
// forwards directly to Aurora GXFlush. This is the first exact blocker exposed
// after the first successful real RMCP01 GXCopyDisp/present.
template <>
struct KnownNativeCpuCall<0x8016E654u> {
    static constexpr bool kAvailable = true;

    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_flush(cpu);
    }
};

// Bounded copy configuration: native state and pinned guest/HLE mirrors.
// Headless calls refuse; synthetic probes retain these dispatches only.
template <>
struct KnownNativeCpuCall<0x8016F618u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_copy_clamp(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x8016F478u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tex_copy_src(cpu);
    }
};
template <>
struct KnownNativeCpuCall<0x8016F4DCu> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_set_tex_copy_dst(cpu);
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

// Hardware-observed Mii RGB5A3 EFB copy; unknown state still refuses.
template <>
struct KnownNativeCpuCall<0x8016FD74u> {
    static constexpr bool kAvailable = true;
    static inline void Invoke(CpuContext* cpu) noexcept {
        mkw_switch_hle_gx_copy_tex(cpu);
    }
};
