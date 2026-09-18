#include <switch.h>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_VI_NN
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sys/stat.h>
#include <vector>

u32 __nx_applet_type = AppletType_Application;
size_t __nx_heap_size = 0;

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance, const char* name);

namespace {

constexpr const char* kReportDir = "sdmc:/switch/WiiCompiled-Switch";
constexpr const char* kReportPath =
    "sdmc:/switch/WiiCompiled-Switch/m3-vulkan-clear-probe.txt";
constexpr std::uint32_t kFallbackWidth = 1280;
constexpr std::uint32_t kFallbackHeight = 720;

FILE* g_report = nullptr;

void report(const char* format, ...) {
    char buffer[768];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (g_report) {
        std::fprintf(g_report, "%s\n", buffer);
        std::fflush(g_report);
    }
}

template <typename T>
T load_global(const char* name) {
    return reinterpret_cast<T>(vk_icdGetInstanceProcAddr(VK_NULL_HANDLE, name));
}

template <typename T>
T load_instance(VkInstance instance, const char* name) {
    return reinterpret_cast<T>(vk_icdGetInstanceProcAddr(instance, name));
}

struct DeviceFunctions {
    PFN_vkDestroyDevice destroy_device = nullptr;
    PFN_vkGetDeviceQueue get_device_queue = nullptr;
    PFN_vkCreateSwapchainKHR create_swapchain = nullptr;
    PFN_vkDestroySwapchainKHR destroy_swapchain = nullptr;
    PFN_vkGetSwapchainImagesKHR get_swapchain_images = nullptr;
    PFN_vkAcquireNextImageKHR acquire_next_image = nullptr;
    PFN_vkQueuePresentKHR queue_present = nullptr;
    PFN_vkCreateCommandPool create_command_pool = nullptr;
    PFN_vkDestroyCommandPool destroy_command_pool = nullptr;
    PFN_vkAllocateCommandBuffers allocate_command_buffers = nullptr;
    PFN_vkResetCommandBuffer reset_command_buffer = nullptr;
    PFN_vkBeginCommandBuffer begin_command_buffer = nullptr;
    PFN_vkCmdPipelineBarrier cmd_pipeline_barrier = nullptr;
    PFN_vkCmdClearColorImage cmd_clear_color_image = nullptr;
    PFN_vkEndCommandBuffer end_command_buffer = nullptr;
    PFN_vkCreateSemaphore create_semaphore = nullptr;
    PFN_vkDestroySemaphore destroy_semaphore = nullptr;
    PFN_vkQueueSubmit queue_submit = nullptr;
    PFN_vkQueueWaitIdle queue_wait_idle = nullptr;
    PFN_vkDeviceWaitIdle device_wait_idle = nullptr;
};

struct Probe {
    PFN_vkCreateInstance create_instance = nullptr;
    PFN_vkDestroyInstance destroy_instance = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extensions = nullptr;
    PFN_vkEnumeratePhysicalDevices enumerate_physical_devices = nullptr;
    PFN_vkGetPhysicalDeviceProperties get_physical_device_properties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR get_surface_support = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR get_surface_capabilities = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR get_present_modes = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties enumerate_device_extensions = nullptr;
    PFN_vkCreateDevice create_device = nullptr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr = nullptr;
    PFN_vkCreateViSurfaceNN create_vi_surface = nullptr;
    PFN_vkDestroySurfaceKHR destroy_surface = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    std::uint32_t queue_family = std::numeric_limits<std::uint32_t>::max();
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_semaphore = VK_NULL_HANDLE;
    VkExtent2D extent{};
    VkFormat format = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> images;
    std::vector<VkCommandBuffer> command_buffers;
    DeviceFunctions d{};

    template <typename T>
    T load_device(const char* name) const {
        return reinterpret_cast<T>(get_device_proc_addr(device, name));
    }

