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
    "sdmc:/switch/WiiCompiled-Switch/m3-dawn-clear-probe.txt";

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
                       std::uint32_t height) {
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
    report("STAGE CONFIGURE_SURFACE PASS format=%u presentMode=%u\n",
           static_cast<unsigned>(config.format),
           static_cast<unsigned>(config.presentMode));
    return true;
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

bool draw_clear_frame(std::uint64_t frame,
                      const wgpu::Surface& surface,
                      const wgpu::Device& device,
                      const wgpu::Queue& queue) {
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
                      const wgpu::Queue& queue) {
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
        if (!draw_clear_frame(frame, surface, device, queue)) {
            return false;
        }

        if (frame == 1) {
            report("PASS FIRST_DAWN_PRESENT\n");
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

    report("WiiCompiled-Switch M3 Dawn/WebGPU clear probe\n");
    report("dawn-switch pin: 77029ea85250c9bdddfc2f88034afb6b5356a031\n");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6\n");
    report("goal: WebGPU/Dawn -> Vulkan -> loaderless NVK -> VK_NN_vi_surface -> present\n");
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
    if (!configure_surface(surface, adapter, device, 1280, 720)) {
        report("RESULT=FAIL\n");
        close_report();
        return 1;
    }

    const bool passed = run_present_loop(surface, device, queue);

    report("STAGE TEARDOWN begin\n");

    report("STAGE TEARDOWN surface-unconfigure begin\n");
    surface.Unconfigure();
    report("STAGE TEARDOWN surface-unconfigure PASS\n");

    report("STAGE TEARDOWN queue-release begin\n");
    queue = nullptr;
    report("STAGE TEARDOWN queue-release PASS\n");

    report("STAGE TEARDOWN surface-release begin\n");
    surface = nullptr;
    report("STAGE TEARDOWN surface-release PASS\n");

    report("STAGE TEARDOWN device-release begin\n");
    device = nullptr;
    report("STAGE TEARDOWN device-release PASS\n");

    report("STAGE TEARDOWN adapter-release begin\n");
    adapter = nullptr;
    report("STAGE TEARDOWN adapter-release PASS\n");

    report("STAGE TEARDOWN instance-release begin\n");
    instance = nullptr;
    report("STAGE TEARDOWN instance-release PASS\n");

    report("STAGE TEARDOWN PASS\n");
    report("RESULT=%s\n", passed ? "PASS" : "FAIL");
    close_report();
    return passed ? 0 : 1;
}
