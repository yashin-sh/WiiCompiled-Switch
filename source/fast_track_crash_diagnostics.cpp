#include <cstdint>

#if (defined(MKW_LOCAL_FAST_TRACK) && MKW_LOCAL_FAST_TRACK) || \
    (defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK)
#define MKW_FAST_TRACK_DIAGNOSTICS 1
#include "abi_bridge.h"
#include "guest_flat_memory.h"
#include "memory.h"
#include "switch_guest_fiber.hpp"
#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
#include "rendered_fast_track_graphics.hpp"
#endif
#include <switch.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
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
constexpr const char* kHeartbeatPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-heartbeat.txt";
constexpr const char* kMainReachedPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-main-reached.txt";
constexpr const char* kPostMainDispatchPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-post-main-dispatch.txt";
constexpr const char* kPostMainLastDispatchPath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-post-main-last-dispatch.txt";
constexpr const char* kPostMainTracePath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-post-main-trace.txt";
constexpr const char* kPostVideoTracePath =
    "sdmc:/switch/WiiCompiled-Switch/fast-track-post-video-trace.txt";
constexpr std::uint32_t kPalMainAddress = 0x8000B6B0u;
constexpr std::uint32_t kRkSystemRunAddress = 0x8000951Cu;
constexpr std::uint32_t kEggVideoConfigureAddress = 0x80243D6Cu;
constexpr std::uint32_t kViWaitForRetraceAddress = 0x801B99ECu;
constexpr std::uint32_t kPostRetraceCallbackAddress = 0x8020FCD4u;
constexpr std::uint32_t kOsReceiveMessageAddress = 0x801A7424u;
constexpr std::uint32_t kOsSleepThreadAddress = 0x801AA9B8u;
constexpr std::uint32_t kOsWakeupThreadAddress = 0x801AAAA4u;
constexpr std::uint32_t kSelectThreadAddress = 0x801A9C08u;
constexpr std::uint32_t kOsLoadContextAddress = 0x801A1F58u;
constexpr std::uint32_t kTaskThreadRunAddress = 0x80242D7Cu;
constexpr std::uint32_t kEggAsyncDisplayEndRenderAddress = 0x8020FF9Cu;
constexpr std::uint32_t kGxSetCopyFilterAddress = 0x8016FA40u;
constexpr std::uint32_t kGxFlushAddress = 0x8016E654u;
constexpr std::uint32_t kGxSetProjectionAddress = 0x8017301Cu;
constexpr std::uint32_t kGxSetViewportAddress = 0x801733B4u;
constexpr std::uint32_t kGxSetScissorAddress = 0x80173430u;
constexpr std::uint32_t kGxLoadPosMtxImmAddress = 0x8017310Cu;
constexpr std::uint32_t kGxSetCurrentMtxAddress = 0x80173214u;
constexpr std::uint32_t kGxClearVtxDescAddress = 0x8016DC34u;
constexpr std::uint32_t kGxSetVtxDescAddress = 0x8016D3A4u;
constexpr std::uint32_t kGxSetVtxAttrFmtAddress = 0x8016DC68u;
constexpr std::uint32_t kGxSetNumTexGensAddress = 0x8016E5A4u;
constexpr std::uint32_t kGxSetNumIndStagesAddress = 0x80171B38u;
constexpr std::uint32_t kGxSetNumTevStagesAddress = 0x801722A8u;
constexpr std::uint32_t kGxSetTevOpAddress = 0x80171C4Cu;
constexpr std::uint32_t kGxSetTevOrderAddress = 0x8017214Cu;
constexpr std::uint32_t kGxSetBlendModeAddress = 0x8017277Cu;
constexpr std::uint32_t kGxSetColorUpdateAddress = 0x801727CCu;
constexpr std::uint32_t kGxSetAlphaUpdateAddress = 0x801727F8u;
constexpr std::uint32_t kGxSetZModeAddress = 0x80172824u;
constexpr std::uint32_t kGxSetCullModeAddress = 0x8016F3B8u;
constexpr std::uint32_t kGxBeginAddress = 0x8016F0F0u;
constexpr std::uint32_t kGxSetNumChansAddress = 0x8017054Cu;
constexpr std::uint32_t kGxSetChanMatColorAddress = 0x80170474u;
constexpr std::uint32_t kGxSetChanCtrlAddress = 0x80170570u;
constexpr std::uint32_t kDefaultThreadContextAddr = 0x80347498u;
constexpr std::uint32_t kOSCurrentContextAddr = 0x800000D4u;
constexpr std::uint32_t kOSRunningContextAddr = 0x800000E4u;
constexpr std::uint32_t kThreadStateOffset = 0x2C8u;
constexpr std::uint32_t kThreadSuspendOffset = 0x2CCu;
constexpr std::uint32_t kThreadPriorityOffset = 0x2D0u;
constexpr std::uint32_t kThreadQueueOffset = 0x2DCu;
constexpr std::uint32_t kStaticRTextStart = 0x805103B4u;
constexpr std::uint32_t kStaticRTextEnd = 0x8088F400u;
constexpr std::uint32_t kFstAddressLowMem = 0x80000038u;
constexpr std::uint32_t kFstSizeLowMem = 0x8000003Cu;
constexpr std::uint32_t kMaxRuntimeFstSize = 0x00200000u;
constexpr std::uint64_t kDurableEarlyPostMainDispatches = 16u;
constexpr std::uint64_t kDensePostMainTraceDispatches = 48u;
constexpr std::uint64_t kMaxPostMainTraceEntries = 64u;
constexpr std::uint64_t kPostMainTraceFsyncStride = 8u;
constexpr std::uint64_t kMaxPostVideoTraceEntries = 128u;
constexpr std::uint64_t kPostVideoTraceFsyncStride = 16u;

