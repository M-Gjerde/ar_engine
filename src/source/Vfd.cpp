//
// Created by magnus on 9/20/20.
//


#include "../headers/Vfd.h"
#include "../libs/Utils.h"


Vfd::Vfd() = default;
Vfd::~Vfd() = default;

int Vfd::init(GLFWwindow *newWindow) {
    window = newWindow;
    vkAllocationCallbacks = Allocator().operator VkAllocationCallbacks();
    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    return 0;
}


void Vfd::cleanUp() {


    for (auto image :swapChainImages) {
        vkDestroyImageView(mainDevice.logicalDevice, image.imageView, &vkAllocationCallbacks);
    }

    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, &vkAllocationCallbacks);
    vkDestroyDevice(mainDevice.logicalDevice, &vkAllocationCallbacks);
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, &vkAllocationCallbacks);
    vkDestroySurfaceKHR(instance, surface, &vkAllocationCallbacks);
    vkDestroyInstance(instance, &vkAllocationCallbacks);

}

// - Create functions
void Vfd::createInstance() {

    // Setup debug messenger
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested but not supported");

    // App info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan app";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine name";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Create list to hold instance extensions
    std::vector<const char *> instanceExtensions = std::vector<const char *>();

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    // Get GLFW extensions
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extensions to list of extensions
    instanceExtensions.reserve(glfwExtensionCount);
    for (size_t i = 0; i < glfwExtensionCount; i++) {
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    if (enableValidationLayers) instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    //Check instance extensions supported...
    if (!checkInstanceExtensionSupport(&instanceExtensions))
        throw std::runtime_error("vkInstance does not support required extensions");

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()); // .size() values are compatible with uint32_t but this explicitly ensures maybe future changes.
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else
        createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, &vkAllocationCallbacks, &instance);
    if (result != VK_SUCCESS) throw std::runtime_error ("Failed to create vulkan instance");

}

void Vfd::createSurface() {
    //Create surface (Creating a surface create info struct that is specific to how GLFW is handling it's window)
    VkResult result = glfwCreateWindowSurface(instance, window, &vkAllocationCallbacks, &surface);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create surface");
}

void Vfd::selectPhysicalDevice() {
    // Enumerate physical devices the vkInstance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    //check if any devices was found at all
    if (deviceCount == 0)
        throw std::runtime_error("Cant find any GPUs that support vulcan instance");

    //Get list of physical Device
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

    for (const auto &device : deviceList) {
        if (checkDeviceSuitable(device)) {
            mainDevice.physicalDevice = device;
            break;
        }
    }

    // Get properties of our new device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

}

void Vfd::createLogicalDevice() {
    //  Get the QueueFamily indices for the physical device
    Utils::QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

    //  Vector for queue creation information, and set for family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};


    //  Queues the logical device needs to create and info to do so
    for (int queueFamilyIndex : queueFamilyIndices) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;                //The index of the family to create a queue from
        queueCreateInfo.queueCount = 1;                                     //Number of queues to create
        float priority = 1;
        queueCreateInfo.pQueuePriorities = &priority;                       //If we have multiple queues, we need to know which one to prioritise, 1 is highest and 0 is lowest.
        queueCreateInfos.push_back(queueCreateInfo);
    }
    //  Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());       //Number of Queue create infos
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();                                 //List of queue create infos so device can create required queues.
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(Utils::deviceExtensions.size());      //Number of enabled logical device extensions
    deviceInfo.ppEnabledExtensionNames = Utils::deviceExtensions.data();                           //List of enabled logical device extensions

    //Physical Device features the logical device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;                 // Enable Anisotropy
    //  No features for now
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    //  Create the logical device (interface) for the given physical device.
    VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceInfo, &vkAllocationCallbacks, &mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Could not create Logical device");


    //  Queues are created at the same time as the device...
    // So we want a handle to queues, QueueIndex is 0 because we only create one queue.
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);

}

