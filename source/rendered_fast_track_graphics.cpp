#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK

#include "rendered_fast_track_graphics.hpp"

#include "abi_bridge.h"
#include "memory.h"

#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp.h"

#include "dolphin/gx.h"
#include "gfx/common.hpp"
#include "gx/fifo.hpp"
#include "gx_internal.h"
#include "internal.hpp"
#include "webgpu/gpu.hpp"

#include <switch.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <mutex>
#include <sys/stat.h>
#include <thread>
#include <utility>

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

extern "C" void mkw_switch_set_fast_track_stage(const char* stage) noexcept;

std::atomic_bool g_auroraFrameActive{false};
std::atomic_bool g_auroraFrameHadWork{false};
GxDisplayListState g_dlRecordState{};
bool g_alphaCompareValid = false;

extern "C" {
int g_gxFrameCount = 0;
}

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/rendered-fast-track-graphics.txt";

std::mutex g_rendererMutex;
FILE* g_report = nullptr;
bool g_mountedSdmcHere = false;
bool g_initialized = false;
uint64_t g_presentedFrames = 0;
std::atomic_bool g_loggedFirstFifoWrite{false};
std::atomic_bool g_loggedFirstFifoWork{false};

wgpu::Instance g_instance;
wgpu::Adapter g_adapter;
wgpu::Device g_device;
wgpu::Queue g_queue;
wgpu::Surface g_surface;
wgpu::Texture g_currentSurfaceTexture;

void init_report() {
    ::mkdir(kReportDir, 0777);
    g_report = std::fopen(kReportPath, "w");
    if (!g_report) {
        const Result result = fsdevMountSdmc();
        if (R_SUCCEEDED(result)) {
            g_mountedSdmcHere = true;
            ::mkdir(kReportDir, 0777);
            g_report = std::fopen(kReportPath, "w");
        }
    }
    if (g_report) {
        std::setvbuf(g_report, nullptr, _IONBF, 0);
    }
}

void close_report() {
    if (g_report) {
        std::fclose(g_report);
        g_report = nullptr;
    }
    if (g_mountedSdmcHere) {
        fsdevUnmountDevice("sdmc");
        g_mountedSdmcHere = false;
    }
}

void report(const char* format, ...) {
    char buffer[2048];

    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0) {
        return;
    }

    if (g_report) {
        std::fputs(buffer, g_report);
        std::fflush(g_report);
    }
}

void report_string_view(const char* prefix, wgpu::StringView message) {
    if (message.data == nullptr) {
        report("%s<none>\n", prefix);
        return;
    }
    if (message.length == wgpu::kStrlen) {
        report("%s%s\n", prefix, message.data);
        return;
    }
    const size_t length = std::min<size_t>(message.length, 1024);
    report("%s%.*s\n", prefix, static_cast<int>(length), message.data);
}

