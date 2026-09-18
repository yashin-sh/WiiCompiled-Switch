cmake_minimum_required(VERSION 3.20)

include(FetchContent)

if(NOT DEFINED M3_AURORA_ROOT OR NOT EXISTS "${M3_AURORA_ROOT}/lib/gx/gx.cpp")
    message(FATAL_ERROR "M3_AURORA_ROOT must point at pinned WiiCompiled/aurora-main")
endif()

if(NOT TARGET fmt)
    FetchContent_Declare(fmt
        URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.tar.gz
        URL_HASH SHA256=ac366b7b4c2e9f0dde63a59b3feb5ee59b67974b14ee5dc9ea8ad78aa2c1ee1e
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(fmt)
endif()

if(NOT TARGET xxhash)
    FetchContent_Declare(xxhash
        URL https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.3.tar.gz
        URL_HASH SHA256=aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR cmake_unofficial)
    set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(xxhash)
endif()

if(NOT TARGET TracyClient)
    set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(tracy
        URL https://github.com/wolfpld/tracy/archive/a64b9a20294d59421a2f57aeca3c6383d8c48169.tar.gz
        URL_HASH SHA256=24d342b5127d7f659dc3cf94f24347b348cc736f25b17a691fd7f69541937658
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(tracy)
endif()

set(A "${M3_AURORA_ROOT}")

add_library(m3_aurora_gx STATIC
    "${A}/lib/gfx/clear.cpp"
    "${A}/lib/gfx/common.cpp"
    "${A}/lib/gfx/depth_peek.cpp"
    "${A}/lib/gfx/efb_ram_copy.cpp"
    "${A}/lib/gfx/efb_ram_encoder.cpp"
    "${A}/lib/gfx/tex_copy_conv.cpp"
    "${A}/lib/gfx/tex_palette_conv.cpp"
    "${A}/lib/gfx/texture.cpp"
    "${A}/lib/gfx/texture_convert.cpp"
    "${A}/lib/gx/command_processor.cpp"
    "${A}/lib/gx/fifo.cpp"
    "${A}/lib/gx/frame_interpolation.cpp"
    "${A}/lib/gx/gx.cpp"
    "${A}/lib/gx/pipeline.cpp"
    "${A}/lib/gx/shader.cpp"
    "${A}/lib/gx/shader_info.cpp"
    "${A}/lib/dolphin/gx/GXBump.cpp"
    "${A}/lib/dolphin/gx/GXCull.cpp"
    "${A}/lib/dolphin/gx/GXCpu2Efb.cpp"
    "${A}/lib/dolphin/gx/GXDispList.cpp"
    "${A}/lib/dolphin/gx/GXDraw.cpp"
    "${A}/lib/dolphin/gx/GXExtra.cpp"
    "${A}/lib/dolphin/gx/GXFifo.cpp"
    "${A}/lib/dolphin/gx/GXFrameBuffer.cpp"
    "${A}/lib/dolphin/gx/GXGeometry.cpp"
    "${A}/lib/dolphin/gx/GXGet.cpp"
    "${A}/lib/dolphin/gx/GXLighting.cpp"
    "${A}/lib/dolphin/gx/GXManage.cpp"
    "${A}/lib/dolphin/gx/GXPixel.cpp"
    "${A}/lib/dolphin/gx/GXTev.cpp"
    "${A}/lib/dolphin/gx/GXTexture.cpp"
    "${A}/lib/dolphin/gx/GXTransform.cpp"
    "${A}/lib/dolphin/gx/GXVert.cpp"
    "${A}/lib/dolphin/gx/GXAurora.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/aurora_switch_shim.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/probe_pipeline_cache.cpp"
)

target_include_directories(m3_aurora_gx PUBLIC
    "${A}/include"
    "${A}/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

target_compile_features(m3_aurora_gx PUBLIC cxx_std_20)
target_compile_definitions(m3_aurora_gx PUBLIC
    AURORA
    AURORA_ENABLE_GX
    TARGET_PC
    WEBGPU_DAWN
    __SWITCH__
    NX
    VK_USE_PLATFORM_VI_NN
)

target_link_libraries(m3_aurora_gx PUBLIC
    webgpu_dawn
    fmt
    xxhash
    absl::btree
    absl::flat_hash_map
    TracyClient
)

target_compile_options(m3_aurora_gx PRIVATE
    -ffunction-sections
    -fdata-sections
)