void Vfd::createSwapChain() {
    //Get details so we can pick best settings
    Utils::SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

    //  1. Choose best surface format
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    //  2. Choose best presentation MODE
    VkPresentModeKHR presentMode = chooseBestPresentMode(swapChainDetails.presentationModes);
    //  3. Choose swap chain image resolution
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    //  How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
    //  If imageCount is higher than max, then clamp down to max
    //  0 means that there is no max, limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < imageCount) {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;                                                      //SwapChain surface
    swapChainCreateInfo.imageFormat = surfaceFormat.format;                                     //SwapChain format
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;                             //SwapChain color space
    swapChainCreateInfo.presentMode = presentMode;                                              //SwapChain presentation mode
    swapChainCreateInfo.imageExtent = extent;                                                   //SwapChain image extents
    swapChainCreateInfo.minImageCount = imageCount;                                             //Minimum images in SwapChain
    swapChainCreateInfo.imageArrayLayers = 1;                                                   //Number of layers for each image in chain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                       //What attachment images will be used as
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;   //Transform to perform on swap chain images
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                     //How to handle blending images with external graphics (e.g. other windows)
    swapChainCreateInfo.clipped = VK_TRUE;

    // Get Queue Family Indices
    Utils::QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

    // If graphics and presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily) {
        //Queues to share between
        uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentationFamily};

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;      //Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2;                          //Number of queues to share images between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;           //Array of queues to share between
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    // If old swap chain has been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    //  Create Swapchain
    VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, &vkAllocationCallbacks, &swapchain);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Could not create swapchain");

    // -- get images from the created swapchain --
    //  Store for later reference
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    //  Get swap chain images (first count, then values)
    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data());

    for (VkImage image : images) {
        //  Store image handle
        Utils::SwapchainImage swapchainImage = {};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        //Add to swapchain image list
        swapChainImages.push_back(swapchainImage);
    }
}



// - Helpers
bool Vfd::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;

    }
    return true;
}

bool Vfd::checkInstanceExtensionSupport(std::vector<const char *> *checkExtensions) {
    //Need to get number of extensions to create array of correct size to hold extensions.
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    //Create a list of VkExtensionProperties using count.
    std::vector<VkExtensionProperties> extensions(extensionCount);
    //Populate the list of specific/named extensions
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    //Check if given extensions are in list of available extensions
    for (const auto &checkExtension : *checkExtensions) {
        bool hasExtensions = false;
        for (const auto &extension: extensions) {
            if (strcmp(checkExtension, extension.extensionName) == 0) {
                hasExtensions = true;
                break;
            }
        }
        if (!hasExtensions) {
            return false;
        }
    }
    return true;
}

bool Vfd::checkDeviceSuitable(VkPhysicalDevice device) {
    /*
//Information about the device itself (ID, name, type, vendor, etc..)
VkPhysicalDeviceProperties deviceProperties;
vkGetPhysicalDeviceProperties(device, &deviceProperties);
*/

    //Info about what the device can do (geo shader, tess shader, wide lines, etc..)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


    Utils::QueueFamilyIndices indices = getQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;

    //  If the surface is supported then we can do some checks for surface support
    if (extensionsSupported) {
        Utils::SwapChainDetails swapChainDetails = getSwapChainDetails(device);
        swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
    }
    return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;

}

Utils::QueueFamilyIndices Vfd::getQueueFamilies(VkPhysicalDevice device) {
    Utils::QueueFamilyIndices indices;
    //Get all queue family property info for the given device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());
    int i = 0;
    //Go through each queue family and check if it has at least one of the required types of queue.
    for (const auto &queueFamily : queueFamilyList) {
        //Check if queue family has at least 1 queue in that family
        //Queue can be multiple types defined through bitfield. Need to & VK_QUEUEU_*_BIT if has required flags
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        //Check if Queue Family supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
        //  Check if Queue is presentation type (Can also be graphics queue)
        if (queueFamily.queueCount > 0 && presentationSupport)
            indices.presentationFamily = 1;

        if (indices.isValid())                                          //If we found a valid queue.. dont waste time searching for more.
            break;
        i++;
    }
    return indices;
}

bool Vfd::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    //  Get device extension count
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    if (extensionCount == 0)
        throw std::runtime_error("No Extensions found");

    //  Populate list of extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    //Check for extension
    for (const auto &deviceExtension : Utils::deviceExtensions) {
        bool hasExtension = false;
        for (const auto &extension: extensions) {
            if (strcmp(deviceExtension, extension.extensionName) == 0) {
                hasExtension = true;
                break;
            }
        }
        if (!hasExtension) {
            return false;
        }
    }
    return true;
}

