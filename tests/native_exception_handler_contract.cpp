// Compile the actual patched Switch handler with a memory-query seam. Verify
// that libnx's weak 1 KiB exception stack is overridden and corrupt frame
// pointers are rejected before dereferencing them.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
enum { Perm_R = 1 };
struct MemoryInfo {
    u64 addr, size;
    u32 perm;
};
struct Register {
    u64 x = 0;
};
struct ThreadExceptionDump {
    u32 error_desc = 0;
    Register pc, lr, sp, far, fp;
    std::array<Register, 29> cpu_gprs{};
};
#define R_FAILED(result) ((result) != 0)
std::vector<const char*> lines;
u64 readableBegin = 0, readableSize = 0;
u32 readablePermission = Perm_R;
int queryResult = 0, queries = 0;
int svcQueryMemory(MemoryInfo* info, u32* pageInfo, u64) {
    ++queries;
    *info = {readableBegin, readableSize, readablePermission};
    *pageInfo = 0;
    return queryResult;
}
u64 SwitchModuleOffset(u64 value) {
    return value;
}
void SwitchCrashWrite(const char* format, ...) {
    lines.push_back(format);
}
struct Exited {};
[[noreturn]] void svcExitProcess() {
    throw Exited{};
}
extern "C" u64 LibnxExceptionStackSize();
extern "C" u8* LibnxExceptionStackAddress();

#include "exception-production.inc"

void Run(u64 fp, int expectedFrames, int expectedQueries) {
    lines.clear();
    queries = 0;
    ThreadExceptionDump context;
    context.fp.x = fp;
    try {
        __libnx_exception_handler(&context);
    } catch (const Exited&) {
        int frames = 0;
        for (const auto* line : lines) {
            if (std::strstr(line, "#%02d"))
                ++frames;
        }
        assert(frames == expectedFrames);
        assert(queries == expectedQueries);
        assert(lines.size() == 10u + expectedFrames);
        assert(std::strstr(lines.back(), "end"));
        return;
    }
    assert(false && "handler must terminate the faulted process");
}

int main() {
    static_assert(sizeof(__nx_exception_stack) == 64 * 1024);
    assert(__nx_exception_stack_size == sizeof(__nx_exception_stack));
    assert(LibnxExceptionStackSize() == 64 * 1024);
    assert(LibnxExceptionStackAddress() == __nx_exception_stack);
    assert(reinterpret_cast<uintptr_t>(__nx_exception_stack) % 16 == 0);
    alignas(16) std::array<u64, 4> chain{};
    readableBegin = reinterpret_cast<uintptr_t>(chain.data());
    readableSize = sizeof(chain);
    chain = {readableBegin + 16, 0x1234, 0, 0x5678};
    Run(readableBegin, 2, 2);
    Run(0, 0, 0);
    Run(readableBegin + 1, 0, 0); // Unaligned.
    queryResult = 1;
    Run(0x1000, 0, 1); // Query failure, no actual dereference.
    queryResult = 0;
    readablePermission = 0;
    Run(0x1000, 0, 1); // Unreadable region.
    readablePermission = Perm_R;
    Run(readableBegin - 16, 0, 1);            // Before the queried region.
    Run(readableBegin + sizeof(chain), 0, 1); // End of the queried region.
    readableSize = 8;
    Run(readableBegin, 0, 1); // One word is insufficient for FP/LR.
    readableSize = sizeof(chain);
    chain[0] = readableBegin;
    Run(readableBegin, 1, 1); // Cycle stops after one frame.
    chain[0] = readableBegin + 16;
    chain[1] = 0;
    Run(readableBegin, 0, 1); // Empty return address.
}
