#if defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK

#include "rendered_fast_track_graphics.hpp"
#include "switch_texture_copy_lifetime.hpp"
#include "surface_presenter.hpp"
#include <memory>

#include "abi_bridge.h"
#include "memory.h"

#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp.h"

#include "dolphin/gx.h"
#include "gfx/common.hpp"
#include "aurora_fifo_transport.hpp"
#include "gx_internal.h"
#include "internal.hpp"
#include "webgpu/gpu.hpp"

#include <switch.h>
#if defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
#include "frame_dump_readback.hpp"
#include "rendered_frame_dump.hpp"
#include <memory>
#endif
#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
extern "C" void mkw_switch_fifo_capture_start() noexcept;
extern "C" void mkw_switch_fifo_capture_present(bool) noexcept;
extern "C" void mkw_switch_fifo_capture_begin_frame() noexcept;
extern "C" void mkw_switch_fifo_capture_shutdown() noexcept;
#endif

#include <algorithm>
#include <array>
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
std::unique_ptr<mkw::presentation::Presenter> g_presenter;
uint64_t g_presentedFrames = 0;
std::atomic_bool g_loggedFirstFifoWrite{false};
std::atomic_bool g_loggedFirstFifoWork{false};
std::atomic_uint64_t g_fifoWriteCalls{0};
std::atomic_uint64_t g_fifoWrite8Calls{0};
std::atomic_uint64_t g_fifoWrite16Calls{0};
std::atomic_uint64_t g_fifoWrite32Calls{0};
std::atomic_uint64_t g_fifoWriteFloatCalls{0};
std::atomic_uint64_t g_bpReg49Calls{0};
std::atomic_uint64_t g_bpReg4aCalls{0};
std::atomic_uint64_t g_bpReg4dCalls{0};
std::atomic_uint64_t g_bpRegOtherCalls{0};
std::atomic_uint32_t g_lastFifoValue{0};
std::atomic_uint32_t g_lastBpWord{0};
std::atomic_uint8_t g_lastFifoSize{0};
std::atomic_bool g_pendingBpWord{false};
std::atomic_uint64_t g_displayListCalls{0};
std::atomic_uint64_t g_gxCopyDispCalls{0};
std::atomic_uint64_t g_presentSuccesses{0};
std::atomic_uint64_t g_presentFailures{0};

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

    const wgpu::TextureDescriptor colorDescriptor{
        .label = "RMCP01 persistent EFB color",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc,
        .size = {1280, 720, 1},
        .format = surfaceConfig.format,
    };
    auto& efb = aurora::webgpu::g_frameBuffer;
    efb = {};
    efb.texture = g_device.CreateTexture(&colorDescriptor);
    efb.view = efb.texture.CreateView();
    efb.size = colorDescriptor.size;
    efb.format = colorDescriptor.format;
    g_presenter = std::make_unique<mkw::presentation::Presenter>(g_device, surfaceConfig.format);

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

    if (!aurora::gfx::begin_frame()) {
        report("FRAME BEGIN FAIL aurora::gfx::begin_frame\n");
        g_currentSurfaceTexture = {};
        return false;
    }

#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
    mkw_switch_fifo_capture_begin_frame();
#endif
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
    const auto displayCopy = aurora::webgpu::current_present_source();

    wgpu::CommandEncoder encoder = g_device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);
    g_presenter->encode(encoder, displayCopy.texture, g_currentSurfaceTexture);

    wgpu::CommandBuffer commands = encoder.Finish();
    g_queue.Submit(1, &commands);
    aurora::gfx::after_submit();

#if defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
    std::array<std::unique_ptr<mkw::frame_dump::Readback>, 3> readbacks;
    if (mkw::frame_dump::enabled()) {
        try {

            // The final surface includes Aurora's presentation scaling. Copy
            // after render, before Present may release the swapchain image.
            // A separate submission prevents a failed diagnostic copy from
            // invalidating the already submitted guest render commands.
            auto copyEncoder = g_device.CreateCommandEncoder();
            const std::array textures{g_currentSurfaceTexture, displayCopy.texture, aurora::webgpu::g_frameBuffer.texture};
            for (std::size_t i = 0; i < textures.size(); ++i) {
                readbacks[i] = std::make_unique<mkw::frame_dump::Readback>();
                readbacks[i]->encode(g_device, copyEncoder, textures[i]);
            }
            auto copyCommands = copyEncoder.Finish();
            g_queue.Submit(1, &copyCommands);
        } catch (const std::exception& error) {
            for (auto it = readbacks.rbegin(); it != readbacks.rend(); ++it)
                it->reset();
            mkw::frame_dump::failure(error.what());
        } catch (...) {
            for (auto it = readbacks.rbegin(); it != readbacks.rend(); ++it)
                it->reset();
            mkw::frame_dump::failure("readback allocation failure");
        }
    }
