#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uint32_t base = 0x70000000u, source = base + 16, directory = base + 128;
constexpr std::uint32_t target = 0x8019bee8u, size = 4096;
constexpr char sourcePath[] = "/tmp/banner.bin", directoryPath[] = "/title/00010004/524d4350/data";
std::vector<std::uint8_t> bytes, refusedMemory;
bool active = false, failRename = false;
fs::path root, oracleRoot;
unsigned valid = 0, refusals = 0, notes = 0, polls = 0, roots = 0, expectedRoots = 0;
const char* stage = nullptr;
CpuContext refusedCpu{};
int proofFd = -1;
fs::path Home(const fs::path& p) {
    return p / "title/00010004/524d4350/data";
}
std::string Read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& p, const std::string& s) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << s;
    assert(out.good());
}
CpuContext Cpu(std::uint32_t src = source, std::uint32_t dst = directory) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = src;
    cpu.gpr[4] = dst;
    return cpu;
}
void Paths(std::uint32_t src = source, std::uint32_t dst = directory) {
    std::fill(bytes.begin(), bytes.end(), 0xa5);
    std::memcpy(bytes.data() + src - base, sourcePath, sizeof(sourcePath));
    std::memcpy(bytes.data() + dst - base, directoryPath, sizeof(directoryPath));
}
void Setup() {
    for (const auto& p : {root, oracleRoot}) {
        fs::remove_all(p);
        fs::create_directories(Home(p));
        Write(p / "tmp/banner.bin", std::string("binary\0payload\xff", 15));
        Write(p / "unrelated.bin", "preserve unrelated data");
    }
    Paths();
}
std::int32_t Invoke(CpuContext cpu) {
    auto expected = cpu;
    const auto memory = bytes;
    const auto oldNotes = notes, oldPolls = polls, oldRoots = roots;
    InvokeDirectCpu<target>(&cpu);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&expected, &cpu, sizeof(cpu)) == 0 && bytes == memory);
    assert(notes == oldNotes + 1 && polls == oldPolls + 1 && roots == oldRoots + 1);
    assert(stage && std::strcmp(stage, "RMCP01_NAND_MOVE") == 0);
    const auto result = static_cast<std::int32_t>(cpu.gpr[3]);
    const auto status = Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-move-status.txt");
    assert(status.find("source_match=YES\ndirectory_match=YES\n") != std::string::npos);
    assert(status.find("result=" + std::to_string(result) + "\n") != std::string::npos);
    ++valid;
    return result;
}
void Refusal(CpuContext cpu) {
    refusedCpu = cpu;
    refusedMemory = bytes;
    expectedRoots = roots;
    const auto payload = Read(root / "tmp/banner.bin");
    assert(!fs::exists(Home(root) / "banner.bin"));
    int fds[2];
    assert(pipe(fds) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (!child) {
        close(fds[0]);
        proofFd = fds[1];
        const rlimit noCore{0, 0};
        assert(setrlimit(RLIMIT_CORE, &noCore) == 0 && prctl(PR_SET_DUMPABLE, 0) == 0);
        InvokeDirectCpu<target>(&cpu);
        _exit(9);
    }
    close(fds[1]);
    char proof = 0;
    assert(read(fds[0], &proof, 1) == 1 && proof == 'P');
    close(fds[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(Read(root / "tmp/banner.bin") == payload && !fs::exists(Home(root) / "banner.bin"));
    assert(Read(root / "unrelated.bin") == "preserve unrelated data");
    ++refusals;
}
} // namespace

namespace Pinned {
constexpr std::int32_t NAND_RESULT_OK = 0, NAND_RESULT_INVALID = -8;
constexpr std::int32_t NAND_RESULT_EXISTS = -6, NAND_RESULT_NOEXISTS = -12, NAND_RESULT_UNKNOWN = -64;
fs::path TranslateNandPath(const char* p) {
    if (std::strcmp(p, sourcePath) == 0)
        return oracleRoot / "tmp/banner.bin";
    assert(std::strcmp(p, directoryPath) == 0);
    return Home(oracleRoot);
}
bool PathExists(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p, ec);
}
bool IsDirectory(const fs::path& p) {
    std::error_code ec;
    return fs::is_directory(p, ec);
}
template <typename... Args>
void LogNandError(const char*, const char*, Args...) {}
#include "pinned-nand-move.inc"
} // namespace Pinned
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == size);
    bytes.assign(size, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t p) {
    return p == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
namespace mkw::horizon_runtime_services {
fs::path nand_root() {
    ++roots;
    return root;
}
} // namespace mkw::horizon_runtime_services
// Wrap the actual error-code filesystem overload used by both bodies. Wrapping
// libc rename alone cannot intercept calls inside a shared libstdc++ library.
extern "C" void RealRename(const fs::path&, const fs::path&, std::error_code&) noexcept
    asm("__real__ZNSt10filesystem6renameERKNS_7__cxx114pathES3_RSt10error_code");
extern "C" void WrappedRename(const fs::path& src, const fs::path& dst, std::error_code& error) noexcept
    asm("__wrap__ZNSt10filesystem6renameERKNS_7__cxx114pathES3_RSt10error_code");
extern "C" void WrappedRename(const fs::path& src, const fs::path& dst, std::error_code& error) noexcept {
    if (failRename) {
        error = std::make_error_code(std::errc::io_error);
        return;
    }
    RealRename(src, dst, error);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* p) noexcept {
    stage = p;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t t, CpuContext*) noexcept {
    assert(t == target);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t t, CpuContext* cpu) noexcept {
    assert(t == target && std::strcmp(reason, "NAND_MOVE_UNPROVEN_PATHS") == 0);
    assert(std::memcmp(cpu, &refusedCpu, sizeof(*cpu)) == 0 && bytes == refusedMemory && roots == expectedRoots);
    assert(Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-move-status.txt").find("result=NOT_CALLED\n") != std::string::npos);
    assert(write(proofFd, "P", 1) == 1);
}
#define UNEXERCISED(function)                        \
    extern "C" void function(CpuContext*) noexcept { \
        std::abort();                                \
    }
UNEXERCISED(mkw_switch_hle_gx_load_light_obj_imm)
UNEXERCISED(mkw_switch_hle_gx_load_nrm_mtx_imm)
UNEXERCISED(mkw_switch_hle_gx_set_z_texture)
UNEXERCISED(mkw_switch_hle_gx_set_tev_color_s10)
UNEXERCISED(mkw_switch_hle_lyt_draw_quad)
UNEXERCISED(mkw_switch_hle_nand_create)
UNEXERCISED(mkw_switch_hle_nand_write)
#undef UNEXERCISED

int main() {
    static_assert(!KnownNativeCpuCall<target>::kAvailable);
    const auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry);
    entry(nullptr);
    assert(roots == 0);
    Memory::Config config;
    config.regions = {{"synthetic-move", base, size}};
    Memory::Init(config);
    root = fs::absolute("production");
    oracleRoot = fs::absolute("oracle");
    for (unsigned offset = 0; offset < 10; ++offset) {
        Setup();
        const auto src = offset == 8 ? base + size - sizeof(sourcePath) : source + (offset % 8);
        const auto dst = offset == 9 ? base + size - sizeof(directoryPath) : directory + (offset % 8);
        Paths(src, dst);
        assert(Pinned::NANDMove_HLE(src, dst) == 0 && Invoke(Cpu(src, dst)) == 0);
        assert(!fs::exists(root / "tmp/banner.bin") && !fs::exists(oracleRoot / "tmp/banner.bin"));
        assert(Read(Home(root) / "banner.bin") == Read(Home(oracleRoot) / "banner.bin"));
        assert(Read(Home(root) / "banner.bin") == std::string("binary\0payload\xff", 15));
        assert(Read(root / "unrelated.bin") == "preserve unrelated data");
    }
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        Setup();
        for (const auto& p : {root, oracleRoot}) {
            if (scenario == 0)
                fs::remove(p / "tmp/banner.bin");
            if (scenario == 1 || scenario == 2)
                fs::remove_all(Home(p));
            if (scenario == 2)
                Write(Home(p), "directory occupied by file");
            if (scenario == 1 || scenario == 2)
                assert(Read(root / "tmp/banner.bin") == std::string("binary\0payload\xff", 15));
            if (scenario == 3)
                Write(Home(p) / "banner.bin", "existing destination");
            if (scenario == 4)
                fs::create_directory(Home(p) / "banner.bin");
            if (scenario == 6) {
                fs::remove(p / "tmp/banner.bin");
                Write(p / "tmp/banner.bin/child", "directory child");
            }
        }
        failRename = scenario == 5;
        const auto expected = scenario < 3 ? -12 : scenario < 5 ? -6
                                               : scenario == 5  ? -64
                                                                : 0;
        const auto oracle = Pinned::NANDMove_HLE(source, directory);
        const auto actual = Invoke(Cpu());
        assert(oracle == expected && actual == expected);
        failRename = false;
        if (scenario == 1 || scenario == 2)
            assert(Read(root / "tmp/banner.bin") == std::string("binary\0payload\xff", 15));
        if (scenario == 3)
            assert(Read(Home(root) / "banner.bin") == "existing destination");
        if (scenario == 4)
            assert(fs::is_directory(Home(root) / "banner.bin"));
        if (scenario >= 3 && scenario <= 5)
            assert(Read(root / "tmp/banner.bin") == std::string("binary\0payload\xff", 15));
        if (scenario == 6)
            assert(Read(Home(root) / "banner.bin/child") == "directory child");
        assert(Read(root / "unrelated.bin") == "preserve unrelated data");
    }
    Setup();
    for (const auto pair : {std::pair{source, sizeof(sourcePath)}, std::pair{directory, sizeof(directoryPath)}}) {
        for (unsigned i = 0; i < pair.second; ++i) {
            bytes[pair.first - base + i] ^= 1;
            Refusal(Cpu());
            bytes[pair.first - base + i] ^= 1;
        }
    }
    for (const auto p : {0u, 0xffffffffu, base - 1u, base + size - static_cast<unsigned>(sizeof(sourcePath)) + 1u})
        Refusal(Cpu(p));
    for (const auto p : {0u, 0xffffffffu, base - 1u, base + size - static_cast<unsigned>(sizeof(directoryPath)) + 1u})
        Refusal(Cpu(source, p));
    Memory::Reset();
    std::printf("PASS: NANDMove valid=%u refusals=%u; production dispatch/Memory, pinned oracle, basename-to-directory rename, binary/existing/error preservation, complete CPU/guest snapshots\n", valid, refusals);
}