const char* volatile g_fast_track_stage = "PROCESS_START";
bool g_liveness_files_reset = false;
bool g_main_reached = false;
bool g_post_main_dispatch_recorded = false;
std::uint64_t g_dispatch_count = 0u;
std::uint64_t g_post_main_dispatch_count = 0u;
std::uint64_t g_post_main_trace_entries = 0u;
std::uint64_t g_post_video_trace_entries = 0u;
bool g_post_video_trace_started = false;
std::uint64_t g_last_heartbeat_tick = 0u;
std::uint64_t g_rksystem_run_dispatch_count = 0u;
std::uint64_t g_staticr_dispatch_count = 0u;
std::uint64_t g_vi_wait_for_retrace_dispatch_count = 0u;
std::uint64_t g_post_retrace_callback_dispatch_count = 0u;
std::uint64_t g_os_receive_message_dispatch_count = 0u;
std::uint64_t g_os_sleep_thread_dispatch_count = 0u;
std::uint64_t g_os_wakeup_thread_dispatch_count = 0u;
std::uint64_t g_select_thread_dispatch_count = 0u;
std::uint64_t g_os_load_context_dispatch_count = 0u;
std::uint64_t g_task_thread_run_dispatch_count = 0u;
std::uint64_t g_egg_async_display_end_render_dispatch_count = 0u;
std::uint64_t g_gx_set_copy_filter_dispatch_count = 0u;
std::uint64_t g_gx_flush_dispatch_count = 0u;
std::uint64_t g_gx_set_projection_dispatch_count = 0u;
std::uint64_t g_gx_set_viewport_dispatch_count = 0u;
std::uint64_t g_gx_set_scissor_dispatch_count = 0u;
std::uint64_t g_gx_load_pos_mtx_imm_dispatch_count = 0u;
std::uint64_t g_gx_set_current_mtx_dispatch_count = 0u;
std::uint64_t g_gx_clear_vtx_desc_dispatch_count = 0u;
std::uint64_t g_gx_set_vtx_desc_dispatch_count = 0u;
std::uint64_t g_gx_set_vtx_attr_fmt_dispatch_count = 0u;
std::uint64_t g_gx_set_num_tex_gens_dispatch_count = 0u;
std::uint64_t g_gx_set_num_ind_stages_dispatch_count = 0u;
std::uint64_t g_gx_set_num_tev_stages_dispatch_count = 0u;
std::uint64_t g_gx_set_tev_op_dispatch_count = 0u;
std::uint64_t g_gx_set_tev_order_dispatch_count = 0u;
std::uint64_t g_gx_set_blend_mode_dispatch_count = 0u;
std::uint64_t g_gx_set_color_update_dispatch_count = 0u;
std::uint64_t g_gx_set_alpha_update_dispatch_count = 0u;
std::uint64_t g_gx_set_z_mode_dispatch_count = 0u;
std::uint64_t g_gx_set_cull_mode_dispatch_count = 0u;
std::uint64_t g_gx_begin_dispatch_count = 0u;
std::uint64_t g_gx_set_num_chans_dispatch_count = 0u;
std::uint64_t g_gx_set_chan_mat_color_dispatch_count = 0u;
std::uint64_t g_gx_set_chan_ctrl_dispatch_count = 0u;

struct FstSnapshot {
    std::uint32_t address = 0u;
    std::uint32_t size = 0u;
    bool valid = false;
};

std::uint32_t read32_or_zero(std::uint32_t address) noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(address, 4u)) {
        return 0u;
    }
    try {
        return Memory::Read32(address);
    } catch (...) {
        return 0u;
    }
}

std::uint16_t read16_or_zero(std::uint32_t address) noexcept {
    if (!Memory::IsInitialized() || !Memory::Contains(address, 2u)) {
        return 0u;
    }
    try {
        return Memory::Read16(address);
    } catch (...) {
        return 0u;
    }
}

struct SchedulerSnapshot {
    std::uint32_t fiber_current = 0u;
    std::uint32_t os_current = 0u;
    std::uint32_t os_running = 0u;
    std::uint16_t default_state = 0u;
    std::int32_t default_suspend = 0;
    std::int32_t default_priority = 0;
    std::uint32_t default_queue = 0u;
    std::uint16_t active_state = 0u;
    std::int32_t active_suspend = 0;
    std::int32_t active_priority = 0;
    std::uint32_t active_queue = 0u;
};