#endif

    const bool presented = g_surface.Present();
#if defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
    if (readbacks[0] && presented) {
        try {
            // Error scopes are nested on the device; finish in reverse encode order.
            auto efb = readbacks[2]->finish(g_instance);
            auto display = readbacks[1]->finish(g_instance);
            auto surface = readbacks[0]->finish(g_instance);
            mkw::frame_dump::completed(std::move(surface), std::move(display), std::move(efb), g_presentedFrames + 1u);
        } catch (const std::exception& error) {
            mkw::frame_dump::failure(error.what());
        } catch (...) {
            mkw::frame_dump::failure("readback completion failure");
        }
    } else if (readbacks[0]) {
        mkw::frame_dump::failure("surface present failed");
    }
#endif
#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
    mkw_switch_fifo_capture_present(presented);
#endif
    const bool hadWork =
        g_auroraFrameHadWork.load(std::memory_order_acquire);

    // Aurora has completed this frame before Present returns. Release the
    // consumed surface state even on failure so a later call begins a frame
    // instead of reusing Aurora's already-unmapped staging buffers.
    g_currentSurfaceTexture = {};
    g_auroraFrameActive.store(false, std::memory_order_release);

    if (!presented) {
        g_presentFailures.fetch_add(1u, std::memory_order_relaxed);
        report("FRAME PRESENT FAIL frame=%llu\n",
               static_cast<unsigned long long>(g_presentedFrames + 1));
        return false;
    }
    g_presentSuccesses.fetch_add(1u, std::memory_order_relaxed);

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
#if defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
    mkw::frame_dump::reset();
#endif
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
        g_fifoWriteCalls.store(0u, std::memory_order_release);
        g_fifoWrite8Calls.store(0u, std::memory_order_release);
        g_fifoWrite16Calls.store(0u, std::memory_order_release);
        g_fifoWrite32Calls.store(0u, std::memory_order_release);
        g_fifoWriteFloatCalls.store(0u, std::memory_order_release);
        g_bpReg49Calls.store(0u, std::memory_order_release);
        g_bpReg4aCalls.store(0u, std::memory_order_release);
        g_bpReg4dCalls.store(0u, std::memory_order_release);
        g_bpRegOtherCalls.store(0u, std::memory_order_release);
        g_lastFifoValue.store(0u, std::memory_order_release);
        g_lastBpWord.store(0u, std::memory_order_release);
        g_lastFifoSize.store(0u, std::memory_order_release);
        g_pendingBpWord.store(false, std::memory_order_release);
        g_displayListCalls.store(0u, std::memory_order_release);
        g_gxCopyDispCalls.store(0u, std::memory_order_release);
        g_presentSuccesses.store(0u, std::memory_order_release);
        g_presentFailures.store(0u, std::memory_order_release);
        g_initialized = true;
#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
        mkw_switch_fifo_capture_start();
#endif

        if (!begin_frame_locked()) {
            g_initialized = false;
            report("RESULT=FAIL first-frame-begin\n");
#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
            mkw_switch_fifo_capture_shutdown();
#endif
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
#if defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP
    mkw_switch_frame_dump_checkpoint();
#endif

#if defined(MKW_RENDERED_FIFO_CAPTURE) && MKW_RENDERED_FIFO_CAPTURE
    mkw_switch_fifo_capture_shutdown();
#endif
    mkw_switch_gx_forget_copy_destinations();
    bool teardownSucceeded = true;
    try {
        if (g_auroraFrameActive.exchange(false, std::memory_order_acq_rel)) {
            aurora::gfx::abort_frame();
        }

        g_presenter.reset();
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
        teardownSucceeded = false;
        report("STAGE RENDERER_TEARDOWN FAIL exception\n");
    }

    g_initialized = false;
    report("RESULT=%s renderer-shutdown\n", teardownSucceeded ? "PASS" : "FAIL");
    close_report();
}

