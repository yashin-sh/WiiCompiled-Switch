// Compile the actual texture-copy command cases. The render/compute engine
// retains a deferred producer and dirty data while the transfer engine can read
// memory. A full barrier must finish/flush the producer before any transfer.
// This models the explicit Deko3D ordering contract, not physical Tegra timing.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
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
namespace wgpu {
enum class TextureDimension { e2D,
                              e3D };
}
struct Origin3D {
    uint32_t x, y, z;
    Origin3D ToOrigin3D() const {
        return *this;
    }
};
struct Extent3D {
    uint32_t width, height, depthOrArrayLayers;
    bool IsEmpty() const {
        return !width || !height || !depthOrArrayLayers;
    }
    Extent3D ToExtent3D() const {
        return *this;
    }
};
struct SubresourceRange {};
struct Texture {
    wgpu::TextureDimension dimension;
    std::vector<uint32_t> memory, dirty;
    bool initialized = false, fail = false;
    Texture(wgpu::TextureDimension dim)
        : dimension(dim), memory(4 * 4 * 5, 0xe7), dirty(memory) {}
    auto GetDimension() const {
        return dimension;
    }
    MaybeError EnsureSubresourceContentInitialized(const SubresourceRange&) {
        return {fail};
    }
    void SetIsSubresourceContentInitialized(bool value, const SubresourceRange&) {
        initialized = value;
    }
    size_t Index(uint32_t x, uint32_t y, uint32_t z) const {
        return (z * 4 + y) * 4 + x;
    }
};
struct Buffer {
    std::vector<uint8_t> memory = std::vector<uint8_t>(1024, 0xe7);
    int tracked = 0;
    bool fail = false;
    template <class T>
    MaybeError EnsureDataInitializedAsDestination(T*) {
        return {fail};
    }
    uintptr_t GetGpuAddr() {
        return reinterpret_cast<uintptr_t>(memory.data());
    }
    void TrackUsage() {
        ++tracked;
    }
};
template <class T>
struct Ref {
    T* value;
    T* Get() const {
        return value;
    }
};
template <class T>
T* ToBackend(T* value) {
    return value;
}
template <class T, class V>
T dchecked_cast(V value) {
    return static_cast<T>(value);
}
struct TextureCopy {
    Ref<Texture> texture;
    uint32_t mipLevel = 0;
    Origin3D origin{0, 0, 0};
    int aspect = 0;
};
struct BufferCopy {
    Ref<Buffer> buffer;
    uint64_t offset = 0, blocksPerRow = 0, rowsPerImage = 0;
};
struct CopyTextureToBufferCmd {
    TextureCopy source;
    BufferCopy destination;
    Extent3D copySize;
};
struct CopyTextureToTextureCmd {
    TextureCopy source, destination;
    Extent3D copySize;
};
struct TypedTexelBlockInfo {
    uint64_t ToBytes(uint64_t blocks) const {
        return blocks * 4;
    }
};
const TypedTexelBlockInfo& GetBlockInfo(const TextureCopy&) {
    static TypedTexelBlockInfo value;
    return value;
}
SubresourceRange GetSubresourcesAffectedByCopy(const TextureCopy&, Extent3D) {
    return {};
}
Extent3D ComputeTextureCopyExtent(const TextureCopy&, Extent3D size) {
    return size;
}
bool IsCompleteSubresourceCopiedTo(Texture*, Extent3D size, uint32_t, int) {
    return size.width == 4 && size.height == 4;
}
struct DkImageView {
    Texture* texture;
    uint32_t layer;
};
struct DkImageRect {
    uint32_t x, y, z, width, height, depth;
};
struct DkCopyBuf {
    uintptr_t addr;
    uint32_t rowLength, imageHeight;
};
DkImageView SingleSubresourceView(Texture* texture, uint32_t, uint32_t layer, int) {
    return {texture, texture->dimension == wgpu::TextureDimension::e3D ? 0 : layer};
}
enum DkBarrier { DkBarrier_Full };
enum { DkInvalidateFlags_L2Cache = 16 };
struct Device {
    std::vector<std::function<void()>> commands, producers;
    std::vector<Texture*> dirtyImages;
    std::vector<int> order;
    void Submit() {
        for (auto& command : commands)
            command();
    }
};
void dkCmdBufBarrier(Device* device, DkBarrier mode, uint32_t flags) {
    assert(mode == DkBarrier_Full && flags == DkInvalidateFlags_L2Cache);
    device->order.push_back(1);
    device->commands.push_back([device] {
        for (auto& producer : device->producers)
            producer();
        device->producers.clear();
        for (auto* texture : device->dirtyImages)
            texture->memory = texture->dirty;
    });
}
void dkCmdBufCopyImageToBuffer(Device* device, const DkImageView* view, const DkImageRect* rect,
                               const DkCopyBuf* dst, uint32_t flags) {
    assert(flags == 0);
    device->order.push_back(2);
    device->commands.push_back([v = *view, r = *rect, d = *dst] {
        for (uint32_t z = 0; z < r.depth; ++z)
            for (uint32_t y = 0; y < r.height; ++y)
                for (uint32_t x = 0; x < r.width; ++x) {
                    auto value = v.texture->memory[v.texture->Index(r.x + x, r.y + y, v.layer + r.z + z)];
                    std::memcpy(reinterpret_cast<void*>(d.addr + z * d.imageHeight + y * d.rowLength + x * 4), &value, 4);
                }
    });
}
void dkCmdBufCopyImage(Device* device, const DkImageView* src, const DkImageRect* srcRect,
                       const DkImageView* dst, const DkImageRect* dstRect, uint32_t flags) {
    assert(flags == 0);
    device->order.push_back(2);
    device->commands.push_back([s = *src, sr = *srcRect, d = *dst, dr = *dstRect] {
        for (uint32_t z = 0; z < sr.depth; ++z)
            for (uint32_t y = 0; y < sr.height; ++y)
                for (uint32_t x = 0; x < sr.width; ++x)
                    d.texture->memory[d.texture->Index(dr.x + x, dr.y + y, d.layer + dr.z + z)] =
                        s.texture->memory[s.texture->Index(sr.x + x, sr.y + y, s.layer + sr.z + z)];
    });
}
enum class Command { CopyTextureToBuffer,
                     CopyTextureToTexture };