SchedulerSnapshot read_scheduler_snapshot() noexcept {
    SchedulerSnapshot snapshot{};
    snapshot.fiber_current = mkw::switch_guest_fiber::current_thread();
    snapshot.os_current = read32_or_zero(kOSCurrentContextAddr);
    snapshot.os_running = read32_or_zero(kOSRunningContextAddr);

    if (Memory::IsInitialized() &&
        Memory::Contains(kDefaultThreadContextAddr + kThreadQueueOffset, 4u)) {
        snapshot.default_state =
            read16_or_zero(kDefaultThreadContextAddr + kThreadStateOffset);
        snapshot.default_suspend = static_cast<std::int32_t>(
            read32_or_zero(kDefaultThreadContextAddr + kThreadSuspendOffset));
        snapshot.default_priority = static_cast<std::int32_t>(
            read32_or_zero(kDefaultThreadContextAddr + kThreadPriorityOffset));
        snapshot.default_queue =
            read32_or_zero(kDefaultThreadContextAddr + kThreadQueueOffset);
    }

    const std::uint32_t active =
        snapshot.fiber_current != 0u ? snapshot.fiber_current : snapshot.os_running;
    if (active != 0u &&
        Memory::IsInitialized() &&
        Memory::Contains(active + kThreadQueueOffset, 4u)) {
        snapshot.active_state = read16_or_zero(active + kThreadStateOffset);
        snapshot.active_suspend = static_cast<std::int32_t>(
            read32_or_zero(active + kThreadSuspendOffset));
        snapshot.active_priority = static_cast<std::int32_t>(
            read32_or_zero(active + kThreadPriorityOffset));
        snapshot.active_queue = read32_or_zero(active + kThreadQueueOffset);
    }
    return snapshot;
}

FstSnapshot read_fst_snapshot() noexcept {
    FstSnapshot snapshot{};
    if (!Memory::IsInitialized() ||
        !Memory::Contains(kFstAddressLowMem, 8u)) {
        return snapshot;
    }

    snapshot.address = Memory::Read32(kFstAddressLowMem);
    snapshot.size = Memory::Read32(kFstSizeLowMem);
    if (snapshot.address == 0u || snapshot.size < 12u ||
        snapshot.size > kMaxRuntimeFstSize ||
        !Memory::Contains(snapshot.address, snapshot.size)) {
        return snapshot;
    }

    const std::uint32_t rootWord = Memory::Read32(snapshot.address);
    const std::uint32_t entryCount = Memory::Read32(snapshot.address + 8u);
    snapshot.valid =
        (rootWord & 0xFF000000u) == 0x01000000u &&
        entryCount != 0u && entryCount <= 0x00010000u;
    return snapshot;
}

const char* post_main_phase_name(std::uint32_t target) noexcept {
    switch (target) {
    case 0x80008EF0u:
        return "System::RKSystem::main";
    case 0x80008FB4u:
        return "EGG::BaseSystem::initialize";
    case 0x80009194u:
        return "System::RKSystem::initialize";
    case 0x8000951Cu:
        return "System::RKSystem::run";
    case 0x80243D18u:
        return "EGG::Video::initialize";
    case 0x80243D6Cu:
        return "EGG::Video::configure";
    case 0x80242D7Cu:
        return "EGG::TaskThread::run";
    case 0x8020FF9Cu:
        return "EGG::AsyncDisplay::endRender";
    case 0x8016FA40u:
        return "GXSetCopyFilter";
    case 0x8016E654u:
        return "GXFlush";
    case 0x8017301Cu:
        return "GXSetProjection";
    case 0x801733B4u:
        return "GXSetViewport";
    case 0x80173430u:
        return "GXSetScissor";
    case 0x8017310Cu:
        return "GXLoadPosMtxImm";
    case 0x80173214u:
        return "GXSetCurrentMtx";
    case 0x8016DC34u:
        return "GXClearVtxDesc";
    case 0x8016D3A4u:
        return "GXSetVtxDesc";
    case 0x8016DC68u:
        return "GXSetVtxAttrFmt";
    case 0x8016E5A4u:
        return "GXSetNumTexGens";
    case 0x80171B38u:
        return "GXSetNumIndStages";
    case 0x801722A8u:
        return "GXSetNumTevStages";
    case 0x80171C4Cu:
        return "GXSetTevOp";
    case 0x8017214Cu:
        return "GXSetTevOrder";
    case 0x8017277Cu:
        return "GXSetBlendMode";
    case 0x801727CCu:
        return "GXSetColorUpdate";
    case 0x801727F8u:
        return "GXSetAlphaUpdate";
    case 0x80172824u:
        return "GXSetZMode";
    case 0x8016F3B8u:
        return "GXSetCullMode";
    case 0x8016F0F0u:
        return "GXBegin";
    case 0x8017054Cu:
        return "GXSetNumChans";
    case 0x80170474u:
        return "GXSetChanMatColor";
    case 0x80170570u:
        return "GXSetChanCtrl";
    default:
        return "-";
    }
}

