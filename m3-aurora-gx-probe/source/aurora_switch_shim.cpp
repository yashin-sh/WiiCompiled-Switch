#include <switch.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "gfx/texture_replacement.hpp"
#include "internal.hpp"
#include "webgpu/gpu.hpp"
#include "window.hpp"
#include "dolphin/vi/vi_internal.hpp"

extern "C" void m3_probe_log(const char* message);

namespace aurora {

AuroraConfig g_config{};
uint32_t g_sdlCustomEventsStart = 0;
char g_gameName[4] = {'M', '3', 'G', 'X'};

void wait_for_frame_worker() noexcept {}

std::chrono::nanoseconds wait_for_frame_worker_sealed() noexcept {
    return std::chrono::nanoseconds::zero();
}

bool wait_for_frame_worker_for(std::chrono::microseconds) noexcept {
    return true;
}

std::recursive_mutex& renderer_gpu_mutex() noexcept {
    static std::recursive_mutex mutex;
    return mutex;
}

void log_internal(AuroraLogLevel level,
                  const char* module,
                  const char* message,
                  unsigned int len) noexcept {
    char buffer[1024];
    const int written =
        std::snprintf(buffer,
                      sizeof(buffer),
                      "Aurora[%d] %s: %.*s\n",
                      static_cast<int>(level),
                      module ? module : "unknown",
                      static_cast<int>(std::min<unsigned int>(len, 800u)),
                      message ? message : "");
    if (written > 0) {
        m3_probe_log(buffer);
    }
}

void Module::show_fatal_dialog(const char*, std::string_view) noexcept {}

} // namespace aurora

auto fmt::formatter<AuroraLogLevel>::format(AuroraLogLevel level,
                                             format_context& ctx) const
    -> format_context::iterator {
    return fmt::format_to(ctx.out(), "{}", static_cast<int>(level));
}

namespace aurora::window {

AuroraWindowSize get_window_size() {
    return {
        .width = 1280,
        .height = 720,
        .fb_width = 1280,
        .fb_height = 720,
        .native_fb_width = 1280,
        .native_fb_height = 720,
        .scale = 1.0f,
    };
}

void set_frame_buffer_aspect_fit(bool) {}
void set_present_surface_fill(bool) {}

} // namespace aurora::window

namespace aurora::vi {

Vec2<uint32_t> configured_fb_size() noexcept {
    return {1280, 720};
}

Vec2<uint32_t> visible_fb_size() noexcept {
    return {1280, 720};
}

float present_aspect_correction() noexcept {
    return 1.0f;
}

void configure(const GXRenderModeObj*) noexcept {}

} // namespace aurora::vi

namespace aurora::gfx::texture_replacement {

void initialize() noexcept {}
void shutdown() noexcept {}
void register_tlut(const GXTlutObj*, const void*, GXTlutFmt, uint16_t) noexcept {}
void load_tlut(const GXTlutObj*, uint32_t) noexcept {}

std::optional<TextureHandle> find_replacement(const GXTexObj_&) noexcept {
    return std::nullopt;
}

std::string build_texture_replacement_name(const GXTexObj_&) noexcept {
    return {};
}

} // namespace aurora::gfx::texture_replacement

