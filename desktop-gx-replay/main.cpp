#include "capture.hpp"
#include "frame_dump_readback.hpp"
#include "surface_presenter.hpp"
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
#include <bit>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace aurora::gx::fifo {
bool submit_raw_draw(GXPrimitive, GXVtxFmt, const std::uint8_t*, std::uint16_t, std::uint32_t);
}
namespace {
std::unique_ptr<mkw::presentation::Presenter> outputPresenter;
unsigned renderedFrames = 0;
std::atomic_bool gpuError{false};
std::uint32_t width = 256, height = 256;
extern "C" void mkw_replay_set_size(unsigned, unsigned);
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
    integer(header, width);
    integer(header, height);
    header.insert(header.end(), {8, 6, 0, 0, 0});
    chunk("IHDR", header);
    replay::Bytes rows;
    for (std::size_t y = 0; y < height; ++y) {
        rows.push_back(0);
        rows.insert(rows.end(), pixels.begin() + y * width * 4, pixels.begin() + (y + 1) * width * 4);
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
        .surfaceConfiguration = {.format = wgpu::TextureFormat::RGBA8Unorm, .width = width, .height = height},
        .depthFormat = wgpu::TextureFormat::Depth32Float,
        .msaaSamples = 1,
        .textureAnisotropy = 1,
        .maxTextureDimension2D = limits.maxTextureDimension2D,
    };
    const wgpu::TextureDescriptor color{
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding,
        .size = {width, height, 1},
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
    // Read the image selected for presentation. GXCopyDisp(clear=true) clears
    // the EFB after copying it; reading only the EFB would hide a valid image.
    const auto presented = current_present_source();
    check(static_cast<bool>(presented.texture), "no rendered presentation source");
    width = presented.size.width;
    height = presented.size.height;
    auto encoder = g_device.CreateCommandEncoder();
    aurora::gfx::end_frame(encoder);
    aurora::gfx::render(encoder);
    const wgpu::TextureDescriptor outputDescriptor{
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc,
        .size = presented.size,
        .format = wgpu::TextureFormat::RGBA8Unorm};
    auto output = g_device.CreateTexture(&outputDescriptor);
    if (!outputPresenter)
        outputPresenter = std::make_unique<mkw::presentation::Presenter>(g_device, outputDescriptor.format);
    outputPresenter->encode(encoder, presented.texture, output);
    mkw::frame_dump::Readback readback;
    readback.encode(g_device, encoder, output);
    auto commands = encoder.Finish();
    g_queue.Submit(1, &commands);
    aurora::gfx::after_submit();
    auto image = readback.finish(g_instance);
    check(!gpuError, "GPU rendering failed");
    ++renderedFrames;
    return std::move(image.rgba);
}
void setup() {
    GXInit(nullptr, 0);
    const float identity[3][4]{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
    const float ortho[4][4]{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, -1, 0}, {0, 0, 0, 1}};
    GXLoadPosMtxImm(identity, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetProjection(ortho, GX_ORTHOGRAPHIC);
    GXSetViewport(0, 0, width, height, 0, 1);
    GXSetScissor(0, 0, width, height);
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
void triangle(float center, bool raw, bool indexed = false, bool directSubmission = false, bool axisSamples = false) {
    // Isolate S outside-range on the left and T outside-range on the right.
    // Both sampler axes must independently affect the pixel oracles.
    const auto u = axisSamples ? (center < 0.f ? 1.25f : 0.25f) : 0.5f;
    const auto v = axisSamples ? (center < 0.f ? 0.25f : -0.75f) : 0.5f;
    if (raw) {
        replay::Bytes vertices;
        for (auto xy : {std::array<float, 2>{center, 0.7f}, {center - 0.4f, -0.7f}, {center + 0.4f, -0.7f}}) {
            for (float value : {xy[0], xy[1], 0.f, u, v})
                integer(vertices, std::bit_cast<std::uint32_t>(value));
        }
        if (directSubmission) {
            check(aurora::gx::fifo::submit_raw_draw(GX_TRIANGLES, GX_VTXFMT0, vertices.data(), 3, vertices.size()), "direct raw draw submission failed");
        } else {
            aurora::gx::fifo::write_u8(0x90);
            aurora::gx::fifo::write_u16(3);
            aurora::gx::fifo::write_data(vertices.data(), vertices.size());
        }
    } else {
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
        unsigned index = 0;
        for (auto xy : {std::array<float, 2>{center, 0.7f}, {center - 0.4f, -0.7f}, {center + 0.4f, -0.7f}}) {
            if (indexed) {
                GXPosition1x8(index++);
                GXTexCoord1x8(0);
            } else {
                GXPosition3f32(xy[0], xy[1], 0);
                GXTexCoord2f32(u, v);
            }
        }
        GXEnd();
    }
    aurora::gx::fifo::drain();
}
void fill_texture(std::span<std::uint8_t> tex, bool blue) {
    check(tex.size() == 64u, "RGBA8 fixture needs one complete tile");
    // One 4x4 tiled RGBA8 block: AR plane followed by GB plane.
    for (std::size_t i = 0; i < 16; ++i) {
        tex[i * 2] = 255;
        tex[i * 2 + 1] = blue ? 0 : 255;
        tex[32 + i * 2] = 0;
        tex[33 + i * 2] = blue ? 255 : 0;
    }
}
void check_scene(const replay::Bytes& pixels, bool copies = false, bool reversed = false, bool intensity = false, bool quads = false, bool clampIntensity = false, bool ia4Intensity = false) {
    const auto pixel = [&](int x, int y, int r, int g, int b) {
        const auto offset = (y * width + x) * 4;
        if (pixels[offset] != r || pixels[offset + 1] != g || pixels[offset + 2] != b || pixels[offset + 3] != 255) {
            std::fprintf(stderr, "Pixel (%d,%d): expected %d,%d,%d,255; got %u,%u,%u,%u\n", x, y, r, g, b, pixels[offset], pixels[offset + 1], pixels[offset + 2], pixels[offset + 3]);
        }
        check(pixels[offset] == r && pixels[offset + 1] == g && pixels[offset + 2] == b && pixels[offset + 3] == 255, "synthetic pixel oracle failed");
    };
    if (intensity) {
        const auto high = ia4Intensity ? 119 : 255;
        const auto low = ia4Intensity ? 34 : 0;
        const auto left = clampIntensity ? low : high;
        const auto right = clampIntensity ? high : low;
        pixel(width / 4, height / 2, left, left, left);
        pixel(width * 3 / 4, height / 2, right, right, right);
    } else {
        pixel(width / 4, height / 2, copies ? 64 : (reversed ? 0 : 255), copies ? 64 : (intensity ? 255 : 0), copies ? 64 : (reversed || intensity ? 255 : 0));
        pixel(width * 3 / 4, height / 2, copies || reversed ? 255 : 0, 0, copies || reversed || intensity ? 0 : 255);
    }
    pixel(8, 8, 64, 64, 64);
    if (quads) {
        for (int y : {height / 4, height * 3 / 4}) {
            pixel(width / 8, y, 255, 0, 0);
            pixel(width * 3 / 8, y, 255, 0, 0);
            pixel(width * 5 / 8, y, 0, 0, 255);
            pixel(width * 7 / 8, y, 0, 0, 255);
        }
    }
}
} // namespace

extern "C" void mkw_replay_log(const char* message) {
    std::fputs(message, stderr);
}

void draw_lyt_quad(float center, bool colors, bool blue);

int main(int argc, char** argv) {
    bool initialized = false;
    try {
        check(argc == 4 && (std::string(argv[1]) == "capture-lyt-quads" || std::string(argv[1]) == "replay-lyt-quads-check" || std::string(argv[1]) == "capture-lyt-colors" || std::string(argv[1]) == "replay-lyt-colors-check" || std::string(argv[1]) == "capture-ia4-large-clamp" || std::string(argv[1]) == "replay-ia4-large-clamp-check" || std::string(argv[1]) == "capture-ia4-clamp" || std::string(argv[1]) == "replay-ia4-clamp-check" || std::string(argv[1]) == "capture-ia4-repeat" || std::string(argv[1]) == "replay-ia4-repeat-check" || std::string(argv[1]) == "capture-ia8-clamp" || std::string(argv[1]) == "replay-ia8-clamp-check" || std::string(argv[1]) == "capture-ia8-repeat" || std::string(argv[1]) == "replay-ia8-repeat-check" || std::string(argv[1]) == "capture-i4" || std::string(argv[1]) == "replay-i4-check" || std::string(argv[1]) == "capture-rgb5a3" || std::string(argv[1]) == "capture-sequence" || std::string(argv[1]) == "replay-sequence-check" || std::string(argv[1]) == "capture" || std::string(argv[1]) == "capture-copies" || std::string(argv[1]) == "capture-direct-copies" || std::string(argv[1]) == "replay-direct-copies-check" || std::string(argv[1]) == "capture-wide" || std::string(argv[1]) == "capture-indexed" || std::string(argv[1]) == "replay" || std::string(argv[1]) == "replay-check" || std::string(argv[1]) == "replay-copies-check"), "usage: mkw-gx-replay capture|replay|replay-check capture.mkwr output.png");
        const bool ia4 = std::string(argv[1]).find("ia4-") != std::string::npos;
        const bool ia4Large = std::string(argv[1]).find("ia4-large-") != std::string::npos;
        const bool ia8 = std::string(argv[1]).find("ia8-") != std::string::npos;
        const bool ia = ia4 || ia8;
        const bool iaClamp = ia && std::string(argv[1]).find("-clamp") != std::string::npos;
        const bool iaRepeat = ia && !iaClamp;
        const bool i4 = std::string(argv[1]).find("i4") != std::string::npos;
        const bool rgb5a3 = std::string(argv[1]) == "capture-rgb5a3";
        const bool sequence = std::string(argv[1]).find("sequence") != std::string::npos;
        const bool lyt = std::string(argv[1]).find("lyt-") != std::string::npos;
        const bool lytColors = std::string(argv[1]).find("lyt-colors") != std::string::npos;
        const bool indexed = std::string(argv[1]) == "capture-indexed";
        const bool copies = std::string(argv[1]).find("copies") != std::string::npos;
        const bool directCopies = std::string(argv[1]).find("direct-copies") != std::string::npos;
        const bool capture = std::string(argv[1]).starts_with("capture");
        if (std::string(argv[1]) == "capture-wide") {
            width = 617;
            height = 341;
        }
        std::unique_ptr<replay::Playback> playback;
        if (!capture) {
            // Parse and validate the whole file before allocating a GPU device.
            playback = std::make_unique<replay::Playback>(load(argv[2]));
            width = playback->width;
            height = playback->height;
        }
        mkw_replay_set_size(width, height);
        initialize_gpu();
        initialized = true;
        replay::Bytes pixels;
        if (capture) {
            replay::Recorder recorder(width, height);
            const auto textureFormat = ia4 ? GX_TF_IA4 : (ia8 ? GX_TF_IA8 : (i4 ? GX_TF_I4 : (rgb5a3 ? GX_TF_RGB5A3 : GX_TF_RGBA8)));
            const auto textureWidth = ia4Large ? 1024u : (ia ? 32u : (i4 ? 9u : (rgb5a3 ? 17u : 4u)));
            const auto textureHeight = ia4Large ? 1024u : (ia ? 32u : (i4 || rgb5a3 ? 9u : 4u));
            std::vector<std::uint8_t> texture(ia4 ? textureWidth * textureHeight : (ia8 ? 2048u : (i4 ? 128u : (rgb5a3 ? 480u : 64u))));
            std::array<std::uint8_t, 64> copyDestination{};
            const auto fill = [&](bool second) {
                if (ia4) {
                    // IA4 uses 8x4 byte tiles, alpha in the high nibble.
                    // Midrange intensities independently check 4-bit expansion.
                    for (unsigned y = 0; y < textureHeight; ++y)
                        for (unsigned x = 0; x < textureWidth; ++x) {
                            const auto at = ((y / 4u) * (textureWidth / 8u) + x / 8u) * 32u + (y % 4u) * 8u + x % 8u;
                            texture[at] = 0xf0u | (((x < textureWidth / 2u && y >= textureHeight / 8u && y < textureHeight / 2u) != second) ? 7u : 2u);
                        }
                } else if (ia8) {
                    // Tiled IA8: alpha followed by intensity. Sample outside
                    // each axis separately; clamp and repeat give opposite pixels.
                    for (unsigned y = 0; y < 32u; ++y)
                        for (unsigned x = 0; x < 32u; ++x) {
                            const auto at = ((y / 4u) * 8u + x / 4u) * 32u + ((y % 4u) * 4u + x % 4u) * 2u;
                            texture[at] = 255;
                            texture[at + 1u] = ((x < 16u && y >= 4u && y < 16u) != second) ? 255 : 0;
                        }
                } else if (i4)
                    std::fill(texture.begin(), texture.end(), second ? 0 : 255);
                else if (rgb5a3) {
                    for (std::size_t at = 0; at < texture.size(); at += 2) {
                        texture[at] = second ? 0x80 : 0xfc;
                        texture[at + 1] = second ? 0x1f : 0;
                    }
                } else
                    fill_texture(texture, second);
            };
            fill(false);
            const std::array<float, 9> positions{-0.5f, 0.7f, 0.f, -0.9f, -0.7f, 0.f, -0.1f, -0.7f, 0.f};
            const std::array<float, 2> uv{0.5f, 0.5f};
            recorder.memory(texture);
            if (indexed) {
                recorder.memory({reinterpret_cast<const std::uint8_t*>(positions.data()), sizeof(positions)});
                recorder.memory({reinterpret_cast<const std::uint8_t*>(uv.data()), sizeof(uv)});
            }
            if (copies)
                recorder.memory(copyDestination);
            replay::set_recorder(&recorder);
            setup();
            if (lytColors) {
                GXSetNumChans(1);
                GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX,
                              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
                GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
                GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
            }
            if (directCopies) {
                // Alpha-bearing EFB and matching RGBA8 dimensions select the
                // exact texture-copy path rather than conversion/alpha blits.
                GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
                GXSetCopyFilter(GX_FALSE, nullptr, GX_FALSE, nullptr);
            }
            GXTexObj obj{};
            GXInitTexObj(&obj, texture.data(), textureWidth, textureHeight, textureFormat, iaRepeat ? GX_REPEAT : GX_CLAMP, iaRepeat ? GX_REPEAT : GX_CLAMP, GX_FALSE);
            if (ia)
                GXInitTexObjLOD(&obj, GX_LINEAR, GX_LINEAR, 0.f, 0.f, 0.f, GX_FALSE, GX_FALSE, GX_ANISO_1);
            GXLoadTexObj(&obj, GX_TEXMAP0);
            aurora::gx::fifo::drain();
            recorder.begin();
            check(aurora::gfx::begin_frame(), "Aurora begin_frame failed");
            if (indexed) {
                GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
                GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
                GXSetArray(GX_VA_POS, positions.data(), sizeof(positions), 12, true);
                GXSetArray(GX_VA_TEX0, uv.data(), sizeof(uv), 8, true);
            }
            if (lyt)
                draw_lyt_quad(-0.5f, lytColors, false);
            else
                triangle(-0.5f, false, indexed, false, ia);
            if (indexed) {
                GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
                GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            }
            if (copies) {
                if (directCopies)
                    AuroraSetViewportPolicy(AURORA_VIEWPORT_NATIVE);
                GXSetCopyClear(GXColor{64, 64, 64, 255}, 0xffffff);
                GXSetTexCopySrcRender(62, 126, 4, 4);
                GXSetTexCopyDst(4, 4, directCopies ? GX_TF_RGBA8 : GX_TF_RGB5A3, GX_FALSE);
                GXCopyTex(copyDestination.data(), GX_TRUE);
                if (directCopies)
                    AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);
            }
            fill(true);
            GXInvalidateTexAll();
            GXInitTexObj(&obj, copies ? copyDestination.data() : texture.data(), copies ? 4u : textureWidth, copies ? 4u : textureHeight, copies ? (directCopies ? GX_TF_RGBA8 : GX_TF_RGB5A3) : textureFormat, iaRepeat ? GX_REPEAT : GX_CLAMP, iaRepeat ? GX_REPEAT : GX_CLAMP, GX_FALSE);
            if (ia)
                GXInitTexObjLOD(&obj, GX_LINEAR, GX_LINEAR, 0.f, 0.f, 0.f, GX_FALSE, GX_FALSE, GX_ANISO_1);
            GXLoadTexObj(&obj, GX_TEXMAP0);
            // Flush pending native state, then use raw FIFO.
            GXFlush();
            if (lyt)
                draw_lyt_quad(0.5f, lytColors, true);
            else
                triangle(0.5f, true, false, copies, ia);
            if (copies) {
                GXSetDispCopySrc(0, 0, width, height);
                GXSetDispCopyDst(width, height);
                GXCopyDisp(nullptr, GX_TRUE);
                GXDestroyCopyTex(copyDestination.data());
                aurora::gx::fifo::drain();
            }
            check(replay::recording(), replay::failure());
            if (sequence) {
                GXSetDispCopySrc(0, 0, width, height);
                GXSetDispCopyDst(width, height);
                GXCopyDisp(nullptr, GX_TRUE);
                aurora::gx::fifo::drain();
                pixels = finish_frame();
                check_scene(pixels);
                recorder.frame();
                recorder.begin();
                check(aurora::gfx::begin_frame(), "second Aurora frame failed");
                // Slot/VAT/projection/sampler state is retained, without GXInit.
                triangle(-0.5f, false, false);
                fill(false);
                GXInvalidateTexAll();
                GXInitTexObj(&obj, texture.data(), 4, 4, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
                GXLoadTexObj(&obj, GX_TEXMAP0);
                GXFlush();
                triangle(0.5f, true, false);
                GXCopyDisp(nullptr, GX_TRUE);
                aurora::gx::fifo::drain();
                pixels = finish_frame();
                check_scene(pixels, false, true);
                recorder.frame();
                const auto complete = recorder.checkpoint();
                // A partially recorded third frame must not enter the saved prefix.
                recorder.begin();
                check(aurora::gfx::begin_frame(), "partial Aurora frame failed");
                triangle(-0.5f, false, false);
                check(recorder.checkpoint() == complete, "partial frame changed completed prefix");
                replay::set_recorder(nullptr);
                aurora::gfx::abort_frame();
                save(argv[2], complete);
            } else {
                recorder.end();
                replay::set_recorder(nullptr);
                pixels = finish_frame();
                check_scene(pixels, copies, false, i4 || ia, lyt, iaClamp, ia4);
                save(argv[2], recorder.finish());
            }
        } else {
            // GXInit establishes Aurora's non-FIFO defaults. Discard its queued
            // bytes: the capture includes the original initialization stream.

            playback->run([&](replay::Kind kind, std::span<const std::uint8_t> bytes) {
                if (kind == replay::Kind::Begin) {
                    check(aurora::gfx::begin_frame(), "Aurora begin_frame failed");
                } else if (kind == replay::Kind::Fifo) {
                    aurora::gx::fifo::write_data(bytes.data(), bytes.size());
                    aurora::gx::fifo::drain();
                } else if (kind == replay::Kind::End || kind == replay::Kind::Frame) {
                    pixels = finish_frame();
                } else {
                    replay::apply_direct(kind, bytes);
                }
            });
            if (std::string(argv[1]).ends_with("check")) {
                check_scene(pixels, copies, sequence, i4 || ia, lyt, iaClamp, ia4);
            }
        }
        if (sequence)
            check(renderedFrames == 2, "replay lost frame boundaries");
        png(argv[3], pixels);
        outputPresenter.reset();
        aurora::gfx::shutdown();
        aurora::webgpu::shutdown();
        std::printf("PASS: %u completed Aurora frame(s), selected-XFB presentation, GPU readback and PNG output\n", renderedFrames);
        return 0;
    } catch (const std::exception& error) {
        replay::set_recorder(nullptr);
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        if (initialized) {
            outputPresenter.reset();
            aurora::gfx::shutdown();
            aurora::webgpu::shutdown();
        }
        return 1;
    }
}