bool is_durable_post_main_phase_target(std::uint32_t target) noexcept {
    switch (target) {
    // Keep this list aligned with the pinned WiiCompiled shard emitter's
    // runtime-toggle diagnostics / phase-tracing cold path. RKSystem::run is
    // an explicit application milestone in addition to that keep-list.
    case 0x80008EF0u:
    case 0x80008FB4u:
    case 0x80009194u:
    case 0x8000951Cu:
    case 0x80243D18u:
    case 0x80243D6Cu:
    case 0x808897F0u:
    case 0x802226D8u:
    case 0x805C3218u:
    case 0x805E7460u:
    case 0x8063C470u:
    case 0x8063C4D4u:
    case 0x8063C560u:
    case 0x8063C714u:
    case 0x80198CA8u:
    case 0x80199038u:
    case 0x801992A8u:
    case 0x801998A4u:
    case 0x80226C78u:
    case 0x80226EBCu:
    case 0x80229814u:
    case 0x80229C5Cu:
    case 0x80229DCCu:
    case 0x80229DD8u:
    case 0x801A7424u:
    case 0x80242D7Cu:
    case 0x8020FF9Cu:
    case 0x8017301Cu:
    case 0x801733B4u:
    case 0x80173430u:
    case 0x8017310Cu:
    case 0x80173214u:
    case 0x8016DC34u:
    case 0x8016D3A4u:
    case 0x8016DC68u:
    case 0x8017054Cu:
    case 0x80170474u:
    case 0x80672CC8u:
        return true;
    default:
        return false;
    }
}

void write_all(int fd, const char* data, std::size_t size) noexcept {
    std::size_t written = 0u;
    while (written < size) {
        const ssize_t rc = ::write(fd, data + written, size - written);
        if (rc <= 0) {
            break;
        }
        written += static_cast<std::size_t>(rc);
    }
}

void write_atomicish(const char* path, const char* data, std::size_t size) noexcept {
    const int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return;
    }
    write_all(fd, data, size);
    ::fsync(fd);
    ::close(fd);
}

std::size_t format_post_main_trace_line(
    char* buffer,
    std::size_t capacity,
    std::uint64_t trace_index,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
    if (!buffer || capacity == 0u) {
        return 0u;
    }

    const std::uint32_t guest_pc = cpu ? cpu->pc : 0u;
    const std::uint32_t r1 = cpu ? cpu->gpr[1] : 0u;
    const std::uint32_t r2 = cpu ? cpu->gpr[2] : 0u;
    const std::uint32_t r3 = cpu ? cpu->gpr[3] : 0u;
    const std::uint32_t r13 = cpu ? cpu->gpr[13] : 0u;
    const std::uint32_t lr = cpu ? cpu->lr : 0u;
    const std::uint32_t fiber = mkw::switch_guest_fiber::current_thread();
    const int n = std::snprintf(
        buffer,
        capacity,
        "[%02llu] post-main=%llu dispatch=%llu target=0x%08x pc=0x%08x "
        "r1=0x%08x r2=0x%08x r3=0x%08x r13=0x%08x lr=0x%08x fiber=0x%08x stage=%s phase=%s\n",
        static_cast<unsigned long long>(trace_index),
        static_cast<unsigned long long>(g_post_main_dispatch_count),
        static_cast<unsigned long long>(g_dispatch_count),
        target,
        guest_pc,
        r1,
        r2,
        r3,
        r13,
        lr,
        fiber,
        g_fast_track_stage,
        post_main_phase_name(target));
    if (n <= 0) {
        return 0u;
    }
    return static_cast<std::size_t>(n) < capacity ? static_cast<std::size_t>(n) : capacity - 1u;
}

void append_post_main_trace(std::uint32_t target, CpuContext* cpu, bool phase_target) noexcept {
    if (g_post_main_trace_entries >= kMaxPostMainTraceEntries) {
        return;
    }
    if (g_post_main_dispatch_count > kDensePostMainTraceDispatches && !phase_target) {
        return;
    }

    const int fd = ::open(kPostMainTracePath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        return;
    }

    if (g_post_main_trace_entries == 0u) {
        constexpr const char kHeader[] =
            "WiiCompiled-Switch bounded post-main translated trace\n"
            "=====================================================\n";
        write_all(fd, kHeader, sizeof(kHeader) - 1u);
    }

    ++g_post_main_trace_entries;
    char buffer[512];
    const std::size_t size = format_post_main_trace_line(
        buffer,
        sizeof(buffer),
        g_post_main_trace_entries,
        target,
        cpu);
    if (size != 0u) {
        write_all(fd, buffer, size);
    }

    const bool durable_now =
        phase_target || g_post_main_trace_entries == 1u ||
        g_post_main_trace_entries == kMaxPostMainTraceEntries ||
        g_post_main_dispatch_count == kDensePostMainTraceDispatches ||
        (g_post_main_dispatch_count <= kDensePostMainTraceDispatches &&
         g_post_main_dispatch_count % kPostMainTraceFsyncStride == 0u);
    if (durable_now) {
        ::fsync(fd);
    }
    ::close(fd);
}

