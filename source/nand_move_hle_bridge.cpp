#include <cstdlib>

#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION) ||           \
    (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK)
#include "abi_bridge.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <system_error>

extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept;
namespace {
constexpr std::uint32_t target = 0x8019bee8u;
constexpr char sourcePath[] = "/tmp/banner.bin";
constexpr char directoryPath[] = "/title/00010004/524d4350/data";

template <std::size_t N>
bool Match(std::uint32_t pointer, const char (&expected)[N],
           std::array<std::uint32_t, (N + 3) / 4>& words) noexcept {
    if (!pointer || !Memory::Contains(pointer, N))
        return false;
    bool match = true;
    for (std::size_t i = 0; i < N; ++i) {
        const auto byte = Memory::Read8(pointer + static_cast<std::uint32_t>(i));
        words[i / 4] |= std::uint32_t(byte) << (24 - 8 * (i % 4));
        match &= byte == static_cast<std::uint8_t>(expected[i]);
    }
    return match;
}

void Status(std::uint32_t source, std::uint32_t directory, bool sourceMatch,
            bool directoryMatch, const std::array<std::uint32_t, 4>& sourceWords,
            const std::array<std::uint32_t, 8>& directoryWords,
            bool called, std::int32_t result) noexcept {
    if (FILE* out = std::fopen("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-move-status.txt", "w")) {
        std::fprintf(out, "status=%s\nsource_ptr=0x%08x\ndirectory_ptr=0x%08x\n"
                          "source_match=%s\ndirectory_match=%s\nsource_words=",
                     !called ? "unproven-paths" : result == 0 ? "move-pass"
                                              : result == -6  ? "move-exists"
                                                              : "move-error",
                     source, directory, sourceMatch ? "YES" : "NO", directoryMatch ? "YES" : "NO");
        for (std::size_t i = 0; i < sourceWords.size(); ++i)
            std::fprintf(out, "%s%08x", i ? "/" : "", sourceWords[i]);
        std::fputs("\ndirectory_words=", out);
        for (std::size_t i = 0; i < directoryWords.size(); ++i)
            std::fprintf(out, "%s%08x", i ? "/" : "", directoryWords[i]);
        if (called)
            std::fprintf(out, "\nresult=%d\n", result);
        else
            std::fputs("\nresult=NOT_CALLED\n", out);
        std::fclose(out);
    }
}
} // namespace

// Pinned NANDMove appends the source basename to a destination directory.
// Admit only the observed temporary banner and PAL title-home paths after
// checking every live byte, including both terminators. Other moves abort.
extern "C" void mkw_switch_hle_nand_move(CpuContext* cpu) noexcept {
    if (!cpu)
        return;
    const auto source = cpu->gpr[3], directory = cpu->gpr[4];
    std::array<std::uint32_t, 4> sourceWords{};
    std::array<std::uint32_t, 8> directoryWords{};
    const bool sourceMatch = Match(source, sourcePath, sourceWords);
    const bool directoryMatch = Match(directory, directoryPath, directoryWords);
    if (!sourceMatch || !directoryMatch) {
        Status(source, directory, sourceMatch, directoryMatch, sourceWords, directoryWords, false, 0);
        mkw_switch_report_unsupported_translated_dispatch("NAND_MOVE_UNPROVEN_PATHS", target, cpu);
        std::abort();
    }
    mkw_switch_set_fast_track_stage("RMCP01_NAND_MOVE");
    std::int32_t result = -64;
    try {
        const auto root = mkw::horizon_runtime_services::nand_root();
        const auto srcHost = root / "tmp" / "banner.bin";
        const auto dstDirectoryHost = root / "title" / "00010004" / "524d4350" / "data";
        const auto dstHost = dstDirectoryHost / srcHost.filename();
        std::error_code error;
        if (!std::filesystem::exists(srcHost, error) ||
            !std::filesystem::is_directory(dstDirectoryHost, error)) {
            result = -12;
        } else if (std::filesystem::exists(dstHost, error)) {
            result = -6;
        } else {
            std::filesystem::rename(srcHost, dstHost, error);
            result = error ? -64 : 0;
        }
    } catch (...) {
        // Preserve the virtual NAND unknown-error result on host failure.
    }
    cpu->gpr[3] = static_cast<std::uint32_t>(result);
    Status(source, directory, sourceMatch, directoryMatch, sourceWords, directoryWords, true, result);
}
#else
struct CpuContext;
extern "C" void mkw_switch_hle_nand_move(CpuContext*) noexcept {
    std::abort();
}
#endif
