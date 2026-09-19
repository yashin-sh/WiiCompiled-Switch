cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED M3_REPO_ROOT OR NOT EXISTS "${M3_REPO_ROOT}/source/main.cpp")
    message(FATAL_ERROR "M3_REPO_ROOT must point at WiiCompiled-Switch")
endif()

if(NOT DEFINED M3_AURORA_ROOT OR NOT EXISTS "${M3_AURORA_ROOT}/lib/gx/gx.cpp")
    message(FATAL_ERROR "M3_AURORA_ROOT must point at pinned WiiCompiled/aurora-main")
endif()

if(NOT DEFINED DAWN_SWITCH_NVK_LIBRARY OR NOT EXISTS "${DAWN_SWITCH_NVK_LIBRARY}")
    message(FATAL_ERROR "DAWN_SWITCH_NVK_LIBRARY must point at the proven static NVK Vulkan archive")
endif()

get_filename_component(M3_WIICOMPILED_ROOT "${M3_AURORA_ROOT}" DIRECTORY)

# Reuse the exact Aurora GX/GFX + Dawn seams that already passed on hardware.
# The normal entry path already created this target through m3-hle-fifo-probe;
# keep this CMake usable standalone as well.
if(NOT TARGET m3_aurora_gx)
    add_subdirectory(
        "${M3_REPO_ROOT}/m3-aurora-gx-probe"
        "${CMAKE_CURRENT_BINARY_DIR}/aurora-gx-base"
    )
endif()

enable_language(ASM)

file(GLOB M3_FAST_TRACK_SOURCE_CPP CONFIGURE_DEPENDS
    "${M3_REPO_ROOT}/source/*.cpp"
    "${M3_REPO_ROOT}/third_party/WiiCompiled/runtime/src/platform/*.cpp"
    "${M3_REPO_ROOT}/local-product-support/*.cpp"
    "${M3_REPO_ROOT}/local-execution-support/*.cpp"
    "${M3_REPO_ROOT}/local-product/generated/*.cpp"
    "${M3_REPO_ROOT}/local-product/generated/build_shards/base_common/*.cpp"
    "${M3_REPO_ROOT}/local-product/generated/build_shards/base_dispatch/*.cpp"
    "${M3_REPO_ROOT}/local-product/generated/build_shards/base_portable_sensitive/*.cpp"
)

# The stable baseline keeps this sink. The rendered variant substitutes the
# hardware-proven pinned FIFO decoder and renderer bridge instead.
list(REMOVE_ITEM M3_FAST_TRACK_SOURCE_CPP
    "${M3_REPO_ROOT}/source/gx_fast_track_fifo_bridge.cpp"
)

file(GLOB M3_FAST_TRACK_SOURCE_ASM CONFIGURE_DEPENDS
    "${M3_REPO_ROOT}/local-product/generated/*.S"
)

add_executable(mkw_switch_rendered_fast_track
    ${M3_FAST_TRACK_SOURCE_CPP}
    ${M3_FAST_TRACK_SOURCE_ASM}
    # Hand-written AArch64 coroutine switch (mkw_switch_co_init/switch);
    # the SOURCES glob above only covers .cpp/.S.
    "${M3_REPO_ROOT}/source/context_switch.s"
    "${M3_WIICOMPILED_ROOT}/runtime/src/hle/gx/gx_fifo.cpp"
)

target_include_directories(mkw_switch_rendered_fast_track PRIVATE
    # Probe-style seams first: hle_stubs.h here shadows the desktop
    # runtime's parallel ABI world (see seams/hle_stubs.h).
    "${M3_REPO_ROOT}/local-rendered-fast-track/seams"
    "${M3_REPO_ROOT}/include"
    "${M3_REPO_ROOT}/local-product/generated"
    "${M3_WIICOMPILED_ROOT}/runtime/include"
    "${M3_WIICOMPILED_ROOT}/runtime/src"
    "${M3_WIICOMPILED_ROOT}/runtime/src/hle/gx"
    # Header-only toml11 used by runtime_config.h (mirrors mkw::toml11).
    "${M3_WIICOMPILED_ROOT}/runtime/third_party/toml11"
)

target_compile_features(mkw_switch_rendered_fast_track PRIVATE cxx_std_20)

target_compile_definitions(mkw_switch_rendered_fast_track PRIVATE
    __SWITCH__
    NX
    VK_USE_PLATFORM_VI_NN
    MKW_PLATFORM_SWITCH=1
    MKW_LOCAL_PRODUCT=1
    MKW_LOCAL_FUNCTION_EXECUTION=1
    MKW_LOCAL_FAST_TRACK=1
    MKW_LOCAL_RENDERED_FAST_TRACK=1
    MKW_ENABLE_DATA_INIT_HANDOFF=1
    MKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1
)

target_compile_options(mkw_switch_rendered_fast_track PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-O2>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-fast-math>
    $<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-tree-slp-vectorize>
    $<$<COMPILE_LANGUAGE:CXX>:-include>
    $<$<COMPILE_LANGUAGE:CXX>:${M3_REPO_ROOT}/include/devkita64_gcc_compat.hpp>
)

target_link_libraries(mkw_switch_rendered_fast_track PRIVATE
    -Wl,--start-group
    m3_aurora_gx
    webgpu_dawn
    "${DAWN_SWITCH_NVK_LIBRARY}"
    ${DAWN_SWITCH_EXTRA_LIBRARIES}
    nx
    m
    -Wl,--end-group
)

target_link_options(mkw_switch_rendered_fast_track PRIVATE
    -Wl,--gc-sections
    -Wl,--allow-multiple-definition
    -Wl,-u,func_800060A4
    -Wl,-u,mkw_switch_report_unsupported_translated_dispatch
    -Wl,-u,__libnx_exception_handler
    -Wl,-u,vk_icdGetInstanceProcAddr
    -Wl,-u,vk_icdNegotiateLoaderICDInterfaceVersion
)

find_program(M3_ELF2NRO elf2nro REQUIRED)
find_program(M3_NACPTOOL nacptool REQUIRED)

set(M3_NACP "${CMAKE_CURRENT_BINARY_DIR}/WiiCompiled-Switch-local-rendered-fast-track.nacp")
set(M3_NRO "${CMAKE_CURRENT_BINARY_DIR}/WiiCompiled-Switch-local-rendered-fast-track.nro")

add_custom_command(
    OUTPUT "${M3_NACP}"
    COMMAND "${M3_NACPTOOL}" --create
            "WiiCompiled-Switch Rendered Fast Track"
            "WiiCompiled-Switch"
            "0.0.11-rendered-fast-track"
            "${M3_NACP}"
    VERBATIM
)

add_custom_command(
    OUTPUT "${M3_NRO}"
    COMMAND "${M3_ELF2NRO}"
            "$<TARGET_FILE:mkw_switch_rendered_fast_track>"
            "${M3_NRO}"
            --nacp=${M3_NACP}
    DEPENDS mkw_switch_rendered_fast_track "${M3_NACP}"
    VERBATIM
)

add_custom_target(mkw_switch_rendered_fast_track_nro
    DEPENDS "${M3_NRO}"
)
