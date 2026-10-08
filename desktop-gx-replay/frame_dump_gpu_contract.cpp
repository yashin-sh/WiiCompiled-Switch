#include "frame_dump_readback.hpp"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace {
std::atomic_bool error{false};
void require(bool condition, const char* reason) {
    if (!condition)
        throw std::runtime_error(reason);
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "pass a private synthetic output directory");
        std::filesystem::create_directories(argv[1]);
        const wgpu::InstanceFeatureName feature = wgpu::InstanceFeatureName::TimedWaitAny;
        const wgpu::InstanceDescriptor descriptor{.requiredFeatureCount = 1, .requiredFeatures = &feature};
        auto instance = wgpu::CreateInstance(&descriptor);
        require(static_cast<bool>(instance), "no Dawn instance");
        wgpu::Adapter adapter;
        const wgpu::RequestAdapterOptions options{.backendType = wgpu::BackendType::Vulkan};
        instance.WaitAny(instance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly,
                                                 [&](wgpu::RequestAdapterStatus status, wgpu::Adapter found, wgpu::StringView) {
                                                     if (status == wgpu::RequestAdapterStatus::Success)
                                                         adapter = std::move(found);
                                                 }),
                         UINT64_MAX);
        require(static_cast<bool>(adapter), "no Vulkan adapter");
        wgpu::Device device;
        wgpu::DeviceDescriptor deviceDescriptor{};
        deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView) { error = true; });
        instance.WaitAny(adapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
                                               [&](wgpu::RequestDeviceStatus status, wgpu::Device found, wgpu::StringView) {
                                                   if (status == wgpu::RequestDeviceStatus::Success)
                                                       device = std::move(found);
                                               }),
                         UINT64_MAX);
        require(static_cast<bool>(device), "no Vulkan device");
        auto queue = device.GetQueue();
        for (auto format : {wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureFormat::BGRA8Unorm, wgpu::TextureFormat::RGBA8UnormSrgb, wgpu::TextureFormat::BGRA8UnormSrgb}) {
            const auto size = mkw::frame_dump::layout(617, 3);
            const wgpu::TextureDescriptor textureDescriptor{.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::RenderAttachment,
                                                            .size = {size.width, size.height, 1},
                                                            .format = format};
            auto texture = device.CreateTexture(&textureDescriptor);
            const bool bgra = format == wgpu::TextureFormat::BGRA8Unorm || format == wgpu::TextureFormat::BGRA8UnormSrgb;
            std::vector<std::uint8_t> padded(size.bufferBytes, 0xa5), expected;
            for (std::uint32_t y = 0; y < size.height; ++y)
                for (std::uint32_t x = 0; x < size.width; ++x) {
                    const std::uint8_t r = x & 255u, g = 31u + y, b = 251u - (x & 127u), a = (x & 1u) ? 255u : 128u;
                    const auto at = y * size.rowBytes + x * 4u;
                    padded[at] = bgra ? b : r;
                    padded[at + 1] = g;
                    padded[at + 2] = bgra ? r : b;
                    padded[at + 3] = a;
                    expected.insert(expected.end(), {r, g, b, a});
                }
            const wgpu::TexelCopyTextureInfo destination{.texture = texture};
            const wgpu::TexelCopyBufferLayout upload{.bytesPerRow = size.rowBytes, .rowsPerImage = size.height};
            const wgpu::Extent3D extent{size.width, size.height, 1};
            queue.WriteTexture(&destination, padded.data(), padded.size(), &upload, &extent);
            auto encoder = device.CreateCommandEncoder();
            mkw::frame_dump::Readback readback;
            readback.encode(device, encoder, texture);
            auto commands = encoder.Finish();
            queue.Submit(1, &commands);
            const auto pixels = readback.finish(instance);
            require(pixels.rgba == expected && pixels.nonBlackPixels == size.width * size.height && !pixels.uniform, "GPU padded/channel pixel oracle failed");
            const auto output = std::filesystem::path(argv[1]) / ("gpu-" + std::to_string(static_cast<unsigned>(format)) + ".png");
            const auto temporary = output.string() + ".tmp";
            mkw::frame_dump::save(output.c_str(), temporary.c_str(), pixels);
            // A GPU render pass then overwrites the same source with opaque black.
            encoder = device.CreateCommandEncoder();
            const wgpu::RenderPassColorAttachment color{.view = texture.CreateView(), .loadOp = wgpu::LoadOp::Clear, .storeOp = wgpu::StoreOp::Store, .clearValue = {0, 0, 0, 1}};
            const wgpu::RenderPassDescriptor passDescriptor{.colorAttachmentCount = 1, .colorAttachments = &color};
            auto pass = encoder.BeginRenderPass(&passDescriptor);
            pass.End();
            mkw::frame_dump::Readback black;
            black.encode(device, encoder, texture);
            commands = encoder.Finish();
            queue.Submit(1, &commands);
            const auto cleared = black.finish(instance);
            require(cleared.uniform && !cleared.nonBlackPixels && !cleared.nonOpaquePixels, "GPU render-before-copy black/alpha oracle failed");
        }
        const wgpu::TextureDescriptor textureDescriptor{.usage = wgpu::TextureUsage::CopySrc, .size = {4, 4, 1}, .format = wgpu::TextureFormat::RGBA8Unorm};
        auto destroyed = device.CreateTexture(&textureDescriptor);
        destroyed.Destroy();
        auto encoder = device.CreateCommandEncoder();
        mkw::frame_dump::Readback invalid;
        invalid.encode(device, encoder, destroyed);
        auto commands = encoder.Finish();
        queue.Submit(1, &commands);
        bool rejected = false;
        try {
            invalid.finish(instance);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        require(rejected, "invalid GPU copy was reported as successful");
        require(!error, "GPU validation escaped the readback error scope");
        std::puts("PASS: real Vulkan RGBA/BGRA/sRGB readbacks, padded rows, render-before-copy pixels and captured validation failure");
    } catch (const std::exception& failure) {
        std::fprintf(stderr, "FAIL: %s\n", failure.what());
        return 1;
    }
}
