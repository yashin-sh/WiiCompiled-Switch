#include "gfx/clear.hpp"
#include "gfx/pipeline_cache.hpp"
#include "gx/pipeline.hpp"
#include "internal.hpp"

#include <absl/container/flat_hash_map.h>

#include <atomic>
#include <mutex>
#include <utility>

namespace aurora::gfx {
namespace {

std::mutex g_probePipelineMutex;
absl::flat_hash_map<PipelineRef, wgpu::RenderPipeline> g_probePipelines;
std::atomic_bool g_skipUnready{false};

template <typename Config>
PipelineRef create_or_find(ShaderType type,
                           const Config& config,
                           NewPipelineCallback&& create) {
    const PipelineRef ref = xxh3_hash(config, static_cast<HashType>(type));
    std::scoped_lock lock(g_probePipelineMutex);
    if (!g_probePipelines.contains(ref)) {
        g_probePipelines.emplace(ref, create());
    }
    return ref;
}

} // namespace

template <>
PipelineRef find_pipeline(ShaderType type,
                          const clear::PipelineConfig& config,
                          NewPipelineCallback&& create) {
    return create_or_find(type, config, std::move(create));
}

template <>
PipelineRef find_pipeline(ShaderType type,
                          const gx::PipelineConfig& config,
                          NewPipelineCallback&& create) {
    return create_or_find(type, config, std::move(create));
}

void initialize_pipeline_cache() {
    std::scoped_lock lock(g_probePipelineMutex);
    g_probePipelines.clear();
}

void shutdown_pipeline_cache() {
    std::scoped_lock lock(g_probePipelineMutex);
    g_probePipelines.clear();
}

void begin_pipeline_frame() {}
void end_pipeline_frame() {}

void set_skip_unready_pipelines(bool enabled) noexcept {
    g_skipUnready.store(enabled, std::memory_order_relaxed);
}

bool skip_unready_pipelines() noexcept {
    return g_skipUnready.load(std::memory_order_relaxed);
}

uint32_t queued_pipeline_count() noexcept {
    return 0;
}

bool try_pipeline(PipelineRef ref, wgpu::RenderPipeline& pipeline) {
    std::scoped_lock lock(g_probePipelineMutex);
    const auto it = g_probePipelines.find(ref);
    if (it == g_probePipelines.end()) {
        return false;
    }
    pipeline = it->second;
    return true;
}

bool wait_pipeline(PipelineRef ref, wgpu::RenderPipeline& pipeline) {
    return try_pipeline(ref, pipeline);
}

bool wait_pipeline_for_persistent_pass(PipelineRef ref,
                                       wgpu::RenderPipeline& pipeline) {
    return try_pipeline(ref, pipeline);
}

} // namespace aurora::gfx
