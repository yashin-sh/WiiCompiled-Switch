// Compile production Deko3D method bodies with deterministic CPU/GPU seams.
// ASan sees exact minimum-size WebGPU source allocations, without row padding
// after the last row. This does not emulate the Switch GPU or claim playability.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace wgpu {
enum class TextureDimension { e2D,
                              e3D };
}
enum class Aspect { Color,
                    Stencil };
enum { DkDsSource_Stencil = 7 };
using DkGpuAddr = uintptr_t;
struct Extent3D {
    uint32_t width, height, depthOrArrayLayers;
};
struct Origin3D {
    uint32_t x, y, z;
    Origin3D ToOrigin3D() const {
        return *this;
    }
};
struct TexelBlockInfo {
    uint32_t width, height, byteSize;
};
struct Format {
    TexelBlockInfo block;
    struct Info {
        TexelBlockInfo block;
    };
    Info GetAspectInfo(Aspect) const {
        return {block};
    }
};
struct MaybeError {
    bool error = false;
};
#define DAWN_TRY(expr)        \
    do {                      \
        auto result = (expr); \
        if (result.error)     \
            return result;    \
    } while (false)
struct SubresourceRange {};
struct Texture {
    wgpu::TextureDimension dimension;
    Format format;
    bool initialized = false;
    auto GetDimension() const {
        return dimension;
    }
    const Format& GetFormat() const {
        return format;
    }
    Texture* GetDkImage() {
        return this;
    }
    void SetIsSubresourceContentInitialized(bool value, SubresourceRange) {
        initialized = value;
    }
    MaybeError EnsureSubresourceContentInitialized(SubresourceRange) {
        initialized = true;
        return {};
    }
};
template <class T>
T* ToBackend(T* value) {
    return value;
}
struct TextureRef {
    Texture* value;
    TextureRef(Texture* texture)
        : value(texture) {}
    Texture* Get() const {
        return value;
    }
};
struct TextureCopy {
    TextureRef texture;
    uint32_t mipLevel;
    Origin3D origin;
    Aspect aspect;
};
struct TexelCopyBufferLayout {
    uint64_t offset;
    uint32_t bytesPerRow, rowsPerImage;
};
SubresourceRange GetSubresourcesAffectedByCopy(const TextureCopy&, const Extent3D&) {
    return {};
}
bool IsCompleteSubresourceCopiedTo(Texture*, const Extent3D&, uint32_t, Aspect) {
    return false;
}
struct DkImageView {
    uint8_t mipLevelOffset, mipLevelCount;
    uint16_t layerOffset, layerCount;
    int dsSource;
};
void dkImageViewDefaults(DkImageView* view, Texture*) {
    *view = {};
}
struct DkCopyBuf {
    DkGpuAddr addr;
    uint32_t rowLength, imageHeight;
};
struct DkImageRect {
    uint32_t x, y, z, width, height, depth;
};
struct Copy {
    DkCopyBuf src;
    DkImageView view;
    DkImageRect rect;
};
struct InFlight {
    std::vector<uint32_t> releasedImageDescriptors, releasedSamplerDescriptors;
};
struct Device {
    std::vector<std::vector<uint8_t>> uploads;
    std::vector<Copy> copies;
    bool allocationFails = false;
    bool mHasPendingCommands = false;
    std::vector<int> mInFlight;
    std::mutex mDescriptorMutex;
    std::vector<uint32_t> mFreeImageDescriptors, mFreeSamplerDescriptors;
    std::vector<uint32_t> mPendingReleasedImageDescriptors, mPendingReleasedSamplerDescriptors;
    MaybeError AllocateUploadMemory(uint64_t size, uint8_t** cpu, DkGpuAddr* gpu) {
        if (allocationFails)
            return {true};
        uploads.emplace_back(size, 0);
        *cpu = uploads.back().data();
        *gpu = reinterpret_cast<uintptr_t>(*cpu);
        return {};
    }
    Device* GetPendingCommandBuffer() {
        return this;
    }
    MaybeError RecordTextureUpload(const TextureCopy&, const uint8_t*, const TexelCopyBufferLayout&, const Extent3D&);
    void ReleaseImageDescriptor(uint32_t);
    void ReleaseSamplerDescriptor(uint32_t);
    void Retire(InFlight*);
    void Reclaim(InFlight*);
};
void dkCmdBufCopyBufferToImage(Device* device, const DkCopyBuf* src,
                               const DkImageView* view, const DkImageRect* rect, int) {
    device->copies.push_back({*src, *view, *rect});
}
struct BindGroupTrackerBase {
    const void* layout = nullptr;
    const void* appliedLayout = nullptr;
    bool AreLayoutsCompatible() {
        return layout == appliedLayout;
    }
};
struct Tracker : BindGroupTrackerBase {
    const void* mPipeline = nullptr;
    const void* mLastAppliedPipeline = nullptr;
    bool AreLayoutsCompatible();
};

#include "production.inc"

