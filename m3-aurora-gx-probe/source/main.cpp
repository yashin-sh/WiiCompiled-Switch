#include <switch.h>

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sys/stat.h>
#include <utility>

#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp.h"

#include "dolphin/gx.h"
#include "gfx/common.hpp"
#include "gx/fifo.hpp"
#include "internal.hpp"
#include "webgpu/gpu.hpp"

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/m3-aurora-gx-probe.txt";

FILE* g_report = nullptr;
bool g_mountedSdmcHere = false;

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

    std::fputs(buffer, stdout);
    std::fflush(stdout);
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

wgpu::Instance create_instance() {
    report("STAGE CREATE_INSTANCE begin\n");

    wgpu::InstanceDescriptor descriptor{};
    dawn::native::DawnInstanceDescriptor dawnDescriptor{};
    dawnDescriptor.backendValidationLevel =
        dawn::native::BackendValidationLevel::Disabled;
    descriptor.nextInChain = &dawnDescriptor;

    static constexpr wgpu::InstanceFeatureName kFeatures[] = {
        wgpu::InstanceFeatureName::TimedWaitAny,
    };
    descriptor.requiredFeatureCount = std::size(kFeatures);
    descriptor.requiredFeatures = kFeatures;

    wgpu::Instance instance = wgpu::CreateInstance(&descriptor);
    report("STAGE CREATE_INSTANCE %s\n", instance ? "PASS" : "FAIL");
    return instance;
}

wgpu::Adapter request_adapter(const wgpu::Instance& instance,
                              const wgpu::Surface& surface) {
    report("STAGE REQUEST_ADAPTER begin backend=Vulkan\n");

    wgpu::RequestAdapterOptions options{};
    options.backendType = wgpu::BackendType::Vulkan;
    options.powerPreference = wgpu::PowerPreference::HighPerformance;
    options.compatibleSurface = surface;

    wgpu::Adapter adapter;
    instance.WaitAny(
        instance.RequestAdapter(
            &options,
            wgpu::CallbackMode::WaitAnyOnly,
            [&adapter](wgpu::RequestAdapterStatus status,
                       wgpu::Adapter result,
                       wgpu::StringView message) {
                report("RequestAdapter callback status=%u\n",
                       static_cast<unsigned>(status));
                if (status != wgpu::RequestAdapterStatus::Success) {
                    report_string_view("RequestAdapter failed: ", message);
                    return;
                }
                adapter = std::move(result);
            }),
        UINT64_MAX);

    report("STAGE REQUEST_ADAPTER %s\n", adapter ? "PASS" : "FAIL");
    return adapter;
}

wgpu::Device request_device(const wgpu::Instance& instance,
                            const wgpu::Adapter& adapter) {
    report("STAGE REQUEST_DEVICE begin\n");

    wgpu::DeviceDescriptor descriptor{};
    descriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            report("Dawn uncaptured error type=%u: ",
                   static_cast<unsigned>(type));
            report_string_view("", message);
        });
    descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&,
           wgpu::DeviceLostReason reason,
           wgpu::StringView message) {
            report("Dawn device lost reason=%u: ",
                   static_cast<unsigned>(reason));
            report_string_view("", message);
        });

    wgpu::Device device;
    instance.WaitAny(
        adapter.RequestDevice(
            &descriptor,
            wgpu::CallbackMode::WaitAnyOnly,
            [&device](wgpu::RequestDeviceStatus status,
                      wgpu::Device result,
                      wgpu::StringView message) {
                report("RequestDevice callback status=%u\n",
                       static_cast<unsigned>(status));
                if (status != wgpu::RequestDeviceStatus::Success) {
                    report_string_view("RequestDevice failed: ", message);
                    return;
                }
                device = std::move(result);
            }),
        UINT64_MAX);

    report("STAGE REQUEST_DEVICE %s\n", device ? "PASS" : "FAIL");
    return device;
}

bool configure_surface(const wgpu::Surface& surface,
                       const wgpu::Adapter& adapter,
                       const wgpu::Device& device,
                       wgpu::TextureFormat* formatOut,
                       wgpu::PresentMode* presentModeOut) {
    report("STAGE CONFIGURE_SURFACE begin extent=1280x720\n");

    wgpu::SurfaceCapabilities capabilities{};
    if (!surface.GetCapabilities(adapter, &capabilities) ||
        capabilities.formatCount == 0 ||
        capabilities.presentModeCount == 0) {
        report("STAGE CONFIGURE_SURFACE FAIL capabilities\n");
        return false;
    }

    wgpu::SurfaceConfiguration config{};
    config.device = device;
    config.format = capabilities.formats[0];
    config.usage = wgpu::TextureUsage::RenderAttachment |
                   wgpu::TextureUsage::CopySrc;
    config.width = 1280;
    config.height = 720;
    config.presentMode = capabilities.presentModes[0];

    surface.Configure(&config);
    *formatOut = config.format;
    *presentModeOut = config.presentMode;

    report("STAGE CONFIGURE_SURFACE PASS format=%u presentMode=%u\n",
           static_cast<unsigned>(config.format),
           static_cast<unsigned>(config.presentMode));
    return true;
}

