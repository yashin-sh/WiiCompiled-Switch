cmake_minimum_required(VERSION 3.20)
project(mkw_desktop_gx_replay LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static replay dependencies" FORCE)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(MKW_DAWN_ROOT "${REPO_ROOT}/.deps/m3/dawn-switch" CACHE PATH "Pinned dawn-switch source")
set(MKW_AURORA_ROOT "${REPO_ROOT}/third_party/WiiCompiled/aurora-main" CACHE PATH "Pinned Aurora source")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(ZLIB REQUIRED)

foreach(flag DAWN_BUILD_SAMPLES DAWN_BUILD_TESTS TINT_BUILD_TESTS TINT_BUILD_CMD_TOOLS
    DAWN_USE_X11 DAWN_USE_WAYLAND DAWN_USE_GLFW DAWN_ENABLE_OPENGLES DAWN_ENABLE_DESKTOP_GL
    DAWN_ENABLE_SPIRV_VALIDATION DAWN_ENABLE_D3D11 DAWN_ENABLE_D3D12 DAWN_ENABLE_METAL
    DAWN_PLATFORM_SWITCH DAWN_FETCH_DEPENDENCIES)
    set(${flag} OFF CACHE BOOL "" FORCE)
endforeach()
set(DAWN_ENABLE_VULKAN ON CACHE BOOL "" FORCE)
set(DAWN_ENABLE_NULL OFF CACHE BOOL "" FORCE)
set(DAWN_BUILD_MONOLITHIC_LIBRARY STATIC CACHE STRING "" FORCE)
add_subdirectory("${MKW_DAWN_ROOT}" dawn EXCLUDE_FROM_ALL)
# GCC 16 diagnoses the pinned generated Tint matcher lambdas as invalid
# constexpr, although they are used at runtime. Keep this compatibility option
# confined to the intrinsic-table targets; do not alter pinned source.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    foreach(intrinsic tint_lang_core_intrinsic tint_lang_wgsl_intrinsic tint_lang_spirv_intrinsic)
        if(TARGET ${intrinsic})
            target_compile_options(${intrinsic} PRIVATE -Wno-invalid-constexpr)
        endif()
    endforeach()
endif()

include(FetchContent)
FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.tar.gz
    URL_HASH SHA256=ac366b7b4c2e9f0dde63a59b3feb5ee59b67974b14ee5dc9ea8ad78aa2c1ee1e)
FetchContent_Declare(xxhash
    URL https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.3.tar.gz
    URL_HASH SHA256=aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80
    SOURCE_SUBDIR cmake_unofficial)
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(tracy
    URL https://github.com/wolfpld/tracy/archive/a64b9a20294d59421a2f57aeca3c6383d8c48169.tar.gz
    URL_HASH SHA256=24d342b5127d7f659dc3cf94f24347b348cc736f25b17a691fd7f69541937658)
FetchContent_MakeAvailable(fmt xxhash tracy)

set(A "${CMAKE_CURRENT_BINARY_DIR}/checked-aurora")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REPO_ROOT}/scripts/prepare-replay-aurora.py"
    "${MKW_AURORA_ROOT}" "${A}" "${MKW_DAWN_ROOT}"
    RESULT_VARIABLE prepare_result)
if(NOT prepare_result EQUAL 0)
    message(FATAL_ERROR "Cannot prepare pinned replay source mirror")
endif()
file(GLOB_RECURSE mirror_inputs CONFIGURE_DEPENDS "${MKW_AURORA_ROOT}/lib/*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${mirror_inputs}
    "${REPO_ROOT}/scripts/prepare-replay-aurora.py" "${REPO_ROOT}/scripts/prepare-checked-aurora-fifo.py")

add_library(replay_aurora STATIC
    "${A}/lib/gfx/clear.cpp" "${A}/lib/gfx/common.cpp"
    "${A}/lib/gfx/depth_peek.cpp" "${A}/lib/gfx/efb_ram_copy.cpp" "${A}/lib/gfx/efb_ram_encoder.cpp"
    "${A}/lib/gfx/tex_copy_conv.cpp" "${A}/lib/gfx/tex_palette_conv.cpp"
    "${A}/lib/gfx/texture.cpp" "${A}/lib/gfx/texture_convert.cpp"
    "${A}/lib/gx/command_processor.cpp" "${A}/lib/gx/fifo.cpp"
    "${A}/lib/gx/frame_interpolation.cpp" "${A}/lib/gx/gx.cpp"
    "${A}/lib/gx/pipeline.cpp" "${A}/lib/gx/shader.cpp" "${A}/lib/gx/shader_info.cpp"
    "${A}/lib/dolphin/gx/GXBump.cpp" "${A}/lib/dolphin/gx/GXCull.cpp"
    "${A}/lib/dolphin/gx/GXCpu2Efb.cpp" "${A}/lib/dolphin/gx/GXDispList.cpp"
    "${A}/lib/dolphin/gx/GXDraw.cpp" "${A}/lib/dolphin/gx/GXExtra.cpp"
    "${A}/lib/dolphin/gx/GXFifo.cpp" "${A}/lib/dolphin/gx/GXFrameBuffer.cpp"
    "${A}/lib/dolphin/gx/GXGeometry.cpp" "${A}/lib/dolphin/gx/GXGet.cpp"
    "${A}/lib/dolphin/gx/GXLighting.cpp" "${A}/lib/dolphin/gx/GXManage.cpp"
    "${A}/lib/dolphin/gx/GXPixel.cpp" "${A}/lib/dolphin/gx/GXTev.cpp"
    "${A}/lib/dolphin/gx/GXTexture.cpp" "${A}/lib/dolphin/gx/GXTransform.cpp"
    "${A}/lib/dolphin/gx/GXVert.cpp" "${A}/lib/dolphin/gx/GXAurora.cpp"
    aurora_capture.cpp shim.cpp "${REPO_ROOT}/m3-aurora-gx-probe/source/probe_pipeline_cache.cpp")
target_include_directories(replay_aurora PUBLIC "${A}/include" "${A}/lib"
    "${REPO_ROOT}/m3-aurora-gx-probe/compat")
target_compile_definitions(replay_aurora PUBLIC AURORA AURORA_ENABLE_GX TARGET_PC WEBGPU_DAWN)
target_link_libraries(replay_aurora PUBLIC webgpu_dawn fmt xxhash absl::btree absl::flat_hash_map)
get_target_property(tracy_includes TracyClient INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(replay_aurora PRIVATE ${tracy_includes})

add_library(replay_capture STATIC capture.cpp)
target_include_directories(replay_capture PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(mkw-gx-replay main.cpp)
target_link_libraries(mkw-gx-replay PRIVATE replay_aurora replay_capture ZLIB::ZLIB)
add_executable(replay-format-test format_test.cpp)
target_link_libraries(replay-format-test PRIVATE replay_capture)
enable_testing()
add_test(NAME replay-format COMMAND replay-format-test)
