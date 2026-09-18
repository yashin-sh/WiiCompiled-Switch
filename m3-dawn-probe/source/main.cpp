#include <switch.h>

#include <webgpu/webgpu_cpp.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <sys/stat.h>

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/m3-dawn-nvk-probe.txt";

FILE* g_report = nullptr;
wgpu::Adapter g_adapter;
wgpu::Device g_device;
bool g_queue_done = false;
wgpu::QueueWorkDoneStatus g_queue_status = wgpu::QueueWorkDoneStatus::Error;

void report(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (g_report) {
        std::fprintf(g_report, "%s\n", buffer);
        std::fflush(g_report);
    }
}

std::string_view as_string_view(wgpu::StringView value) {
    if (value.IsUndefined() || value.data == nullptr) {
        return {};
    }
    return std::string_view(value);
}

bool open_report(bool& mounted_sdmc_here) {
    ::mkdir(kReportDir, 0777);
    g_report = std::fopen(kReportPath, "w");
    if (g_report) {
        return true;
    }

    const Result mount_result = fsdevMountSdmc();
    if (R_FAILED(mount_result)) {
        return false;
    }
    mounted_sdmc_here = true;
    ::mkdir(kReportDir, 0777);
    g_report = std::fopen(kReportPath, "w");
    return g_report != nullptr;
}

void wait_for_plus() {
    PadState pad{};
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    report("WAIT_FOR_EXIT -- press +");
    while (appletMainLoop()) {
        padUpdate(&pad);
        if ((padGetButtonsDown(&pad) & HidNpadButton_Plus) != 0) {
            report("user requested exit");
            break;
        }
        svcSleepThread(16'000'000);
    }
}