    bool check_instance_extensions() {
        enumerate_instance_extensions =
            load_global<PFN_vkEnumerateInstanceExtensionProperties>(
                "vkEnumerateInstanceExtensionProperties");
        if (!enumerate_instance_extensions) {
            report("FAIL global: vkEnumerateInstanceExtensionProperties");
            return false;
        }

        std::uint32_t count = 0;
        if (enumerate_instance_extensions(nullptr, &count, nullptr) != VK_SUCCESS) {
            report("FAIL: enumerate instance extension count");
            return false;
        }

        std::vector<VkExtensionProperties> extensions(count);
        if (count != 0 &&
            enumerate_instance_extensions(nullptr, &count, extensions.data()) != VK_SUCCESS) {
            report("FAIL: enumerate instance extensions");
            return false;
        }

        bool have_surface = false;
        bool have_vi = false;
        for (const auto& extension : extensions) {
            have_surface |= std::strcmp(extension.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0;
            have_vi |= std::strcmp(extension.extensionName, VK_NN_VI_SURFACE_EXTENSION_NAME) == 0;
        }

        report("instance extensions: count=%u KHR_surface=%s NN_vi_surface=%s",
               count,
               have_surface ? "YES" : "NO",
               have_vi ? "YES" : "NO");
        return have_surface && have_vi;
    }

    bool init_instance() {
        create_instance = load_global<PFN_vkCreateInstance>("vkCreateInstance");
        if (!create_instance || !check_instance_extensions()) {
            report("FAIL stage INSTANCE_GLOBALS");
            return false;
        }

        const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_NN_VI_SURFACE_EXTENSION_NAME,
        };
        const VkApplicationInfo app_info{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "WiiCompiled-Switch M3 Vulkan Probe",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "WiiCompiled-Switch",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_API_VERSION_1_1,
        };
        const VkInstanceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 2,
            .ppEnabledExtensionNames = extensions,
        };

        const VkResult result = create_instance(&create_info, nullptr, &instance);
        report("vkCreateInstance -> %d", result);
        if (result != VK_SUCCESS) {
            return false;
        }

        destroy_instance = load_instance<PFN_vkDestroyInstance>(instance, "vkDestroyInstance");
        enumerate_physical_devices =
            load_instance<PFN_vkEnumeratePhysicalDevices>(instance, "vkEnumeratePhysicalDevices");
        get_physical_device_properties =
            load_instance<PFN_vkGetPhysicalDeviceProperties>(instance,
                                                             "vkGetPhysicalDeviceProperties");
        get_queue_family_properties =
            load_instance<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
                instance, "vkGetPhysicalDeviceQueueFamilyProperties");
        get_surface_support = load_instance<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
        get_surface_capabilities = load_instance<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        get_surface_formats = load_instance<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        get_present_modes = load_instance<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
        enumerate_device_extensions = load_instance<PFN_vkEnumerateDeviceExtensionProperties>(
            instance, "vkEnumerateDeviceExtensionProperties");
        create_device = load_instance<PFN_vkCreateDevice>(instance, "vkCreateDevice");
        get_device_proc_addr =
            load_instance<PFN_vkGetDeviceProcAddr>(instance, "vkGetDeviceProcAddr");
        create_vi_surface =
            load_instance<PFN_vkCreateViSurfaceNN>(instance, "vkCreateViSurfaceNN");
        destroy_surface =
            load_instance<PFN_vkDestroySurfaceKHR>(instance, "vkDestroySurfaceKHR");

