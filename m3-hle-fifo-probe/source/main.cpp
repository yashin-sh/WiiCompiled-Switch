#include <switch.h>

#include <algorithm>
#include <array>
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
#include "gx_internal.h"
#include "gx_stream_common.h"
#include "internal.hpp"
#include "webgpu/gpu.hpp"

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/m3-hle-fifo-aurora-probe.txt";

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

void setup_gx_fixed_state() {
    report("STAGE GX_FIXED_STATE begin\n");

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
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);

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

    // Deliberately leave the vertex descriptor empty. The synthetic FIFO must
    // publish VCD/VAT state through the pinned HleFifoWrite decoder before the
    // triangle can be submitted.
    GXClearVtxDesc();

    aurora::gx::fifo::drain();
    report("STAGE GX_FIXED_STATE PASS\n");
}

struct FifoPacket {
    std::array<uint8_t, 96> bytes{};
    size_t size = 0;

    void u8(uint8_t value) {
        bytes[size++] = value;
    }

    void be16(uint16_t value) {
        u8(static_cast<uint8_t>(value >> 8));
        u8(static_cast<uint8_t>(value));
    }

    void be32(uint32_t value) {
        u8(static_cast<uint8_t>(value >> 24));
        u8(static_cast<uint8_t>(value >> 16));
        u8(static_cast<uint8_t>(value >> 8));
        u8(static_cast<uint8_t>(value));
    }

    void f32(float value) {
        uint32_t raw = 0;
        static_assert(sizeof(raw) == sizeof(value));
        std::memcpy(&raw, &value, sizeof(raw));
        be32(raw);
    }

    void cp(uint8_t reg, uint32_t value) {
        u8(GxCmd::GX_LOAD_CP_REG_CMD);
        u8(reg);
        be32(value);
    }

    void vertex(float x,
                float y,
                float z,
                uint8_t r,
                uint8_t g,
                uint8_t b,
                uint8_t a) {
        f32(x);
        f32(y);
        f32(z);
        u8(r);
        u8(g);
        u8(b);
        u8(a);
    }
};

FifoPacket build_hle_fifo_triangle_packet() {
    FifoPacket packet{};

    // CP VCD_LO: POS=GX_DIRECT and CLR0=GX_DIRECT, all matrix/normal/CLR1
    // descriptors disabled.
    constexpr uint32_t kVcdLo =
        (static_cast<uint32_t>(GX_DIRECT) << 9) |
        (static_cast<uint32_t>(GX_DIRECT) << 13);

    // VAT A / VTXFMT0:
    // POS = XYZ/F32/frac0
    // CLR0 = RGBA/RGBA8
    constexpr uint32_t kVatA =
        (1u << 0) |
        (static_cast<uint32_t>(GX_F32) << 1) |
        (1u << 13) |
        (static_cast<uint32_t>(GX_RGBA8) << 14);

    packet.cp(0x50, kVcdLo);
    packet.cp(0x60, 0);
    packet.cp(0x70, kVatA);

    packet.u8(static_cast<uint8_t>(
        GxCmd::GX_DRAW_TRIANGLES_CMD | static_cast<uint8_t>(GX_VTXFMT0)));
    packet.be16(3);

    packet.vertex(0.0f, 0.62f, 0.0f, 255, 48, 40, 255);
    packet.vertex(-0.62f, -0.48f, 0.0f, 40, 224, 92, 255);
    packet.vertex(0.62f, -0.48f, 0.0f, 46, 122, 255, 255);

    return packet;
}

bool validate_hle_fifo_state() {
    const auto& pos = g_hleGxState.vtxAttrFmt[GX_VTXFMT0][GX_VA_POS];
    const auto& color = g_hleGxState.vtxAttrFmt[GX_VTXFMT0][GX_VA_CLR0];

    return g_hleGxState.vtxDesc[GX_VA_POS] == GX_DIRECT &&
           g_hleGxState.vtxDesc[GX_VA_CLR0] == GX_DIRECT &&
           pos.cnt == GX_POS_XYZ &&
           pos.type == GX_F32 &&
           pos.frac == 0 &&
           color.cnt == GX_CLR_RGBA &&
           color.type == GX_RGBA8 &&
           !g_hleGxState.inBegin &&
           g_hleGxState.vertsRemaining == 0 &&
           g_hleGxState.fifoByteCount == 0 &&
           g_auroraFrameHadWork.load(std::memory_order_acquire);
}

bool emit_hle_fifo_triangle(uint64_t frame) {
    const FifoPacket packet = build_hle_fifo_triangle_packet();

    if (frame == 1) {
        report("STAGE HLE_FIFO_STREAM begin bytes=%zu mode=bytewise\n",
               packet.size);
    }

    // Exercise the exact pinned decoder one gather-pipe byte at a time. This
    // intentionally avoids the burst helper's direct CP fast path so VCD, VAT,
    // draw header, vertex floats and colors all pass through HleFifoWrite.
    for (size_t i = 0; i < packet.size; ++i) {
        HleFifoWrite(packet.bytes[i], 1);
    }

    const bool valid = validate_hle_fifo_state();
    if (frame == 1) {
        report("STAGE HLE_FIFO_STREAM %s posDesc=%u clr0Desc=%u "
               "fifoBytes=%zu inBegin=%u hadWork=%u\n",
               valid ? "PASS" : "FAIL",
               static_cast<unsigned>(g_hleGxState.vtxDesc[GX_VA_POS]),
               static_cast<unsigned>(g_hleGxState.vtxDesc[GX_VA_CLR0]),
               g_hleGxState.fifoByteCount,
               g_hleGxState.inBegin ? 1u : 0u,
               g_auroraFrameHadWork.load(std::memory_order_acquire) ? 1u : 0u);
    }
    return valid;
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

    g_auroraFrameHadWork.store(false, std::memory_order_release);
    g_auroraFrameActive.store(true, std::memory_order_release);

    if (!emit_hle_fifo_triangle(frame)) {
        report("FAIL pinned HleFifoWrite state validation frame=%llu\n",
               static_cast<unsigned long long>(frame));
        g_auroraFrameActive.store(false, std::memory_order_release);
        aurora::gfx::abort_frame();
        return false;
    }
    aurora::gx::fifo::drain();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    aurora::gfx::after_submit();
    g_auroraFrameActive.store(false, std::memory_order_release);

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

    report("ENTER HLE FIFO -> AURORA GX PRESENT LOOP -- press + to exit\n");

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
            report("PASS FIRST_HLE_FIFO_AURORA_TRIANGLE_PRESENT\n");
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

    report("WiiCompiled-Switch M3 HleFifoWrite -> Aurora GX probe\n");
    report("wiicompiled pin: a135beb201042b20f390c6695ca6b26768820fb4\n");
    report("dawn-switch pin: 77029ea85250c9bdddfc2f88034afb6b5356a031\n");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6\n");
    report("goal: synthetic GX FIFO bytes -> pinned HleFifoWrite -> Aurora GX -> Dawn -> Vulkan/NVK -> Switch present\n");

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

    setup_gx_fixed_state();

    g_hleGxState = HleGxState{};
    g_alphaCompareValid = false;
    g_auroraFrameActive.store(false, std::memory_order_release);
    g_auroraFrameHadWork.store(false, std::memory_order_release);
    report("STAGE HLE_FIFO_DECODER_RESET PASS\n");

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
