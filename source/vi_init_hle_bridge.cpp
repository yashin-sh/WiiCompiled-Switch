#if (defined(MKW_LOCAL_FUNCTION_EXECUTION) && MKW_LOCAL_FUNCTION_EXECUTION) || \
    (defined(MKW_SYNTHETIC_EXECUTION) && MKW_SYNTHETIC_EXECUTION)

#include "abi_bridge.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"

#include <switch.h>

#include <atomic>
#include <chrono>
#include <cstdint>

extern "C" std::uint32_t mkw_switch_hle_vi_commit_pending_config() noexcept;

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
std::atomic<bool> g_viRetraceAdvancing{false};

class ScopedCpuContextRestore {
  public:
    explicit ScopedCpuContextRestore(CpuContext* cpu) noexcept
        : cpu_(cpu), saved_(cpu ? *cpu : CpuContext{}) {}

    ~ScopedCpuContextRestore() noexcept {
        if (cpu_) {
            *cpu_ = saved_;
        }
    }

    ScopedCpuContextRestore(const ScopedCpuContextRestore&) = delete;
    ScopedCpuContextRestore& operator=(const ScopedCpuContextRestore&) = delete;

  private:
    CpuContext* cpu_ = nullptr;
    CpuContext saved_{};
};

std::int64_t NowNs() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}

std::int64_t RetraceIntervalNs(std::uint32_t tvFormat) noexcept {
    // Pinned IntervalForFormat uses 50 Hz only for VI_PAL (1); all other
    // formats, including EURGB60, use the 60 Hz-ish 16.666 ms interval.
    return tvFormat == 1u ? kPalRetraceNs : kNtscRetraceNs;
}

void Write8IfMapped(std::uint32_t address, std::uint8_t value) noexcept {
    if (Memory::Contains(address, 1u)) {
        Memory::Write8(address, value);
    }
}

void Write16IfMapped(std::uint32_t address, std::uint16_t value) noexcept {
    if (Memory::Contains(address, 2u)) {
        Memory::Write16(address, value);
    }
}

void Write32IfMapped(std::uint32_t address, std::uint32_t value) noexcept {
    if (Memory::Contains(address, 4u)) {
        Memory::Write32(address, value);
    }
}

std::uint32_t Read32IfMapped(std::uint32_t address) noexcept {
    if (!Memory::Contains(address, 4u)) {
        return 0u;
    }

    try {
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

bool AdvanceRetrace(CpuContext* cpu, std::int64_t targetNs) noexcept {
    if (!cpu) {
        return false;
    }

    bool expected = false;
    if (!g_viRetraceAdvancing.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel)) {
        return false;
    }

    // Pinned AdvanceRetrace commits pending state only when VIFlush armed it,
    // then clears the arm before publishing the new retrace count.
    if (g_viFlushArmed.exchange(false, std::memory_order_acq_rel)) {
        const std::uint32_t committedTvFormat = mkw_switch_hle_vi_commit_pending_config();
        g_viActiveTvFormat.store(committedTvFormat, std::memory_order_release);
        g_viBlack.store(g_viPendingBlack.load(std::memory_order_acquire),
                        std::memory_order_release);
        g_viNextFrameBuffer.store(
            g_viPendingNextFrameBuffer.load(std::memory_order_acquire),
            std::memory_order_release);
    }

    const std::uint32_t retraceValue =
        g_viRetraceCount.fetch_add(1u, std::memory_order_acq_rel) + 1u;
    g_viFieldOdd.store(!g_viFieldOdd.load(std::memory_order_acquire),
                       std::memory_order_release);
    g_viLastRetraceNs.store(targetNs, std::memory_order_release);

    Write32IfMapped(kViRetraceCountAddr, retraceValue);
    Write32IfMapped(kViNextFrameBufferAddr,
                    g_viPendingNextFrameBuffer.load(std::memory_order_acquire));
    Write32IfMapped(kViNextFrameBufferHwAddr,
                    g_viPendingNextFrameBuffer.load(std::memory_order_acquire));

    // Pinned AdvanceRetrace wakes the VI queue after publishing the count.
    // OSWakeupThread may cooperatively switch to a higher-priority guest fiber;
    // this host stack resumes when that guest yields again.
    cpu->gpr[3] = kViRetraceQueueAddr;
    InvokeDirectCpu<0x801AAAA4u>(cpu);

    const std::uint32_t preCallback = Read32IfMapped(kViPreRetraceCallbackAddr);
    const std::uint32_t postCallback = Read32IfMapped(kViPostRetraceCallbackAddr);
    cpu->gpr[3] = retraceValue;
    if (preCallback != 0u) {
        InvokeIndirectCpu(preCallback, cpu);
    }
    if (postCallback != 0u && Read32IfMapped(kEggSSystemAddr) != 0u) {
        InvokeIndirectCpu(postCallback, cpu);
    }

    g_viRetraceAdvancing.store(false, std::memory_order_release);
    return true;
}

} // namespace