        const bool complete = destroy_instance && enumerate_physical_devices &&
                              get_physical_device_properties && get_queue_family_properties &&
                              get_surface_support && get_surface_capabilities &&
                              get_surface_formats && get_present_modes &&
                              enumerate_device_extensions && create_device &&
                              get_device_proc_addr && create_vi_surface && destroy_surface;
        if (!complete) {
            report("FAIL stage INSTANCE_FUNCTIONS");
        }
        return complete;
    }

    bool init_surface() {
        NWindow* window = nwindowGetDefault();
        if (!window) {
            report("FAIL: nwindowGetDefault returned null");
            return false;
        }

        const VkViSurfaceCreateInfoNN create_info{
            .sType = VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN,
            .pNext = nullptr,
            .flags = 0,
            .window = window,
        };
        const VkResult result = create_vi_surface(instance, &create_info, nullptr, &surface);
        report("vkCreateViSurfaceNN -> %d", result);
        return result == VK_SUCCESS;
    }

    bool choose_device_and_queue() {
        std::uint32_t device_count = 0;
        VkResult result = enumerate_physical_devices(instance, &device_count, nullptr);
        if (result != VK_SUCCESS || device_count == 0) {
            report("FAIL enumerate physical devices -> %d count=%u", result, device_count);
            return false;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        result = enumerate_physical_devices(instance, &device_count, devices.data());
        if (result != VK_SUCCESS) {
            report("FAIL enumerate physical device list -> %d", result);
            return false;
        }

        for (VkPhysicalDevice candidate : devices) {
            VkPhysicalDeviceProperties properties{};
            get_physical_device_properties(candidate, &properties);

            std::uint32_t family_count = 0;
            get_queue_family_properties(candidate, &family_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(family_count);
            get_queue_family_properties(candidate, &family_count, families.data());

            for (std::uint32_t family = 0; family < family_count; ++family) {
                VkBool32 present = VK_FALSE;
                get_surface_support(candidate, family, surface, &present);
                if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && present) {
                    physical_device = candidate;
                    queue_family = family;
                    report("GPU: %s api=%u.%u.%u queueFamily=%u",
                           properties.deviceName,
                           VK_VERSION_MAJOR(properties.apiVersion),
                           VK_VERSION_MINOR(properties.apiVersion),
                           VK_VERSION_PATCH(properties.apiVersion),
                           queue_family);
                    return true;
                }
            }
        }

        report("FAIL: no graphics+present queue family");
        return false;
    }

    bool check_swapchain_extension() {
        std::uint32_t count = 0;
        VkResult result =
            enumerate_device_extensions(physical_device, nullptr, &count, nullptr);
        if (result != VK_SUCCESS) {
            report("FAIL enumerate device extension count -> %d", result);
            return false;
        }

        std::vector<VkExtensionProperties> extensions(count);
        result = enumerate_device_extensions(physical_device, nullptr, &count, extensions.data());
        if (result != VK_SUCCESS) {
            report("FAIL enumerate device extensions -> %d", result);
            return false;
        }

        const bool have_swapchain = std::any_of(
            extensions.begin(), extensions.end(), [](const VkExtensionProperties& extension) {
                return std::strcmp(extension.extensionName,
                                   VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
            });
        report("device extension KHR_swapchain=%s", have_swapchain ? "YES" : "NO");
        return have_swapchain;
    }

    bool init_device() {
        if (!choose_device_and_queue() || !check_swapchain_extension()) {
            return false;
        }

        constexpr float priority = 1.0f;
        const VkDeviceQueueCreateInfo queue_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };
        const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        const VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = extensions,
            .pEnabledFeatures = nullptr,
        };

        const VkResult result =
            create_device(physical_device, &create_info, nullptr, &device);
        report("vkCreateDevice -> %d", result);
        if (result != VK_SUCCESS) {
            return false;
        }

        d.destroy_device = load_device<PFN_vkDestroyDevice>("vkDestroyDevice");
        d.get_device_queue = load_device<PFN_vkGetDeviceQueue>("vkGetDeviceQueue");
        d.create_swapchain = load_device<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR");
        d.destroy_swapchain = load_device<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR");
        d.get_swapchain_images =
            load_device<PFN_vkGetSwapchainImagesKHR>("vkGetSwapchainImagesKHR");
        d.acquire_next_image =
            load_device<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR");
        d.queue_present = load_device<PFN_vkQueuePresentKHR>("vkQueuePresentKHR");
        d.create_command_pool =
            load_device<PFN_vkCreateCommandPool>("vkCreateCommandPool");
        d.destroy_command_pool =
            load_device<PFN_vkDestroyCommandPool>("vkDestroyCommandPool");
        d.allocate_command_buffers =
            load_device<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers");
        d.reset_command_buffer =
            load_device<PFN_vkResetCommandBuffer>("vkResetCommandBuffer");
        d.begin_command_buffer =
            load_device<PFN_vkBeginCommandBuffer>("vkBeginCommandBuffer");
        d.cmd_pipeline_barrier =
            load_device<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier");
        d.cmd_clear_color_image =
            load_device<PFN_vkCmdClearColorImage>("vkCmdClearColorImage");
        d.end_command_buffer = load_device<PFN_vkEndCommandBuffer>("vkEndCommandBuffer");
        d.create_semaphore = load_device<PFN_vkCreateSemaphore>("vkCreateSemaphore");
        d.destroy_semaphore = load_device<PFN_vkDestroySemaphore>("vkDestroySemaphore");
        d.queue_submit = load_device<PFN_vkQueueSubmit>("vkQueueSubmit");
        d.queue_wait_idle = load_device<PFN_vkQueueWaitIdle>("vkQueueWaitIdle");
        d.device_wait_idle = load_device<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle");

        const bool complete =
            d.destroy_device && d.get_device_queue && d.create_swapchain &&
            d.destroy_swapchain && d.get_swapchain_images && d.acquire_next_image &&
            d.queue_present && d.create_command_pool && d.destroy_command_pool &&
            d.allocate_command_buffers && d.reset_command_buffer &&
            d.begin_command_buffer && d.cmd_pipeline_barrier &&
            d.cmd_clear_color_image && d.end_command_buffer &&
            d.create_semaphore && d.destroy_semaphore && d.queue_submit &&
            d.queue_wait_idle && d.device_wait_idle;
        if (!complete) {
            report("FAIL stage DEVICE_FUNCTIONS");
            return false;
        }

        d.get_device_queue(device, queue_family, 0, &queue);
        return queue != VK_NULL_HANDLE;
    }

    VkCompositeAlphaFlagBitsKHR choose_composite_alpha(
        VkCompositeAlphaFlagsKHR supported) const {
        constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> options{
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (auto option : options) {
            if ((supported & option) != 0) {
                return option;
            }
        }
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    bool init_swapchain() {
        VkSurfaceCapabilitiesKHR capabilities{};
        VkResult result =
            get_surface_capabilities(physical_device, surface, &capabilities);
        if (result != VK_SUCCESS) {
            report("FAIL surface capabilities -> %d", result);
            return false;
        }
        if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0) {
            report("FAIL: surface images do not support TRANSFER_DST");
            return false;
        }

        std::uint32_t format_count = 0;
        result = get_surface_formats(physical_device, surface, &format_count, nullptr);
        if (result != VK_SUCCESS || format_count == 0) {
            report("FAIL surface formats -> %d count=%u", result, format_count);
            return false;
        }
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        result = get_surface_formats(physical_device, surface, &format_count, formats.data());
        if (result != VK_SUCCESS) {
            report("FAIL surface format list -> %d", result);
            return false;
        }

        VkSurfaceFormatKHR chosen = formats.front();
        if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED) {
            chosen.format = VK_FORMAT_B8G8R8A8_UNORM;
            chosen.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        }
        for (const auto& candidate : formats) {
            if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM ||
                candidate.format == VK_FORMAT_R8G8B8A8_UNORM) {
                chosen = candidate;
                break;
            }
        }
        format = chosen.format;

        std::uint32_t mode_count = 0;
        result = get_present_modes(physical_device, surface, &mode_count, nullptr);
        if (result != VK_SUCCESS || mode_count == 0) {
            report("FAIL present modes -> %d count=%u", result, mode_count);
            return false;
        }
        std::vector<VkPresentModeKHR> modes(mode_count);
        result = get_present_modes(physical_device, surface, &mode_count, modes.data());
        if (result != VK_SUCCESS) {
            report("FAIL present mode list -> %d", result);
            return false;
        }
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
        if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_FIFO_KHR) == modes.end()) {
            present_mode = modes.front();
        }

        if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            extent = capabilities.currentExtent;
        } else {
            extent.width = std::clamp(
                kFallbackWidth,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            extent.height = std::clamp(
                kFallbackHeight,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);
        }

        std::uint32_t image_count = std::max(capabilities.minImageCount + 1u, 2u);
        if (capabilities.maxImageCount != 0) {
            image_count = std::min(image_count, capabilities.maxImageCount);
        }

        const VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface,
            .minImageCount = image_count,
            .imageFormat = chosen.format,
            .imageColorSpace = chosen.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha =
                choose_composite_alpha(capabilities.supportedCompositeAlpha),
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };

        result = d.create_swapchain(device, &create_info, nullptr, &swapchain);
        report("vkCreateSwapchainKHR -> %d extent=%ux%u requestedImages=%u format=%d mode=%d",
               result,
               extent.width,
               extent.height,
               image_count,
               static_cast<int>(chosen.format),
               static_cast<int>(present_mode));
        if (result != VK_SUCCESS) {
            return false;
        }

        std::uint32_t actual_count = 0;
        result = d.get_swapchain_images(device, swapchain, &actual_count, nullptr);
        if (result != VK_SUCCESS || actual_count == 0) {
            report("FAIL swapchain image count -> %d count=%u", result, actual_count);
            return false;
        }
        images.resize(actual_count);
        result = d.get_swapchain_images(device, swapchain, &actual_count, images.data());
        report("swapchain images=%u result=%d", actual_count, result);
        return result == VK_SUCCESS;
    }

    bool init_commands() {
        const VkCommandPoolCreateInfo pool_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queue_family,
        };
        VkResult result =
            d.create_command_pool(device, &pool_info, nullptr, &command_pool);
        if (result != VK_SUCCESS) {
            report("FAIL command pool -> %d", result);
            return false;
        }

        command_buffers.resize(images.size());
        const VkCommandBufferAllocateInfo allocation_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount =
                static_cast<std::uint32_t>(command_buffers.size()),
        };
        result =
            d.allocate_command_buffers(device, &allocation_info, command_buffers.data());
        if (result != VK_SUCCESS) {
            report("FAIL command buffers -> %d", result);
            return false;
        }

        const VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        result =
            d.create_semaphore(device, &semaphore_info, nullptr, &acquire_semaphore);
        if (result != VK_SUCCESS) {
            report("FAIL acquire semaphore -> %d", result);
            return false;
        }
        result = d.create_semaphore(device, &semaphore_info, nullptr, &render_semaphore);
        if (result != VK_SUCCESS) {
            report("FAIL render semaphore -> %d", result);
            return false;
        }
        return true;
    }

    bool record_clear(std::uint32_t image_index, std::uint64_t frame) {
        VkCommandBuffer command_buffer = command_buffers[image_index];
        if (d.reset_command_buffer(command_buffer, 0) != VK_SUCCESS) {
            report("FAIL reset command buffer frame=%llu",
                   static_cast<unsigned long long>(frame));
            return false;
        }

        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        if (d.begin_command_buffer(command_buffer, &begin_info) != VK_SUCCESS) {
            report("FAIL begin command buffer frame=%llu",
                   static_cast<unsigned long long>(frame));
            return false;
        }

        const VkImageSubresourceRange range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        const VkImageMemoryBarrier to_transfer{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = images[image_index],
            .subresourceRange = range,
        };
        d.cmd_pipeline_barrier(command_buffer,
                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                               0,
                               0,
                               nullptr,
                               0,
                               nullptr,
                               1,
                               &to_transfer);

        const float phase =
            static_cast<float>(frame % 240u) / 239.0f;
        const VkClearColorValue color{
            .float32 = {
                0.12f + 0.75f * phase,
                0.18f + 0.55f * (1.0f - phase),
                0.85f - 0.65f * phase,
                1.0f,
            },
        };
        d.cmd_clear_color_image(command_buffer,
                                images[image_index],
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                &color,
                                1,
                                &range);

        const VkImageMemoryBarrier to_present{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = images[image_index],
            .subresourceRange = range,
        };
        d.cmd_pipeline_barrier(command_buffer,
                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                               0,
                               0,
                               nullptr,
                               0,
                               nullptr,
                               1,
                               &to_present);

        return d.end_command_buffer(command_buffer) == VK_SUCCESS;
    }

    bool run() {
        PadState pad{};
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);

        report("ENTER PRESENT LOOP -- press + to exit");
        std::uint64_t frame = 0;
        while (appletMainLoop()) {
            padUpdate(&pad);
            if ((padGetButtonsDown(&pad) & HidNpadButton_Plus) != 0) {
                report("user requested exit");
                break;
            }

            std::uint32_t image_index = 0;
            VkResult result = d.acquire_next_image(device,
                                                   swapchain,
                                                   UINT64_MAX,
                                                   acquire_semaphore,
                                                   VK_NULL_HANDLE,
                                                   &image_index);
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                report("FAIL acquire frame=%llu result=%d",
                       static_cast<unsigned long long>(frame),
                       result);
                return false;
            }
            if (image_index >= command_buffers.size()) {
                report("FAIL acquire invalid image index=%u", image_index);
                return false;
            }
            if (!record_clear(image_index, frame)) {
                return false;
            }

            const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            const VkCommandBuffer command_buffer = command_buffers[image_index];
            const VkSubmitInfo submit_info{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &acquire_semaphore,
                .pWaitDstStageMask = &wait_stage,
                .commandBufferCount = 1,
                .pCommandBuffers = &command_buffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &render_semaphore,
            };
            result = d.queue_submit(queue, 1, &submit_info, VK_NULL_HANDLE);
            if (result != VK_SUCCESS) {
                report("FAIL submit frame=%llu result=%d",
                       static_cast<unsigned long long>(frame),
                       result);
                return false;
            }

            const VkPresentInfoKHR present_info{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &render_semaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &image_index,
                .pResults = nullptr,
            };
            result = d.queue_present(queue, &present_info);
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                report("FAIL present frame=%llu result=%d",
                       static_cast<unsigned long long>(frame),
                       result);
                return false;
            }

            result = d.queue_wait_idle(queue);
            if (result != VK_SUCCESS) {
                report("FAIL queue idle frame=%llu result=%d",
                       static_cast<unsigned long long>(frame),
                       result);
                return false;
            }

            ++frame;
            if (frame == 1) {
                report("PASS FIRST_PRESENT");
            } else if ((frame % 120u) == 0u) {
                report("ACTIVE frames=%llu", static_cast<unsigned long long>(frame));
            }
        }

        report("PASS LOOP frames=%llu", static_cast<unsigned long long>(frame));
        return frame != 0;
    }

    void shutdown() {
        if (device != VK_NULL_HANDLE && d.device_wait_idle) {
            d.device_wait_idle(device);
        }
        if (device != VK_NULL_HANDLE && render_semaphore != VK_NULL_HANDLE &&
            d.destroy_semaphore) {
            d.destroy_semaphore(device, render_semaphore, nullptr);
            render_semaphore = VK_NULL_HANDLE;
        }
        if (device != VK_NULL_HANDLE && acquire_semaphore != VK_NULL_HANDLE &&
            d.destroy_semaphore) {
            d.destroy_semaphore(device, acquire_semaphore, nullptr);
            acquire_semaphore = VK_NULL_HANDLE;
        }
        if (device != VK_NULL_HANDLE && command_pool != VK_NULL_HANDLE &&
            d.destroy_command_pool) {
            d.destroy_command_pool(device, command_pool, nullptr);
            command_pool = VK_NULL_HANDLE;
        }
        if (device != VK_NULL_HANDLE && swapchain != VK_NULL_HANDLE &&
            d.destroy_swapchain) {
            d.destroy_swapchain(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }
        if (device != VK_NULL_HANDLE && d.destroy_device) {
            d.destroy_device(device, nullptr);
            device = VK_NULL_HANDLE;
        }
        if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE &&
            destroy_surface) {
            destroy_surface(instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
        if (instance != VK_NULL_HANDLE && destroy_instance) {
            destroy_instance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }
};

} // namespace

int main(int, char**) {
    const Result mount_result = fsdevMountSdmc();
    if (R_SUCCEEDED(mount_result)) {
        ::mkdir(kReportDir, 0777);
        g_report = std::fopen(kReportPath, "w");
    }

    report("WiiCompiled-Switch M3 Vulkan clear probe");
    report("mesa-switch pin: b297e230ef88c6c88df2561becf864f979f494a6");
    report("goal: VK_NN_vi_surface -> NVK swapchain -> clear -> present");

    setenv("NVK_I_WANT_A_BROKEN_VULKAN_DRIVER", "1", 1);
    setenv("MESA_SHADER_CACHE_DISABLE", "1", 1);

    Probe probe;
    const bool initialized = probe.init_instance() && probe.init_surface() &&
                             probe.init_device() && probe.init_swapchain() &&
                             probe.init_commands();
    bool passed = false;
    if (initialized) {
        passed = probe.run();
    } else {
        report("FAIL INITIALIZATION");
    }

    probe.shutdown();
    report("RESULT=%s", passed ? "PASS" : "FAIL");

    if (g_report) {
        std::fclose(g_report);
        g_report = nullptr;
    }
    if (R_SUCCEEDED(mount_result)) {
        fsdevUnmountDevice("sdmc");
    }
    return passed ? 0 : 1;
}
