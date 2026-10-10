#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <set>
#include <string>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uint32_t base = 0x70000000u, path = base + 16, target = 0x8019b43cu;
constexpr char name[] = "/tmp/banner.bin";
std::vector<std::uint8_t> bytes;
bool active = false, failOpen = false;
fs::path root;
unsigned rootCalls = 0, notes = 0, polls = 0, valid = 0, refusals = 0;
unsigned opens = 0, closes = 0;
std::set<int> descriptors;
const char* stage = nullptr;
CpuContext refusalCpu{};
std::vector<std::uint8_t> refusalMemory;
const char* refusalReason = nullptr;
int proofFd = -1;
unsigned expectedRootCalls = 0;

CpuContext Cpu(std::uint32_t pointer = path) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = pointer;
    cpu.gpr[4] = 0x30;
    cpu.gpr[5] = 0;
    return cpu;
}
void Path(std::uint32_t pointer = path) {
    std::fill(bytes.begin(), bytes.end(), 0xa5);
    assert(pointer >= base && pointer + sizeof(name) <= base + bytes.size());
    std::memcpy(bytes.data() + pointer - base, name, sizeof(name));
}
std::string Read(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& file, const std::string& value) {
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out << value;
    assert(out.good());
}
std::int32_t Invoke(CpuContext cpu) {
    const auto memory = bytes;
    auto expected = cpu;
    const auto oldNotes = notes, oldPolls = polls, oldRoots = rootCalls;
    InvokeDirectCpu<target>(&cpu);
    const auto result = static_cast<std::int32_t>(cpu.gpr[3]);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0 && bytes == memory);
    assert(notes == oldNotes + 1 && polls == oldPolls + 1 && rootCalls == oldRoots + 1);
    assert(stage && std::strcmp(stage, "RMCP01_NAND_CREATE") == 0);
    assert(descriptors.empty() && opens == closes);
    ++valid;
    return result;
}
void Refusal(CpuContext cpu, const char* reason) {
    refusalCpu = cpu;
    refusalMemory = bytes;
    refusalReason = reason;
    expectedRootCalls = rootCalls;
    const auto before = fs::exists(root / "tmp/banner.bin");
    const auto payload = before ? Read(root / "tmp/banner.bin") : std::string{};
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
    assert(fs::exists(root / "tmp/banner.bin") == before);
    if (before)
        assert(Read(root / "tmp/banner.bin") == payload);
    ++refusals;
}
} // namespace

namespace Pinned {
constexpr std::int32_t NAND_RESULT_OK = 0, NAND_RESULT_INVALID = -8;
constexpr std::int32_t NAND_RESULT_EXISTS = -6, NAND_RESULT_UNKNOWN = -64;
fs::path oracleRoot;
fs::path TranslateNandPath(const char* value) {
    assert(std::strcmp(value, name) == 0);
    return oracleRoot / "tmp" / "banner.bin";
}
bool CreateParentDirectories(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    return !ec;
}
bool PathExists(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p, ec);
}
FILE* NandFopen(const fs::path& p, const char* mode) {
    assert(std::strcmp(mode, "wb") == 0);
    return failOpen ? nullptr : std::fopen(p.c_str(), mode);
}
#include "pinned-nand-create.inc"
} // namespace Pinned

namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 128u);
    bytes.assign(128, 0xa5);
    active = true;
}
std::uint8_t* HostPointer(std::uint32_t address) {
    return address == base ? bytes.data() : nullptr;
}
void Shutdown() noexcept {
    active = false;
    bytes.clear();
}
} // namespace GuestFlat
namespace mkw::horizon_runtime_services {
fs::path nand_root() {
    ++rootCalls;
    return root;
}
} // namespace mkw::horizon_runtime_services