bool run_dawn_probe() {
    report("stage=INSTANCE");

    const wgpu::InstanceFeatureName instance_features[] = {
        wgpu::InstanceFeatureName::TimedWaitAny,
    };
    wgpu::InstanceDescriptor instance_descriptor{};
    instance_descriptor.requiredFeatureCount = 1;
    instance_descriptor.requiredFeatures = instance_features;

    wgpu::Instance instance = wgpu::CreateInstance(&instance_descriptor);
    if (!instance) {
        report("FAIL: wgpu::CreateInstance");
        return false;
    }
    report("PASS INSTANCE");

    report("stage=ADAPTER");
    wgpu::RequestAdapterOptions adapter_options{};
    adapter_options.powerPreference = wgpu::PowerPreference::HighPerformance;
    adapter_options.backendType = wgpu::BackendType::Vulkan;

    const auto adapter_future = instance.RequestAdapter(
        &adapter_options,
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestAdapterStatus status,
           wgpu::Adapter adapter,
           wgpu::StringView message) {
            const auto text = as_string_view(message);
            report("RequestAdapter status=%u message=%.*s",
                   static_cast<unsigned>(status),
                   static_cast<int>(text.size()),
                   text.data() ? text.data() : "");
            if (status == wgpu::RequestAdapterStatus::Success) {
                g_adapter = std::move(adapter);
            }
        });

    const auto adapter_wait = instance.WaitAny(adapter_future, 5'000'000'000ull);
    report("WaitAny(adapter) -> %u", static_cast<unsigned>(adapter_wait));
    if (adapter_wait != wgpu::WaitStatus::Success || !g_adapter) {
        report("FAIL ADAPTER");
        return false;
    }

    wgpu::AdapterInfo adapter_info{};
    const wgpu::Status info_status = g_adapter.GetInfo(&adapter_info);
    const auto device_name = as_string_view(adapter_info.device);
    const auto description = as_string_view(adapter_info.description);
    report("adapter info status=%u backend=%u type=%u vendor=0x%08x device=0x%08x",
           static_cast<unsigned>(info_status),
           static_cast<unsigned>(adapter_info.backendType),
           static_cast<unsigned>(adapter_info.adapterType),
           adapter_info.vendorID,
           adapter_info.deviceID);
    report("adapter device=%.*s",
           static_cast<int>(device_name.size()),
           device_name.data() ? device_name.data() : "");
    report("adapter description=%.*s",
           static_cast<int>(description.size()),
           description.data() ? description.data() : "");

    if (adapter_info.backendType != wgpu::BackendType::Vulkan) {
        report("FAIL: Dawn did not select Vulkan backend");
        return false;
    }
    report("PASS ADAPTER_VULKAN");

    report("stage=DEVICE");
    wgpu::DeviceDescriptor device_descriptor{};
    device_descriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            const auto text = as_string_view(message);
            report("DAWN_ERROR type=%u message=%.*s",
                   static_cast<unsigned>(type),
                   static_cast<int>(text.size()),
                   text.data() ? text.data() : "");
        });
    device_descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&,
           wgpu::DeviceLostReason reason,
           wgpu::StringView message) {
            const auto text = as_string_view(message);
            report("DAWN_DEVICE_LOST reason=%u message=%.*s",
                   static_cast<unsigned>(reason),
                   static_cast<int>(text.size()),
                   text.data() ? text.data() : "");
        });

    const auto device_future = g_adapter.RequestDevice(
        &device_descriptor,
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status,
           wgpu::Device device,
           wgpu::StringView message) {
            const auto text = as_string_view(message);
            report("RequestDevice status=%u message=%.*s",
                   static_cast<unsigned>(status),
                   static_cast<int>(text.size()),
                   text.data() ? text.data() : "");
            if (status == wgpu::RequestDeviceStatus::Success) {
                g_device = std::move(device);
            }
        });

    const auto device_wait = instance.WaitAny(device_future, 5'000'000'000ull);
    report("WaitAny(device) -> %u", static_cast<unsigned>(device_wait));
    if (device_wait != wgpu::WaitStatus::Success || !g_device) {
        report("FAIL DEVICE");
        return false;
    }
    report("PASS DEVICE");

    wgpu::Queue queue = g_device.GetQueue();
    if (!queue) {
        report("FAIL QUEUE");
        return false;
    }

    report("stage=WEBGPU_RENDER_PIPELINE");
    constexpr const char* kShader = R"WGSL(
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) index: u32) -> VertexOutput {
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(0.0, -0.75),
        vec2<f32>(0.75, 0.65),
        vec2<f32>(-0.75, 0.65)
    );
    var colors = array<vec3<f32>, 3>(
        vec3<f32>(1.0, 0.1, 0.1),
        vec3<f32>(0.1, 1.0, 0.2),
        vec3<f32>(0.1, 0.3, 1.0)
    );
    var out: VertexOutput;
    out.position = vec4<f32>(positions[index], 0.0, 1.0);
    out.color = colors[index];
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}
)WGSL";

    wgpu::ShaderSourceWGSL shader_source{};
    shader_source.code = kShader;
    wgpu::ShaderModuleDescriptor shader_descriptor{};
    shader_descriptor.nextInChain = &shader_source;
    shader_descriptor.label = "M3 Dawn NVK offscreen triangle";
    wgpu::ShaderModule shader = g_device.CreateShaderModule(&shader_descriptor);
    if (!shader) {
        report("FAIL SHADER_MODULE");
        return false;
    }
    report("PASS SHADER_MODULE");

    wgpu::PipelineLayoutDescriptor layout_descriptor{};
    wgpu::PipelineLayout pipeline_layout =
        g_device.CreatePipelineLayout(&layout_descriptor);
    if (!pipeline_layout) {
        report("FAIL PIPELINE_LAYOUT");
        return false;
    }

    wgpu::ColorTargetState color_target{};
    color_target.format = wgpu::TextureFormat::RGBA8Unorm;
    color_target.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragment{};
    fragment.module = shader;
    fragment.entryPoint = "fs_main";
    fragment.targetCount = 1;
    fragment.targets = &color_target;

    wgpu::RenderPipelineDescriptor pipeline_descriptor{};
    pipeline_descriptor.layout = pipeline_layout;
    pipeline_descriptor.vertex.module = shader;
    pipeline_descriptor.vertex.entryPoint = "vs_main";
    pipeline_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.multisample.mask = UINT32_MAX;
    pipeline_descriptor.fragment = &fragment;

    wgpu::RenderPipeline pipeline =
        g_device.CreateRenderPipeline(&pipeline_descriptor);
    if (!pipeline) {
        report("FAIL RENDER_PIPELINE");
        return false;
    }
    report("PASS RENDER_PIPELINE");

    wgpu::TextureDescriptor texture_descriptor{};
    texture_descriptor.label = "M3 Dawn NVK offscreen target";
    texture_descriptor.usage =
        wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    texture_descriptor.dimension = wgpu::TextureDimension::e2D;
    texture_descriptor.size.width = 64;
    texture_descriptor.size.height = 64;
    texture_descriptor.size.depthOrArrayLayers = 1;
    texture_descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    texture_descriptor.mipLevelCount = 1;
    texture_descriptor.sampleCount = 1;

    wgpu::Texture texture = g_device.CreateTexture(&texture_descriptor);
    if (!texture) {
        report("FAIL TEXTURE");
        return false;
    }
    wgpu::TextureView view = texture.CreateView();
    if (!view) {
        report("FAIL TEXTURE_VIEW");
        return false;
    }

    wgpu::CommandEncoder encoder = g_device.CreateCommandEncoder();
    if (!encoder) {
        report("FAIL COMMAND_ENCODER");
        return false;
    }

    wgpu::RenderPassColorAttachment color_attachment{};
    color_attachment.view = view;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.clearValue = {0.025, 0.035, 0.06, 1.0};

    wgpu::RenderPassDescriptor render_pass_descriptor{};
    render_pass_descriptor.colorAttachmentCount = 1;
    render_pass_descriptor.colorAttachments = &color_attachment;

    wgpu::RenderPassEncoder pass =
        encoder.BeginRenderPass(&render_pass_descriptor);
    if (!pass) {
        report("FAIL RENDER_PASS");
        return false;
    }
    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    if (!commands) {
        report("FAIL COMMAND_BUFFER");
        return false;
    }

    queue.Submit(1, &commands);
    report("PASS SUBMIT");

    g_queue_done = false;
    g_queue_status = wgpu::QueueWorkDoneStatus::Error;
    const auto queue_future = queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::QueueWorkDoneStatus status, wgpu::StringView message) {
            g_queue_status = status;
            g_queue_done = true;
            const auto text = as_string_view(message);
            report("OnSubmittedWorkDone status=%u message=%.*s",
                   static_cast<unsigned>(status),
                   static_cast<int>(text.size()),
                   text.data() ? text.data() : "");
        });

    const auto queue_wait = instance.WaitAny(queue_future, 5'000'000'000ull);
    report("WaitAny(queue) -> %u", static_cast<unsigned>(queue_wait));
    if (queue_wait != wgpu::WaitStatus::Success || !g_queue_done ||
        g_queue_status != wgpu::QueueWorkDoneStatus::Success) {
        report("FAIL GPU_COMPLETION");
        return false;
    }

    report("PASS DAWN_WEBGPU_VULKAN_NVK_OFFSCREEN_TRIANGLE");
    return true;
}

} // namespace

int main(int, char**) {
    bool mounted_sdmc_here = false;
    open_report(mounted_sdmc_here);

    report("WiiCompiled-Switch M3 Dawn/NVK offscreen probe");
    report("WiiCompiled pin: a135beb201042b20f390c6695ca6b26768820fb4");
    report("Dawn tag: v20260603.191052");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6");
    report("goal: Dawn/WebGPU -> Vulkan -> loaderless NVK -> offscreen triangle -> GPU completion");

    setenv("NVK_I_WANT_A_BROKEN_VULKAN_DRIVER", "1", 1);
    setenv("MESA_SHADER_CACHE_DISABLE", "1", 1);

    const bool passed = run_dawn_probe();
    report("RESULT=%s", passed ? "PASS" : "FAIL");

    wait_for_plus();

    g_device = {};
    g_adapter = {};

    if (g_report) {
        std::fclose(g_report);
        g_report = nullptr;
    }
    if (mounted_sdmc_here) {
        fsdevUnmountDevice("sdmc");
    }
    return passed ? 0 : 1;
}