bool create_dawn_objects() {
    report("STAGE CREATE_INSTANCE begin\n");

    wgpu::InstanceDescriptor instanceDescriptor{};
    dawn::native::DawnInstanceDescriptor dawnDescriptor{};
    dawnDescriptor.backendValidationLevel =
        dawn::native::BackendValidationLevel::Disabled;
    instanceDescriptor.nextInChain = &dawnDescriptor;

    static constexpr wgpu::InstanceFeatureName kFeatures[] = {
        wgpu::InstanceFeatureName::TimedWaitAny,
    };
    instanceDescriptor.requiredFeatureCount = std::size(kFeatures);
    instanceDescriptor.requiredFeatures = kFeatures;

    g_instance = wgpu::CreateInstance(&instanceDescriptor);
    if (!g_instance) {
        report("STAGE CREATE_INSTANCE FAIL\n");
        return false;
    }
    report("STAGE CREATE_INSTANCE PASS\n");

    NWindow* window = nwindowGetDefault();
    if (!window) {
        report("STAGE NWINDOW FAIL\n");
        return false;
    }
    report("STAGE NWINDOW PASS\n");

    wgpu::SurfaceSourceSwitchNWindow switchWindow{};
    switchWindow.window = window;
    wgpu::SurfaceDescriptor surfaceDescriptor{};
    surfaceDescriptor.nextInChain = &switchWindow;
    g_surface = g_instance.CreateSurface(&surfaceDescriptor);
    if (!g_surface) {
        report("STAGE CREATE_SURFACE FAIL\n");
        return false;
    }
    report("STAGE CREATE_SURFACE PASS\n");

    wgpu::RequestAdapterOptions adapterOptions{};
    adapterOptions.backendType = wgpu::BackendType::Vulkan;
    adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
    adapterOptions.compatibleSurface = g_surface;

    g_instance.WaitAny(
        g_instance.RequestAdapter(
            &adapterOptions,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestAdapterStatus status,
               wgpu::Adapter adapter,
               wgpu::StringView message) {
                if (status != wgpu::RequestAdapterStatus::Success) {
                    report_string_view("RequestAdapter failed: ", message);
                    return;
                }
                g_adapter = std::move(adapter);
            }),
        UINT64_MAX);
    if (!g_adapter) {
        report("STAGE REQUEST_ADAPTER FAIL\n");
        return false;
    }
    report("STAGE REQUEST_ADAPTER PASS\n");

    wgpu::DeviceDescriptor deviceDescriptor{};
    deviceDescriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            report("Dawn uncaptured error type=%u: ",
                   static_cast<unsigned>(type));
            report_string_view("", message);
        });
    deviceDescriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&,
           wgpu::DeviceLostReason reason,
           wgpu::StringView message) {
            report("Dawn device lost reason=%u: ",
                   static_cast<unsigned>(reason));
            report_string_view("", message);
        });

    g_instance.WaitAny(
        g_adapter.RequestDevice(
            &deviceDescriptor,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestDeviceStatus status,
               wgpu::Device device,
               wgpu::StringView message) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    report_string_view("RequestDevice failed: ", message);
                    return;
                }
                g_device = std::move(device);
            }),
        UINT64_MAX);
    if (!g_device) {
        report("STAGE REQUEST_DEVICE FAIL\n");
        return false;
    }
    report("STAGE REQUEST_DEVICE PASS\n");

    g_queue = g_device.GetQueue();

    wgpu::SurfaceCapabilities capabilities{};
    if (!g_surface.GetCapabilities(g_adapter, &capabilities) ||
        capabilities.formatCount == 0 ||
        capabilities.presentModeCount == 0) {
        report("STAGE CONFIGURE_SURFACE FAIL capabilities\n");
        return false;
    }

    wgpu::SurfaceConfiguration surfaceConfig{};
    surfaceConfig.device = g_device;
    surfaceConfig.format = capabilities.formats[0];
    surfaceConfig.usage =
        wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    surfaceConfig.width = 1280;
    surfaceConfig.height = 720;
    surfaceConfig.presentMode = capabilities.presentModes[0];
    g_surface.Configure(&surfaceConfig);

    wgpu::Limits limits{};
    g_adapter.GetLimits(&limits);

    aurora::g_config = {};
    aurora::g_config.logLevel = LOG_DEBUG;
    aurora::g_config.msaa = 1;
    aurora::g_config.maxTextureAnisotropy = 1;
    aurora::g_config.allowTextureReplacements = false;
    aurora::g_config.allowTextureDumps = false;

    aurora::webgpu::g_instance = g_instance;
    aurora::webgpu::g_surface = g_surface;
    aurora::webgpu::g_device = g_device;
    aurora::webgpu::g_queue = g_queue;
    aurora::webgpu::g_backendType = wgpu::BackendType::Vulkan;
    aurora::webgpu::g_graphicsConfig = {
        .surfaceConfiguration = surfaceConfig,
        .depthFormat = wgpu::TextureFormat::Depth32Float,
        .msaaSamples = 1,
        .textureAnisotropy = 1,
        .maxTextureDimension2D = limits.maxTextureDimension2D,
    };

    const wgpu::TextureDescriptor depthDescriptor{
        .label = "RMCP01 rendered fast-track depth",
        .usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .size = {1280, 720, 1},
        .format = wgpu::TextureFormat::Depth32Float,
    };
    aurora::webgpu::g_depthBuffer = {};
    aurora::webgpu::g_depthBuffer.texture =
        g_device.CreateTexture(&depthDescriptor);
    aurora::webgpu::g_depthBuffer.view =
        aurora::webgpu::g_depthBuffer.texture.CreateView();
    aurora::webgpu::g_depthBuffer.size = depthDescriptor.size;
    aurora::webgpu::g_depthBuffer.format = depthDescriptor.format;

    report("STAGE CONFIGURE_SURFACE PASS format=%u presentMode=%u\n",
           static_cast<unsigned>(surfaceConfig.format),
           static_cast<unsigned>(surfaceConfig.presentMode));
    return true;
}

