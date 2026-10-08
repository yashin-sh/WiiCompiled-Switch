#if (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK) || defined(MKW_FRAME_DUMP_DESKTOP)
#include "surface_presenter.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace mkw::presentation {
namespace {
bool colorFormat(wgpu::TextureFormat format) {
    return format == wgpu::TextureFormat::RGBA8Unorm || format == wgpu::TextureFormat::BGRA8Unorm ||
           format == wgpu::TextureFormat::RGBA8UnormSrgb || format == wgpu::TextureFormat::BGRA8UnormSrgb;
}
} // namespace
Presenter::Presenter(wgpu::Device device, wgpu::TextureFormat destinationFormat)
    : device_(std::move(device)), format_(destinationFormat) {
    if (!device_ || !colorFormat(format_))
        throw std::runtime_error("presentation requires an RGBA8/BGRA8 device target");
    // Adapted from pinned Aurora lib/webgpu/gpu.cpp create_copy_pipeline.
    // The fullscreen triangle preserves RGB and emits opaque display alpha.
    wgpu::ShaderSourceWGSL shader{};
    shader.code = R"(
@group(0) @binding(0) var sourceSampler: sampler;
@group(0) @binding(1) var sourceImage: texture_2d<f32>;
struct Output { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs_main(@builtin(vertex_index) index: u32) -> Output {
    let positions = array<vec2<f32>, 3>(vec2(-1.0, 1.0), vec2(-1.0, -3.0), vec2(3.0, 1.0));
    let uvs = array<vec2<f32>, 3>(vec2(0.0, 0.0), vec2(0.0, 2.0), vec2(2.0, 0.0));
    var result: Output;
    result.position = vec4(positions[index], 0.0, 1.0);
    result.uv = uvs[index];
    return result;
}
@fragment fn fs_main(input: Output) -> @location(0) vec4<f32> {
    let color = textureSample(sourceImage, sourceSampler, input.uv);
    return vec4(color.rgb, 1.0);
}
)";
    const wgpu::ShaderModuleDescriptor moduleDescriptor{.nextInChain = &shader, .label = "Aurora XFB presentation"};
    const auto module = device_.CreateShaderModule(&moduleDescriptor);
    const wgpu::ColorTargetState target{.format = format_, .writeMask = wgpu::ColorWriteMask::All};
    const wgpu::FragmentState fragment{.module = module, .entryPoint = "fs_main", .targetCount = 1, .targets = &target};
    const wgpu::RenderPipelineDescriptor pipeline{
        .vertex = {.module = module, .entryPoint = "vs_main"},
        .primitive = {.topology = wgpu::PrimitiveTopology::TriangleList},
        .fragment = &fragment,
    };
    pipeline_ = device_.CreateRenderPipeline(&pipeline);
    const wgpu::SamplerDescriptor sampler{.addressModeU = wgpu::AddressMode::ClampToEdge, .addressModeV = wgpu::AddressMode::ClampToEdge, .magFilter = wgpu::FilterMode::Linear, .minFilter = wgpu::FilterMode::Linear};
    sampler_ = device_.CreateSampler(&sampler);
}
void Presenter::encode(wgpu::CommandEncoder encoder, wgpu::Texture source, wgpu::Texture destination) const {
    if (!encoder || !source || !destination || source.Get() == destination.Get() || !colorFormat(source.GetFormat()) ||
        source.GetSampleCount() != 1u || destination.GetSampleCount() != 1u || source.GetDepthOrArrayLayers() != 1u ||
        destination.GetDepthOrArrayLayers() != 1u || destination.GetFormat() != format_ ||
        (source.GetUsage() & wgpu::TextureUsage::TextureBinding) == wgpu::TextureUsage::None ||
        (destination.GetUsage() & wgpu::TextureUsage::RenderAttachment) == wgpu::TextureUsage::None)
        throw std::runtime_error("invalid or aliased presentation textures");
    const std::array entries{wgpu::BindGroupEntry{.binding = 0, .sampler = sampler_},
                             wgpu::BindGroupEntry{.binding = 1, .textureView = source.CreateView()}};
    const wgpu::BindGroupDescriptor binding{.layout = pipeline_.GetBindGroupLayout(0), .entryCount = entries.size(), .entries = entries.data()};
    const auto group = device_.CreateBindGroup(&binding);
    const wgpu::RenderPassColorAttachment color{
        .view = destination.CreateView(), .loadOp = wgpu::LoadOp::Clear, .storeOp = wgpu::StoreOp::Store, .clearValue = {0, 0, 0, 1}};
    const wgpu::RenderPassDescriptor descriptor{.label = "Present selected XFB", .colorAttachmentCount = 1, .colorAttachments = &color};
    const auto pass = encoder.BeginRenderPass(&descriptor);
    pass.SetPipeline(pipeline_);
    pass.SetBindGroup(0, group);
    pass.SetViewport(0, 0, destination.GetWidth(), destination.GetHeight(), 0, 1);
    pass.Draw(3);
    pass.End();
}
} // namespace mkw::presentation
#endif