extern "C" void mkw_switch_hle_vi_init(CpuContext* cpu) noexcept {
    if (cpu) {
        cpu->gpr[3] = 0u;
    }

    if (!Memory::IsInitialized()) {
        return;
    }

    bool expected = false;
    if (!g_viInitialized.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    // Pinned WiiCompiled's VIInit/__VIInit HLE skips Wii VI MMIO and seeds the
    // guest-visible defaults from ViState::EnsureInitializedLocked(). Keep the
    // Switch fast-track headless: this is VI bookkeeping only, not a presenter.
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
    if (!cpu) {
        return;
    }

    const bool makeBlack = cpu->gpr[3] != 0u;

    // Pinned VISetBlack writes only the pending VI state. It becomes active
    // after VIFlush/retrace; do not fabricate either event in the headless
    // fast-track. Ensure the same minimal VI initialization has happened first.
    mkw_switch_hle_vi_init(cpu);
    g_viPendingBlack.store(makeBlack, std::memory_order_release);
    cpu->gpr[3] = 0u;
}

extern "C" void mkw_switch_hle_vi_flush(CpuContext* cpu) noexcept {
    // Pinned VIFlush first ensures VI state exists, opportunistically recovers a
    // pending framebuffer from the SDK-visible guest globals when the internal
    // pending slot is still zero, then only arms pending state for a later
    // retrace. It does not itself commit VI state or present a frame.
    mkw_switch_hle_vi_init(cpu);

    if (Memory::IsInitialized() &&
        g_viPendingNextFrameBuffer.load(std::memory_order_acquire) == 0u) {
        std::uint32_t guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferAddr);
        if (guestNextFrameBuffer == 0u) {
            guestNextFrameBuffer = Read32IfMapped(kViNextFrameBufferHwAddr);
        }
        if (guestNextFrameBuffer != 0u) {
            g_viPendingNextFrameBuffer.store(guestNextFrameBuffer, std::memory_order_release);
        }
    }

    g_viFlushArmed.store(true, std::memory_order_release);

    if (cpu) {
        cpu->gpr[3] = 0u;
    }
}

extern "C" void mkw_switch_hle_vi_set_post_retrace_callback(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const std::uint32_t newCallback = cpu->gpr[3];
    mkw_switch_hle_vi_init(cpu);
    const std::uint32_t previousCallback = Read32IfMapped(kViPostRetraceCallbackAddr);
    Write32IfMapped(kViPostRetraceCallbackAddr, newCallback);
    cpu->gpr[3] = previousCallback;
}

extern "C" void mkw_switch_hle_vi_poll_retrace(CpuContext* cpu) noexcept {
    if (!cpu || !g_viInitialized.load(std::memory_order_acquire) ||
        !mkw::switch_guest_fiber::available() ||
        mkw::switch_guest_fiber::current_thread() == 0u ||
        !mkw_switch_hle_os_interrupts_enabled()) {
        return;
    }

    // A translated call boundary is only a safe interrupt-like service point
    // while guest interrupts are enabled. In particular, OSReceiveMessage
    // disables interrupts across its dequeue/output/wakeup critical section;
    // injecting a VI retrace there lets callback stack frames overwrite the
    // caller-owned r1-relative message slot. The pinned runtime services normal
    // VI polling from interrupt-enabled scheduler points and gates deferred
    // retrace callbacks on the same interrupt-enabled state.
    //
    // This poll runs at translated call boundaries, which are interrupt-like
    // service points rather than ABI calls made by the guest. AdvanceRetrace
    // intentionally uses r3 for OSWakeupThread and retrace callbacks. Preserve
    // the complete interrupted register file so a due retrace cannot clobber
    // the next translated callee's arguments (the #188 hardware regression
    // first reproduced this at EGG::Thread::start, where r3 is the EGG object).
    // Guest memory, wait queues and scheduler state remain shared and visible.
    ScopedCpuContextRestore restoreInterruptedCpu(cpu);

    // Mirror pinned VI_HLE_PollRetrace: service already-due boundaries from a
    // safe synchronous guest execution point. Never mutate guest RAM from a
    // concurrent Horizon thread. Bound catch-up so one translated call cannot
    // monopolize the host after a long pause.
    for (int catchUp = 0; catchUp < 8; ++catchUp) {
        const std::uint32_t activeTvFormat =
            g_viActiveTvFormat.load(std::memory_order_acquire);
        const std::int64_t intervalNs = RetraceIntervalNs(activeTvFormat);
        const std::int64_t targetNs =
            g_viLastRetraceNs.load(std::memory_order_acquire) + intervalNs;
        if (NowNs() < targetNs) {
            return;
        }
        if (!AdvanceRetrace(cpu, targetNs)) {
            return;
        }
    }
}