bool begin_frame_locked() {
    if (!g_initialized || g_auroraFrameActive.load(std::memory_order_acquire)) {
        return g_initialized;
    }

    wgpu::SurfaceTexture surfaceTexture{};
    g_surface.GetCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status !=
            wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surfaceTexture.status !=
            wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
        report("FRAME BEGIN FAIL GetCurrentTexture status=%u\n",
               static_cast<unsigned>(surfaceTexture.status));
        return false;
    }

    g_currentSurfaceTexture = surfaceTexture.texture;

    auto& frameBuffer = aurora::webgpu::g_frameBuffer;
    frameBuffer = {};
    frameBuffer.texture = g_currentSurfaceTexture;
    frameBuffer.view = frameBuffer.texture.CreateView();
    frameBuffer.size = {1280, 720, 1};
    frameBuffer.format =
        aurora::webgpu::g_graphicsConfig.surfaceConfiguration.format;

    if (!aurora::gfx::begin_frame()) {
        report("FRAME BEGIN FAIL aurora::gfx::begin_frame\n");
        frameBuffer = {};
        g_currentSurfaceTexture = {};
        return false;
    }

    g_auroraFrameHadWork.store(false, std::memory_order_release);
    g_auroraFrameActive.store(true, std::memory_order_release);
    return true;
}

bool present_frame_locked(bool clear) {
    if (!g_initialized) {
        return false;
    }
    if (!g_auroraFrameActive.load(std::memory_order_acquire) &&
        !begin_frame_locked()) {
        return false;
    }

    if (aurora::gx::fifo::get_buffer_size() != 0) {
        aurora::gx::fifo::drain();
    }

    GXCopyDisp(nullptr, clear ? GX_TRUE : GX_FALSE);

    wgpu::CommandEncoder encoder = g_device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);

    wgpu::CommandBuffer commands = encoder.Finish();
    g_queue.Submit(1, &commands);
    aurora::gfx::after_submit();

    if (!g_surface.Present()) {
        report("FRAME PRESENT FAIL frame=%llu\n",
               static_cast<unsigned long long>(g_presentedFrames + 1));
        return false;
    }

    const bool hadWork =
        g_auroraFrameHadWork.load(std::memory_order_acquire);
    ++g_presentedFrames;
    ++g_gxFrameCount;

    if (g_presentedFrames == 1) {
        report("PASS FIRST_RMCP01_GX_PRESENT hadWork=%u\n",
               hadWork ? 1u : 0u);
    } else if ((g_presentedFrames % 120u) == 0u) {
        report("ACTIVE RMCP01_PRESENT frames=%llu hadWork=%u\n",
               static_cast<unsigned long long>(g_presentedFrames),
               hadWork ? 1u : 0u);
    }

    aurora::webgpu::g_frameBuffer = {};
    g_currentSurfaceTexture = {};
    g_auroraFrameActive.store(false, std::memory_order_release);

    // Match the pinned runtime's pre-warm strategy: keep a valid frame open so
    // CP/BP/XF state emitted before the next draw is not dropped by Aurora.
    if (!begin_frame_locked()) {
        report("WARN next Aurora frame could not be pre-warmed\n");
    }
    return true;
}

} // namespace

extern "C" void m3_probe_log(const char* message) {
    if (!message || !g_report) {
        return;
    }
    std::fputs(message, g_report);
    std::fflush(g_report);
}

