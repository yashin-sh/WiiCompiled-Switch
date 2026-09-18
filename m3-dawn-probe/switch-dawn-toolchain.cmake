set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(DAWN_SWITCH ON CACHE BOOL "Build Dawn for Nintendo Switch/Horizon" FORCE)

if(NOT DEFINED ENV{DEVKITPRO})
  message(FATAL_ERROR "DEVKITPRO is not set")
endif()

set(DEVKITPRO "$ENV{DEVKITPRO}")
set(CMAKE_C_COMPILER   "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++")
set(CMAKE_ASM_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc")
set(CMAKE_AR           "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ar")
set(CMAKE_RANLIB       "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ranlib")

set(_switch_common "-D__SWITCH__ -D_GNU_SOURCE -I${DEVKITPRO}/libnx/include -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC")
set(CMAKE_C_FLAGS_INIT "${_switch_common}")
set(CMAKE_CXX_FLAGS_INIT "${_switch_common}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=${DEVKITPRO}/libnx/switch.specs -L${DEVKITPRO}/libnx/lib -pthread")

set(THREADS_PREFER_PTHREAD_FLAG ON)