namespace aurora::webgpu {

wgpu::Device g_device;
wgpu::Queue g_queue;
wgpu::Surface g_surface;
wgpu::BackendType g_backendType = wgpu::BackendType::Undefined;
GraphicsConfig g_graphicsConfig{};
TextureWithSampler g_frameBuffer{};
TextureWithSampler g_frameBufferResolved{};
TextureWithSampler g_depthBuffer{};
wgpu::RenderPipeline g_CopyPipeline;
wgpu::BindGroup g_CopyBindGroup;
wgpu::Instance g_instance;
bool g_bcTexturesSupported = false;

namespace {
PresentSource g_presentOverride{};
bool g_hasPresentOverride = false;
}

bool initialize(AuroraBackend) {
    return static_cast<bool>(g_device);
}

void shutdown() {
    clear_present_source_override();
    g_CopyBindGroup = {};
    g_CopyPipeline = {};
    g_frameBuffer = {};
    g_frameBufferResolved = {};
    g_depthBuffer = {};
    g_queue = {};
    g_surface = {};
    g_device = {};
    g_instance = {};
    g_backendType = wgpu::BackendType::Undefined;
}

void fail_if_device_lost() noexcept {}

void release_surface() noexcept {
    if (g_surface) {
        g_surface.Unconfigure();
    }
    g_surface = {};
}

bool refresh_surface(bool) {
    return static_cast<bool>(g_surface);
}

void resize_swapchain(uint32_t,
                      uint32_t,
                      uint32_t,
                      uint32_t,
                      bool) {}

TextureWithSampler create_render_texture(uint32_t width,
                                         uint32_t height,
                                         bool multisampled) {
    const uint32_t sampleCount =
        multisampled ? std::max(g_graphicsConfig.msaaSamples, 1u) : 1u;
    const wgpu::TextureDescriptor descriptor{
        .label = "M3 Aurora render texture",
        .usage = wgpu::TextureUsage::RenderAttachment |
                 wgpu::TextureUsage::TextureBinding |
                 wgpu::TextureUsage::CopySrc |
                 wgpu::TextureUsage::CopyDst,
        .size = {width, height, 1},
        .format = g_graphicsConfig.surfaceConfiguration.format,
        .sampleCount = sampleCount,
    };
    TextureWithSampler result{};
    result.texture = g_device.CreateTexture(&descriptor);
    result.view = result.texture.CreateView();
    result.size = {width, height, 1};
    result.format = descriptor.format;
    if (!multisampled) {
        result.sampler = g_device.CreateSampler();
    }
    return result;
}

const TextureWithSampler& present_source() noexcept {
    if (g_graphicsConfig.msaaSamples > 1 && g_frameBufferResolved.texture) {
        return g_frameBufferResolved;
    }
    return g_frameBuffer;
}

PresentSource current_present_source() noexcept {
    if (g_hasPresentOverride) {
        return g_presentOverride;
    }
    const auto& source = present_source();
    return {
        .bindGroup = g_CopyBindGroup,
        .texture = source.texture,
        .size = source.size,
        .format = source.format,
    };
}

void set_present_source_override(wgpu::BindGroup bindGroup,
                                 wgpu::Texture texture,
                                 wgpu::Extent3D size,
                                 wgpu::TextureFormat format) noexcept {
    g_presentOverride = {
        .bindGroup = std::move(bindGroup),
        .texture = std::move(texture),
        .size = size,
        .format = format,
    };
    g_hasPresentOverride = true;
}

void clear_present_source_override() noexcept {
    g_presentOverride = {};
    g_hasPresentOverride = false;
}

wgpu::BindGroup create_copy_bind_group(const TextureWithSampler&) {
    return {};
}

wgpu::BindGroup create_copy_bind_group(wgpu::TextureView, wgpu::Sampler) {
    return {};
}

Viewport calculate_present_viewport(uint32_t surfaceWidth,
                                    uint32_t surfaceHeight,
                                    uint32_t,
                                    uint32_t) noexcept {
    return {
        .left = 0.0f,
        .top = 0.0f,
        .width = static_cast<float>(surfaceWidth),
        .height = static_cast<float>(surfaceHeight),
        .znear = 0.0f,
        .zfar = 1.0f,
    };
}

Viewport calculate_present_viewport_for_aspect(uint32_t surfaceWidth,
                                               uint32_t surfaceHeight,
                                               float) noexcept {
    return calculate_present_viewport(surfaceWidth, surfaceHeight, surfaceWidth, surfaceHeight);
}

void draw_clear(const wgpu::RenderPassEncoder&,
                bool,
                bool,
                bool,
                const Vec4<float>&,
                float) {}

size_t load_from_cache(void const*, size_t, void*, size_t, void*) {
    return 0;
}

void store_to_cache(void const*, size_t, void const*, size_t, void*) {}

BlobCacheStats blob_cache_stats() noexcept {
    return {};
}

void serialize_pipeline_caches() noexcept {}

void cache_shutdown() {}

} // namespace aurora::webgpu
