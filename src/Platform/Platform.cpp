//
// Created by magnus on 10/3/20.
//

#include <set>
#include "Platform.h"

Platform::Platform(GLFWwindow *window, ArEngine *_arEngine) {
    createInstance();
    setDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createSwapchainImageViews();
    createCommandPool();
    *_arEngine = arEngine;
}


void Platform::createInstance() {
    if (!Validation::checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    // Generic Vulkan app info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello SmartMirror";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "AR_ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;


    auto extensions = Validation::getRequiredExtensions();

    //Check instance extensions supported...
    if (!Validation::checkInstanceExtensionSupport(&extensions))
        throw std::runtime_error("vkInstance does not support required extensions");

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        Validation::populateDebugMessengerCreateInfo(debugCreateInfo);
        //createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &arEngine.instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");


}

void Platform::createSurface(GLFWwindow *window) {
    if (glfwCreateWindowSurface(arEngine.instance, window, nullptr, &arEngine.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");

    }

}


void Platform::setDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    Validation::populateDebugMessengerCreateInfo(createInfo);

    if (Validation::CreateDebugUtilsMessengerEXT(arEngine.instance, &createInfo, nullptr, &arEngine.debugMessenger) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Platform::cleanUp() {

    for (auto imageView : arEngine.swapChainImageViews) {
        vkDestroyImageView(arEngine.mainDevice.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(arEngine.mainDevice.device, arEngine.swapchain, nullptr);

    vkDestroyCommandPool(arEngine.mainDevice.device, arEngine.commandPool, nullptr);

    vkDestroyDevice(arEngine.mainDevice.device, nullptr);
    if (enableValidationLayers) {
        Validation::DestroyDebugUtilsMessengerEXT(arEngine.instance, arEngine.debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(arEngine.instance, arEngine.surface, nullptr);

    vkDestroyInstance(arEngine.instance, nullptr);
}

VkPhysicalDevice_T *Platform::selectPhysicalDevice() const {
    uint32_t deviceCount;

    VkResult result = vkEnumeratePhysicalDevices(arEngine.instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to fetch physical device");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    std::vector<VkPhysicalDeviceProperties> deviceProperties(deviceCount);
    result = vkEnumeratePhysicalDevices(arEngine.instance, &deviceCount, devices.data());
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to fetch physical device");

    for (int i = 0; i < deviceCount; ++i) {

        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties[i]);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);
    }

    return devices[0];
}

void Platform::pickPhysicalDevice() {
    arEngine.mainDevice.physicalDevice = selectPhysicalDevice();
}

Platform::QueueFamilyIndices Platform::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, arEngine.surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            indices.computeFamily = i;

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

void Platform::createLogicalDevice() {
    // Check that we got all device extensions available
    if (!hasDeviceExtensionsSupport()) throw std::runtime_error("Device extension not available");

    // Specify which Queues we want to be able to handle
    QueueFamilyIndices indices = findQueueFamilies(arEngine.mainDevice.physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    // Specify which extra additional device features we want to be able to use || not relevant for now
    //VkPhysicalDeviceFeatures deviceFeatures{};

    // Create Logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = VK_NULL_HANDLE;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();


    // We dont want to bother with old versions, skip setting ppEnabledLayerNames ..

    // Create logical device
    if (vkCreateDevice(arEngine.mainDevice.physicalDevice, &createInfo, nullptr, &arEngine.mainDevice.device) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // Also get queue handles
    vkGetDeviceQueue(arEngine.mainDevice.device, indices.graphicsFamily.value(), 0, &arEngine.graphicsQueue);
    vkGetDeviceQueue(arEngine.mainDevice.device, indices.presentFamily.value(), 0, &arEngine.presentQueue);
    vkGetDeviceQueue(arEngine.mainDevice.device, indices.computeFamily.value(), 0, &arEngine.computeQueue);
}

void Platform::createSwapchain() {
    SwapChainSupportDetails swapChainSupport = querySwapchainSupport(arEngine.mainDevice.physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = arEngine.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //Different queues are on the same channel
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(arEngine.mainDevice.device, &swapchainCreateInfo, nullptr, &arEngine.swapchain) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(arEngine.mainDevice.device, arEngine.swapchain, &imageCount, nullptr);
    arEngine.swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(arEngine.mainDevice.device, arEngine.swapchain, &imageCount,
                            arEngine.swapchainImages.data());

    // Set variables for later use
    arEngine.swapchainExtent = extent;
    arEngine.swapchainFormat = surfaceFormat.format;
    arEngine.swapChainImageViews.resize(imageCount);
}

bool Platform::hasDeviceExtensionsSupport() {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(arEngine.mainDevice.physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(arEngine.mainDevice.physicalDevice, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

Platform::SwapChainSupportDetails Platform::querySwapchainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, arEngine.surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, arEngine.surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, arEngine.surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, arEngine.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, arEngine.surface, &presentModeCount,
                                                  details.presentModes.data());
    }


    return details;
}

VkSurfaceFormatKHR Platform::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Platform::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Platform::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {static_cast<uint32_t>(viewport.WIDTH), static_cast<uint32_t>(viewport.HEIGHT)};

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Platform::createSwapchainImageViews() {
    for (size_t i = 0; i < arEngine.swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = arEngine.swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = arEngine.swapchainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(arEngine.mainDevice.device, &createInfo, nullptr, &arEngine.swapChainImageViews[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void Platform::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(arEngine.mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(arEngine.mainDevice.device, &poolInfo, nullptr, &arEngine.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

}


Platform::~Platform() = default;