void append_post_video_trace(std::uint32_t target, CpuContext* cpu) noexcept {
    if (!g_post_video_trace_started ||
        g_post_video_trace_entries >= kMaxPostVideoTraceEntries) {
        return;
    }

    const int fd = ::open(kPostVideoTracePath, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        return;
    }

    if (g_post_video_trace_entries == 0u) {
        constexpr const char kHeader[] =
            "WiiCompiled-Switch bounded post-video translated trace\n"
            "======================================================\n";
        write_all(fd, kHeader, sizeof(kHeader) - 1u);
    }

    ++g_post_video_trace_entries;
    char buffer[512];
    const std::size_t size = format_post_main_trace_line(
        buffer,
        sizeof(buffer),
        g_post_video_trace_entries,
        target,
        cpu);
    if (size != 0u) {
        write_all(fd, buffer, size);
    }

    if (g_post_video_trace_entries == 1u ||
        g_post_video_trace_entries == kMaxPostVideoTraceEntries ||
        g_post_video_trace_entries % kPostVideoTraceFsyncStride == 0u) {
        ::fsync(fd);
    }
    ::close(fd);
}

void reset_liveness_files_once() noexcept {
    if (g_liveness_files_reset) {
        return;
    }
    g_liveness_files_reset = true;
    ::unlink(kDispatchPath);
    ::unlink(kExceptionPath);
    ::unlink(kHeartbeatPath);
    ::unlink(kMainReachedPath);
    ::unlink(kPostMainDispatchPath);
    ::unlink(kPostMainLastDispatchPath);
    ::unlink(kPostMainTracePath);
    ::unlink(kPostVideoTracePath);
}