aurora::webgpu::TextureWithSampler create_depth_texture(
    const wgpu::Device& device,
    uint32_t width,
    uint32_t height) {
    const wgpu::TextureDescriptor descriptor{
        .label = "M3 Aurora depth",
        .usage = wgpu::TextureUsage::RenderAttachment |
                 wgpu::TextureUsage::TextureBinding,
        .size = {width, height, 1},
        .format = wgpu::TextureFormat::Depth32Float,
    };

    aurora::webgpu::TextureWithSampler result{};
    result.texture = device.CreateTexture(&descriptor);
    result.view = result.texture.CreateView();
    result.size = descriptor.size;
    result.format = descriptor.format;
    return result;
}

void setup_gx_state() {
    report("STAGE GX_INIT begin\n");

    GXInit(nullptr, 0);

    static const float kIdentity[3][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
    };
    static const float kOrtho[4][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };

    GXLoadPosMtxImm(kIdentity, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetProjection(kOrtho, GX_ORTHOGRAPHIC);

    GXSetViewport(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 1280, 720);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetBlendMode(GX_BM_NONE,
                   GX_BL_ONE,
                   GX_BL_ZERO,
                   GX_LO_COPY);

    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0,
                  GX_DISABLE,
                  GX_SRC_REG,
                  GX_SRC_VTX,
                  GX_LIGHT_NULL,
                  GX_DF_NONE,
                  GX_AF_NONE);

    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0,
                  GX_TEXCOORD_NULL,
                  GX_TEXMAP_NULL,
                  GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0,
                    GX_CC_ZERO,
                    GX_CC_ZERO,
                    GX_CC_ZERO,
                    GX_CC_RASC);
    GXSetTevAlphaIn(GX_TEVSTAGE0,
                    GX_CA_ZERO,
                    GX_CA_ZERO,
                    GX_CA_ZERO,
                    GX_CA_RASA);
    GXSetTevColorOp(GX_TEVSTAGE0,
                    GX_TEV_ADD,
                    GX_TB_ZERO,
                    GX_CS_SCALE_1,
                    GX_TRUE,
                    GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0,
                    GX_TEV_ADD,
                    GX_TB_ZERO,
                    GX_CS_SCALE_1,
                    GX_TRUE,
                    GX_TEVPREV);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    aurora::gx::fifo::drain();
    report("STAGE GX_INIT PASS\n");
}

void emit_gx_triangle() {
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);

    GXPosition3f32(0.0f, 0.62f, 0.0f);
    GXColor4u8(255, 48, 40, 255);

    GXPosition3f32(-0.62f, -0.48f, 0.0f);
    GXColor4u8(40, 224, 92, 255);

    GXPosition3f32(0.62f, -0.48f, 0.0f);
    GXColor4u8(46, 122, 255, 255);

    GXEnd();
}

GXColor background_color(uint64_t frame) {
    switch ((frame / 120u) % 4u) {
    case 0:
        return {8, 25, 86, 255};
    case 1:
        return {14, 76, 36, 255};
    case 2:
        return {96, 20, 25, 255};
    default:
        return {70, 20, 86, 255};
    }
}

bool draw_frame(uint64_t frame,
                const wgpu::Surface& surface,
                const wgpu::Device& device,
                const wgpu::Queue& queue) {
    GXSetCopyClear(background_color(frame), 0x00ffffff);
    aurora::gx::fifo::drain();

    wgpu::SurfaceTexture surfaceTexture{};
    surface.GetCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status !=
            wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surfaceTexture.status !=
            wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
        report("FAIL GetCurrentTexture frame=%llu status=%u\n",
               static_cast<unsigned long long>(frame),
               static_cast<unsigned>(surfaceTexture.status));
        return false;
    }

    auto& frameBuffer = aurora::webgpu::g_frameBuffer;
    frameBuffer = {};
    frameBuffer.texture = surfaceTexture.texture;
    frameBuffer.view = frameBuffer.texture.CreateView();
    frameBuffer.size = {1280, 720, 1};
    frameBuffer.format =
        aurora::webgpu::g_graphicsConfig.surfaceConfiguration.format;

    if (!aurora::gfx::begin_frame()) {
        report("FAIL Aurora gfx::begin_frame frame=%llu\n",
               static_cast<unsigned long long>(frame));
        return false;
    }

    emit_gx_triangle();
    aurora::gx::fifo::drain();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    aurora::gfx::after_submit();

    if (!surface.Present()) {
        report("FAIL Present frame=%llu\n",
               static_cast<unsigned long long>(frame));
        return false;
    }

    frameBuffer = {};
    return true;
}

bool run_loop(const wgpu::Surface& surface,
              const wgpu::Device& device,
              const wgpu::Queue& queue) {
    PadState pad{};
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    report("ENTER AURORA GX PRESENT LOOP -- press + to exit\n");

    uint64_t frame = 0;
    while (appletMainLoop()) {
        padUpdate(&pad);
        if ((padGetButtonsDown(&pad) & HidNpadButton_Plus) != 0) {
            report("user requested exit\n");
            break;
        }

        ++frame;
        if (!draw_frame(frame, surface, device, queue)) {
            return false;
        }

        if (frame == 1) {
            report("PASS FIRST_AURORA_GX_TRIANGLE_PRESENT\n");
        } else if ((frame % 120u) == 0u) {
            report("ACTIVE frames=%llu\n",
                   static_cast<unsigned long long>(frame));
        }

        svcSleepThread(16'666'667);
    }

    report("PASS LOOP frames=%llu\n",
           static_cast<unsigned long long>(frame));
    return frame != 0;
}

} // namespace

