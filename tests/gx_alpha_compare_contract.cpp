#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static_assert(KnownNativeCpuCall<0x80172088u>::kAvailable);

// The production owner lives in rendered_fast_track_graphics.cpp. This sink
// observes its required ordering without supplying a renderer or guest memory.
bool g_alphaCompareValid = false;

namespace {
using Args = std::array<std::uint32_t, 5>;
constexpr const char* expectedStage = "RMCP01_GX_SET_ALPHA_COMPARE";
const char* stage = "PREVIOUS_STAGE";
std::uint32_t stageCalls = 0;
std::uint32_t nativeCalls = 0;
std::uint32_t previousStageCalls = 0;
std::uint32_t previousNativeCalls = 0;
std::uint32_t validCases = 0;
std::uint32_t refusalCases = 0;
bool previousFlag = false;
CpuContext expectedCpu{};
CpuContext* liveCpu = nullptr;
int reportPipe = -1;
std::uint32_t reportCalls = 0;

CpuContext MakeCpu(const Args& args) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x43u + i * 37u);
    for (std::size_t i = 0; i < args.size(); ++i)
        cpu.gpr[3u + i] = args[i];
    return cpu;
}

bool ContextPreserved() {
    return liveCpu && std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) == 0;
}

bool StagePublished() {
    return stage && std::strcmp(stage, expectedStage) == 0 &&
           stageCalls == previousStageCalls + 1u;
}

void Prepare(CpuContext& cpu, bool initialFlag) {
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    liveCpu = &cpu;
    previousStageCalls = stageCalls;
    previousNativeCalls = nativeCalls;
    previousFlag = initialFlag;
    g_alphaCompareValid = initialFlag;
    stage = "PREVIOUS_STAGE";
}

void InvokeAndCheck(const Args& args, bool initialFlag) {
    auto cpu = MakeCpu(args);
    Prepare(cpu, initialFlag);
    KnownNativeCpuCall<0x80172088u>::Invoke(&cpu);
    assert(ContextPreserved() && StagePublished());
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == previousNativeCalls + 1u && g_alphaCompareValid);
#else
    assert(nativeCalls == previousNativeCalls && g_alphaCompareValid == initialFlag);
#endif
    ++validCases;
    liveCpu = nullptr;
}

void CheckNull() {
    for (const bool initialFlag : {false, true}) {
        g_alphaCompareValid = initialFlag;
        const auto oldStageCalls = stageCalls;
        const auto oldNativeCalls = nativeCalls;
        const auto* oldStage = stage;
        KnownNativeCpuCall<0x80172088u>::Invoke(nullptr);
        assert(stageCalls == oldStageCalls && nativeCalls == oldNativeCalls &&
               stage == oldStage && g_alphaCompareValid == initialFlag);
    }
}

void ExpectAbort(const Args& args, bool initialFlag) {
    // Shared CPU catches even writes made after the diagnostic callback.
    void* storage = mmap(nullptr, sizeof(CpuContext), PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(storage != MAP_FAILED);
    auto* sharedCpu = new (storage) CpuContext;
    const auto initialCpu = MakeCpu(args);
    std::memcpy(sharedCpu, &initialCpu, sizeof(initialCpu));
    int descriptors[2]{};
    assert(pipe(descriptors) == 0);
    const auto child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        reportPipe = descriptors[1];
        reportCalls = 0;
        Prepare(*sharedCpu, initialFlag);
        KnownNativeCpuCall<0x80172088u>::Invoke(sharedCpu);
        _exit(90);
    }
    close(descriptors[1]);
    std::array<char, 2> reports{};
    std::size_t received = 0;
    while (received < reports.size()) {
        const auto count = read(descriptors[0], reports.data() + received,
                                reports.size() - received);
        assert(count >= 0);
        if (count == 0)
            break;
        received += static_cast<std::size_t>(count);
    }
    close(descriptors[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(received == 1u && reports[0] == 'R');
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(std::memcmp(sharedCpu, &initialCpu, sizeof(initialCpu)) == 0);
    sharedCpu->~CpuContext();
    assert(munmap(storage, sizeof(CpuContext)) == 0);
    ++refusalCases;
}
} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}