void TextureCase(bool volume, TexelBlockInfo block, Extent3D size, uint32_t rowPadding,
                 uint32_t imagePadding, uint32_t mip, Origin3D origin, Aspect aspect) {
    const uint64_t rowBytes = ((uint64_t(size.width) + block.width - 1) / block.width) * block.byteSize;
    const uint64_t rows = (uint64_t(size.height) + block.height - 1) / block.height;
    const uint32_t rowStride = rowBytes + rowPadding;
    const uint32_t imageRows = rows + imagePadding;
    const uint64_t imageStride = uint64_t(rowStride) * imageRows;
    const uint64_t offset = 13;
    const uint64_t minimum = offset + imageStride * (size.depthOrArrayLayers - 1) + rowStride * (rows - 1) + rowBytes;
    auto input = std::make_unique<uint8_t[]>(minimum);
    std::memset(input.get(), 0xe7, minimum);
    std::vector<uint8_t> expected;
    for (uint32_t image = 0; image < size.depthOrArrayLayers; ++image) {
        for (uint64_t row = 0; row < rows; ++row) {
            for (uint64_t byte = 0; byte < rowBytes; ++byte) {
                uint8_t value = (image * 37 + row * 11 + byte) % 251;
                input[offset + image * imageStride + row * rowStride + byte] = value;
                expected.push_back(value);
            }
        }
    }
    Texture texture{volume ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D, {block}};
    Device device;
    assert(!device.RecordTextureUpload({&texture, mip, origin, aspect}, input.get(),
                                       {offset, rowStride, imageRows}, size)
                .error);
    assert(device.copies.size() == (volume ? 1 : size.depthOrArrayLayers));
    std::vector<uint8_t> result;
    for (size_t i = 0; i < device.copies.size(); ++i) {
        const auto& copy = device.copies[i];
        assert(copy.src.rowLength == rowBytes);
        assert(copy.src.imageHeight == rowBytes * rows);
        assert(copy.rect.width == size.width && copy.rect.height == size.height);
        assert(copy.rect.depth == (volume ? size.depthOrArrayLayers : 1));
        assert(copy.rect.x == origin.x && copy.rect.y == origin.y);
        assert(copy.rect.z == (volume ? origin.z : 0));
        assert(copy.view.mipLevelOffset == mip && copy.view.mipLevelCount == 1);
        assert(copy.view.layerOffset == (volume ? 0 : origin.z + i));
        assert(copy.view.layerCount == (volume ? 0 : 1));
        assert(copy.view.dsSource == (aspect == Aspect::Stencil ? DkDsSource_Stencil : 0));
        result.insert(result.end(), device.uploads[i].begin(), device.uploads[i].end());
    }
    assert(result == expected);
    Device failed;
    failed.allocationFails = true;
    assert(failed.RecordTextureUpload({&texture, mip, origin, aspect}, input.get(),
                                      {offset, rowStride, imageRows}, size)
               .error);
    assert(failed.copies.empty());
}

int main() {
    TextureCase(false, {1, 1, 4}, {2, 3, 1}, 0, 0, 0, {0, 0, 0}, Aspect::Color);
    TextureCase(false, {1, 1, 4}, {2, 3, 1}, 248, 5, 2, {3, 4, 0}, Aspect::Color);
    TextureCase(false, {1, 1, 4}, {3, 2, 4}, 244, 3, 1, {0, 0, 2}, Aspect::Color);
    TextureCase(true, {1, 1, 4}, {2, 3, 3}, 248, 4, 1, {1, 2, 3}, Aspect::Color);
    TextureCase(false, {4, 4, 8}, {8, 8, 2}, 240, 2, 3, {4, 4, 1}, Aspect::Color);
    TextureCase(false, {4, 4, 8}, {1, 1, 1}, 248, 0, 5, {0, 0, 0}, Aspect::Color);
    TextureCase(false, {1, 1, 1}, {3, 2, 1}, 253, 0, 0, {0, 0, 0}, Aspect::Stencil);
    Device device;
    device.ReleaseImageDescriptor(1);
    device.ReleaseSamplerDescriptor(2);
    assert(device.mFreeImageDescriptors == std::vector<uint32_t>{1});
    assert(device.mFreeSamplerDescriptors == std::vector<uint32_t>{2});
    device.mHasPendingCommands = true;
    device.ReleaseImageDescriptor(3);
    device.ReleaseSamplerDescriptor(4);
    assert(device.mFreeImageDescriptors == std::vector<uint32_t>{1});
    assert(device.mFreeSamplerDescriptors == std::vector<uint32_t>{2});
    InFlight submission;
    device.Retire(&submission);
    assert(submission.releasedImageDescriptors == std::vector<uint32_t>{3});
    assert(submission.releasedSamplerDescriptors == std::vector<uint32_t>{4});
    assert(device.mPendingReleasedImageDescriptors.empty());
    device.mHasPendingCommands = false;
    device.mInFlight.push_back(1);
    device.ReleaseImageDescriptor(5);
    device.ReleaseSamplerDescriptor(6);
    device.Reclaim(&submission); // Called only after the GPU fence completes.
    assert((device.mFreeImageDescriptors == std::vector<uint32_t>{1, 3}));
    assert((device.mFreeSamplerDescriptors == std::vector<uint32_t>{2, 4}));
    assert(device.mPendingReleasedImageDescriptors == std::vector<uint32_t>{5});
    assert(device.mPendingReleasedSamplerDescriptors == std::vector<uint32_t>{6});
    int shaderA, shaderB, layoutA, layoutB;
    Tracker tracker;
    tracker.layout = tracker.appliedLayout = &layoutA;
    tracker.mPipeline = tracker.mLastAppliedPipeline = &shaderA;
    assert(tracker.AreLayoutsCompatible());
    tracker.mPipeline = &shaderB; // Same layout, different reflected combined texture units.
    assert(!tracker.AreLayoutsCompatible());
    tracker.mLastAppliedPipeline = &shaderB;
    assert(tracker.AreLayoutsCompatible());
    tracker.layout = &layoutB;
    assert(!tracker.AreLayoutsCompatible());
    std::cout << "texture rows/layers/BC/stencil/allocation failure; descriptor retirement; shader rebinding OK\n";
}