void write_liveness_record(
    const char* path,
    const char* title,
    std::uint32_t target,
    CpuContext* cpu) noexcept {
    char buffer[4096];
    const std::uint32_t guest_pc = cpu ? cpu->pc : 0u;
    const std::uint32_t r1 = cpu ? cpu->gpr[1] : 0u;
    const std::uint32_t r2 = cpu ? cpu->gpr[2] : 0u;
    const std::uint32_t r3 = cpu ? cpu->gpr[3] : 0u;
    const std::uint32_t r13 = cpu ? cpu->gpr[13] : 0u;
    const FstSnapshot fst = read_fst_snapshot();
    const SchedulerSnapshot scheduler = read_scheduler_snapshot();

    const int n = std::snprintf(
        buffer,
        sizeof(buffer),
        "%s\n"
        "========================================\n"
        "dispatch count        : %llu\n"
        "post-main dispatch    : %llu\n"
        "last target           : 0x%08x\n"
        "guest pc              : 0x%08x\n"
        "r1                    : 0x%08x\n"
        "r2                    : 0x%08x\n"
        "r3                    : 0x%08x\n"
        "r13                   : 0x%08x\n"
        "fast-track stage      : %s\n"
        "PAL main              : 0x%08x\n"
        "main reached          : %s\n"
        "RKSystem::run hits    : %llu\n"
        "StaticR dispatches    : %llu\n"
        "VIWaitForRetrace hits : %llu\n"
        "PostRetrace cb hits   : %llu\n"
        "OSReceiveMessage hits : %llu\n"
        "OSSleepThread hits    : %llu\n"
        "OSWakeupThread hits   : %llu\n"
        "SelectThread hits     : %llu\n"
        "OSLoadContext hits    : %llu\n"
        "TaskThread::run hits  : %llu\n"
        "AsyncDisplay endRender: %llu\n"
        "GXSetCopyFilter hits  : %llu\n"
        "GXFlush hits          : %llu\n"
        "GXSetProjection hits  : %llu\n"
        "GXSetViewport hits    : %llu\n"
        "GXSetScissor hits     : %llu\n"
        "GXLoadPosMtxImm hits  : %llu\n"
        "GXSetCurrentMtx hits  : %llu\n"
        "GXClearVtxDesc hits   : %llu\n"
        "GXSetVtxDesc hits     : %llu\n"
        "GXSetVtxAttrFmt hits  : %llu\n"
        "GXSetNumTexGens hits  : %llu\n"
        "GXSetNumIndStages hits: %llu\n"
        "GXSetNumTevStages hits: %llu\n"
        "GXSetTevOp hits       : %llu\n"
        "GXSetTevOrder hits    : %llu\n"
        "GXSetBlendMode hits   : %llu\n"
        "GXSetColorUpdate hits : %llu\n"
        "GXSetAlphaUpdate hits : %llu\n"
        "GXSetZMode hits       : %llu\n"
        "GXSetCullMode hits    : %llu\n"
        "GXBegin hits          : %llu\n"
        "GXSetNumChans hits    : %llu\n"
        "GXSetChanMatColor hits: %llu\n"
        "GXSetChanCtrl hits    : %llu\n"
        "guest fiber current   : 0x%08x\n"
        "OS current/running    : 0x%08x / 0x%08x\n"
        "default thread s/s/p  : %u / %d / %d\n"
        "default thread queue  : 0x%08x\n"
        "active thread s/s/p   : %u / %d / %d\n"
        "active thread queue   : 0x%08x\n"
        "FST address           : 0x%08x\n"
        "FST size              : 0x%08x\n"
        "FST structurally valid: %s\n",
        title,
        static_cast<unsigned long long>(g_dispatch_count),
        static_cast<unsigned long long>(g_post_main_dispatch_count),
        target,
        guest_pc,
        r1,
        r2,
        r3,
        r13,
        g_fast_track_stage,
        kPalMainAddress,
        g_main_reached ? "YES" : "NO",
        static_cast<unsigned long long>(g_rksystem_run_dispatch_count),
        static_cast<unsigned long long>(g_staticr_dispatch_count),
        static_cast<unsigned long long>(g_vi_wait_for_retrace_dispatch_count),
        static_cast<unsigned long long>(g_post_retrace_callback_dispatch_count),
        static_cast<unsigned long long>(g_os_receive_message_dispatch_count),
        static_cast<unsigned long long>(g_os_sleep_thread_dispatch_count),
        static_cast<unsigned long long>(g_os_wakeup_thread_dispatch_count),
        static_cast<unsigned long long>(g_select_thread_dispatch_count),
        static_cast<unsigned long long>(g_os_load_context_dispatch_count),
        static_cast<unsigned long long>(g_task_thread_run_dispatch_count),
        static_cast<unsigned long long>(g_egg_async_display_end_render_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_copy_filter_dispatch_count),
        static_cast<unsigned long long>(g_gx_flush_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_projection_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_viewport_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_scissor_dispatch_count),
        static_cast<unsigned long long>(g_gx_load_pos_mtx_imm_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_current_mtx_dispatch_count),
        static_cast<unsigned long long>(g_gx_clear_vtx_desc_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_vtx_desc_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_vtx_attr_fmt_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_num_tex_gens_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_num_ind_stages_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_num_tev_stages_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_tev_op_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_tev_order_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_blend_mode_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_color_update_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_alpha_update_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_z_mode_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_cull_mode_dispatch_count),
        static_cast<unsigned long long>(g_gx_begin_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_num_chans_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_chan_mat_color_dispatch_count),
        static_cast<unsigned long long>(g_gx_set_chan_ctrl_dispatch_count),
        scheduler.fiber_current,
        scheduler.os_current,
        scheduler.os_running,
        static_cast<unsigned>(scheduler.default_state),
        scheduler.default_suspend,
        scheduler.default_priority,
        scheduler.default_queue,
        static_cast<unsigned>(scheduler.active_state),
        scheduler.active_suspend,
        scheduler.active_priority,
        scheduler.active_queue,
        fst.address,
        fst.size,
        fst.valid ? "YES" : "NO");
    if (n <= 0) {
        return;
    }

    std::size_t size =
        static_cast<std::size_t>(n) < sizeof(buffer) ? static_cast<std::size_t>(n)
                                                     : sizeof(buffer) - 1u;

#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK
    const auto renderer = mkw_switch_renderer_diagnostics_snapshot();
    if (size < sizeof(buffer) - 1u) {
        const int renderer_n = std::snprintf(
            buffer + size,
            sizeof(buffer) - size,
            "renderer initialized  : %s\n"
            "renderer frame active : %s\n"
            "RMCP01 FIFO writes    : %llu\n"
            "FIFO write8/16/32/f32 : %llu/%llu/%llu/%llu\n"
            "BP regs 49/4a/4d/other: %llu/%llu/%llu/%llu\n"
            "last FIFO size/value  : %u / 0x%08x\n"
            "last BP word          : 0x%08x\n"
            "display-list calls    : %llu\n"
            "FIFO produced work    : %s\n"
            "GXCopyDisp calls      : %llu\n"
            "present successes     : %llu\n"
            "present failures      : %llu\n",
            renderer.initialized ? "YES" : "NO",
            renderer.frame_active ? "YES" : "NO",
            static_cast<unsigned long long>(renderer.fifo_write_calls),
            static_cast<unsigned long long>(renderer.fifo_write8_calls),
            static_cast<unsigned long long>(renderer.fifo_write16_calls),
            static_cast<unsigned long long>(renderer.fifo_write32_calls),
            static_cast<unsigned long long>(renderer.fifo_write_float_calls),
            static_cast<unsigned long long>(renderer.bp_reg_49_calls),
            static_cast<unsigned long long>(renderer.bp_reg_4a_calls),
            static_cast<unsigned long long>(renderer.bp_reg_4d_calls),
            static_cast<unsigned long long>(renderer.bp_reg_other_calls),
            static_cast<unsigned>(renderer.last_fifo_size),
            renderer.last_fifo_value,
            renderer.last_bp_word,
            static_cast<unsigned long long>(renderer.display_list_calls),
            renderer.fifo_work_seen ? "YES" : "NO",
            static_cast<unsigned long long>(renderer.gx_copy_disp_calls),
            static_cast<unsigned long long>(renderer.present_successes),
            static_cast<unsigned long long>(renderer.present_failures));
        if (renderer_n > 0) {
            const std::size_t appended =
                static_cast<std::size_t>(renderer_n) < sizeof(buffer) - size
                    ? static_cast<std::size_t>(renderer_n)
                    : sizeof(buffer) - size - 1u;
            size += appended;
        }
    }
#endif

    write_atomicish(path, buffer, size);
}

} // namespace
#endif

