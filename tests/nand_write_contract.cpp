#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "switch_nand_runtime.hpp"

#include <algorithm>
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
constexpr std::uint32_t base = 0x70000000, path = base + 16, info = base + 128;
constexpr std::uint32_t buffer = base + 1024, length = 0x72a0, target = 0x8019b884;
std::vector<std::uint8_t> bytes;
bool active = false, diskFull = false, allowFullClose = false;
fs::path root;
unsigned valid = 0, refusals = 0, notes = 0, polls = 0;
const char* stage = nullptr;
CpuContext refusedCpu{};
std::vector<std::uint8_t> refusedMemory;
int proofFd = -1;
std::string Read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& p, const std::string& s) {
    if (!p.parent_path().empty())
        fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << s;
    assert(out.good());
}
CpuContext Cpu(std::uint32_t fileInfo = info, std::uint32_t data = buffer) {
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = fileInfo;
    cpu.gpr[4] = data;
    cpu.gpr[5] = length;
    return cpu;
}
void Path(const char* value = "/tmp/banner.bin") {
    std::memcpy(bytes.data() + path - base, value, std::strlen(value) + 1);
}
void Payload(std::uint32_t ptr = buffer, unsigned seed = 0) {
    for (unsigned i = 0; i < length; ++i)
        bytes[ptr - base + i] = static_cast<std::uint8_t>((i * 113u + seed * 17u) ^ (i >> 5u));
}
std::int32_t Invoke(CpuContext cpu) {
    const auto memory = bytes;
    auto expected = cpu;
    const auto oldNotes = notes, oldPolls = polls;
    InvokeDirectCpu<target>(&cpu);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&expected, &cpu, sizeof(cpu)) == 0 && bytes == memory);
    assert(notes == oldNotes + 1 && polls == oldPolls + 1);
    assert(stage && std::strcmp(stage, "RMCP01_NAND_WRITE") == 0);
    const auto status = Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-write-status.txt");
    assert(status.find("result=" + std::to_string(static_cast<std::int32_t>(cpu.gpr[3]))) != std::string::npos);
    assert(status.find("mode=2\nopen_flag=1\n") != std::string::npos);
    ++valid;
    return static_cast<std::int32_t>(cpu.gpr[3]);
}
void Refusal(CpuContext cpu) {
    refusedCpu = cpu;
    refusedMemory = bytes;
    const auto retained = Read(root / "tmp/banner.bin");
    const auto other = Read(root / "other.bin");
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
    assert(Read(root / "tmp/banner.bin") == retained && Read(root / "other.bin") == other);
    ++refusals;
}
} // namespace

