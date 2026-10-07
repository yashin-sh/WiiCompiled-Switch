#include "capture.hpp"
#include "dawn/native/DawnNative.h"
#include "dawn/webgpu_cpp.h"
#include "dolphin/gx.h"
#include "gfx/common.hpp"
#include "gx/fifo.hpp"
#include "internal.hpp"
#include "webgpu/gpu.hpp"

#include <zlib.h>

#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
std::atomic_bool gpuError{false};
void check(bool passed, const char* message) {
    if (!passed) {
        throw std::runtime_error(message);
    }
}
replay::Bytes load(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    check(static_cast<bool>(stream), "cannot open capture");
    const auto size = stream.tellg();
    check(size > 0 && size <= static_cast<std::streamoff>(replay::MaxBytes), "capture exceeds size bound");
    replay::Bytes data(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(data.data()), size);
    check(static_cast<bool>(stream), "cannot read capture");
    return data;
}
void save(const char* path, std::span<const std::uint8_t> data) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    stream.flush();
    check(static_cast<bool>(stream), "cannot save output");
}
void integer(replay::Bytes& out, std::uint32_t value) {
    for (int i = 3; i >= 0; --i) {
        out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
    }
}
void png(const char* path, const replay::Bytes& pixels) {
    replay::Bytes out{137, 80, 78, 71, 13, 10, 26, 10};
    const auto chunk = [&](const char* type, const replay::Bytes& bytes) {
        integer(out, bytes.size());
        const auto start = out.size();
        out.insert(out.end(), type, type + 4);
        out.insert(out.end(), bytes.begin(), bytes.end());
        integer(out, crc32(0, out.data() + start, bytes.size() + 4));
    };
    replay::Bytes header;
    integer(header, 256);
    integer(header, 256);
    header.insert(header.end(), {8, 6, 0, 0, 0});
    chunk("IHDR", header);
    replay::Bytes rows;
    for (std::size_t y = 0; y < 256; ++y) {
        rows.push_back(0);
        rows.insert(rows.end(), pixels.begin() + y * 1024, pixels.begin() + (y + 1) * 1024);
    }
    uLongf length = compressBound(rows.size());
    replay::Bytes compressed(length);
    check(compress2(compressed.data(), &length, rows.data(), rows.size(), 9) == Z_OK, "PNG compression failed");
    compressed.resize(length);
    chunk("IDAT", compressed);
    chunk("IEND", {});
    save(path, out);
}
void initialize_gpu() {
    wgpu::InstanceDescriptor descriptor{};
    const wgpu::InstanceFeatureName feature = wgpu::InstanceFeatureName::TimedWaitAny;
    descriptor.requiredFeatureCount = 1;
    descriptor.requiredFeatures = &feature;
    auto instance = wgpu::CreateInstance(&descriptor);
    check(static_cast<bool>(instance), "Dawn instance creation failed");
    wgpu::RequestAdapterOptions options{};
    options.backendType = wgpu::BackendType::Vulkan;
    wgpu::Adapter adapter;
    const auto adapterWait = instance.WaitAny(instance.RequestAdapter(
                                                  &options, wgpu::CallbackMode::WaitAnyOnly,
                                                  [&](wgpu::RequestAdapterStatus status, wgpu::Adapter result, wgpu::StringView) {
                                                      if (status == wgpu::RequestAdapterStatus::Success) {
                                                          adapter = std::move(result);
                                                      }
                                                  }),
                                              UINT64_MAX);
    check(adapterWait == wgpu::WaitStatus::Success && adapter, "no Vulkan adapter available");
    wgpu::DeviceDescriptor deviceDescriptor{};
    deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView message) {
        gpuError = true;
        std::fprintf(stderr, "Dawn error: %.*s\n", static_cast<int>(message.length == wgpu::kStrlen ? std::strlen(message.data) : message.length), message.data);
    });
    deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView) {
        if (reason != wgpu::DeviceLostReason::Destroyed) {
            gpuError = true;
        }
    });
    wgpu::Device device;
    const auto deviceWait = instance.WaitAny(adapter.RequestDevice(
                                                 &deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
                                                 [&](wgpu::RequestDeviceStatus status, wgpu::Device result, wgpu::StringView) {
                                                     if (status == wgpu::RequestDeviceStatus::Success) {
                                                         device = std::move(result);
                                                     }
                                                 }),
                                             UINT64_MAX);
    check(deviceWait == wgpu::WaitStatus::Success && device, "Dawn device creation failed");
    wgpu::Limits limits{};
    adapter.GetLimits(&limits);
    aurora::g_config = {};
    aurora::g_config.logLevel = LOG_WARNING;
    aurora::g_config.msaa = 1;
    aurora::g_config.maxTextureAnisotropy = 1;
    using namespace aurora::webgpu;
    g_instance = instance;
    g_device = device;
    g_queue = device.GetQueue();
    g_backendType = wgpu::BackendType::Vulkan;
    g_graphicsConfig = {
        .surfaceConfiguration = {.format = wgpu::TextureFormat::RGBA8Unorm, .width = 256, .height = 256},
        .depthFormat = wgpu::TextureFormat::Depth32Float,
        .msaaSamples = 1,
        .textureAnisotropy = 1,
        .maxTextureDimension2D = limits.maxTextureDimension2D,
    };
    const wgpu::TextureDescriptor color{
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding,
        .size = {256, 256, 1},
        .format = wgpu::TextureFormat::RGBA8Unorm,
    };
    g_frameBuffer.texture = device.CreateTexture(&color);
    g_frameBuffer.view = g_frameBuffer.texture.CreateView();
    g_frameBuffer.size = color.size;
    g_frameBuffer.format = color.format;
    auto depth = color;
    depth.format = wgpu::TextureFormat::Depth32Float;
    depth.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
    g_depthBuffer.texture = device.CreateTexture(&depth);
    g_depthBuffer.view = g_depthBuffer.texture.CreateView();
    g_depthBuffer.size = depth.size;
    g_depthBuffer.format = depth.format;
    aurora::gfx::initialize();
}
replay::Bytes finish_frame() {
    using namespace aurora::webgpu;
    const wgpu::BufferDescriptor desc{
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = 256 * 1024,
    };
    auto staging = g_device.CreateBuffer(&desc);
    auto encoder = g_device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);
    const wgpu::TexelCopyTextureInfo source{.texture = g_frameBuffer.texture};
    const wgpu::TexelCopyBufferInfo destination{.layout = {.bytesPerRow = 1024, .rowsPerImage = 256}, .buffer = staging};
    const wgpu::Extent3D extent{256, 256, 1};
    encoder.CopyTextureToBuffer(&source, &destination, &extent);
    auto commands = encoder.Finish();
    g_queue.Submit(1, &commands);
    aurora::gfx::after_submit();
    bool mapped = false;
    const auto wait = g_instance.WaitAny(staging.MapAsync(
                                             wgpu::MapMode::Read, 0, desc.size, wgpu::CallbackMode::WaitAnyOnly,
                                             [&](wgpu::MapAsyncStatus status, wgpu::StringView) { mapped = status == wgpu::MapAsyncStatus::Success; }),
                                         UINT64_MAX);
    check(wait == wgpu::WaitStatus::Success && mapped && !gpuError, "GPU rendering/readback failed");
    const auto* data = static_cast<const std::uint8_t*>(staging.GetConstMappedRange());
    replay::Bytes pixels(data, data + desc.size);
    staging.Unmap();
    return pixels;
}
void setup() {
    GXInit(nullptr, 0);
    const float identity[3][4]{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
    const float ortho[4][4]{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, -1, 0}, {0, 0, 0, 1}};
    GXLoadPosMtxImm(identity, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetProjection(ortho, GX_ORTHOGRAPHIC);
    GXSetViewport(0, 0, 256, 256, 0, 1);
    GXSetScissor(0, 0, 256, 256);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    aurora::gx::fifo::drain();
}
void triangle(float center, bool raw) {
    if (raw) {
        // Identical big-endian draw payload via the FIFO producer API. This
        // exercises a second producer without claiming a guest HLE integration.
        aurora::gx::fifo::write_u8(0x90);
        aurora::gx::fifo::write_u16(3);
        for (auto xy : {std::array<float, 2>{center, 0.7f}, {center - 0.4f, -0.7f}, {center + 0.4f, -0.7f}}) {
            aurora::gx::fifo::write_f32(xy[0]);
            aurora::gx::fifo::write_f32(xy[1]);
            aurora::gx::fifo::write_f32(0);
            aurora::gx::fifo::write_f32(0.5f);
            aurora::gx::fifo::write_f32(0.5f);
        }
    } else {
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
        for (auto xy : {std::array<float, 2>{center, 0.7f}, {center - 0.4f, -0.7f}, {center + 0.4f, -0.7f}}) {
            GXPosition3f32(xy[0], xy[1], 0);
            GXTexCoord2f32(0.5f, 0.5f);
        }
        GXEnd();
    }
    aurora::gx::fifo::drain();
}
void fill_texture(std::array<std::uint8_t, 64>& tex, bool blue) {
    // One 4x4 tiled RGBA8 block: AR plane followed by GB plane.
    for (std::size_t i = 0; i < 16; ++i) {
        tex[i * 2] = 255;
        tex[i * 2 + 1] = blue ? 0 : 255;
        tex[32 + i * 2] = 0;
        tex[33 + i * 2] = blue ? 255 : 0;
    }
}
void check_scene(const replay::Bytes& pixels) {
    const auto pixel = [&](int x, int y, int r, int g, int b) {
        const auto offset = (y * 256 + x) * 4;
        if (pixels[offset] != r || pixels[offset + 1] != g || pixels[offset + 2] != b || pixels[offset + 3] != 255) {
            std::fprintf(stderr, "Pixel (%d,%d): expected %d,%d,%d,255; got %u,%u,%u,%u\n", x, y, r, g, b, pixels[offset], pixels[offset + 1], pixels[offset + 2], pixels[offset + 3]);
        }
        check(pixels[offset] == r && pixels[offset + 1] == g && pixels[offset + 2] == b && pixels[offset + 3] == 255, "synthetic pixel oracle failed");
    };
    pixel(64, 128, 255, 0, 0);
    pixel(192, 128, 0, 0, 255);
    pixel(8, 8, 64, 64, 64);
}
} // namespace