extern "C" bool mkw_switch_hle_vi_retrace_advancing() noexcept {
    return g_viRetraceAdvancing.load(std::memory_order_acquire);
}

extern "C" void mkw_switch_hle_vi_wait_for_next_retrace_poll() noexcept {
    if (!g_viInitialized.load(std::memory_order_acquire)) {
        return;
    }

    // Match pinned VI_HLE_WaitForNextRetracePoll's host-side idle pacing. The
    // current hardware frontier proves VI/post-retrace is the wake source for
    // AsyncDisplay's sync queue. Keep the wait bounded to 1 ms so this remains
    // an idle service point rather than manufacturing any timer/alarm/audio
    // guest event that hardware has not requested.
    constexpr std::int64_t kIdleSliceNs = 1'000'000ll;
    const std::uint32_t activeTvFormat =
        g_viActiveTvFormat.load(std::memory_order_acquire);
    const std::int64_t intervalNs = RetraceIntervalNs(activeTvFormat);
    const std::int64_t targetNs =
        g_viLastRetraceNs.load(std::memory_order_acquire) + intervalNs;
    const std::int64_t nowNs = NowNs();
    if (nowNs >= targetNs) {
        return;
    }

    const std::int64_t remainingNs = targetNs - nowNs;
    svcSleepThread(remainingNs < kIdleSliceNs ? remainingNs : kIdleSliceNs);
}

extern "C" void mkw_switch_hle_vi_wait_for_retrace(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    mkw_switch_hle_vi_init(cpu);

    // The current Switch runtime now has HostContext-backed guest fibers. Match
    // pinned WiiCompiled's fiber path once a real guest OSThread owns the host
    // stack: park that guest thread on VI's retrace queue and let SelectThread
    // run another READY guest until the time-driven VI poll advances the count.
    if (mkw::switch_guest_fiber::available() &&
        mkw::switch_guest_fiber::current_thread() != 0u) {
        mkw_switch_hle_os_disable_interrupts(cpu);
        const std::uint32_t irqState = cpu->gpr[3];
        const std::uint32_t retraceCount =
            g_viRetraceCount.load(std::memory_order_acquire);

        do {
            cpu->gpr[3] = kViRetraceQueueAddr;
            mkw_switch_hle_os_sleep_thread(cpu);
        } while (g_viRetraceCount.load(std::memory_order_acquire) == retraceCount);

        cpu->gpr[3] = irqState;
        mkw_switch_hle_os_restore_interrupts(cpu);
        cpu->gpr[3] = 0u;
        return;
    }

    // Before a guest fiber is registered (notably the two Video::configure
    // waits), preserve the pin's non-fiber path: pace this host stack to one VI
    // deadline and advance exactly one retrace synchronously.
    const std::uint32_t activeTvFormat =
        g_viActiveTvFormat.load(std::memory_order_acquire);
    const std::int64_t intervalNs = RetraceIntervalNs(activeTvFormat);
    const std::int64_t targetNs =
        g_viLastRetraceNs.load(std::memory_order_acquire) + intervalNs;
    const std::int64_t nowNs = NowNs();
    if (nowNs < targetNs) {
        svcSleepThread(targetNs - nowNs);
    }

    (void)AdvanceRetrace(cpu, targetNs);
    cpu->gpr[3] = 0u;
}

#endif
