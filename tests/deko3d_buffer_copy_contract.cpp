// Compile the complete production CopyBufferToBuffer command case. The GPU
// producer is deferred until submission, so recording-time CPU snapshots see
// old bytes. This reproduces Depth Peek's compute -> copy -> readback sequence.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

namespace wgpu {
enum class BufferUsage { CopySrc = 1,
                         CopyDst = 2,
                         MapWrite = 4,
                         Storage = 8,
                         QueryResolve = 16 };
constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<int>(a) | static_cast<int>(b));
}
} // namespace wgpu
enum DkBarrier { DkBarrier_Primitives,
                 DkBarrier_Full };
enum { DkInvalidateFlags_Shader = 2,
       DkInvalidateFlags_L2Cache = 16 };
using DkGpuAddr = uintptr_t;
constexpr bool kDeko3dTrace = false;
struct MaybeError {
    bool error = false;
};
#define DAWN_TRY(expr)        \
    do {                      \
        auto result = (expr); \
        if (result.error)     \
            return result;    \
    } while (false)
struct Buffer {
    wgpu::BufferUsage usage;
    std::vector<uint8_t> cpu, gpu;
    int cpuReads = 0, tracked = 0;
    bool failInitialization = false;
    Buffer(wgpu::BufferUsage flags, size_t size)
        : usage(flags), cpu(size, 0), gpu(size, 0) {}
    auto GetUsage() const {
        return usage;
    }
    uint8_t* GetCpuAddr() {
        ++cpuReads;
        return cpu.data();
    }
    DkGpuAddr GetGpuAddr() {
        return reinterpret_cast<uintptr_t>(gpu.data());
    }
    MaybeError EnsureDataInitialized() {
        return {failInitialization};
    }
    MaybeError EnsureDataInitializedAsDestination(uint64_t, uint64_t) {
        return {failInitialization};
    }
    void TrackUsage() {
        ++tracked;
    }
};
template <class T>
T* ToBackend(T* value) {
    return value;
}
struct BufferRef {
    Buffer* value;
    Buffer* Get() const {
        return value;
    }
};
struct CopyBufferToBufferCmd {
    BufferRef source, destination;
    uint64_t sourceOffset, destinationOffset, size;
};
enum class Command { CopyBufferToBuffer };
struct Iterator {
    CopyBufferToBufferCmd* command;
    template <class T>
    T* NextCommand() {
        return command;
    }
};
struct Device {
    std::vector<std::function<void()>> gpuCommands;
    std::vector<int> kinds;
    void RecordBufferWrite(DkGpuAddr address, const uint8_t* data, uint64_t size) {
        // Real inline/upload writes capture CPU bytes now and execute later.
        std::vector<uint8_t> snapshot(data, data + size);
        kinds.push_back(0);
        gpuCommands.push_back([address, snapshot] {
            std::memcpy(reinterpret_cast<void*>(address), snapshot.data(), snapshot.size());
        });
    }
    void TraceFlush(const char*) {}
    void Submit() {
        for (auto& work : gpuCommands)
            work();
    }
};
void dkCmdBufBarrier(Device* device, DkBarrier mode, uint32_t flags) {
    if (mode == DkBarrier_Primitives) {
        assert(flags == DkInvalidateFlags_L2Cache);
        device->kinds.push_back(1);
    } else {
        assert(flags == (DkInvalidateFlags_Shader | DkInvalidateFlags_L2Cache));
        device->kinds.push_back(3);
    }
    device->gpuCommands.push_back([] {});
}
void dkCmdBufCopyBuffer(Device* device, DkGpuAddr source, DkGpuAddr destination, uint32_t size) {
    device->kinds.push_back(2);
    device->gpuCommands.push_back([source, destination, size] {
        std::memcpy(reinterpret_cast<void*>(destination), reinterpret_cast<void*>(source), size);
    });
}

#include "buffer-copy-production.inc"

bool Case(wgpu::BufferUsage usage, size_t length, bool gpuProducer) {
    Buffer source(usage, length + 32), destination(wgpu::BufferUsage::CopyDst, length + 48);
    const uint64_t srcOffset = 12, dstOffset = 20;
    std::vector<uint8_t> expected(length);
    for (size_t i = 0; i < length; ++i)
        expected[i] = (i * 17 + 3) % 251;
    std::fill(destination.gpu.begin(), destination.gpu.end(), 0xe7);
    Device device;
    if (gpuProducer) {
        device.gpuCommands.push_back([&] { std::copy(expected.begin(), expected.end(), source.gpu.begin() + srcOffset); });
    } else {
        std::copy(expected.begin(), expected.end(), source.cpu.begin() + srcOffset);
    }
    CopyBufferToBufferCmd command{{&source}, {&destination}, srcOffset, dstOffset, length};
    assert(!RecordCopy(&device, &command).error);
    if (length == 0) {
        assert(device.kinds.empty() && source.tracked == 0 && destination.tracked == 0);
        return true;
    }
    device.Submit();
    if (!std::equal(expected.begin(), expected.end(), destination.gpu.begin() + dstOffset)) {
        std::cerr << "stale GPU-produced buffer copied before its compute producer executes\n";
        return false;
    }
    assert(std::all_of(destination.gpu.begin(), destination.gpu.begin() + dstOffset, [](uint8_t v) { return v == 0xe7; }));
    assert(std::all_of(destination.gpu.begin() + dstOffset + length, destination.gpu.end(), [](uint8_t v) { return v == 0xe7; }));
    assert(source.tracked == 1 && destination.tracked == 1);
    if (gpuProducer) {
        assert((device.kinds == std::vector<int>{1, 2, 3}));
        assert(source.cpuReads == 0);
    } else {
        assert((device.kinds == std::vector<int>{0}));
        assert(source.cpuReads == 1);
    }
    return true;
}

int main() {
    // The baseline fails here before assertions about barrier shape/CPU reads.
    if (!Case(wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc, 1024, true))
        return 1;
    for (size_t length : {size_t(0), size_t(4), size_t(64), size_t(65536)}) {
        assert(Case(wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc, length, true));
        assert(Case(wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc, length, true));
        assert(Case(wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc, length, true));
        assert(Case(wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst, length, true));
        assert(Case(wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc, length, false));
    }
    Buffer source(wgpu::BufferUsage::CopySrc, 64), destination(wgpu::BufferUsage::CopyDst, 64);
    CopyBufferToBufferCmd command{{&source}, {&destination}, 0, 0, 4};
    Device failed;
    source.failInitialization = true;
    assert(RecordCopy(&failed, &command).error && failed.kinds.empty());
    source.failInitialization = false;
    destination.failInitialization = true;
    assert(RecordCopy(&failed, &command).error && failed.kinds.empty());
    std::cout << "deferred compute/query/write -> GPU copy; staging fast path; offsets/zero/error contracts OK\n";
}
