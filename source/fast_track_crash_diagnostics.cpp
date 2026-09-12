#include <cstdint>

#if (defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)
#define MKW_FAST_TRACK_DIAGNOSTICS 1
#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include <switch.h>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#else
#define MKW_FAST_TRACK_DIAGNOSTICS 0
struct CpuContext;
#endif

#if MKW_FAST_TRACK_DIAGNOSTICS
namespace {

constexpr const char* kDispatchPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-dispatch-blocker.txt";
constexpr const char* kExceptionPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-exception.txt";

void write_atomicish(const char* path, const char* data, std::size_t size) noexcept {
    const int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return;
    }

    std::size_t written = 0;
    while (written < size) {
        const ssize_t rc = ::write(fd, data + written, size - written);
        if (rc <= 0) {
            break;
        }
        written += static_cast<std::size_t>(rc);
    }
    ::fsync(fd);
    ::close(fd);
}

} // namespace
#endif

extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* kind,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
#if MKW_FAST_TRACK_DIAGNOSTICS
    char buffer[1024];
    const std::uint32_t guest_pc = cpu ? cpu->pc : 0u;
    const std::uint32_t r1 = cpu ? cpu->gpr[1] : 0u;
    const std::uint32_t r2 = cpu ? cpu->gpr[2] : 0u;
    const std::uint32_t r3 = cpu ? cpu->gpr[3] : 0u;
    const std::uint32_t r13 = cpu ? cpu->gpr[13] : 0u;

    const int n = std::snprintf(
        buffer,
        sizeof(buffer),
        "WiiCompiled-Switch unsupported translated dispatch\n"
        "=================================================\n"
        "kind                  : %s\n"
        "target                : 0x%08x\n"
        "guest pc              : 0x%08x\n"
        "r1                    : 0x%08x\n"
        "r2                    : 0x%08x\n"
        "r3                    : 0x%08x\n"
        "r13                   : 0x%08x\n"
        "action                : abort after durable blocker record\n",
        kind ? kind : "UNKNOWN",
        target,
        guest_pc,
        r1,
        r2,
        r3,
        r13);
    if (n > 0) {
        const std::size_t size = static_cast<std::size_t>(n) < sizeof(buffer)
            ? static_cast<std::size_t>(n)
            : sizeof(buffer) - 1;
        write_atomicish(kDispatchPath, buffer, size);
    }
#else
    (void)kind;
    (void)target;
    (void)cpu;
#endif
}

#if MKW_FAST_TRACK_DIAGNOSTICS
extern "C" {
// libnx defaults to a very small exception stack. Crash reporting uses only
// fixed buffers/no heap, but give it enough room for libc formatting and FS I/O.
alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
}

extern "C" void __libnx_exception_handler(ThreadExceptionDump* ctx) {
    if (!ctx) {
        return;
    }

    // libnx exception handling may observe a different TLS view than the code
    // that entered translated execution. WiiCompiled's own fatal reporting uses
    // the persistent context as a fallback for exactly this reason.
    CpuContext* tls_guest = TryGetCpuContext();
    CpuContext* guest = tls_guest;
    bool persistent_fallback = false;
    if (!guest) {
        guest = &GetPersistentCpuContext();
        persistent_fallback = true;
    }

    const std::uint32_t guest_pc = guest ? guest->pc : 0u;
    const std::uint32_t guest_r1 = guest ? guest->gpr[1] : 0u;
    const std::uint32_t guest_r2 = guest ? guest->gpr[2] : 0u;
    const std::uint32_t guest_r3 = guest ? guest->gpr[3] : 0u;
    const std::uint32_t guest_r13 = guest ? guest->gpr[13] : 0u;

    const std::uintptr_t guest_base =
        reinterpret_cast<std::uintptr_t>(GuestFlat::Base());
    const std::uintptr_t far = static_cast<std::uintptr_t>(ctx->far.x);
    const bool far_in_guest_window =
        guest_base != 0u && far >= guest_base &&
        static_cast<std::uint64_t>(far - guest_base) < GuestFlat::kGuestSpaceSize;
    const std::uint32_t derived_guest_address = far_in_guest_window
        ? static_cast<std::uint32_t>(far - guest_base)
        : 0u;

    char buffer[2048];
    const int n = std::snprintf(
        buffer,
        sizeof(buffer),
        "WiiCompiled-Switch libnx exception\n"
        "==================================\n"
        "error desc            : 0x%08x\n"
        "aarch64 pc            : 0x%016llx\n"
        "aarch64 lr            : 0x%016llx\n"
        "aarch64 sp            : 0x%016llx\n"
        "fault address (FAR)   : 0x%016llx\n"
        "ESR                   : 0x%08x\n"
        "guest context active  : %s\n"
        "guest context source  : %s\n"
        "guest pc              : 0x%08x\n"
        "guest r1              : 0x%08x\n"
        "guest r2              : 0x%08x\n"
        "guest r3              : 0x%08x\n"
        "guest r13             : 0x%08x\n"
        "guest flat base       : 0x%016llx\n"
        "FAR in guest window   : %s\n"
        "derived guest address : 0x%08x\n"
        "note                  : process will still terminate after this handler\n",
        ctx->error_desc,
        static_cast<unsigned long long>(ctx->pc.x),
        static_cast<unsigned long long>(ctx->lr.x),
        static_cast<unsigned long long>(ctx->sp.x),
        static_cast<unsigned long long>(ctx->far.x),
        ctx->esr,
        tls_guest ? "YES" : "NO",
        persistent_fallback ? "PERSISTENT_FALLBACK" : "TLS",
        guest_pc,
        guest_r1,
        guest_r2,
        guest_r3,
        guest_r13,
        static_cast<unsigned long long>(guest_base),
        far_in_guest_window ? "YES" : "NO",
        derived_guest_address);

    if (n > 0) {
        const std::size_t size = static_cast<std::size_t>(n) < sizeof(buffer)
            ? static_cast<std::size_t>(n)
            : sizeof(buffer) - 1;
        write_atomicish(kExceptionPath, buffer, size);
    }
}
#endif
