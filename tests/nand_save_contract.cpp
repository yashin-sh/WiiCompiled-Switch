#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "horizon_runtime_services.hpp"
#include "memory.h"
#include "switch_nand_runtime.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr unsigned base = 0x70000000, path = base + 16, info = base + 2048;
constexpr unsigned secondInfo = base + 2304, data = base + 4096;
constexpr unsigned callback = 0x12345678, command = 0x87654321;
std::vector<std::uint8_t> bytes;
fs::path root;
bool active = false, diskFull = false, permitFullCleanup = false;
bool flushFailure = false, syncFailure = false, closeFailure = false;
unsigned directorySyncs = 0, callbacks = 0, cases = 0;
std::int32_t callbackResult = 0;

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
void Path(const char* s) {
    assert(std::strlen(s) < 1024);
    std::memcpy(bytes.data() + path - base, s, std::strlen(s) + 1);
}
template <unsigned Target>
std::int32_t Invoke(unsigned r3, unsigned r4 = 0, unsigned r5 = 0,
                    unsigned r6 = 0, unsigned r7 = 0, unsigned r8 = 0, unsigned r9 = 0) {
    static_assert(KnownNativeCpuCall<Target>::kAvailable);
    CpuContext cpu;
    std::memset(&cpu, 0xa5, sizeof(cpu));
    cpu.gpr[3] = r3;
    cpu.gpr[4] = r4;
    cpu.gpr[5] = r5;
    cpu.gpr[6] = r6;
    cpu.gpr[7] = r7;
    cpu.gpr[8] = r8;
    cpu.gpr[9] = r9;
    auto expected = cpu;
    InvokeDirectCpu<Target>(&cpu);
    expected.gpr[3] = cpu.gpr[3];
    assert(std::memcmp(&cpu, &expected, sizeof(cpu)) == 0);
    ++cases;
    return static_cast<std::int32_t>(cpu.gpr[3]);
}
} // namespace

namespace GuestFlat {
bool IsActive() {
    return active;
}
void Initialize(const std::vector<RegionRequest>& requests) {
    assert(requests.size() == 1 && requests[0].base == base && requests[0].size == 65536);
    bytes.assign(65536, 0);
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
extern "C" int __real_fflush(FILE*);
extern "C" int __real_fsync(int);
extern "C" int __real_fclose(FILE*);
extern "C" FILE* __wrap_fopen(const char* p, const char* m) {
    return diskFull && fs::path(p) == root / "full.bin" ? __real_fopen("/dev/full", "wb") : __real_fopen(p, m);
}
extern "C" int __wrap_fflush(FILE* p) {
    if (flushFailure) {
        errno = EIO;
        return EOF;
    }
    return __real_fflush(p);
}
extern "C" int __wrap_fsync(int fd) {
    struct stat status;
    assert(fstat(fd, &status) == 0);
    if (S_ISDIR(status.st_mode))
        ++directorySyncs;
    if (syncFailure) {
        errno = EIO;
        return -1;
    }
    return diskFull && permitFullCleanup ? 0 : __real_fsync(fd);
}
extern "C" int __wrap_fclose(FILE* p) {
    const auto result = __real_fclose(p);
    if (closeFailure) {
        closeFailure = false;
        errno = EIO;
        return EOF;
    }
    return result;
}
extern "C" void mkw_switch_set_fast_track_stage(const char*) noexcept {}
extern "C" void mkw_switch_note_translated_dispatch(std::uint32_t, CpuContext*) noexcept {}
extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext*) noexcept {}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char*, std::uint32_t, CpuContext*) noexcept {
    std::abort();
}
bool mkw_switch_try_dispatch_indirect(std::uint32_t target, CpuContext* cpu) {
    assert(target == callback && cpu == TryGetCpuContext());
    assert(cpu->gpr[3] == static_cast<unsigned>(callbackResult) && cpu->gpr[4] == command);
    // Callback mutations must remain in the callback's private CpuContext.
    cpu->gpr[5] = 0;
    ++callbacks;
    return true;
}

