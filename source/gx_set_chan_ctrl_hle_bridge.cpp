#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"

#include <cstdint>

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include <dolphin/gx.h>
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

extern "C" void mkw_switch_hle_gx_set_chan_ctrl(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t channel = cpu->gpr[3];
    const std::uint32_t enable = cpu->gpr[4];
    const std::uint32_t ambientSource = cpu->gpr[5];
    const std::uint32_t materialSource = cpu->gpr[6];
    const std::uint32_t lightMask = cpu->gpr[7];
    const std::uint32_t diffuseFn = cpu->gpr[8];
    const std::uint32_t attenuationFn = cpu->gpr[9];

    mkw_switch_set_fast_track_stage("RMCP01_GX_SET_CHAN_CTRL");

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    GXSetChanCtrl(static_cast<GXChannelID>(channel),
                  enable != 0u,
                  static_cast<GXColorSrc>(ambientSource),
                  static_cast<GXColorSrc>(materialSource),
                  lightMask,
                  static_cast<GXDiffuseFn>(diffuseFn),
                  static_cast<GXAttnFn>(attenuationFn));
#else
    (void)channel;
    (void)enable;
    (void)ambientSource;
    (void)materialSource;
    (void)lightMask;
    (void)diffuseFn;
    (void)attenuationFn;
#endif
}

#endif