extern "C" bool mkw_switch_renderer_initialize() noexcept {
    std::scoped_lock lock(g_rendererMutex);
    if (g_initialized) {
        return true;
    }

    init_report();
    report("WiiCompiled-Switch RMCP01 rendered fast-track\n");
    report("wiicompiled pin: a135beb201042b20f390c6695ca6b26768820fb4\n");
    report("dawn-switch pin: 77029ea85250c9bdddfc2f88034afb6b5356a031\n");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6\n");
    report("goal: RMCP01 GX_HLE_FIFO_Write* -> pinned HleFifoWrite -> Aurora GX -> Dawn/NVK -> Switch\n");

    try {
        if (!create_dawn_objects()) {
            report("RESULT=FAIL renderer-init\n");
            return false;
        }

        report("STAGE AURORA_GFX_INIT begin\n");
        aurora::gfx::initialize();
        report("STAGE AURORA_GFX_INIT PASS\n");

        g_hleGxState = HleGxState{};
        g_alphaCompareValid = false;
        g_auroraFrameActive.store(false, std::memory_order_release);
        g_auroraFrameHadWork.store(false, std::memory_order_release);
        g_presentedFrames = 0;
        g_gxFrameCount = 0;
        g_loggedFirstFifoWrite.store(false, std::memory_order_release);
        g_loggedFirstFifoWork.store(false, std::memory_order_release);
        g_initialized = true;

        if (!begin_frame_locked()) {
            g_initialized = false;
            report("RESULT=FAIL first-frame-begin\n");
            return false;
        }

        report("STAGE RENDERER_READY PASS\n");
        return true;
    } catch (...) {
        g_initialized = false;
        report("RESULT=FAIL renderer-init-exception\n");
        return false;
    }
}

extern "C" void mkw_switch_renderer_shutdown() noexcept {
    std::scoped_lock lock(g_rendererMutex);
    if (!g_initialized) {
        close_report();
        return;
    }

    report("STAGE RENDERER_TEARDOWN begin frames=%llu\n",
           static_cast<unsigned long long>(g_presentedFrames));

    try {
        if (g_auroraFrameActive.exchange(false, std::memory_order_acq_rel)) {
            aurora::gfx::abort_frame();
        }

        aurora::webgpu::g_frameBuffer = {};
        g_currentSurfaceTexture = {};
        aurora::gfx::shutdown();
        aurora::webgpu::g_depthBuffer = {};

        g_surface.Unconfigure();
        aurora::webgpu::g_queue = {};
        g_queue = nullptr;
        aurora::webgpu::g_surface = {};
        g_surface = nullptr;
        aurora::webgpu::g_device = {};
        g_device = nullptr;
        g_adapter = nullptr;
        aurora::webgpu::g_instance = {};
        g_instance = nullptr;

        report("STAGE RENDERER_TEARDOWN PASS\n");
    } catch (...) {
        report("STAGE RENDERER_TEARDOWN FAIL exception\n");
    }

    g_initialized = false;
    report("RESULT=PASS renderer-shutdown\n");
    close_report();
}

void* GuestToHostPtr(uint32_t addr, size_t len) {
    if (addr == 0 || len == 0) {
        return nullptr;
    }
    return Memory::GetPointer(addr, len);
}

void BeginDisplayListRecording(uint32_t listAddr, uint32_t sizeBytes) {
    g_dlRecordState.base = listAddr;
    g_dlRecordState.size = sizeBytes;
    g_dlRecordState.writePtr = listAddr;
    g_dlRecordState.count = 0;
    g_dlRecordState.active = listAddr != 0 && sizeBytes != 0;
}

void EndDisplayListRecording() {
    if (!g_dlRecordState.active) {
        return;
    }
    g_dlRecordState.active = false;
    try {
        Memory::Write32(kDlWritePtrAddr, g_dlRecordState.writePtr);
        Memory::Write32(kDlCountAddr, g_dlRecordState.count);
    } catch (...) {
    }
}

void WriteDisplayListData(uint32_t value, uint32_t sizeBytes) {
    auto& dl = g_dlRecordState;
    if (!dl.active || dl.base == 0 || dl.size == 0 || dl.writePtr == 0) {
        return;
    }

    try {
        const uint32_t writePtr = dl.writePtr;
        switch (sizeBytes) {
        case 1:
            Memory::Write8(writePtr, static_cast<uint8_t>(value));
            break;
        case 2:
            Memory::Write16(writePtr, static_cast<uint16_t>(value));
            break;
        default:
            Memory::Write32(writePtr, value);
            sizeBytes = 4;
            break;
        }

        uint32_t nextPtr = writePtr + sizeBytes;
        const uint32_t end = dl.base + dl.size;
        if (nextPtr > end) {
            Memory::Write8(kDlFifoAddr + kDlWrapFlagOffset, 1);
            nextPtr = dl.base + (nextPtr - end);
        }
        dl.writePtr = nextPtr;
        dl.count += sizeBytes;
    } catch (...) {
    }
}

