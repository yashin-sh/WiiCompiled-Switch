#include <switch.h>

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

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/m3-dawn-triangle-probe.txt";

FILE* g_report = nullptr;
bool g_mounted_sdmc_here = false;

void init_report() {
    ::mkdir(kReportDir, 0777);
    g_report = std::fopen(kReportPath, "w");
    if (!g_report) {
        const Result mount_result = fsdevMountSdmc();
        if (R_SUCCEEDED(mount_result)) {
            g_mounted_sdmc_here = true;
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
    if (g_mounted_sdmc_here) {
        fsdevUnmountDevice("sdmc");
        g_mounted_sdmc_here = false;
    }
}

void report(const char* format, ...) {
    va_list args;
    va_start(args, format);

    va_list file_args;
    va_copy(file_args, args);

    std::vprintf(format, args);
    std::fflush(stdout);
    if (g_report) {
        std::vfprintf(g_report, format, file_args);
        std::fflush(g_report);
    }

    va_end(file_args);
    va_end(args);
}

void write_string_view(FILE* output, wgpu::StringView message) {
    if (!output || message.data == nullptr) {
        return;
    }
    if (message.length == wgpu::kStrlen) {
        std::fprintf(output, "%s", message.data);
        return;
    }
    std::fprintf(output, "%.*s", static_cast<int>(message.length), message.data);
}

void report_string_view(wgpu::StringView message) {
    write_string_view(stdout, message);
    std::fflush(stdout);
    if (g_report) {
        write_string_view(g_report, message);
        std::fflush(g_report);
    }
}

void report_error(const char* prefix, wgpu::StringView message) {
    report("%s", prefix);
    report_string_view(message);
    report("\n");
}

const char* logging_type_name(wgpu::LoggingType type) {
    switch (type) {
    case wgpu::LoggingType::Verbose:
        return "verbose";
    case wgpu::LoggingType::Info:
        return "info";
    case wgpu::LoggingType::Warning:
        return "warning";
    case wgpu::LoggingType::Error:
        return "error";
    }
    return "unknown";
}

const char* error_type_name(wgpu::ErrorType type) {
    switch (type) {
    case wgpu::ErrorType::NoError:
        return "no-error";
    case wgpu::ErrorType::Validation:
        return "validation";
    case wgpu::ErrorType::OutOfMemory:
        return "out-of-memory";
    case wgpu::ErrorType::Internal:
        return "internal";
    case wgpu::ErrorType::Unknown:
        return "unknown";
    }
    return "unknown";
}

const char* device_lost_reason_name(wgpu::DeviceLostReason reason) {
    switch (reason) {
    case wgpu::DeviceLostReason::Unknown:
        return "unknown";
    case wgpu::DeviceLostReason::Destroyed:
        return "destroyed";
    case wgpu::DeviceLostReason::CallbackCancelled:
        return "callback-cancelled";
    case wgpu::DeviceLostReason::FailedCreation:
        return "failed-creation";
    }
    return "unknown";
}

void dawn_logging_callback(wgpu::LoggingType type, wgpu::StringView message) {
    report("Dawn %s: ", logging_type_name(type));
    report_string_view(message);
    report("\n");
}

void device_uncaptured_error_callback(const wgpu::Device&,
                                      wgpu::ErrorType type,
                                      wgpu::StringView message) {
    report("Device uncaptured error %s: ", error_type_name(type));
    report_string_view(message);
    report("\n");
}

void device_lost_callback(const wgpu::Device&,
                          wgpu::DeviceLostReason reason,
                          wgpu::StringView message) {
    report("Device lost %s: ", device_lost_reason_name(reason));
    report_string_view(message);
    report("\n");
}

wgpu::Instance create_instance() {
    report("STAGE CREATE_INSTANCE begin\n");

    wgpu::InstanceDescriptor descriptor{};
    dawn::native::DawnInstanceDescriptor dawn_descriptor{};
    dawn_descriptor.SetLoggingCallback(dawn_logging_callback);
    descriptor.nextInChain = &dawn_descriptor;

    static constexpr wgpu::InstanceFeatureName kRequiredFeatures[] = {
        wgpu::InstanceFeatureName::TimedWaitAny,
    };
    descriptor.requiredFeatureCount = std::size(kRequiredFeatures);
    descriptor.requiredFeatures = kRequiredFeatures;

    wgpu::Instance instance = wgpu::CreateInstance(&descriptor);
    report("STAGE CREATE_INSTANCE %s\n", instance == nullptr ? "FAIL" : "PASS");
    return instance;
}

wgpu::Adapter request_vulkan_adapter(const wgpu::Instance& instance,
                                     const wgpu::Surface& surface) {
    report("STAGE REQUEST_ADAPTER begin backend=Vulkan\n");

    wgpu::RequestAdapterOptions options{};
    options.backendType = wgpu::BackendType::Vulkan;
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
                    report_error("RequestAdapter failed: ", message);
                    return;
                }
                adapter = std::move(result);
            }),
        UINT64_MAX);

    report("STAGE REQUEST_ADAPTER %s\n", adapter == nullptr ? "FAIL" : "PASS");
    return adapter;
}