Utils::SwapChainDetails Vfd::getSwapChainDetails(VkPhysicalDevice device) {
    Utils::SwapChainDetails swapChainDetails;

    //  -- CAPABILITIES --
    //  Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

    //  -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    //  If formats returned, get list of formats
    if (formatCount != 0) {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

    //If presentationModes returned get list of presentation modes
    if (presentationCount != 0) {
        swapChainDetails.presentationModes.resize(formatCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount,
                                                  swapChainDetails.presentationModes.data());
    }
    return swapChainDetails;
}

VkSurfaceFormatKHR Vfd::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
    //   If only 1 format is available and is undefined, then this means all formats available
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    //  Or else we want to check the ones we want the most
    for (const auto &format : formats) {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return formats[0];
}

VkPresentModeKHR Vfd::chooseBestPresentMode(const std::vector<VkPresentModeKHR> &presentationModes) {
    //Look for mailbox presentation mode
    for (const auto &presentationMode : presentationModes) {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentationMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR; // According to the specs this one is always available, or else there is something wrong with the system
}

VkExtent2D Vfd::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
    //  If current extent is a numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return surfaceCapabilities.currentExtent;
    } else {
        //  If value can vary, need to set manually
        //  Get window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        //  Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        //  Surface also defines max and min, so make sure within boundaries by clamping value
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
                                   std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));

        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
                                    std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));
        return newExtent;
    }
}

VkImageView Vfd::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;                                           //Image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                        //Working with 2D images (CUBE flag is usefull for skyboxes)
    viewCreateInfo.format = format;                                         //Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;            //Allows re-mapping rgba components of rba values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    //  SubResources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;               //Which aspect of image to view (e.g. COLOR_BIT for viewing color)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;                       //Start mimap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;                         //Number of mimap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;                     //Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;                         //Number of array levels to view

    //Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, &vkAllocationCallbacks, &imageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an image view");
    return imageView;}


// - Getters
void Vfd::getSurface(VkSurfaceKHR *pSurface) {
    *pSurface = surface;
}

void Vfd::getInstance(VkInstance* pInstance) {
    *pInstance = instance;
}

void Vfd::getSelectedPhysicalDevice(VkPhysicalDevice *pPhysicalDevice) const {
    *pPhysicalDevice = mainDevice.physicalDevice;
}

void Vfd::getLogicalDevice(VkDevice *pLogicalDevice) const {
    *pLogicalDevice = mainDevice.logicalDevice;
}

void Vfd::getPresentationQueue(VkQueue *pQueue) {
    *pQueue = presentationQueue;
}

void Vfd::getGraphicsQueue(VkQueue *pQueue) {
    *pQueue = graphicsQueue;
}

void Vfd::getSwapchain(VkSwapchainKHR *pSwapchain) {
    *pSwapchain = swapchain;

}

void Vfd::getSwapchainImages(std::vector<Utils::SwapchainImage> *pSwapchainImages) {
    *pSwapchainImages = swapChainImages;

}

void Vfd::getSwapchainImageFormat(VkFormat *pFormat) {
    *pFormat = swapChainImageFormat;
}

void Vfd::getSwapchainExtent(VkExtent2D *pExtent2D) {
    *pExtent2D = swapChainExtent;
}

void Vfd::getSwapchainFramebuffers(std::vector<VkFramebuffer> *pFrameBuffers) {
    *pFrameBuffers = swapChainFramebuffers;

}


// - Debug utilities
VkResult Vfd::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
VkBool32 Vfd::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, [[maybe_unused]] void *pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        // Message is important enough to show
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
void Vfd::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
void Vfd::getSetDebugMessenger(VkDebugUtilsMessengerEXT *pDebugMessenger) {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, &vkAllocationCallbacks, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }

    *pDebugMessenger = debugMessenger;
}

VkAllocationCallbacks Vfd::getAllocator() {
    return vkAllocationCallbacks;
}

void Vfd::getCommandPool(VkCommandPool *commandPool) {
    //  Get indices of queue families from device
    Utils::QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;      //  Queue Family type that buffers from this command pool will use

    //  Create a Graphics queue family command pool
    VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, commandPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");

}

void Vfd::getCommandBuffer(VkCommandBuffer *commandBuffer, VkCommandPool commandPool) const {
    //  This is called allocated because we have already "created" it in the pool when we reserved memory
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;     // vkCmdExecuteCommands(buffer) executing a command buffer from a command buffer. Primary means that it is executed by queue, and secondary means it is executed by a commandbuffer
    cbAllocInfo.commandBufferCount = 1;                      // Amount of command buffers that we are going to create

    //  Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");

}