extern "C" void mkw_switch_report_unsupported_translated_dispatch(
    const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    if (reportPipe < 0 || reportCalls++ != 0u)
        _exit(92);
    if (target != 0x80172088u ||
        std::strcmp(reason, "GX_SET_ALPHA_COMPARE_UNPROVEN_ARGS") != 0 ||
        cpu != liveCpu || !ContextPreserved() || !StagePublished() ||
        g_alphaCompareValid != previousFlag || nativeCalls != previousNativeCalls)
        _exit(93);
    const char marker = 'R';
    if (write(reportPipe, &marker, 1) != 1)
        _exit(94);
}

extern "C" [[noreturn]] void __real_abort();
extern "C" [[noreturn]] void __wrap_abort() {
    // The flag is a real bool global, not a shared test mirror. Check again at
    // the abort boundary to catch a write after the report, then use real abort.
    if (reportPipe >= 0 &&
        (reportCalls != 1u || !ContextPreserved() || !StagePublished() ||
         g_alphaCompareValid != previousFlag || nativeCalls != previousNativeCalls))
        _exit(95);
    __real_abort();
}

void GXSetAlphaCompare(GXCompare compare0, u8 reference0, GXAlphaOp operation,
                       GXCompare compare1, u8 reference1) {
    // A distinct exit prevents a native assertion's SIGABRT from satisfying
    // the expected refused case, including an attempt after the report.
    if (reportPipe >= 0)
        _exit(91);
    assert(ContextPreserved() && StagePublished());
    assert(g_alphaCompareValid && nativeCalls == previousNativeCalls);
    assert(static_cast<std::uint32_t>(compare0) == expectedCpu.gpr[3]);
    assert(reference0 == static_cast<u8>(expectedCpu.gpr[4]));
    assert(static_cast<std::uint32_t>(operation) == expectedCpu.gpr[5]);
    assert(static_cast<std::uint32_t>(compare1) == expectedCpu.gpr[6]);
    assert(reference1 == static_cast<u8>(expectedCpu.gpr[7]));
    ++nativeCalls;
}

int main() {
    CheckNull();
    constexpr std::array<std::uint32_t, 7> wideReferences{
        0u, 255u, 256u, 0x100ffu, 0x80000000u, 0xffffff00u, 0xffffffffu};
    for (std::uint32_t compare0 = 0; compare0 < 8u; ++compare0)
        for (std::uint32_t compare1 = 0; compare1 < 8u; ++compare1)
            for (std::uint32_t op = 0; op < 4u; ++op)
                for (const bool initialFlag : {false, true}) {
                    for (std::uint32_t ref = 0; ref < 256u; ++ref) {
                        InvokeAndCheck({compare0, ref, op, compare1, 0xa7u}, initialFlag);
                        InvokeAndCheck({compare0, 0x59u, op, compare1, ref}, initialFlag);
                    }
                    for (std::size_t i = 0; i < wideReferences.size(); ++i)
                        InvokeAndCheck({compare0, wideReferences[i], op, compare1,
                                        wideReferences[wideReferences.size() - 1u - i]},
                                       initialFlag);
                }
    // Every reference pair for the console's ALWAYS/AND/ALWAYS combination.
    for (std::uint32_t ref0 = 0; ref0 < 256u; ++ref0)
        for (std::uint32_t ref1 = 0; ref1 < 256u; ++ref1)
            InvokeAndCheck({7u, ref0, 0u, 7u, ref1}, ((ref0 + ref1) & 1u) != 0u);
    constexpr std::array<std::uint32_t, 6> invalidCompares{
        8u, 16u, 256u, 0xffffu, 0x80000000u, 0xffffffffu};
    constexpr std::array<std::uint32_t, 6> invalidOperations{
        4u, 7u, 256u, 0xffffu, 0x80000000u, 0xffffffffu};
    for (const bool initialFlag : {false, true}) {
        for (const auto value : invalidCompares) {
            ExpectAbort({value, 0x100ffu, 0u, 7u, 0xffffff00u}, initialFlag);
            ExpectAbort({7u, 0x100ffu, 0u, value, 0xffffff00u}, initialFlag);
        }
        for (const auto value : invalidOperations)
            ExpectAbort({7u, 0x100ffu, value, 7u, 0xffffff00u}, initialFlag);
        ExpectAbort({0xffffffffu, 0u, 0xffffffffu, 0xffffffffu, 0u}, initialFlag);
    }
    CheckNull();
    assert(validCases == 331264u && refusalCases == 38u);
    std::printf("PASS AlphaCompare valid=%u diagnosed-aborts=%u native=%u rendered=%d\n",
                validCases, refusalCases, nativeCalls, MKW_LOCAL_RENDERED_FAST_TRACK);
}
