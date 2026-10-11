#include <cstdlib>

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION) ||           \
    (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK)
#include "abi_bridge.h"
#include "switch_nand_seek_runtime.hpp"

#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
void Status(std::uint32_t fileInfo, std::uint32_t offset, std::uint32_t whence,
            const mkw::switch_nand_runtime::RewindResult& out) noexcept {
    if (FILE* file = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-seek-status.txt", "w")) {
        std::fprintf(file, "status=%s\nfileinfo_ptr=0x%08x\noffset=0x%08x\nwhence=0x%08x\n"
                           "fd=%d\nmode=%d\naccess_type=%u\nopen_flag=%u\n",
                     !out.admitted ? "unproven-seek" : out.result == 0 ? "seek-pass"
                                                                       : "seek-error",
                     fileInfo, offset, whence, out.fd, out.mode, out.accessType, out.openFlag);
        if (out.admitted)
            std::fprintf(file, "result=%d\n", out.result);
        else
            std::fputs("result=NOT_CALLED\n", file);
        std::fclose(file);
    }
}
} // namespace
extern "C" void mkw_switch_hle_nand_seek(CpuContext* cpu) {
    if (!cpu)
        return;
    const auto fileInfo = cpu->gpr[3], offset = cpu->gpr[4], whence = cpu->gpr[5];
    const auto out = mkw::switch_nand_runtime::RewindOwnedSync(fileInfo, offset, whence);
    if (!out.admitted) {
        Status(fileInfo, offset, whence, out);
        mkw_switch_report_unsupported_translated_dispatch("NAND_SEEK_UNPROVEN_CALL", 0x8019b964u, cpu);
        std::abort();
    }
    mkw_switch_set_fast_track_stage("RMCP01_NAND_SEEK");
    cpu->gpr[3] = static_cast<std::uint32_t>(out.result);
    Status(fileInfo, offset, whence, out);
}
#else
struct CpuContext;
extern "C" void mkw_switch_hle_nand_seek(CpuContext*) {
    std::abort();
}
#endif