extern "C" void mkw_replay_log(const char* message) {
    std::fputs(message, stderr);
}

int main(int argc, char** argv) {
    bool initialized = false;
    try {
        check(argc == 4 && (std::string(argv[1]) == "capture" || std::string(argv[1]) == "replay" || std::string(argv[1]) == "replay-check"), "usage: mkw-gx-replay capture|replay|replay-check capture.mkwr output.png");
        const bool capture = std::string(argv[1]) == "capture";
        std::unique_ptr<replay::Playback> playback;
        if (!capture) {
            // Parse and validate the whole file before allocating a GPU device.
            playback = std::make_unique<replay::Playback>(load(argv[2]));
        }
        initialize_gpu();
        initialized = true;
        replay::Bytes pixels;
        if (capture) {
            replay::Recorder recorder;
            std::array<std::uint8_t, 64> texture{};
            fill_texture(texture, false);
            recorder.memory(texture);
            replay::set_recorder(&recorder);
            setup();
            GXTexObj obj{};
            GXInitTexObj(&obj, texture.data(), 4, 4, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GXLoadTexObj(&obj, GX_TEXMAP0);
            aurora::gx::fifo::drain();
            recorder.begin();
            check(aurora::gfx::begin_frame(), "Aurora begin_frame failed");
            triangle(-0.5f, false);
            fill_texture(texture, true);
            GXInvalidateTexAll();
            GXInitTexObj(&obj, texture.data(), 4, 4, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GXLoadTexObj(&obj, GX_TEXMAP0);
            // Flush pending native state, then use raw FIFO.
            GXFlush();
            triangle(0.5f, true);
            recorder.end();
            replay::set_recorder(nullptr);
            pixels = finish_frame();
            check_scene(pixels);
            save(argv[2], recorder.finish());
        } else {
            // GXInit establishes Aurora's non-FIFO defaults. Discard its queued
            // bytes: the capture includes the original initialization stream.
            GXInit(nullptr, 0);
            aurora::gx::fifo::clear_buffer();
            playback->run([&](replay::Kind kind, std::span<const std::uint8_t> bytes) {
                if (kind == replay::Kind::Begin) {
                    check(aurora::gfx::begin_frame(), "Aurora begin_frame failed");
                } else if (kind == replay::Kind::Fifo) {
                    aurora::gx::fifo::write_data(bytes.data(), bytes.size());
                    aurora::gx::fifo::drain();
                } else if (kind == replay::Kind::End) {
                    pixels = finish_frame();
                }
            });
            if (std::string(argv[1]) == "replay-check") {
                check_scene(pixels);
            }
        }
        png(argv[3], pixels);
        aurora::gfx::shutdown();
        aurora::webgpu::shutdown();
        std::puts("PASS: completed Aurora frame, GPU readback and PNG output");
        return 0;
    } catch (const std::exception& error) {
        replay::set_recorder(nullptr);
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        if (initialized) {
            aurora::gfx::shutdown();
            aurora::webgpu::shutdown();
        }
        return 1;
    }
}
