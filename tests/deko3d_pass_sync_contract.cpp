// Compile the complete production pass-begin cases. Cache entries intentionally
// survive writes until an explicit barrier invalidates them. This verifies the
// visibility contract for draw/copy -> sampled EFB/compute reads; it does not
// emulate Tegra's caches or substitute for hardware validation.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>

struct MaybeError {
    bool error = false;
};
#define DAWN_TRY(expr)        \
    do {                      \
        auto result = (expr); \
        if (result.error)     \
            return result;    \
    } while (false)
enum class Command { BeginComputePass,
                     BeginRenderPass };
enum DkBarrier { DkBarrier_Full };
enum { DkInvalidateFlags_Image = 1,
       DkInvalidateFlags_Shader = 2,
       DkInvalidateFlags_L2Cache = 16 };
struct SyncScopeResourceUsage {};
struct SubresourceRange {};
struct TextureBase {
    MaybeError EnsureSubresourceContentInitialized(const SubresourceRange&) {
        return {};
    }
};
template <class T>
T* ToBackend(T* value) {
    return value;
}
struct BeginComputePassCmd {};
struct BeginRenderPassCmd {};
struct Iterator {
    BeginComputePassCmd compute;
    BeginRenderPassCmd render;
    template <class T>
    T* NextCommand() {
        if constexpr (std::is_same_v<T, BeginComputePassCmd>)
            return &compute;
        else
            return &render;
    }
};
struct Device {
    int imageData = 0, storageData = 0;
    int imageCache = -7, shaderCache = -11;
    int sampled = -1, storageRead = -1;
    std::vector<int> order;
    std::vector<std::function<void()>> work;
    void Submit() {
        for (auto& command : work)
            command();
    }
};
void dkCmdBufBarrier(Device* device, DkBarrier mode, uint32_t flags) {
    assert(mode == DkBarrier_Full);
    assert(flags == (DkInvalidateFlags_Image | DkInvalidateFlags_Shader | DkInvalidateFlags_L2Cache));
    device->order.push_back(2);
    device->work.push_back([device] {
        device->imageCache = device->imageData;
        device->shaderCache = device->storageData;
    });
}
struct ComputeUsage {
    std::vector<SyncScopeResourceUsage> dispatchUsages{{}};
};
struct Usages {
    std::vector<ComputeUsage> computePasses{{}};
    std::vector<SyncScopeResourceUsage> renderPasses{{}};
};
template <class Callback>
MaybeError LazyClearRenderPassAttachments(Device* device, BeginRenderPassCmd*, Callback callback) {
    TextureBase texture;
    DAWN_TRY(callback(&texture, SubresourceRange{}));
    device->order.push_back(1);
    device->work.push_back([device] { device->imageData += 3; });
    return {};
}
struct Recorder {
    Device* device;
    Iterator mCommands;
    Usages usages;
    uint32_t nextComputePassNumber = 0, nextRenderPassNumber = 0;
    bool failInitialization = false;
    const Usages& GetResourceUsages() {
        return usages;
    }
    Device* GetDevice() {
        return device;
    }
    MaybeError LazyClearSyncScope(const SyncScopeResourceUsage&) {
        device->order.push_back(0);
        return {failInitialization};
    }
    MaybeError Consume(Device* cmdBuf) {
        assert(cmdBuf == device);
        device->order.push_back(3);
        device->work.push_back([this] {
            device->sampled = device->imageCache;
            device->storageRead = device->shaderCache;
        });
        return {};
    }
    MaybeError RecordComputePass(Device* cmdBuf) {
        return Consume(cmdBuf);
    }
    MaybeError RecordRenderPass(Device* cmdBuf, BeginRenderPassCmd*, uint32_t index) {
        assert(index == nextRenderPassNumber);
        return Consume(cmdBuf);
    }
    MaybeError BeginPass(Command type, Device* cmdBuf);
};

#include "pass-sync-production.inc"

bool Case(Command type, int value) {
    Device device;
    Recorder recorder{&device, {}, {}};
    device.work.push_back([&] { device.imageData = value; device.storageData = value * 2; });
    assert(!recorder.BeginPass(type, &device).error);
    device.Submit();
    const bool render = type == Command::BeginRenderPass;
    if (device.sampled != value + (render ? 3 : 0) || device.storageRead != value * 2) {
        std::cerr << "stale sampled render target or shader cache across pass boundary\n";
        return false;
    }
    assert(device.order == (render ? std::vector<int>{0, 1, 2, 3} : std::vector<int>{0, 2, 3}));
    assert(recorder.nextRenderPassNumber == unsigned(render));
    assert(recorder.nextComputePassNumber == unsigned(!render));
    return true;
}
int main() {
    // Baseline must fail on observed data, before assertions about barrier shape.
    if (!Case(Command::BeginRenderPass, 42))
        return 1;
    for (int value : {0, 1, 37, 255, 1024}) {
        assert(Case(Command::BeginRenderPass, value));
        assert(Case(Command::BeginComputePass, value));
    }
    for (Command type : {Command::BeginRenderPass, Command::BeginComputePass}) {
        Device device;
        Recorder recorder{&device, {}, {}};
        recorder.failInitialization = true;
        assert(recorder.BeginPass(type, &device).error);
        assert(device.work.empty() && device.order == std::vector<int>{0});
    }
    std::cout << "pass visibility after producers and lazy clears; render/compute/error contracts OK\n";
}