void* GuestToHostPtr(uint32_t addr, size_t len) {
    if (addr == 0 || len == 0) {
        return nullptr;
    }
    return Memory::GetPointer(addr, len);
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

void note_rmcp01_fifo_activity(uint32_t value, uint8_t size, bool isFloat) {
    const uint64_t ordinal =
        g_fifoWriteCalls.fetch_add(1u, std::memory_order_relaxed) + 1u;
    g_lastFifoValue.store(value, std::memory_order_relaxed);
    g_lastFifoSize.store(size, std::memory_order_relaxed);

    if (isFloat) {
        g_fifoWriteFloatCalls.fetch_add(1u, std::memory_order_relaxed);
    } else if (size == 1u) {
        g_fifoWrite8Calls.fetch_add(1u, std::memory_order_relaxed);
    } else if (size == 2u) {
        g_fifoWrite16Calls.fetch_add(1u, std::memory_order_relaxed);
    } else if (size == 4u) {
        g_fifoWrite32Calls.fetch_add(1u, std::memory_order_relaxed);
    }

    if (size == 1u) {
        g_pendingBpWord.store(value == 0x61u, std::memory_order_release);
    } else if (size == 4u &&
               g_pendingBpWord.exchange(false, std::memory_order_acq_rel)) {
        g_lastBpWord.store(value, std::memory_order_relaxed);
        switch ((value >> 24u) & 0xFFu) {
        case 0x49u:
            g_bpReg49Calls.fetch_add(1u, std::memory_order_relaxed);
            break;
        case 0x4Au:
            g_bpReg4aCalls.fetch_add(1u, std::memory_order_relaxed);
            break;
        case 0x4Du:
            g_bpReg4dCalls.fetch_add(1u, std::memory_order_relaxed);
            break;
        default:
            g_bpRegOtherCalls.fetch_add(1u, std::memory_order_relaxed);
            break;
        }
    } else {
        g_pendingBpWord.store(false, std::memory_order_release);
    }

    if (ordinal <= 16u) {
        report("FIFO EVENT #%llu size=%u value=0x%08x float=%u\n",
               static_cast<unsigned long long>(ordinal),
               static_cast<unsigned>(size),
               value,
               isFloat ? 1u : 0u);
    }

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
    note_rmcp01_fifo_activity(raw, 4u, true);
}

extern "C" void GX_HLE_FIFO_Write32(uint32_t value) {
    HleFifoWrite(value, 4);
    note_rmcp01_fifo_activity(value, 4u, false);
}

extern "C" void GX_HLE_FIFO_Write16(uint16_t value) {
    HleFifoWrite(static_cast<uint32_t>(value), 2);
    note_rmcp01_fifo_activity(static_cast<uint32_t>(value), 2u, false);
}

extern "C" void GX_HLE_FIFO_Write8(uint8_t value) {
    HleFifoWrite(static_cast<uint32_t>(value), 1);
    note_rmcp01_fifo_activity(static_cast<uint32_t>(value), 1u, false);
}

extern "C" void GX__CallDisplayList_80172f64(uint32_t listAddr, uint32_t sizeBytes) {
    g_displayListCalls.fetch_add(1u, std::memory_order_relaxed);
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
    // Retire GPU-only copies before a reused allocation can be sampled.
    // The broader desktop texture/TLUT/display-list caches remain outside
    // this slice; this hook owns only the new EFB-copy lifetime dependency.
    mkw_switch_gx_invalidate_copy_destinations(address, sizeBytes);
}

extern "C" bool mkw_switch_renderer_has_active_frame() noexcept {
    return g_initialized && g_auroraFrameActive.load(std::memory_order_acquire);
}

extern "C" MkwSwitchRendererDiagnostics mkw_switch_renderer_diagnostics_snapshot() noexcept {
    return MkwSwitchRendererDiagnostics{
        .initialized = g_initialized,
        .frame_active = g_auroraFrameActive.load(std::memory_order_acquire),
        .fifo_work_seen = g_loggedFirstFifoWork.load(std::memory_order_acquire),
        .fifo_write_calls = g_fifoWriteCalls.load(std::memory_order_relaxed),
        .fifo_write8_calls = g_fifoWrite8Calls.load(std::memory_order_relaxed),
        .fifo_write16_calls = g_fifoWrite16Calls.load(std::memory_order_relaxed),
        .fifo_write32_calls = g_fifoWrite32Calls.load(std::memory_order_relaxed),
        .fifo_write_float_calls = g_fifoWriteFloatCalls.load(std::memory_order_relaxed),
        .bp_reg_49_calls = g_bpReg49Calls.load(std::memory_order_relaxed),
        .bp_reg_4a_calls = g_bpReg4aCalls.load(std::memory_order_relaxed),
        .bp_reg_4d_calls = g_bpReg4dCalls.load(std::memory_order_relaxed),
        .bp_reg_other_calls = g_bpRegOtherCalls.load(std::memory_order_relaxed),
        .last_fifo_value = g_lastFifoValue.load(std::memory_order_relaxed),
        .last_bp_word = g_lastBpWord.load(std::memory_order_relaxed),
        .last_fifo_size = g_lastFifoSize.load(std::memory_order_relaxed),
        .display_list_calls = g_displayListCalls.load(std::memory_order_relaxed),
        .gx_copy_disp_calls = g_gxCopyDispCalls.load(std::memory_order_relaxed),
        .present_successes = g_presentSuccesses.load(std::memory_order_relaxed),
        .present_failures = g_presentFailures.load(std::memory_order_relaxed),
    };
}

extern "C" void mkw_switch_hle_gx_copy_disp(CpuContext* cpu) noexcept {
    if (!cpu) {
        return;
    }

    const bool clear = cpu->gpr[4] != 0u;
    g_gxCopyDispCalls.fetch_add(1u, std::memory_order_relaxed);
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
