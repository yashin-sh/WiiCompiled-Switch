// Registry link seam for unrelated-provider contracts; any execution fails.
#include <cstdlib>
struct CpuContext;
extern "C" void mkw_switch_hle_lyt_draw_quad(CpuContext*) {
    std::abort();
}
extern "C" void mkw_switch_hle_nand_create(CpuContext*) {
    std::abort();
}

extern "C" void mkw_switch_hle_nand_write(CpuContext*) {
    std::abort();
}

extern "C" void mkw_switch_hle_nand_move(CpuContext*) {
    std::abort();
}

extern "C" void mkw_switch_hle_nand_seek(CpuContext*) {
    std::abort();
}

extern "C" void mkw_switch_hle_nand_get_length(CpuContext*) {
    std::abort();
}

extern "C" void mkw_switch_hle_gx_set_tev_indirect(CpuContext*) {
    std::abort();
}
