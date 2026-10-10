// Registry link seam for unrelated-provider contracts; any execution fails.
#include <cstdlib>
struct CpuContext;
extern "C" void mkw_switch_hle_lyt_draw_quad(CpuContext*) noexcept {
    std::abort();
}
extern "C" void mkw_switch_hle_nand_create(CpuContext*) noexcept {
    std::abort();
}
