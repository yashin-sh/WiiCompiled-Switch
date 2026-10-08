#pragma once
#include "dawn/webgpu_cpp.h"

namespace mkw::presentation {
// Present Aurora's selected display copy, never the EFB cleared after copying.
class Presenter {
    wgpu::Device device_;
    wgpu::RenderPipeline pipeline_;
    wgpu::Sampler sampler_;
    wgpu::TextureFormat format_;

  public:
    Presenter(wgpu::Device device, wgpu::TextureFormat destinationFormat);
    void encode(wgpu::CommandEncoder encoder, wgpu::Texture source, wgpu::Texture destination) const;
};
} // namespace mkw::presentation