extern "C" int __real_open(const char*, int, ...);
extern "C" int __wrap_open(const char* p, int flags, ...) {
    assert(flags == (O_WRONLY | O_CREAT | O_EXCL));
    va_list args;
    va_start(args, flags);
    const int mode = va_arg(args, int);
    va_end(args);
    assert(mode == 0666);
    if (failOpen) {
        errno = EACCES;
        return -1;
    }
    const auto fd = __real_open(p, flags, mode);
    if (fd >= 0) {
        assert(descriptors.insert(fd).second);
        ++opens;
    }
    return fd;
}
extern "C" int __real_close(int);
extern "C" int __wrap_close(int fd) {
    if (descriptors.erase(fd))
        ++closes;
    return __real_close(fd);
}
extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t address, CpuContext*) noexcept {
    assert(address == target);
    ++notes;
}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {
    ++polls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason,
                                                                  std::uint32_t address,
                                                                  CpuContext* cpu) noexcept {
    assert(address == target && std::strcmp(reason, refusalReason) == 0);
    assert(std::memcmp(cpu, &refusalCpu, sizeof(*cpu)) == 0 && bytes == refusalMemory);
    assert(rootCalls == expectedRootCalls && descriptors.empty());
    assert(Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-create-status.txt").find("result=NOT_CALLED") != std::string::npos);
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
#undef UNEXERCISED

int main() {
    static_assert(!KnownNativeCpuCall<target>::kAvailable);
    auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    assert(rootCalls == 0);
    Memory::Config config;
    config.regions = {{"synthetic-create", base, 128}};
    Memory::Init(config);
    Path();
    for (unsigned test = 0; test < 5; ++test) {
        root = fs::absolute("production-" + std::to_string(test));
        Pinned::oracleRoot = fs::absolute("oracle-" + std::to_string(test));
        if (test == 1) {
            Write(root / "tmp/banner.bin", std::string("existing\0payload", 16));
            Write(Pinned::oracleRoot / "tmp/banner.bin", std::string("existing\0payload", 16));
        }
        if (test == 2) {
            fs::create_directories(root / "tmp/banner.bin");
            fs::create_directories(Pinned::oracleRoot / "tmp/banner.bin");
        }
        if (test == 3) {
            Write(root / "tmp", "blocked parent");
            Write(Pinned::oracleRoot / "tmp", "blocked parent");
        }
        failOpen = test == 4;
        const auto oracle = Pinned::NANDCreate_HLE(path, 0x30, 0);
        assert(oracle == (test == 0 ? 0 : test < 3 ? -6
                                                   : -64));
        assert(Invoke(Cpu()) == oracle);
        failOpen = false;
        if (test == 0 || test == 1)
            assert(Read(root / "tmp/banner.bin") == Read(Pinned::oracleRoot / "tmp/banner.bin"));
    }
    root = fs::absolute("scoped");
    Path();
    assert(Invoke(Cpu()) == 0);
    Write(root / "tmp/banner.bin", std::string("keep\0binary\xff", 12));
    const auto retained = Read(root / "tmp/banner.bin");
    for (unsigned offset = 0; offset < 8; ++offset) {
        Path(path + offset);
        assert(Invoke(Cpu(path + offset)) == -6);
        assert(Read(root / "tmp/banner.bin") == retained);
    }
    Path();
    for (unsigned bit = 0; bit < 32; ++bit) {
        auto cpu = Cpu();
        cpu.gpr[4] ^= 1u << bit;
        Refusal(cpu, "NAND_CREATE_UNPROVEN_ARGS");
        cpu = Cpu();
        cpu.gpr[5] = 1u << bit;
        Refusal(cpu, "NAND_CREATE_UNPROVEN_ARGS");
    }
    for (unsigned i = 0; i < sizeof(name); ++i) {
        Path();
        bytes[path - base + i] ^= 1;
        Refusal(Cpu(), "NAND_CREATE_UNPROVEN_PATH");
    }
    for (const auto pointer : {0u, 0xffffffffu, base - 1u, base + 113u})
        Refusal(Cpu(pointer), "NAND_CREATE_UNPROVEN_PATH");
    root = fs::absolute("concurrent");
    Path();
    pid_t children[2];
    for (auto& child : children) {
        child = fork();
        assert(child >= 0);
        if (!child) {
            const auto result = Invoke(Cpu());
            _exit(result == 0 ? 10 : result == -6 ? 11
                                                  : 12);
        }
    }
    unsigned created = 0, existed = 0;
    for (auto child : children) {
        int status = 0;
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status));
        created += WEXITSTATUS(status) == 10;
        existed += WEXITSTATUS(status) == 11;
    }
    assert(created == 1 && existed == 1 && fs::file_size(root / "tmp/banner.bin") == 0);
    Memory::Reset();
    std::printf("PASS: NANDCreate valid=%u refusals=%u; pinned errors, real filesystem, existing bytes, exclusive race, CPU/guest preservation\n", valid, refusals);
}