struct Iterator {
    CopyTextureToBufferCmd* buffer;
    CopyTextureToTextureCmd* texture;
    template <class T>
    T* NextCommand() {
        if constexpr (std::is_same_v<T, CopyTextureToBufferCmd>)
            return buffer;
        else
            return texture;
    }
};
struct Recorder {
    Iterator mCommands;
    MaybeError RecordCommands(Command type, Device* cmdBuf);
};
#include "texture-copy-production.inc"

bool Case(Command type, wgpu::TextureDimension dim, uint32_t layers, bool whole) {
    Texture src(dim), dst(dim);
    Buffer buffer;
    Device device;
    // The engine's producer is pending and its writes stay dirty until the barrier.
    device.producers.push_back([&] {
        for (size_t i = 0; i < src.dirty.size(); ++i)
            src.dirty[i] = 1000 + i;
    });
    device.dirtyImages.push_back(&src);
    uint32_t side = whole ? 4 : 2;
    uint32_t xy = whole ? 0 : 1;
    TextureCopy source{{&src}, 0, {xy, xy, 1}, 0}, destination{{&dst}, 0, {xy, xy, 0}, 0};
    Extent3D size{side, side, layers};
    CopyTextureToBufferCmd b{source, {{&buffer}, 16, 8, 5}, size};
    CopyTextureToTextureCmd t{source, destination, size};
    Recorder recorder{{&b, &t}};
    assert(!recorder.RecordCommands(type, &device).error);
    device.Submit();
    std::vector<bool> touched(buffer.memory.size(), false);
    for (uint32_t z = 0; z < layers; ++z) {
        for (uint32_t y = 0; y < side; ++y) {
            for (uint32_t x = 0; x < side; ++x) {
                uint32_t expected = 1000 + src.Index(xy + x, xy + y, 1 + z), value;
                if (type == Command::CopyTextureToTexture)
                    value = dst.memory[dst.Index(xy + x, xy + y, z)];
                else {
                    size_t offset = 16 + z * 160 + y * 32 + x * 4;
                    std::memcpy(&value, buffer.memory.data() + offset, 4);
                    std::fill(touched.begin() + offset, touched.begin() + offset + 4, true);
                }
                if (value != expected) {
                    std::cerr << "transfer raced its GPU producer or read dirty cached bytes\n";
                    return false;
                }
            }
        }
    }
    assert(device.order.front() == 1);
    assert(std::count(device.order.begin(), device.order.end(), 1) == 1);
    assert(std::count(device.order.begin(), device.order.end(), 2) == int(dim == wgpu::TextureDimension::e3D ? 1 : layers));
    if (type == Command::CopyTextureToBuffer) {
        assert(buffer.tracked == 1);
        for (size_t i = 0; i < touched.size(); ++i)
            if (!touched[i])
                assert(buffer.memory[i] == 0xe7);
    } else
        assert(dst.initialized == whole);
    return true;
}
int main() {
    // The baseline fails on the actual output before barrier-shape assertions.
    if (!Case(Command::CopyTextureToTexture, wgpu::TextureDimension::e2D, 1, false))
        return 1;
    for (Command type : {Command::CopyTextureToTexture, Command::CopyTextureToBuffer}) {
        for (auto dim : {wgpu::TextureDimension::e2D, wgpu::TextureDimension::e3D})
            for (uint32_t layers : {1, 3})
                for (bool whole : {false, true})
                    assert(Case(type, dim, layers, whole));
        Texture src(wgpu::TextureDimension::e2D), dst(wgpu::TextureDimension::e2D);
        Buffer buffer;
        CopyTextureToBufferCmd b{{{&src}}, {{&buffer}, 0, 8, 5}, {0, 2, 1}};
        CopyTextureToTextureCmd t{{{&src}}, {{&dst}}, {0, 2, 1}};
        Recorder recorder{{&b, &t}};
        Device device;
        assert(!recorder.RecordCommands(type, &device).error && device.commands.empty());
        b.copySize.width = t.copySize.width = 2;
        src.fail = true;
        assert(recorder.RecordCommands(type, &device).error && device.commands.empty());
        src.fail = false;
        dst.fail = buffer.fail = true;
        assert(recorder.RecordCommands(type, &device).error && device.commands.empty());
    }
    std::cout << "render/compute -> image transfer ordering; layers/3D/offsets/padding/zero/error contracts OK\n";
}