#if MKW_FAST_TRACK_DIAGNOSTICS && \
    defined(MKW_SYNTHETIC_FAST_TRACK) && MKW_SYNTHETIC_FAST_TRACK
extern "C" bool mkw_switch_post_main_trace_synthetic_probe() noexcept {
    CpuContext cpu{};
    cpu.pc = 0x800060A4u;
    cpu.gpr[1] = 0x80399178u;
    cpu.gpr[2] = 0x8038EFA0u;
    cpu.gpr[3] = 0u;
    cpu.gpr[13] = 0x8038CC00u;

    char buffer[512];
    const std::size_t size = format_post_main_trace_line(buffer, sizeof(buffer), 1u, 0x80008EF0u, &cpu);
    return size != 0u &&
           std::strstr(buffer, "target=0x80008ef0") != nullptr &&
           std::strstr(buffer, "phase=System::RKSystem::main") != nullptr &&
           is_durable_post_main_phase_target(0x80243D18u);
}
#endif

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept {
#if MKW_FAST_TRACK_DIAGNOSTICS
    g_fast_track_stage = stage ? stage : "<null>";
#else
    (void)stage;
#endif
}

extern "C" void mkw_switch_note_translated_dispatch(
    std::uint32_t target,
    CpuContext* cpu) noexcept {
#if MKW_FAST_TRACK_DIAGNOSTICS
    reset_liveness_files_once();
    ++g_dispatch_count;
    if (target == kRkSystemRunAddress) {
        ++g_rksystem_run_dispatch_count;
    }
    if (target >= kStaticRTextStart && target < kStaticRTextEnd) {
        ++g_staticr_dispatch_count;
    }
    if (target == kViWaitForRetraceAddress) {
        ++g_vi_wait_for_retrace_dispatch_count;
    }
    if (target == kPostRetraceCallbackAddress) {
        ++g_post_retrace_callback_dispatch_count;
    }
    if (target == kOsReceiveMessageAddress) {
        ++g_os_receive_message_dispatch_count;
    }
    if (target == kOsSleepThreadAddress) {
        ++g_os_sleep_thread_dispatch_count;
    }
    if (target == kOsWakeupThreadAddress) {
        ++g_os_wakeup_thread_dispatch_count;
    }
    if (target == kSelectThreadAddress) {
        ++g_select_thread_dispatch_count;
    }
    if (target == kOsLoadContextAddress) {
        ++g_os_load_context_dispatch_count;
    }
    if (target == kTaskThreadRunAddress) {
        ++g_task_thread_run_dispatch_count;
    }
    if (target == kEggAsyncDisplayEndRenderAddress) {
        ++g_egg_async_display_end_render_dispatch_count;
    }
    if (target == kGxSetCopyFilterAddress) {
        ++g_gx_set_copy_filter_dispatch_count;
    }
    if (target == kGxFlushAddress) {
        ++g_gx_flush_dispatch_count;
    }
    if (target == kGxSetProjectionAddress) {
        ++g_gx_set_projection_dispatch_count;
    }
    if (target == kGxSetViewportAddress) {
        ++g_gx_set_viewport_dispatch_count;
    }
    if (target == kGxSetScissorAddress) {
        ++g_gx_set_scissor_dispatch_count;
    }
    if (target == kGxLoadPosMtxImmAddress) {
        ++g_gx_load_pos_mtx_imm_dispatch_count;
    }
    if (target == kGxSetCurrentMtxAddress) {
        ++g_gx_set_current_mtx_dispatch_count;
    }
    if (target == kGxClearVtxDescAddress) {
        ++g_gx_clear_vtx_desc_dispatch_count;
    }
    if (target == kGxSetVtxDescAddress) {
        ++g_gx_set_vtx_desc_dispatch_count;
    }
    if (target == kGxSetVtxAttrFmtAddress) {
        ++g_gx_set_vtx_attr_fmt_dispatch_count;
    }
    if (target == kGxSetNumTexGensAddress) {
        ++g_gx_set_num_tex_gens_dispatch_count;
    }
    if (target == kGxSetNumIndStagesAddress) {
        ++g_gx_set_num_ind_stages_dispatch_count;
    }
    if (target == kGxSetNumTevStagesAddress) {
        ++g_gx_set_num_tev_stages_dispatch_count;
    }
    if (target == kGxSetTevOpAddress) {
        ++g_gx_set_tev_op_dispatch_count;
    }
    if (target == kGxSetTevOrderAddress) {
        ++g_gx_set_tev_order_dispatch_count;
    }
    if (target == kGxSetBlendModeAddress) {
        ++g_gx_set_blend_mode_dispatch_count;
    }
    if (target == kGxSetColorUpdateAddress) {
        ++g_gx_set_color_update_dispatch_count;
    }
    if (target == kGxSetAlphaUpdateAddress) {
        ++g_gx_set_alpha_update_dispatch_count;
    }
    if (target == kGxSetZModeAddress) {
        ++g_gx_set_z_mode_dispatch_count;
    }
    if (target == kGxSetCullModeAddress) {
        ++g_gx_set_cull_mode_dispatch_count;
    }
    if (target == kGxBeginAddress) {
        ++g_gx_begin_dispatch_count;
    }
    if (target == kGxSetNumChansAddress) {
        ++g_gx_set_num_chans_dispatch_count;
    }
    if (target == kGxSetChanMatColorAddress) {
        ++g_gx_set_chan_mat_color_dispatch_count;
    }
    if (target == kGxSetChanCtrlAddress) {
        ++g_gx_set_chan_ctrl_dispatch_count;
    }
    if (target == kEggVideoConfigureAddress && !g_post_video_trace_started) {
        g_post_video_trace_started = true;
    }

    if (target == kPalMainAddress && !g_main_reached) {
        g_main_reached = true;
        g_fast_track_stage = "GUEST_MAIN_REACHED";
        write_liveness_record(
            kMainReachedPath,
            "WiiCompiled-Switch PAL main reached",
            target,
            cpu);
    }

    const bool post_main_dispatch =
        g_main_reached && target != kPalMainAddress;
    if (post_main_dispatch) {
        ++g_post_main_dispatch_count;
    }

    const bool first_post_main_dispatch =
        post_main_dispatch && !g_post_main_dispatch_recorded;
    if (first_post_main_dispatch) {
        g_post_main_dispatch_recorded = true;
        g_fast_track_stage = "GUEST_POST_MAIN_ACTIVE";
        write_liveness_record(
            kPostMainDispatchPath,
            "WiiCompiled-Switch first post-main translated dispatch",
            target,
            cpu);
    }

    const bool phase_target =
        post_main_dispatch && is_durable_post_main_phase_target(target);
    const bool durable_post_main_snapshot =
        post_main_dispatch &&
        (g_post_main_dispatch_count <= kDurableEarlyPostMainDispatches || phase_target);
    if (durable_post_main_snapshot) {
        write_liveness_record(
            kPostMainLastDispatchPath,
            "WiiCompiled-Switch durable post-main translated dispatch",
            target,
            cpu);
    }
    if (post_main_dispatch) {
        append_post_main_trace(target, cpu, phase_target);
        append_post_video_trace(target, cpu);
    }

    const std::uint64_t now = armGetSystemTick();
    const std::uint64_t frequency = armGetSystemTickFreq();
    const bool heartbeat_due =
        g_last_heartbeat_tick == 0u || frequency == 0u ||
        now - g_last_heartbeat_tick >= frequency;
    if (heartbeat_due || target == kPalMainAddress || first_post_main_dispatch) {
        g_last_heartbeat_tick = now;
        write_liveness_record(
            kHeartbeatPath,
            "WiiCompiled-Switch translated liveness heartbeat",
            target,
            cpu);
    }
#else
    (void)target;
    (void)cpu;
#endif
}

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
    const std::uint32_t r4 = cpu ? cpu->gpr[4] : 0u;
    const std::uint32_t r5 = cpu ? cpu->gpr[5] : 0u;
    const std::uint32_t r6 = cpu ? cpu->gpr[6] : 0u;
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
        "r4                    : 0x%08x\n"
        "r5                    : 0x%08x\n"
        "r6                    : 0x%08x\n"
        "r13                   : 0x%08x\n"
        "fast-track stage      : %s\n"
        "action                : abort after durable blocker record\n",
        kind ? kind : "UNKNOWN",
        target,
        guest_pc,
        r1,
        r2,
        r3,
        r4,
        r5,
        r6,
        r13,
        g_fast_track_stage);
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
    const std::uint32_t derived_guest_address =
        far_in_guest_window ? static_cast<std::uint32_t>(far - guest_base) : 0u;

    char buffer[4096];
    const int n = std::snprintf(
        buffer,
        sizeof(buffer),
        "WiiCompiled-Switch libnx exception\n"
        "==================================\n"
        "error desc            : 0x%08x\n"
        "fast-track stage      : %s\n"
        "aarch64 pc            : 0x%016llx\n"
        "aarch64 lr            : 0x%016llx\n"
        "aarch64 sp            : 0x%016llx\n"
        "fault address (FAR)   : 0x%016llx\n"
        "ESR                   : 0x%08x\n"
        "x0                    : 0x%016llx\n"
        "x1                    : 0x%016llx\n"
        "x2                    : 0x%016llx\n"
        "x3                    : 0x%016llx\n"
        "x4                    : 0x%016llx\n"
        "x5                    : 0x%016llx\n"
        "x6                    : 0x%016llx\n"
        "x7                    : 0x%016llx\n"
        "x8                    : 0x%016llx\n"
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
        g_fast_track_stage,
        static_cast<unsigned long long>(ctx->pc.x),
        static_cast<unsigned long long>(ctx->lr.x),
        static_cast<unsigned long long>(ctx->sp.x),
        static_cast<unsigned long long>(ctx->far.x),
        ctx->esr,
        static_cast<unsigned long long>(ctx->cpu_gprs[0].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[1].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[2].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[3].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[4].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[5].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[6].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[7].x),
        static_cast<unsigned long long>(ctx->cpu_gprs[8].x),
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
