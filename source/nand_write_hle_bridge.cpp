#include <cstdlib>

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION) ||           \
    (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK)
#include "abi_bridge.h"
#include "memory.h"
#include "switch_nand_write_runtime.hpp"

#include <cstdio>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
void Status(std::uint32_t fileInfo, std::uint32_t buffer, std::uint32_t length,
            const mkw::switch_nand_runtime::BannerWriteResult& result) noexcept {
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-write-status.txt", "w")) {
        std::fprintf(out, "status=%s\nfileinfo_ptr=0x%08x\nbuffer_ptr=0x%08x\nlength=0x%08x\n"
                          "fd=%d\nmode=%d\nopen_flag=%u\n",
                     !result.admitted ? "unproven-write" : result.result == 0x72a0 ? "write-pass"
                                                                                   : "write-short",
                     fileInfo, buffer, length, result.fd, result.mode, result.openFlag);
        if (result.admitted)
            std::fprintf(out, "result=%d\n", result.result);
        else
            std::fputs("result=NOT_CALLED\n", out);
        std::fclose(out);
    }
}
} // namespace
extern "C" void mkw_switch_hle_nand_write(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    const auto fileInfo = cpu->gpr[3], buffer = cpu->gpr[4], length = cpu->gpr[5];
    const auto result = mkw::switch_nand_runtime::WriteBannerSync(fileInfo, buffer, length);
    if (!result.admitted) {
        Status(fileInfo, buffer, length, result);
        mkw_switch_report_unsupported_translated_dispatch("NAND_WRITE_UNPROVEN_CALL", 0x8019b884u, cpu);
        std::abort();
    }
    mkw_switch_set_fast_track_stage("RMCP01_NAND_WRITE");
    cpu->gpr[3] = static_cast<std::uint32_t>(result.result);
    Status(fileInfo, buffer, length, result);
}
#else
struct CpuContext;
extern "C" void mkw_switch_hle_nand_write(CpuContext*) noexcept {
    std::abort();
}
#endif