namespace Pinned {
constexpr std::int32_t NAND_RESULT_INVALID = -8;
struct FileHandle {
    FILE* file;
};
FileHandle handle{};
FileHandle* ResolveNandFileHandle(const char* who, std::uint32_t ptr) {
    assert(std::strcmp(who, "NANDWrite") == 0 && ptr >= base && Memory::Contains(ptr, 4));
    return &handle;
}
#include "pinned-nand-write.inc"
} // namespace Pinned
namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 65536);
    bytes.assign(65536, 0xa5);
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
    return root;
}
} // namespace mkw::horizon_runtime_services
extern "C" FILE* __real_fopen(const char*, const char*);
extern "C" FILE* __wrap_fopen(const char* p, const char* mode) {
    if (diskFull && fs::path(p) == root / "tmp/banner.bin") {
        assert(std::strcmp(mode, "r+b") == 0);
        FILE* out = __real_fopen("/dev/full", "wb");
        assert(out && setvbuf(out, nullptr, _IONBF, 0) == 0);
        return out;
    }
    return __real_fopen(p, mode);
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    // Permit cleanup only after checking the genuine /dev/full close failure.
    return diskFull && allowFullClose ? 0 : __real_fsync(fd);
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
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason,
                                                                  std::uint32_t t,
                                                                  CpuContext* cpu) noexcept {
    assert(t == target && std::strcmp(reason, "NAND_WRITE_UNPROVEN_CALL") == 0);
    assert(std::memcmp(cpu, &refusedCpu, sizeof(*cpu)) == 0 && bytes == refusedMemory);
    assert(Read("sdmc:/switch/WiiCompiled-Switch/fast-track-nand-write-status.txt").find("result=NOT_CALLED") != std::string::npos);
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
#undef UNEXERCISED

int main() {
    static_assert(!KnownNativeCpuCall<target>::kAvailable);
    auto entry = mkw_switch_find_missing_native_cpu_extension(target);
    assert(entry && !mkw_switch_find_missing_native_cpu_extension(0x12345678));
    entry(nullptr);
    Memory::Config config;
    config.regions = {{"synthetic-write", base, 65536}};
    Memory::Init(config);
    root = fs::absolute("production");
    Path();
    using namespace mkw::switch_nand_runtime;
    for (unsigned offset = 0; offset < 8; ++offset) {
        const auto fi = info + offset, data = buffer + offset;
        const std::string tail("tail\0binary\xff", 12);
        Write(root / "tmp/banner.bin", std::string(length, '\x5a') + tail);
        Write("oracle.bin", std::string(length, '\x5a') + tail);
        Payload(data, offset);
        assert(OpenSync(path, fi, 2) == 0);
        Pinned::handle.file = std::fopen("oracle.bin", "r+b");
        assert(Pinned::handle.file);
        const auto oracle = Pinned::NANDWrite_HLE(fi, data, length);
        assert(oracle == static_cast<std::int32_t>(length) && Invoke(Cpu(fi, data)) == oracle);
        assert(std::fclose(Pinned::handle.file) == 0);
        assert(Read(root / "tmp/banner.bin") == Read("oracle.bin"));
        assert(Read(root / "tmp/banner.bin").substr(length) == tail);
        assert(SeekSync(fi, 0, 1) == static_cast<std::int32_t>(length));
        Refusal(Cpu(fi, data)); // Repeated write would append: outside observed zero-position scope.
        assert(CloseSync(fi) == 0);
    }
    Path();
    Payload();
    assert(OpenSync(path, info, 2) == 0);
    const auto fd = Memory::Read32(info);
    for (unsigned flag = 0; flag < 256; ++flag) {
        if (flag == 1)
            continue;
        Memory::Write8(info + 0x8a, flag);
        Refusal(Cpu());
    }
    Memory::Write8(info + 0x8a, 1);
    for (unsigned bit = 0; bit < 32; ++bit) {
        auto cpu = Cpu();
        cpu.gpr[5] ^= 1u << bit;
        Refusal(cpu);
    }
    for (const auto ptr : {0u, 0xffffffffu, base - 1u, base + 65536u - 138u})
        Refusal(Cpu(ptr));
    for (const auto ptr : {0u, 0xffffffffu, base - 1u, base + 65536u - length + 1u})
        Refusal(Cpu(info, ptr));
    for (const auto badFd : {0u, 0xffffffffu, fd + 0x1000u}) {
        Memory::Write32(info, badFd);
        Refusal(Cpu());
    }
    Memory::Write32(info, fd);
    assert(SeekSync(info, 1, 0) == 1);
    Refusal(Cpu());
    assert(SeekSync(info, 0, 0) == 0 && CloseSync(info) == 0);
    Refusal(Cpu()); // Closed descriptor is not a new writable handle.
    for (const auto mode : {1u, 3u}) {
        assert(OpenSync(path, info, mode) == 0);
        Refusal(Cpu());
        assert(CloseSync(info) == 0);
    }
    Path("/other.bin");
    Write(root / "other.bin", "preserve unrelated bytes");
    assert(OpenSync(path, info, 2) == 0);
    Refusal(Cpu());
    assert(CloseSync(info) == 0);
    Path();
    diskFull = true;
    assert(OpenSync(path, info, 2) == 0);
    Pinned::handle.file = __real_fopen("/dev/full", "wb");
    assert(Pinned::handle.file && setvbuf(Pinned::handle.file, nullptr, _IONBF, 0) == 0);
    const auto oracle = Pinned::NANDWrite_HLE(info, buffer, length);
    assert(oracle == 0 && Invoke(Cpu()) == oracle);
    assert(std::fclose(Pinned::handle.file) == 0);
    assert(CloseSync(info) == -64 && Memory::Read8(info + 0x8a) == 1);
    allowFullClose = true;
    assert(CloseSync(info) == 0 && Memory::Read8(info + 0x8a) == 2);
    diskFull = false;
    Memory::Reset();
    std::printf("PASS: NANDWrite valid=%u refusals=%u; real owned handles, pinned count/flush, binary/tail preservation, storage failure, CPU/guest snapshots\n", valid, refusals);
}
