cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED M3_AURORA_ROOT OR NOT EXISTS "${M3_AURORA_ROOT}/lib/gx/gx.cpp")
    message(FATAL_ERROR "M3_AURORA_ROOT must point at pinned WiiCompiled/aurora-main")
endif()

if(NOT DEFINED M3_REPO_ROOT OR
   NOT EXISTS "${M3_REPO_ROOT}/m3-aurora-gx-probe/CMakeLists.txt")
    message(FATAL_ERROR "M3_REPO_ROOT must point at WiiCompiled-Switch")
endif()

get_filename_component(M3_WIICOMPILED_ROOT "${M3_AURORA_ROOT}" DIRECTORY)

# Reuse the hardware-proven Aurora GX/Dawn integration unchanged, then add only
# the pinned runtime FIFO decoder and the narrow probe-only runtime seams it
# needs. This makes the delta from the successful #176 hardware path explicit.
add_subdirectory(
    "${M3_REPO_ROOT}/m3-aurora-gx-probe"
    "${CMAKE_CURRENT_BINARY_DIR}/aurora-gx-base"
)

target_sources(m3_aurora_gx PRIVATE
    "${M3_WIICOMPILED_ROOT}/runtime/src/hle/gx/gx_fifo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/runtime_fifo_shim.cpp"
)

target_include_directories(m3_aurora_gx BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    "${M3_WIICOMPILED_ROOT}/runtime/include"
    "${M3_WIICOMPILED_ROOT}/runtime/src"
    "${M3_WIICOMPILED_ROOT}/runtime/src/hle/gx"
)
