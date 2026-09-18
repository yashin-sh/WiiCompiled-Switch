import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: patch-m3-dawn-switch.py <dawn-source>")

root = Path(sys.argv[1])


def replace(path: str, old: str, new: str) -> None:
    file = root / path
    text = file.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one patch site, found {count}")
    file.write_text(text.replace(old, new), encoding="utf-8")


# The source snippets below intentionally preserve the exact quoting/layout of
# pinned Dawn so every replacement remains byte-for-byte auditable.
# fmt: off
replace(
    "src/utils/platform.h",
    "#elif defined(__linux__)\n#define DAWN_PLATFORM_IS_LINUX 1\n#define DAWN_PLATFORM_IS_POSIX 1\n",
    "#elif defined(__SWITCH__)\n"
    "#define DAWN_PLATFORM_IS_SWITCH 1\n"
    "#define DAWN_PLATFORM_IS_POSIX 1\n"
    "\n"
    "#elif defined(__linux__)\n"
    "#define DAWN_PLATFORM_IS_LINUX 1\n"
    "#define DAWN_PLATFORM_IS_POSIX 1\n",
)

replace(
    "src/utils/platform.h",
    "#if !defined(DAWN_PLATFORM_IS_POSIX)\n#define DAWN_PLATFORM_IS_POSIX 0\n#endif\n",
    "#if !defined(DAWN_PLATFORM_IS_POSIX)\n"
    "#define DAWN_PLATFORM_IS_POSIX 0\n"
    "#endif\n"
    "#if !defined(DAWN_PLATFORM_IS_SWITCH)\n"
    "#define DAWN_PLATFORM_IS_SWITCH 0\n"
    "#endif\n",
)

