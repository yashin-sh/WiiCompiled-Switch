#include "abi_bridge.h"

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
extern "C" void mkw_switch_hle_gx_load_light_obj_imm(CpuContext*) noexcept;
#endif

extern "C" MkwSwitchNativeCpuExtension mkw_switch_find_missing_native_cpu_extension(
    std::uint32_t target) noexcept {
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)
    switch (target) {
    case 0x80170320u:
        return mkw_switch_hle_gx_load_light_obj_imm;
    default:
        break;
    }
#else
    (void)target;
#endif
    return nullptr;
}