void BeginNextAuroraFrameWithRetry(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::scoped_lock lock(g_rendererMutex);
            if (begin_frame_locked()) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    report("WARN BeginNextAuroraFrameWithRetry expired\n");
}

void EnsureAuroraFrameActive() {
    if (!g_auroraFrameActive.load(std::memory_order_acquire)) {
        BeginNextAuroraFrameWithRetry();
    }
}

namespace {

void note_rmcp01_fifo_activity() {
    if (!g_loggedFirstFifoWrite.exchange(true, std::memory_order_acq_rel)) {
        report("PASS FIRST_RMCP01_FIFO_WRITE\n");
        mkw_switch_set_fast_track_stage("RMCP01_FIFO_ACTIVE");
    }

    if (g_auroraFrameHadWork.load(std::memory_order_acquire) &&
        !g_loggedFirstFifoWork.exchange(true, std::memory_order_acq_rel)) {
        report("PASS FIRST_RMCP01_FIFO_WORK\n");
        mkw_switch_set_fast_track_stage("RMCP01_FIFO_RENDER_WORK");
    }
}

} // namespace

extern "C" void GX_HLE_FIFO_WriteFloat(float value) {
    uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    HleFifoWrite(raw, 4);
    note_rmcp01_fifo_activity();
}

extern "C" void GX_HLE_FIFO_Write32(uint32_t value) {
    HleFifoWrite(value, 4);
    note_rmcp01_fifo_activity();
}

extern "C" void GX_HLE_FIFO_Write16(uint16_t value) {
    HleFifoWrite(static_cast<uint32_t>(value), 2);
    note_rmcp01_fifo_activity();
}

extern "C" void GX_HLE_FIFO_Write8(uint8_t value) {
    HleFifoWrite(static_cast<uint32_t>(value), 1);
    note_rmcp01_fifo_activity();
}

extern "C" void GX__CallDisplayList_80172f64(uint32_t listAddr, uint32_t sizeBytes) {
    if (listAddr == 0 || sizeBytes == 0 || !Memory::Contains(listAddr, sizeBytes)) {
        report("WARN display-list range rejected addr=0x%08x size=%u\n",
               listAddr,
               sizeBytes);
        return;
    }

    thread_local uint32_t depth = 0;
    if (depth >= 16) {
        report("WARN display-list recursion limit addr=0x%08x size=%u\n",
               listAddr,
               sizeBytes);
        return;
    }

    const auto* data = Memory::GetPointer(listAddr, sizeBytes);
    if (!data) {
        return;
    }

    ++depth;
    GX_HLE_FIFO_WriteBurst(data, sizeBytes);
    --depth;
}

extern "C" void mkw_switch_gx_notify_guest_ram_dma_write(
    uint32_t address,
    uint32_t sizeBytes) noexcept {
    // First rendered-RMCP01 gate: the FIFO/renderer path is live, while the
    // texture/display-list invalidation caches from the full desktop runtime
    // remain intentionally out of this minimal closure. Initial boot data is
    // still read directly from guest RAM.
    (void)address;
    (void)sizeBytes;
}

extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const bool clear = cpu->gpr[4] != 0u;
    mkw_switch_set_fast_track_stage("RMCP01_GX_COPY_DISP");

    std::scoped_lock lock(g_rendererMutex);
    try {
        if (!present_frame_locked(clear)) {
            report("FAIL RMCP01 GXCopyDisp present\n");
            mkw_switch_set_fast_track_stage("RMCP01_GX_PRESENT_FAILED");
            return;
        }
        mkw_switch_set_fast_track_stage("RMCP01_GX_PRESENTED");
    } catch (...) {
        report("FAIL RMCP01 GXCopyDisp exception\n");
        mkw_switch_set_fast_track_stage("RMCP01_GX_PRESENT_EXCEPTION");
    }
}

#endif
