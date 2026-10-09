#include <cstdint>

// The public probe does not load translated-runtime headers. The registry only
// needs the opaque context pointer; execution variants provide its definition.
struct CpuContext;
using NativeCpuExtension = void (*)(CpuContext*) noexcept;

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
extern "C" void mkw_switch_hle_gx_load_light_obj_imm(CpuContext*) noexcept;
extern "C" void mkw_switch_hle_gx_load_nrm_mtx_imm(CpuContext*) noexcept;
extern "C" void mkw_switch_hle_gx_set_z_texture(CpuContext*) noexcept;
extern "C" void mkw_switch_hle_gx_set_tev_color_s10(CpuContext*) noexcept;
#endif

extern "C" NativeCpuExtension mkw_switch_find_missing_native_cpu_extension(
    std::uint32_t target) noexcept {
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
    switch (target) {
    case 0x80170320u:
        return mkw_switch_hle_gx_load_light_obj_imm;
    case 0x80173188u:
        return mkw_switch_hle_gx_load_nrm_mtx_imm;
    case 0x801720c0u:
        return mkw_switch_hle_gx_set_z_texture;
    case 0x80171e70u:
        return mkw_switch_hle_gx_set_tev_color_s10;
    default:
        break;
    }
#else
    (void)target;
#endif
    return nullptr;
}
