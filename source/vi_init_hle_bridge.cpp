#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"

#include <switch.h>

#include <atomic>
#include <chrono>
#include <cstdint>

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint32_t kViInitializedFlagAddr = 0x80386B38u;
constexpr std::uint32_t kViTimingGuardAddr = 0x80386B44u;
constexpr std::uint32_t kViTvFormatAddr = 0x80386BA8u;
constexpr std::uint32_t kViRenderWidthAddr = 0x80350864u;
constexpr std::uint32_t kViRenderHeightAddr = 0x80350866u;
constexpr std::uint32_t kViXfbWidthAddr = 0x80350872u;
constexpr std::uint32_t kViXfbHeightAddr = 0x8035087Cu;
constexpr std::uint32_t kViRetraceCountAddr = 0x80386BE4u;
constexpr std::uint32_t kViPreRetraceCallbackAddr = 0x80386BB8u;
constexpr std::uint32_t kViPostRetraceCallbackAddr = 0x80386BB4u;
constexpr std::uint32_t kViNextFrameBufferAddr = 0x80386BA0u;
constexpr std::uint32_t kViNextFrameBufferHwAddr = 0x80350890u;
constexpr std::uint32_t kViRetraceQueueAddr = 0x80386BC0u;
constexpr std::uint32_t kEggSSystemAddr = 0x80386F60u;

constexpr std::int64_t kNtscRetraceNs = 16'666'000ll;
constexpr std::int64_t kPalRetraceNs = 20'000'000ll;

std::atomic<bool> g_viInitialized{false};
std::atomic<bool> g_viPendingBlack{false};
std::atomic<bool> g_viBlack{false};
std::atomic<std::uint32_t> g_viPendingNextFrameBuffer{0u};
std::atomic<std::uint32_t> g_viNextFrameBuffer{0u};
std::atomic<std::uint32_t> g_viActiveTvFormat{0u};
std::atomic<std::uint32_t> g_viRetraceCount{0u};
std::atomic<bool> g_viFieldOdd{false};
std::atomic<bool> g_viFlushArmed{false};
std::atomic<std::int64_t> g_viLastRetraceNs{0};

std::int64_t NowNs() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}

std::int64_t RetraceIntervalNs(std::uint32_t tvFormat) noexcept {
    return tvFormat == 1u ? kPalRetraceNs : kNtscRetraceNs;
}

void Write8IfMapped(std::uint32_t address, std::uint8_t value) noexcept {
    if (Memory::Contains(address, 1u)) Memory::Write8(address, value);
}

void Write16IfMapped(std::uint32_t address, std::uint16_t value) noexcept {
    if (Memory::Contains(address, 2u)) Memory::Write16(address, value);
}

void Write32IfMapped(std::uint32_t address, std::uint32_t value) noexcept {
    if (Memory::Contains(address, 4u)) Memory::Write32(address, value);
}

std::uint32_t Read32IfMapped(std::uint32_t address) noexcept {
    if (!Memory::Contains(address, 4u)) return 0u;
    try { return Memory::Read32(address); } catch (...) { return 0u; }
}

} // namespace

extern "C" std::uint32_t mkw_switch_hle_vi_commit_pending_config() noexcept;

extern "C" void mkw_switch_hle_vi_init(CpuContext* cpu) noexcept {
    if (cpu) cpu->gpr[3] = 0u;
    if (!Memory::IsInitialized()) return;
    bool expected = false;
    if (!g_viInitialized.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    Write8IfMapped(kViInitializedFlagAddr, 1u);
    Write8IfMapped(kViTimingGuardAddr, 1u);
    Write32IfMapped(kViTvFormatAddr, 0u);
    Write16IfMapped(kViRenderWidthAddr, 640u);
    Write16IfMapped(kViRenderHeightAddr, 480u);
    Write16IfMapped(kViXfbWidthAddr, 640u);
    Write16IfMapped(kViXfbHeightAddr, 480u);
    Write32IfMapped(kViRetraceCountAddr, 0u);
    Write32IfMapped(kViPreRetraceCallbackAddr, 0u);
    Write32IfMapped(kViPostRetraceCallbackAddr, 0u);
    Write32IfMapped(kViNextFrameBufferAddr, 0u);
    Write32IfMapped(kViNextFrameBufferHwAddr, 0u);
    g_viActiveTvFormat.store(0u, std::memory_order_release);
    g_viRetraceCount.store(0u, std::memory_order_release);
    g_viFieldOdd.store(false, std::memory_order_release);
    g_viLastRetraceNs.store(NowNs(), std::memory_order_release);
}

extern "C" void mkw_switch_hle_vi_set_black(CpuContext* cpu) noexcept {
    if (!cpu) return;
    const bool makeBlack = cpu->gpr[3] != 0u;
    mkw_switch_hle_vi_init(cpu);
    g_viPendingBlack.store(makeBlack, std::memory_order_release);
    cpu->gpr[3] = 0u;
}

extern "C" void mkw_switch_hle_vi_flush(CpuContext* cpu) noexcept {
    mkw_switch_hle_vi_init(cpu);
    if (Memory::IsInitialized() && g_viPendingNextFrameBuffer.load(std::memory_order_acquire) == 0u) {
        std::uint32_t guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferAddr);
        if (guestNextFrameBuffer == 0u) guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferHwAddr);
        if (guestNextFrameBuffer != 0u) g_viPendingNextFrameBuffer.store(guestNextFrameBuffer, std::memory_order_release);
    }
    g_viFlushArmed.store(true, std::memory_order_release);
    if (cpu) cpu->gpr[3] = 0u;
}