namespace Pinned {
constexpr std::int32_t NAND_RESULT_INVALID = -8, NAND_RESULT_OK = 0, NAND_RESULT_EXISTS = -6, NAND_RESULT_UNKNOWN = -64;
struct FileHandle {
    FILE* file;
} handle{};
FileHandle* ResolveNandFileHandle(const char*, std::uint32_t) {
    return &handle;
}
fs::path TranslateNandPath(const char* p) {
    return root / fs::path(p).relative_path();
}
bool PathExists(const fs::path& p) {
    return fs::exists(p);
}
bool IsDirectory(const fs::path& p) {
    return fs::is_directory(p);
}
bool CreateDirectoryPath(const fs::path& p) {
    return fs::create_directories(p);
}
#include "pinned-nand-save.inc"
} // namespace Pinned

int main() {
    static_assert(!KnownNativeCpuCall<0x8019b43c>::kAvailable);
    static_assert(!KnownNativeCpuCall<0x8019b884>::kAvailable);
    Memory::Config config;
    config.regions = {{"synthetic-save", base, 65536}};
    Memory::Init(config);
    root = fs::absolute("nand");
    fs::create_directories(root);
    using namespace mkw::switch_nand_runtime;
    const std::string original("original\0binary-tail", 20), payload("NEW\0", 4);
    std::memcpy(bytes.data() + data - base, payload.data(), payload.size());

    Path("/exists");
    fs::create_directories(root / "exists");
    assert(Invoke<0x8019bbe0>(path, 0x30, 0) == Pinned::NANDCreateDir_HLE(path, 0x30, 0));
    Write(root / "file.bin", original);
    Path("/file.bin");
    assert(Invoke<0x8019bbe0>(path, 0x30, 0) == Pinned::NANDCreateDir_HLE(path, 0x30, 0));
    callbackResult = -6;
    assert(Invoke<0x8019b524>(path, 0x30, 0, callback, command) == -6 && Read(root / "file.bin") == original);
    Path("/created.bin");
    callbackResult = 0;
    assert(Invoke<0x8019b524>(path, 0x30, 0, callback, command) == 0 && fs::file_size(root / "created.bin") == 0);
    assert(Invoke<0x8019b6e4>(path, callback, command) == 0 && !fs::exists(root / "created.bin"));
    callbackResult = -12;
    assert(Invoke<0x8019b6e4>(path, callback, command) == -12);
    Path("/new/subdir");
    callbackResult = 0;
    assert(Invoke<0x8019bcc8>(path, 0x30, 0, callback, command) == 0 && fs::is_directory(root / "new/subdir"));

    // Async safe open shares sync flag=3, and callback close publishes flag=4.
    for (const auto mode : {1u, 2u, 3u}) {
        for (const bool asynchronous : {false, true}) {
            Write(root / "save.bin", original);
            Write(root / "save.bin.nandsafe.tmp", "stale scratch");
            Path("/save.bin");
            callbackResult = 0;
            const auto opened = asynchronous ? Invoke<0x8019d104>(path, info, mode, 0, 0, callback, command)
                                             : Invoke<0x8019cb74>(path, info, mode);
            assert(opened == 0 && Memory::Read8(info + 0x88) == mode && Memory::Read8(info + 0x8a) == 3);
            assert(Invoke<0x8019ca80>(info) == -8); // Normal close must not consume a safe handle.
            if (mode != 1) {
                const auto snapshot = bytes;
                callbackResult = 4;
                assert(Invoke<0x8019b8ec>(info, data, 4, callback, command) == 0);
                assert(bytes == snapshot && Read(root / "save.bin") == original);
                // A second opener must not remove an active shadow owned by the first.
                assert(SafeOpenSync(path, secondInfo, mode) == -8);
                assert(Read(root / "save.bin.nandsafe.tmp") == payload + original.substr(4));
            } else {
                assert(Invoke<0x8019b7a4>(info, data + 32, 4) == 4);
                assert(std::memcmp(bytes.data() + data + 32 - base, original.data(), 4) == 0);
            }
            const auto before = directorySyncs;
            callbackResult = 0;
            assert((asynchronous ? Invoke<0x8019d720>(info, callback, command) : Invoke<0x8019cf28>(info)) == 0);
            assert(Memory::Read8(info + 0x8a) == 4);
            assert(Read(root / "save.bin") == (mode == 1 ? original : payload + original.substr(4)));
            assert(mode == 1 || (directorySyncs > before && !fs::exists(root / "save.bin.nandsafe.tmp")));
            assert(Invoke<0x8019cf28>(info) == 0); // Safe close is repeatable.
        }
    }

    // The SDK async-chain flag is distinct from the callback wrapper's flag 3.
    Path("/save.bin");
    assert(SafeOpenSync(path, info, 1) == 0);
    Memory::Write8(info + 0x8a, 5);
    assert(Invoke<0x8019ca80>(info) == -8);
    assert(Invoke<0x8019cf28>(info) == 0 && Memory::Read8(info + 0x8a) == 6);
    assert(Invoke<0x8019cf28>(info) == 0);

    // Failures before the rename preserve the original and clean the scratch.
    for (unsigned fault = 0; fault < 4; ++fault) {
        Write(root / "failed.bin", original);
        Path("/failed.bin");
        assert(SafeOpenSync(path, info, 2) == 0 && WriteSync(info, data, 4) == 4);
        flushFailure = fault == 0;
        syncFailure = fault == 1;
        closeFailure = fault == 2;
        if (fault == 3) {
            fs::remove(root / "failed.bin");
            fs::create_directory(root / "failed.bin");
        }
        assert(Invoke<0x8019cf28>(info) == -64 && Memory::Read8(info + 0x8a) == 3);
        flushFailure = syncFailure = closeFailure = false;
        assert(!fs::exists(root / "failed.bin.nandsafe.tmp"));
        assert(fault == 3 ? fs::is_directory(root / "failed.bin") : Read(root / "failed.bin") == original);
        assert(Invoke<0x8019cf28>(info) == -8); // Consumed stream cannot be reused.
        fs::remove_all(root / "failed.bin");
    }

    // The pinned transfer count survives a real buffered fflush failure.
    Path("/full.bin");
    diskFull = true;
    assert(OpenSync(path, info, 2) == 0);
    Pinned::handle.file = __real_fopen("/dev/full", "wb");
    assert(Pinned::handle.file);
    callbackResult = Pinned::NANDWrite_HLE(info, data, 4);
    assert(callbackResult == 4 && Invoke<0x8019b8ec>(info, data, 4, callback, command) == 0);
    __real_fclose(Pinned::handle.file);
    assert(Invoke<0x8019ca80>(info) == -64 && Memory::Read8(info + 0x8a) == 1);
    permitFullCleanup = true;
    assert(Invoke<0x8019ca80>(info) == 0 && Memory::Read8(info + 0x8a) == 2);
    diskFull = false;

    // A fclose error consumes the stream; a retry must not double-free it.
    Path("/file.bin");
    assert(OpenSync(path, info, 2) == 0);
    closeFailure = true;
    assert(Invoke<0x8019ca80>(info) == -64);
    assert(Invoke<0x8019ca80>(info) == 0);

    Path("/missing.bin");
    assert(SafeOpenSync(path, info, 2) == -12 && !fs::exists(root / "missing.bin.nandsafe.tmp"));
    assert(SafeOpenSync(path, info, 0) == -8 && SafeOpenSync(path, info, 4) == -8);
    assert(SafeOpenSync(0, info, 2) == -8 && SafeOpenSync(path, base + 65536 - 138, 2) == -8);
    assert(WriteSync(0, data, 4) == -8 && WriteSync(info, base + 65535, 4) == -8);
    callbackResult = -8;
    assert(Invoke<0x8019b8ec>(info, data, 4, callback, command) == -8);
    assert(callbacks == 17);
    Memory::Reset();
    std::printf("PASS: NAND save native dispatches=%u callbacks=%u; real filesystem, pinned write/directory oracles, safe commit/error preservation, consumed streams, bounded bridges retained\n", cases, callbacks);
}
