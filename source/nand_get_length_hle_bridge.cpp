#include <cstdlib>
#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION) || (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK)
#include "abi_bridge.h"
#include "switch_nand_length_runtime.hpp"
#include <cstdio>
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
void Status(std::uint32_t info, std::uint32_t output, const mkw::switch_nand_runtime::LengthResult& out) noexcept {
    if (FILE* file = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-get-length-status.txt", "w")) {
        std::fprintf(file, "status=%s\nfileinfo_ptr=0x%08x\noutput_ptr=0x%08x\nfd=%d\nmode=%d\naccess_type=%u\nopen_flag=%u\n", out.admitted ? "length-pass" : "unproven-length", info, output, out.fd, out.mode, out.accessType, out.openFlag);
        if (out.admitted)
            std::fprintf(file, "result=%d\nlength=%u\noriginal_position=%ld\nend_seek_result=%d\nrestore_seek_result=%d\n", out.result, out.length, out.position, out.endSeek, out.restoreSeek);
        else
            std::fputs("result=NOT_CALLED\n", file);
        std::fclose(file);
    }
}
} // namespace
extern "C" void mkw_switch_hle_nand_get_length(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    const auto info = cpu->gpr[3], output = cpu->gpr[4];
    const auto out = mkw::switch_nand_runtime::LengthOwnedReadSync(info, output);
    if (!out.admitted) {
        Status(info, output, out);
        mkw_switch_report_unsupported_translated_dispatch("NAND_GET_LENGTH_UNPROVEN_CALL", 0x8019bf4cu, cpu);
        std::abort();
    }
    mkw_switch_set_fast_track_stage("RMCP01_NAND_GET_LENGTH");
    cpu->gpr[3] = static_cast<std::uint32_t>(out.result);
    Status(info, output, out);
}
#else
struct CpuContext;
extern "C" void mkw_switch_hle_nand_get_length(CpuContext*) noexcept {
    std::abort();
}
#endif
