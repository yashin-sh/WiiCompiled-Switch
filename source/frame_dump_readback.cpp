#if (defined(MKW_LOCAL_RENDERED_FAST_TRACK) && MKW_LOCAL_RENDERED_FAST_TRACK && defined(MKW_RENDERED_FRAME_DUMP) && MKW_RENDERED_FRAME_DUMP) || defined(MKW_FRAME_DUMP_DESKTOP)
#include "frame_dump_readback.hpp"

#include <memory>
#include <stdexcept>

namespace mkw::frame_dump {
namespace {
constexpr std::uint64_t WaitNs = 1000000000u;
struct Result {
    bool success = false;
};
} // namespace
Readback::~Readback() {
    try {
        if (scopeOpen_)
            device_.PopErrorScope(wgpu::CallbackMode::AllowSpontaneous,
                                  [](wgpu::PopErrorScopeStatus, wgpu::ErrorType, wgpu::StringView) {});
    } catch (...) {
    }
    if (buffer_)
        buffer_.Destroy();
}
void Readback::encode(wgpu::Device device, wgpu::CommandEncoder encoder, wgpu::Texture source) {
    if (buffer_ || !source || !device || !encoder)
        throw std::runtime_error("invalid or reused readback objects");
    const auto format = source.GetFormat();
    bgra_ = format == wgpu::TextureFormat::BGRA8Unorm || format == wgpu::TextureFormat::BGRA8UnormSrgb;
    if (!bgra_ && format != wgpu::TextureFormat::RGBA8Unorm && format != wgpu::TextureFormat::RGBA8UnormSrgb)
        throw std::runtime_error("readback requires RGBA8/BGRA8 surface");
    if (source.GetSampleCount() != 1u || source.GetDepthOrArrayLayers() != 1u || (source.GetUsage() & wgpu::TextureUsage::CopySrc) == wgpu::TextureUsage::None)
        throw std::runtime_error("readback requires single-sample CopySrc texture");
    layout_ = layout(source.GetWidth(), source.GetHeight());
    device_ = device;
    device_.PushErrorScope(wgpu::ErrorFilter::Validation);
    scopeOpen_ = true;
    const wgpu::BufferDescriptor desc{.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, .size = layout_.bufferBytes};
    buffer_ = device_.CreateBuffer(&desc);
    const wgpu::TexelCopyTextureInfo from{.texture = source};
    const wgpu::TexelCopyBufferInfo to{.layout = {.bytesPerRow = layout_.rowBytes, .rowsPerImage = layout_.height}, .buffer = buffer_};
    const wgpu::Extent3D extent{layout_.width, layout_.height, 1};
    encoder.CopyTextureToBuffer(&from, &to, &extent);
}
Image Readback::finish(wgpu::Instance instance) {
    if (!scopeOpen_ || !buffer_ || !instance)
        throw std::runtime_error("readback was not encoded");
    // Captured heap values outlive a timed-out callback; never capture stack references.
    auto validation = std::make_shared<Result>();
    const auto error = device_.PopErrorScope(wgpu::CallbackMode::WaitAnyOnly,
                                             [validation](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type, wgpu::StringView) {
                                                 validation->success = status == wgpu::PopErrorScopeStatus::Success && type == wgpu::ErrorType::NoError;
                                             });
    scopeOpen_ = false;
    if (instance.WaitAny(error, WaitNs) != wgpu::WaitStatus::Success || !validation->success)
        throw std::runtime_error("GPU readback validation failed or timed out");
    auto mapped = std::make_shared<Result>();
    const auto map = buffer_.MapAsync(wgpu::MapMode::Read, 0, layout_.bufferBytes, wgpu::CallbackMode::WaitAnyOnly,
                                      [mapped](wgpu::MapAsyncStatus status, wgpu::StringView) { mapped->success = status == wgpu::MapAsyncStatus::Success; });
    if (instance.WaitAny(map, WaitNs) != wgpu::WaitStatus::Success || !mapped->success)
        throw std::runtime_error("GPU map failed or timed out");
    const auto* bytes = static_cast<const std::uint8_t*>(buffer_.GetConstMappedRange(0, layout_.bufferBytes));
    if (!bytes)
        throw std::runtime_error("GPU mapped range is null");
    auto image = unpack(layout_, {bytes, static_cast<std::size_t>(layout_.bufferBytes)}, bgra_);
    buffer_.Unmap();
    return image;
}
} // namespace mkw::frame_dump
#endif