extern "C" void m3_probe_log(const char* message) {
    if (!message) {
        return;
    }
    std::fputs(message, stdout);
    std::fflush(stdout);
    if (g_report) {
        std::fputs(message, g_report);
        std::fflush(g_report);
    }
}

int main(int, char**) {
    init_report();

    report("WiiCompiled-Switch M3 Aurora GX triangle probe\n");
    report("wiicompiled pin: a135beb201042b20f390c6695ca6b26768820fb4\n");
    report("dawn-switch pin: 77029ea85250c9bdddfc2f88034afb6b5356a031\n");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6\n");
    report("goal: GX API -> Aurora FIFO/GX -> Dawn -> Vulkan/NVK -> Switch present\n");

    if (!g_report) {
        report("WARN durable report unavailable errno=%d (%s)\n",
               errno,
               std::strerror(errno));
    }

    NWindow* window = nwindowGetDefault();
    if (!window) {
        report("FAIL STAGE NWINDOW\nRESULT=FAIL\n");
        close_report();
        return 1;
    }
    report("STAGE NWINDOW PASS\n");

    wgpu::Instance instance = create_instance();
    if (!instance) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::SurfaceSourceSwitchNWindow switchWindow{};
    switchWindow.window = window;
    wgpu::SurfaceDescriptor surfaceDescriptor{};
    surfaceDescriptor.nextInChain = &switchWindow;

    report("STAGE CREATE_SURFACE begin\n");
    wgpu::Surface surface = instance.CreateSurface(&surfaceDescriptor);
    if (!surface) {
        report("STAGE CREATE_SURFACE FAIL\nRESULT=FAIL\n");
        close_report();
        return 1;
    }
    report("STAGE CREATE_SURFACE PASS\n");

    wgpu::Adapter adapter = request_adapter(instance, surface);
    if (!adapter) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::Device device = request_device(instance, adapter);
    if (!device) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }
    wgpu::Queue queue = device.GetQueue();

    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
    wgpu::PresentMode presentMode{};
    if (!configure_surface(surface,
                           adapter,
                           device,
                           &surfaceFormat,
                           &presentMode)) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::Limits limits{};
    adapter.GetLimits(&limits);

    aurora::g_config = {};
    aurora::g_config.logLevel = LOG_DEBUG;
    aurora::g_config.msaa = 1;
    aurora::g_config.maxTextureAnisotropy = 1;
    aurora::g_config.allowTextureReplacements = false;
    aurora::g_config.allowTextureDumps = false;

    aurora::webgpu::g_instance = instance;
    aurora::webgpu::g_surface = surface;
    aurora::webgpu::g_device = device;
    aurora::webgpu::g_queue = queue;
    aurora::webgpu::g_backendType = wgpu::BackendType::Vulkan;
    aurora::webgpu::g_graphicsConfig = {
        .surfaceConfiguration =
            {
                .format = surfaceFormat,
                .usage = wgpu::TextureUsage::RenderAttachment |
                         wgpu::TextureUsage::CopySrc,
                .width = 1280,
                .height = 720,
                .presentMode = presentMode,
            },
        .depthFormat = wgpu::TextureFormat::Depth32Float,
        .msaaSamples = 1,
        .textureAnisotropy = 1,
        .maxTextureDimension2D = limits.maxTextureDimension2D,
    };
    aurora::webgpu::g_depthBuffer =
        create_depth_texture(device, 1280, 720);

    report("STAGE AURORA_GFX_INIT begin\n");
    aurora::gfx::initialize();
    report("STAGE AURORA_GFX_INIT PASS\n");

    setup_gx_state();

    const bool passed = run_loop(surface, device, queue);

    report("STAGE TEARDOWN begin\n");
    aurora::webgpu::g_frameBuffer = {};
    report("STAGE TEARDOWN aurora-gfx begin\n");
    aurora::gfx::shutdown();
    report("STAGE TEARDOWN aurora-gfx PASS\n");

    aurora::webgpu::g_depthBuffer = {};
    report("STAGE TEARDOWN surface-unconfigure begin\n");
    surface.Unconfigure();
    report("STAGE TEARDOWN surface-unconfigure PASS\n");

    aurora::webgpu::g_queue = {};
    queue = nullptr;
    aurora::webgpu::g_surface = {};
    surface = nullptr;
    aurora::webgpu::g_device = {};
    device = nullptr;
    adapter = nullptr;
    aurora::webgpu::g_instance = {};
    instance = nullptr;

    report("STAGE TEARDOWN PASS\n");
    report("RESULT=%s\n", passed ? "PASS" : "FAIL");
    close_report();
    return passed ? 0 : 1;
}
