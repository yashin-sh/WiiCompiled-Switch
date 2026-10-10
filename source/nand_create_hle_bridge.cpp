#include <cstdlib>

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION) ||           \
    (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK)
#include "abi_bridge.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;

namespace {
constexpr std::uint32_t target = 0x8019b43cu;
constexpr char expectedPath[] = "/tmp/banner.bin";
constexpr char statusPath[] = "sdmc:/switch/WiiCompiled-Switch/fast-track-nand-create-status.txt";

void Status(const char* state, std::uint32_t path, std::uint32_t permission,
            std::uint32_t attributes, bool readable,
            const std::array<std::uint32_t, 4>& words, std::int32_t result,
            bool called = false) noexcept {
    if (FILE* out = std::fopen(statusPath, "w")) {
        std::fprintf(out, "status=%s\npath_ptr=0x%08x\npermission=0x%08x\nattributes=0x%08x\n"
                          "path_readable=%s\npath_words=%08x/%08x/%08x/%08x\n",
                     state, path, permission, attributes, readable ? "YES" : "NO",
                     words[0], words[1], words[2], words[3]);
        if (called)
            std::fprintf(out, "result=%d\n", result);
        else
            std::fputs("result=NOT_CALLED\n", out);
        std::fclose(out);
    }
}

[[noreturn]] void Refuse(const char* reason, CpuContext* cpu) noexcept {
    mkw_switch_report_unsupported_translated_dispatch(reason, target, cpu);
    std::abort();
}
} // namespace

// Pinned virtual NANDCreate ignores host permission/attribute mapping. Admit
// only the observed tuple and statically attributed temporary path after a
// complete live-byte check. Existing files return -6 without truncation.
extern "C" void mkw_switch_hle_nand_create(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    const auto path = cpu->gpr[3], permission = cpu->gpr[4], attributes = cpu->gpr[5];
    std::array<std::uint32_t, 4> words{};
    const bool readable = path != 0 && Memory::Contains(path, sizeof(expectedPath));
    bool match = readable;
    if (readable) {
        for (std::size_t i = 0; i < sizeof(expectedPath); ++i) {
            const auto byte = Memory::Read8(path + static_cast<std::uint32_t>(i));
            words[i / 4] |= std::uint32_t(byte) << (24 - 8 * (i % 4));
            match &= byte == static_cast<std::uint8_t>(expectedPath[i]);
        }
    }
    if (permission != 0x30u || attributes != 0u) {
        Status("unproven-args", path, permission, attributes, readable, words, 0);
        Refuse("NAND_CREATE_UNPROVEN_ARGS", cpu);
    }
    if (!match) {
        Status("unproven-path", path, permission, attributes, readable, words, 0);
        Refuse("NAND_CREATE_UNPROVEN_PATH", cpu);
    }

    mkw_switch_set_fast_track_stage("RMCP01_NAND_CREATE");
    std::int32_t result = -64;
    try {
        const auto host = mkw::horizon_runtime_services::nand_root() / "tmp" / "banner.bin";
        std::error_code error;
        std::filesystem::create_directories(host.parent_path(), error);
        // O_EXCL preserves the pinned existing-file result atomically, including
        // another creator racing between directory creation and this open.
        const int fd = ::open(host.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
        if (fd >= 0) {
            ::close(fd); // Pinned NANDCreate ignores the close result.
            result = 0;
        } else if (errno == EEXIST) {
            result = -6;
        }
    } catch (...) {
        // The virtual NAND API reports host failure as NAND_RESULT_UNKNOWN.
    }
    cpu->gpr[3] = static_cast<std::uint32_t>(result);
    Status(result == 0 ? "create-pass" : result == -6 ? "create-exists"
                                                      : "create-error",
           path, permission, attributes, readable, words, result, true);
}
#else
struct CpuContext;
extern "C" void mkw_switch_hle_nand_create(CpuContext*) noexcept {
    std::abort();
}
#endif