wgpu::Device request_device(const wgpu::Instance& instance,
                            const wgpu::Adapter& adapter) {
    report("STAGE REQUEST_DEVICE begin\n");

    wgpu::DeviceDescriptor descriptor{};
    descriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                     device_lost_callback);
    descriptor.SetUncapturedErrorCallback(device_uncaptured_error_callback);

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
                    report_error("RequestDevice failed: ", message);
                    return;
                }
                device = std::move(result);
            }),
        UINT64_MAX);

    report("STAGE REQUEST_DEVICE %s\n", device == nullptr ? "FAIL" : "PASS");
    if (device != nullptr) {
        device.SetLoggingCallback(dawn_logging_callback);
    }
    return device;
}

bool configure_surface(const wgpu::Surface& surface,
                       const wgpu::Adapter& adapter,
                       const wgpu::Device& device,
                       std::uint32_t width,
                       std::uint32_t height,
                       wgpu::TextureFormat* format_out) {
    report("STAGE CONFIGURE_SURFACE begin extent=%ux%u\n", width, height);

    wgpu::SurfaceCapabilities capabilities{};
    if (!surface.GetCapabilities(adapter, &capabilities)) {
        report("STAGE CONFIGURE_SURFACE FAIL capabilities\n");
        return false;
    }

    report("Surface capabilities formats=%zu presentModes=%zu alphaModes=%zu\n",
           capabilities.formatCount,
           capabilities.presentModeCount,
           capabilities.alphaModeCount);
    if (capabilities.formatCount == 0 || capabilities.presentModeCount == 0) {
        report("STAGE CONFIGURE_SURFACE FAIL empty-capabilities\n");
        return false;
    }

    wgpu::SurfaceConfiguration config{};
    config.device = device;
    config.format = capabilities.formats[0];
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.width = width;
    config.height = height;
    config.presentMode = capabilities.presentModes[0];

    surface.Configure(&config);
    if (format_out) {
        *format_out = config.format;
    }
    report("STAGE CONFIGURE_SURFACE PASS format=%u presentMode=%u\n",
           static_cast<unsigned>(config.format),
           static_cast<unsigned>(config.presentMode));
    return true;
}

wgpu::RenderPipeline create_triangle_pipeline(const wgpu::Device& device,
                                              wgpu::TextureFormat surface_format) {
    report("STAGE CREATE_PIPELINE begin format=%u\n",
           static_cast<unsigned>(surface_format));

    static constexpr char kTriangleShader[] = R"(
struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color : vec3f,
}

@vertex
fn vs(@builtin(vertex_index) vertex_index : u32) -> VertexOut {
    var positions = array<vec2f, 3>(
        vec2f( 0.0,  0.62),
        vec2f(-0.62, -0.48),
        vec2f( 0.62, -0.48)
    );
    var colors = array<vec3f, 3>(
        vec3f(1.00, 0.18, 0.16),
        vec3f(0.15, 0.88, 0.36),
        vec3f(0.18, 0.48, 1.00)
    );

    var out : VertexOut;
    out.position = vec4f(positions[vertex_index], 0.0, 1.0);
    out.color = colors[vertex_index];
    return out;
}

@fragment
fn fs(@location(0) color : vec3f) -> @location(0) vec4f {
    return vec4f(color, 1.0);
}
)";

    wgpu::ShaderSourceWGSL wgsl{};
    wgsl.code = kTriangleShader;

    wgpu::ShaderModuleDescriptor shader_descriptor{};
    shader_descriptor.nextInChain = &wgsl;
    shader_descriptor.label = "m3 Dawn triangle WGSL";
    wgpu::ShaderModule shader = device.CreateShaderModule(&shader_descriptor);
    if (shader == nullptr) {
        report("STAGE CREATE_PIPELINE FAIL shader\n");
        return nullptr;
    }
    report("STAGE CREATE_SHADER PASS\n");

    wgpu::ColorTargetState target{};
    target.format = surface_format;
    target.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragment{};
    fragment.module = shader;
    fragment.entryPoint = "fs";
    fragment.targetCount = 1;
    fragment.targets = &target;

    wgpu::RenderPipelineDescriptor descriptor{};
    descriptor.label = "m3 Dawn triangle pipeline";
    descriptor.vertex.module = shader;
    descriptor.vertex.entryPoint = "vs";
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    descriptor.primitive.frontFace = wgpu::FrontFace::CCW;
    descriptor.primitive.cullMode = wgpu::CullMode::None;
    descriptor.fragment = &fragment;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&descriptor);
    report("STAGE CREATE_PIPELINE %s\n", pipeline == nullptr ? "FAIL" : "PASS");
    return pipeline;
}