replace(
    "src/dawn/common/SystemUtils.cpp",
    "#elif DAWN_PLATFORM_IS(FUCHSIA)\nstd::optional<std::string> GetExecutablePath() {\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "std::optional<std::string> GetExecutablePath() {\n"
    "    return {};\n"
    "}\n"
    "#elif DAWN_PLATFORM_IS(FUCHSIA)\n"
    "std::optional<std::string> GetExecutablePath() {\n",
)

replace(
    "src/dawn/common/SystemUtils.cpp",
    "#elif DAWN_PLATFORM_IS(FUCHSIA)\nstd::optional<std::string> GetModulePath() {\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "std::optional<std::string> GetModulePath() {\n"
    "    return {};\n"
    "}\n"
    "#elif DAWN_PLATFORM_IS(FUCHSIA)\n"
    "std::optional<std::string> GetModulePath() {\n",
)

replace(
    "src/dawn/native/vulkan/BackendVk.cpp",
    "#if DAWN_PLATFORM_IS(LINUX)\n#if DAWN_PLATFORM_IS(ANDROID)\n",
    "#if DAWN_PLATFORM_IS(SWITCH)\n"
    "constexpr char kVulkanLibName[] = \"loaderless-static-nvk\";\n"
    "#elif DAWN_PLATFORM_IS(LINUX)\n"
    "#if DAWN_PLATFORM_IS(ANDROID)\n",
)

replace(
    "src/dawn/native/vulkan/BackendVk.cpp",
    "        case ICD::None: {\n            DAWN_TRY(LoadVulkan(kVulkanLibName));\n            // Succesfully loaded driver; break.\n            break;\n        }\n",
    "        case ICD::None: {\n"
    "#if !DAWN_PLATFORM_IS(SWITCH)\n"
    "            DAWN_TRY(LoadVulkan(kVulkanLibName));\n"
    "#endif\n"
    "            // Switch resolves Vulkan directly from the statically linked mesa-switch ICD.\n"
    "            break;\n"
    "        }\n",
)

replace(
    "src/dawn/native/vulkan/VulkanFunctions.cpp",
    "MaybeError VulkanFunctions::LoadGlobalProcs(const DynamicLib& vulkanLib) {\n"
    "    if (!vulkanLib.GetProc(&GetInstanceProcAddr, \"vkGetInstanceProcAddr\")) {\n"
    "        return DAWN_INTERNAL_ERROR(\"Couldn't get vkGetInstanceProcAddr\");\n"
    "    }\n",
    "#if DAWN_PLATFORM_IS(SWITCH)\n"
    "extern \"C\" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL\n"
    "vk_icdGetInstanceProcAddr(VkInstance instance, const char* name);\n"
    "#endif\n"
    "\n"
    "MaybeError VulkanFunctions::LoadGlobalProcs(const DynamicLib& vulkanLib) {\n"
    "#if DAWN_PLATFORM_IS(SWITCH)\n"
    "    (void)vulkanLib;\n"
    "    GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(vk_icdGetInstanceProcAddr);\n"
    "#else\n"
    "    if (!vulkanLib.GetProc(&GetInstanceProcAddr, \"vkGetInstanceProcAddr\")) {\n"
    "        return DAWN_INTERNAL_ERROR(\"Couldn't get vkGetInstanceProcAddr\");\n"
    "    }\n"
    "#endif\n",
)


replace(
    "src/dawn/common/SystemUtils.cpp",
    "#include <cstdlib>\n#elif DAWN_PLATFORM_IS(MACOS) || DAWN_PLATFORM_IS(IOS)\n",
    "#include <cstdlib>\n"
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "#include <cstdlib>\n"
    "#elif DAWN_PLATFORM_IS(MACOS) || DAWN_PLATFORM_IS(IOS)\n",
)

replace(
    "src/dawn/common/DynamicLib.h",
    "#if DAWN_PLATFORM_IS(WINDOWS)\n"
    "#include \"partition_alloc/pointers/raw_ptr.h\"\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "#include \"partition_alloc/pointers/raw_ptr_exclusion.h\"\n",
    "#if DAWN_PLATFORM_IS(WINDOWS)\n"
    "#include \"partition_alloc/pointers/raw_ptr.h\"\n"
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "#include \"partition_alloc/pointers/raw_ptr_exclusion.h\"\n",
)

replace(
    "src/dawn/common/DynamicLib.h",
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    // On POSIX we use `dlopen`, which returns a \"handle\" which may not be a real pointer:\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "    void* mHandle = nullptr;\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    // On POSIX we use `dlopen`, which returns a \"handle\" which may not be a real pointer:\n",
)

replace(
    "src/dawn/common/DynamicLib.cpp",
    "#elif DAWN_PLATFORM_IS(POSIX)\n#include <dlfcn.h>\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "#include <dlfcn.h>\n",
)

replace(
    "src/dawn/common/DynamicLib.cpp",
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    mHandle = dlopen(filename.c_str(), RTLD_NOW);\n"
    "\n"
    "    if (mHandle == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "    (void)filename;\n"
    "    if (error != nullptr) {\n"
    "        *error = \"Dynamic loading is unavailable on Horizon\";\n"
    "    }\n"
    "    mHandle = nullptr;\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    mHandle = dlopen(filename.c_str(), RTLD_NOW);\n"
    "\n"
    "    if (mHandle == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
)

replace(
    "src/dawn/common/DynamicLib.cpp",
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    mHandle = dlopen(filename.c_str(), RTLD_NOW | RTLD_NOLOAD);\n"
    "\n"
    "    if (mHandle == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "    (void)filename;\n"
    "    if (error != nullptr) {\n"
    "        *error = \"Dynamic loading is unavailable on Horizon\";\n"
    "    }\n"
    "    mHandle = nullptr;\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    mHandle = dlopen(filename.c_str(), RTLD_NOW | RTLD_NOLOAD);\n"
    "\n"
    "    if (mHandle == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
)

replace(
    "src/dawn/common/DynamicLib.cpp",
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "        dlclose(mHandle);\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "        // No dynamic loader on Horizon.\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "        dlclose(mHandle);\n",
)

replace(
    "src/dawn/common/DynamicLib.cpp",
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    proc = reinterpret_cast<void*>(dlsym(mHandle, procName.c_str()));\n"
    "\n"
    "    if (proc == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
    "#elif DAWN_PLATFORM_IS(SWITCH)\n"
    "    (void)procName;\n"
    "    if (error != nullptr) {\n"
    "        *error = \"Dynamic symbol lookup is unavailable on Horizon\";\n"
    "    }\n"
    "#elif DAWN_PLATFORM_IS(POSIX)\n"
    "    proc = reinterpret_cast<void*>(dlsym(mHandle, procName.c_str()));\n"
    "\n"
    "    if (proc == nullptr && error != nullptr) {\n"
    "        *error = dlerror();\n"
    "    }\n",
)

replace(
    "src/dawn/native/CMakeLists.txt",
    "    elseif (UNIX AND NOT APPLE)\n",
    "    elseif (UNIX AND NOT APPLE AND NOT DAWN_SWITCH)\n",
)

# fmt: on

print("patched Dawn for loaderless mesa-switch/NVK")
