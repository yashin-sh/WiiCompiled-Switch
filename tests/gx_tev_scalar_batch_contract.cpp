#include "abi_bridge.h"
#include "switch_gx_hle_traits.hpp"
#include <dolphin/gx.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <new>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>

namespace {
enum class Operation : std::size_t { Direct,
                                     ColorIn,
                                     ColorOp,
                                     AlphaIn,
                                     AlphaOp,
                                     SwapMode };
using Args = std::array<std::uint32_t, 6>;
struct Contract {
    std::uint32_t target;
    const char* stage;
    const char* reason;
    void (*invoke)(CpuContext*) noexcept;
};
constexpr std::array contracts{
    Contract{0x80171b58u, "RMCP01_GX_SET_TEV_DIRECT", "GX_SET_TEV_DIRECT_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171b58u>::Invoke},
    Contract{0x80171ce0u, "RMCP01_GX_SET_TEV_COLOR_IN", "GX_SET_TEV_COLOR_IN_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171ce0u>::Invoke},
    Contract{0x80171d60u, "RMCP01_GX_SET_TEV_COLOR_OP", "GX_SET_TEV_COLOR_OP_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171d60u>::Invoke},
    Contract{0x80171d20u, "RMCP01_GX_SET_TEV_ALPHA_IN", "GX_SET_TEV_ALPHA_IN_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171d20u>::Invoke},
    Contract{0x80171db8u, "RMCP01_GX_SET_TEV_ALPHA_OP", "GX_SET_TEV_ALPHA_OP_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171db8u>::Invoke},
    Contract{0x80171fd0u, "RMCP01_GX_SET_TEV_SWAP_MODE", "GX_SET_TEV_SWAP_MODE_UNPROVEN_ARGS", KnownNativeCpuCall<0x80171fd0u>::Invoke},
};
constexpr std::array<std::uint32_t, 10> colorOps{0u, 1u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u};
constexpr std::array<std::uint32_t, 4> alphaOps{0u, 1u, 14u, 15u};
constexpr std::array<std::uint32_t, 5> clampInputs{0u, 1u, 2u, 0x100u, 0xffffffffu};
constexpr const char* previousStage = "TEV_CONTRACT_PREVIOUS_STAGE";
const char* stage = nullptr;
std::uint32_t stageCalls = 0;
std::uint32_t nativeCalls = 0;
std::uint32_t previousNativeCalls = 0;
std::uint32_t previousStageCalls = 0;
std::uint32_t abortCases = 0;
std::array<std::uint32_t, contracts.size()> validCases{};
Operation expectedOperation = Operation::Direct;
CpuContext expectedCpu{};
CpuContext* liveCpu = nullptr;
int reportPipe = -1;
std::uint32_t reportCalls = 0;

const Contract& For(Operation operation) {
    return contracts[static_cast<std::size_t>(operation)];
}
void ExpectStage(Operation operation) {
    assert(stage && std::strcmp(stage, For(operation).stage) == 0);
    assert(stageCalls == previousStageCalls + 1u);
}
// Keep the large deterministic context generator out of the exhaustive loops.
// Avoid duplicating its byte-initialization loop in the sanitized case matrix.
[[gnu::noinline]] CpuContext MakeCpu(const Args& args) {
    CpuContext cpu;
    auto* bytes = reinterpret_cast<unsigned char*>(&cpu);
    for (std::size_t i = 0; i < sizeof(cpu); ++i)
        bytes[i] = static_cast<unsigned char>(0x43u + i * 37u);
    for (std::size_t i = 0; i < args.size(); ++i)
        cpu.gpr[3u + i] = args[i];
    return cpu;
}
void Prepare(CpuContext& cpu, Operation operation) {
    expectedOperation = operation;
    std::memcpy(&expectedCpu, &cpu, sizeof(cpu));
    liveCpu = &cpu;
    previousNativeCalls = nativeCalls;
    previousStageCalls = stageCalls;
    stage = previousStage;
}
void CheckCpu() {
    assert(liveCpu && std::memcmp(liveCpu, &expectedCpu, sizeof(expectedCpu)) == 0);
}
void Observe(Operation operation, std::initializer_list<std::uint32_t> args) {
    // An assertion would also raise SIGABRT and could falsely satisfy a
    // refused case after its report. A distinct exit makes any native call
    // in the child fail, including one attempted after a valid report.
    if (reportPipe >= 0)
        _exit(91);
    assert(operation == expectedOperation);
    assert(nativeCalls == previousNativeCalls);
    ExpectStage(operation);
    CheckCpu();
    std::size_t index = 0;
    for (const auto value : args) {
        const auto incoming = expectedCpu.gpr[3u + index];
        // The pin converts the complete r7 word to C++ bool, never to u8.
        const auto expected = index == 4u &&
                                      (operation == Operation::ColorOp || operation == Operation::AlphaOp)
                                  ? std::uint32_t(incoming != 0u)
                                  : incoming;
        assert(value == expected);
        ++index;
    }
    ++nativeCalls;
}
void InvokeAndCheck(Operation operation, const Args& args) {
    auto cpu = MakeCpu(args);
    Prepare(cpu, operation);
    For(operation).invoke(&cpu);
    CheckCpu();
    ExpectStage(operation);
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == previousNativeCalls + 1u);
#else
    assert(nativeCalls == previousNativeCalls);
#endif
    ++validCases[static_cast<std::size_t>(operation)];
    liveCpu = nullptr;
}
void CheckNull() {
    const auto oldCalls = nativeCalls;
    const auto oldStageCalls = stageCalls;
    const auto* oldStage = stage;
    for (const auto& contract : contracts)
        contract.invoke(nullptr);
    assert(nativeCalls == oldCalls && stageCalls == oldStageCalls && stage == oldStage);
}
void ExpectAbort(Operation operation, const Args& args) {
    // Shared CPU bytes let the parent observe writes even after the report
    // callback returns and before the child aborts; no abort wrapper or signal
    // handler is required, and the bridge still calls the real std::abort.
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
        Prepare(*sharedCpu, operation);
        For(operation).invoke(sharedCpu);
        _exit(90); // A refused tuple must not return to translated execution.
    }
    close(descriptors[1]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    std::array<char, 2> markers{};
    assert(read(descriptors[0], markers.data(), markers.size()) == 1 && markers[0] == 'R');
    close(descriptors[0]);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(std::memcmp(sharedCpu, &initialCpu, sizeof(initialCpu)) == 0);
    sharedCpu->~CpuContext();
    assert(munmap(storage, sizeof(CpuContext)) == 0);
    ++abortCases;
}
Args Baseline(Operation operation, std::uint32_t id = 7u) {
    switch (operation) {
    case Operation::Direct:
        return {id, 0x12345678u, 0x87654321u, 0xffffu, 0x100u, 0xffffffffu};
    case Operation::ColorIn:
        return {id, 1u, 6u, 10u, 15u, 0x87654321u};
    case Operation::AlphaIn:
        return {id, 0u, 2u, 5u, 7u, 0x12345678u};
    case Operation::ColorOp:
        return {id, 8u, 1u, 2u, 0x100u, 3u};
    case Operation::AlphaOp:
        return {id, 14u, 2u, 1u, 0xffffffffu, 2u};
    case Operation::SwapMode:
        return {id, 1u, 3u, 0xffffu, 0x100u, 0xffffffffu};
    }
    assert(false);
    return {};
}
void CheckValidInputs() {
    for (std::uint32_t id = 0; id < 16u; ++id) {
        InvokeAndCheck(Operation::Direct, Baseline(Operation::Direct, id));
        // Each input position varies independently; the other positions have
        // distinct values so an argument permutation cannot satisfy the sink.
        for (std::size_t position = 1; position <= 4u; ++position) {
            for (std::uint32_t value = 0; value < 16u; ++value) {
                auto args = Baseline(Operation::ColorIn, id);
                args[position] = value;
                InvokeAndCheck(Operation::ColorIn, args);
            }
            for (std::uint32_t value = 0; value < 8u; ++value) {
                auto args = Baseline(Operation::AlphaIn, id);
                args[position] = value;
                InvokeAndCheck(Operation::AlphaIn, args);
            }
        }
        for (std::uint32_t raster = 0; raster < 4u; ++raster) {
            for (std::uint32_t texture = 0; texture < 4u; ++texture) {
                auto args = Baseline(Operation::SwapMode, id);
                args[1] = raster;
                args[2] = texture;
                InvokeAndCheck(Operation::SwapMode, args);
            }
        }
    }
}
template <std::size_t N>
void CheckValidOps(Operation operation, const std::array<std::uint32_t, N>& ops) {
    // All legal op/bias/scale/destination combinations, on every stage, with
    // zero/nonzero and wide clamp words. Only forwarding is observed here.
    for (std::uint32_t id = 0; id < 16u; ++id)
        for (const auto op : ops)
            for (std::uint32_t bias = 0; bias < 3u; ++bias)
                for (std::uint32_t scale = 0; scale < 4u; ++scale)
                    for (const auto clamp : clampInputs)
                        for (std::uint32_t destination = 0; destination < 4u; ++destination)
                            InvokeAndCheck(operation, {id, op, bias, scale, clamp, destination});
}
void RejectAt(Operation operation, std::size_t position, std::initializer_list<std::uint32_t> values) {
    for (const auto id : {0u, 15u})
        for (const auto value : values) {
            auto args = Baseline(operation, id);
            args[position] = value;
            ExpectAbort(operation, args);
        }
}
void CheckRejectedInputs() {
    for (const auto operation : {Operation::Direct, Operation::ColorIn, Operation::ColorOp,
                                 Operation::AlphaIn, Operation::AlphaOp, Operation::SwapMode}) {
        for (const auto id : {16u, 17u, 0x100u, 0x10000u, 0xffffffffu}) {
            auto args = Baseline(operation);
            args[0] = id;
            ExpectAbort(operation, args);
        }
    }
    for (std::size_t position = 1; position <= 4u; ++position) {
        RejectAt(Operation::ColorIn, position, {16u, 17u, 0x100u, 0x1000fu, 0xffffffffu});
        RejectAt(Operation::AlphaIn, position, {8u, 9u, 0x100u, 0x10007u, 0xffffffffu});
    }
    RejectAt(Operation::ColorOp, 1u, {2u, 3u, 4u, 5u, 6u, 7u, 16u, 17u, 0x100u, 0x10008u, 0xffffffffu});
    RejectAt(Operation::AlphaOp, 1u, {2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 16u, 17u, 0x100u, 0x1000eu, 0xffffffffu});
    for (const auto operation : {Operation::ColorOp, Operation::AlphaOp}) {
        RejectAt(operation, 2u, {3u, 4u, 0x100u, 0x10002u, 0xffffffffu});
        RejectAt(operation, 3u, {4u, 5u, 0x100u, 0x10003u, 0xffffffffu});
        RejectAt(operation, 5u, {4u, 5u, 0x100u, 0x10003u, 0xffffffffu});
    }
    for (const auto position : {1u, 2u})
        RejectAt(Operation::SwapMode, position, {4u, 5u, 0x100u, 0x10003u, 0xffffffffu});
}
} // namespace

extern "C" void mkw_switch_set_fast_track_stage(const char* value) noexcept {
    stage = value;
    ++stageCalls;
}
extern "C" void mkw_switch_report_unsupported_translated_dispatch(const char* reason, std::uint32_t target, CpuContext* cpu) noexcept {
    if (reportCalls != 0u)
        _exit(92); // A second report must not become an accepted SIGABRT assertion.
    ++reportCalls;
    const auto& contract = For(expectedOperation);
    assert(reportPipe >= 0 && reason && std::strcmp(reason, contract.reason) == 0);
    assert(target == contract.target && cpu == liveCpu && nativeCalls == previousNativeCalls);
    CheckCpu();
    ExpectStage(expectedOperation);
    const char marker = 'R';
    assert(write(reportPipe, &marker, 1) == 1);
}

// The real Aurora declarations fix every sink signature. No Memory, frame,
// port FIFO or frame-work provider is linked: adding such a dependency to a
// scalar bridge must fail linkage. These sinks do not simulate GPU effects.
extern "C" void GXSetTevDirect(GXTevStageID id) {
    Observe(Operation::Direct, {std::uint32_t(id)});
}
extern "C" void GXSetTevColorIn(GXTevStageID id, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d) {
    Observe(Operation::ColorIn, {std::uint32_t(id), std::uint32_t(a), std::uint32_t(b), std::uint32_t(c), std::uint32_t(d)});
}
extern "C" void GXSetTevAlphaIn(GXTevStageID id, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d) {
    Observe(Operation::AlphaIn, {std::uint32_t(id), std::uint32_t(a), std::uint32_t(b), std::uint32_t(c), std::uint32_t(d)});
}
extern "C" void GXSetTevColorOp(GXTevStageID id, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID destination) {
    Observe(Operation::ColorOp, {std::uint32_t(id), std::uint32_t(op), std::uint32_t(bias), std::uint32_t(scale), std::uint32_t(clamp), std::uint32_t(destination)});
}
extern "C" void GXSetTevAlphaOp(GXTevStageID id, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID destination) {
    Observe(Operation::AlphaOp, {std::uint32_t(id), std::uint32_t(op), std::uint32_t(bias), std::uint32_t(scale), std::uint32_t(clamp), std::uint32_t(destination)});
}
extern "C" void GXSetTevSwapMode(GXTevStageID id, GXTevSwapSel raster, GXTevSwapSel texture) {
    Observe(Operation::SwapMode, {std::uint32_t(id), std::uint32_t(raster), std::uint32_t(texture)});
}

int main() {
    static_assert(KnownNativeCpuCall<0x80171b58u>::kAvailable && KnownNativeCpuCall<0x80171ce0u>::kAvailable);
    static_assert(KnownNativeCpuCall<0x80171d60u>::kAvailable && KnownNativeCpuCall<0x80171d20u>::kAvailable);
    static_assert(KnownNativeCpuCall<0x80171db8u>::kAvailable && KnownNativeCpuCall<0x80171fd0u>::kAvailable);
    static_assert(std::is_same_v<GXBool, bool> && std::is_trivially_copyable_v<CpuContext>);
    static_assert(GX_TEVSTAGE0 == 0 && GX_TEVSTAGE15 == 15 && GX_MAX_TEVSTAGE == 16);
    static_assert(GX_CC_CPREV == 0 && GX_CC_ZERO == 15 && GX_CA_APREV == 0 && GX_CA_ZERO == 7);
    static_assert(GX_TEV_ADD == 0 && GX_TEV_SUB == 1 && GX_TEV_COMP_R8_GT == 8 && GX_TEV_COMP_R8_EQ == 9);
    static_assert(GX_TEV_COMP_GR16_GT == 10 && GX_TEV_COMP_GR16_EQ == 11 && GX_TEV_COMP_BGR24_GT == 12 && GX_TEV_COMP_BGR24_EQ == 13);
    static_assert(GX_TEV_COMP_RGB8_GT == 14 && GX_TEV_COMP_RGB8_EQ == 15 && GX_TEV_COMP_A8_GT == 14 && GX_TEV_COMP_A8_EQ == 15);
    static_assert(GX_TB_ZERO == 0 && GX_TB_ADDHALF == 1 && GX_TB_SUBHALF == 2 && GX_MAX_TEVBIAS == 3);
    static_assert(GX_CS_SCALE_1 == 0 && GX_CS_SCALE_2 == 1 && GX_CS_SCALE_4 == 2 && GX_CS_DIVIDE_2 == 3);
    static_assert(GX_TEVPREV == 0 && GX_TEVREG2 == 3 && GX_MAX_TEVREG == 4);
    static_assert(GX_TEV_SWAP0 == 0 && GX_TEV_SWAP3 == 3 && GX_MAX_TEVSWAP == 4);
    const rlimit noCore{0, 0};
    assert(setrlimit(RLIMIT_CORE, &noCore) == 0);

    CheckNull();
    CheckValidInputs();
    CheckValidOps(Operation::ColorOp, colorOps);
    CheckValidOps(Operation::AlphaOp, alphaOps);
    CheckNull();
    CheckRejectedInputs();
    CheckNull();
    assert((validCases == std::array<std::uint32_t, 6>{16u, 1024u, 38400u, 512u, 15360u, 256u}));
    assert(abortCases == 246u);
    std::uint32_t total = 0;
    for (const auto count : validCases)
        total += count;
#if MKW_LOCAL_RENDERED_FAST_TRACK
    assert(nativeCalls == total);
#else
    assert(nativeCalls == 0u);
#endif
    std::printf("TEV scalar contract: %u valid calls, %u diagnosed SIGABRT cases, %u native calls\n", total, abortCases, nativeCalls);
}