wgpu::Color clear_color(std::uint64_t frame) {
    const std::uint64_t phase = (frame / 120u) % 4u;
    switch (phase) {
    case 0:
        return {0.03, 0.10, 0.34, 1.0};
    case 1:
        return {0.06, 0.30, 0.14, 1.0};
    case 2:
        return {0.38, 0.08, 0.10, 1.0};
    default:
        return {0.28, 0.08, 0.34, 1.0};
    }
}

bool draw_triangle_frame(std::uint64_t frame,
                         const wgpu::Surface& surface,
                         const wgpu::Device& device,
                         const wgpu::Queue& queue,
                         const wgpu::RenderPipeline& pipeline) {
    wgpu::SurfaceTexture surface_texture{};
    surface.GetCurrentTexture(&surface_texture);
    if (surface_texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surface_texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
        report("FAIL GetCurrentTexture frame=%llu status=%u\n",
               static_cast<unsigned long long>(frame),
               static_cast<unsigned>(surface_texture.status));
        return false;
    }

    wgpu::TextureView view = surface_texture.texture.CreateView(nullptr);

    wgpu::RenderPassColorAttachment color_attachment{};
    color_attachment.view = view;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.clearValue = clear_color(frame);

    wgpu::RenderPassDescriptor render_pass{};
    render_pass.colorAttachmentCount = 1;
    render_pass.colorAttachments = &color_attachment;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(nullptr);
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
    pass.SetPipeline(pipeline);
    pass.Draw(3, 1, 0, 0);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish(nullptr);
    queue.Submit(1, &commands);

    if (!surface.Present()) {
        report("FAIL Present frame=%llu\n", static_cast<unsigned long long>(frame));
        return false;
    }

    return true;
}

bool run_present_loop(const wgpu::Surface& surface,
                      const wgpu::Device& device,
                      const wgpu::Queue& queue,
                      const wgpu::RenderPipeline& pipeline) {
    PadState pad{};
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    report("ENTER DAWN PRESENT LOOP -- press + to exit\n");

    std::uint64_t frame = 0;
    while (appletMainLoop()) {
        padUpdate(&pad);
        if ((padGetButtonsDown(&pad) & HidNpadButton_Plus) != 0) {
            report("user requested exit\n");
            break;
        }

        ++frame;
        if (!draw_triangle_frame(frame, surface, device, queue, pipeline)) {
            return false;
        }

        if (frame == 1) {
            report("PASS FIRST_DAWN_TRIANGLE_PRESENT\n");
        } else if ((frame % 120u) == 0u) {
            report("ACTIVE frames=%llu\n", static_cast<unsigned long long>(frame));
        }

        svcSleepThread(16'666'667);
    }

    report("PASS LOOP frames=%llu\n", static_cast<unsigned long long>(frame));
    return frame != 0;
}

} // namespace

int main(int, char**) {
    init_report();

    report("WiiCompiled-Switch M3 Dawn/WebGPU triangle probe\n");
    report("dawn-switch pin: 77029ea85250c9bdddfc2f88034afb6b5356a031\n");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6\n");
    report("goal: WGSL -> Dawn pipeline -> Vulkan -> loaderless NVK -> VK_NN_vi_surface -> triangle present\n");
    if (!g_report) {
        report("WARN durable report unavailable errno=%d (%s)\n", errno, std::strerror(errno));
    }

    NWindow* window = nwindowGetDefault();
    if (!window) {
        report("FAIL STAGE NWINDOW\n");
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }
    report("STAGE NWINDOW PASS\n");

    wgpu::Instance instance = create_instance();
    if (instance == nullptr) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::SurfaceSourceSwitchNWindow switch_window{};
    switch_window.window = window;

    wgpu::SurfaceDescriptor surface_descriptor{};
    surface_descriptor.nextInChain = &switch_window;

    report("STAGE CREATE_SURFACE begin\n");
    wgpu::Surface surface = instance.CreateSurface(&surface_descriptor);
    if (surface == nullptr) {
        report("STAGE CREATE_SURFACE FAIL\n");
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }
    report("STAGE CREATE_SURFACE PASS\n");

    wgpu::Adapter adapter = request_vulkan_adapter(instance, surface);
    if (adapter == nullptr) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::Device device = request_device(instance, adapter);
    if (device == nullptr) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::Queue queue = device.GetQueue();
    wgpu::TextureFormat surface_format = wgpu::TextureFormat::Undefined;
    if (!configure_surface(surface, adapter, device, 1280, 720, &surface_format)) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    wgpu::RenderPipeline pipeline = create_triangle_pipeline(device, surface_format);
    if (pipeline == nullptr) {
        report("RESULT=FAIL\n");
        surface.Unconfigure();
        close_report();
        return 1;
    }

    const bool passed = run_present_loop(surface, device, queue, pipeline);
    surface.Unconfigure();

    report("RESULT=%s\n", passed ? "PASS" : "FAIL");
    close_report();
    return passed ? 0 : 1;
}
