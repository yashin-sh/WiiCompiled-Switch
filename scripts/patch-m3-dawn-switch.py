#!/usr/bin/env python3
from pathlib import Path
import sys

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

print("patched Dawn for loaderless mesa-switch/NVK")