extern "C" void mkw_switch_hle_vi_set_post_retrace_callback(CpuContext* cpu) noexcept {
    if (!cpu) return;
    const std::uint32_t newCallback = cpu->gpr[3];
    mkw_switch_hle_vi_init(cpu);
    const std::uint32_t previousCallback = Read32IfMapped(kViPostRetraceCallbackAddr);
    Write32IfMapped(kViPostRetraceCallbackAddr, newCallback);
    cpu->gpr[3] = previousCallback;
}

extern "C" void mkw_switch_hle_vi_wait_for_retrace(CpuContext* cpu) noexcept {
    if (!cpu) return;
    mkw_switch_hle_vi_init(cpu);
    const std::uint32_t activeTvFormat = g_viActiveTvFormat.load(std::memory_order_acquire);
    const std::int64_t intervalNs = RetraceIntervalNs(activeTvFormat);
    const std::int64_t lastRetraceNs = g_viLastRetraceNs.load(std::memory_order_acquire);
    const std::int64_t targetNs = lastRetraceNs + intervalNs;
    const std::int64_t nowNs = NowNs();
    if (nowNs < targetNs) svcSleepThread(targetNs - nowNs);
    if (g_viFlushArmed.exchange(false, std::memory_order_acq_rel)) {
        const std::uint32_t committedTvFormat = mkw_switch_hle_vi_commit_pending_config();
        g_viActiveTvFormat.store(committedTvFormat, std::memory_order_release);
        g_viBlack.store(g_viPendingBlack.load(std::memory_order_acquire), std::memory_order_release);
        g_viNextFrameBuffer.store(g_viPendingNextFrameBuffer.load(std::memory_order_acquire), std::memory_order_release);
    }
    const std::uint32_t retraceValue = g_viRetraceCount.fetch_add(1u, std::memory_order_acq_rel) + 1u;
    g_viFieldOdd.store(!g_viFieldOdd.load(std::memory_order_acquire), std::memory_order_release);
    g_viLastRetraceNs.store(targetNs, std::memory_order_release);
    Write32IfMapped(kViRetraceCountAddr, retraceValue);
    Write32IfMapped(kViNextFrameBufferAddr, g_viPendingNextFrameBuffer.load(std::memory_order_acquire));
    Write32IfMapped(kViNextFrameBufferHwAddr, g_viPendingNextFrameBuffer.load(std::memory_order_acquire));
    const std::uint32_t queueHead = Read32IfMapped(kViRetraceQueueAddr);
    const std::uint32_t queueTail = Read32IfMapped(kViRetraceQueueAddr + 4u);
    if (queueHead != 0u || queueTail != 0u) {
        cpu->gpr[3] = kViRetraceQueueAddr;
        InvokeDirectCpu<0x801AAAA4u>(cpu);
    }
    const std::uint32_t preCallback = Read32IfMapped(kViPreRetraceCallbackAddr);
    const std::uint32_t postCallback = Read32IfMapped(kViPostRetraceCallbackAddr);
    cpu->gpr[3] = retraceValue;
    if (preCallback != 0u) InvokeIndirectCpu(preCallback, cpu);
    if (postCallback != 0u && Read32IfMapped(kEggSSystemAddr) != 0u) InvokeIndirectCpu(postCallback, cpu);
    cpu->gpr[3] = 0u;
}

#endif
