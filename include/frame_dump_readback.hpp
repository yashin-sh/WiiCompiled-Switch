#pragma once
#include "frame_dump_image.hpp"
#include "dawn/webgpu_cpp.h"

namespace mkw::frame_dump {
class Readback {
    wgpu::Device device_;
    wgpu::Buffer buffer_;
    Layout layout_{};
    bool bgra_ = false, scopeOpen_ = false;

  public:
    Readback() = default;
    Readback(const Readback&) = delete;
    Readback& operator=(const Readback&) = delete;
    ~Readback();
    void encode(wgpu::Device device, wgpu::CommandEncoder encoder, wgpu::Texture source);
    Image finish(wgpu::Instance instance);
};
} // namespace mkw::frame_dump
