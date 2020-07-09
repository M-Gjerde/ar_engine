#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by magnus on 7/4/20.
//
#define STB_IMAGE_IMPLEMENTATION

#include <cstdlib>
#include <vector>
#include <cstring>
#include <iostream>
#include <array>
#include "../headers/VulkanRenderer.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;

int VulkanRenderer::init(GLFWwindow *newWindow) {
    window = newWindow;
    try {
        createInstance();
        createSurface();
        getPhysicalDevice();
        setDebugMessenger();
        createLogicalDevice();
        createSwapChain();
        createRenderPass();
        createDescriptorSetLayout();
        createPushConstantRange();
        createGraphicsPipeline();
        createDepthBufferImage();
        createFramebuffer();
        createCommandPool();

        int firstTexture = createTexture("landscape.jpg");

        uboViewProjection.projection = glm::perspective(glm::radians(45.0f),
                                                        (float) swapChainExtent.width / (float) swapChainExtent.height,
                                                        0.1f,
                                                        100.0f);
        uboViewProjection.view = glm::lookAt(glm::vec3(3.0f, 0.0f, -1.0f),
                                             glm::vec3(0.0f, 0.0f, -4.0f),
                                             glm::vec3(0.0f, 1.0f, 0.0f));

        uboViewProjection.projection[1][1] *= -1; // Flip the y-axis because Vulkan and OpenGL are different and glm was initially made for openGL

        // Create a mesh
        //vertex data
        std::vector<Vertex> meshVertices = {
                {{-0.4, 0.4,  0.0}, {1.0f, 0.0f, 0.0f}},        // 0
                {{-0.4, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},        // 1
                {{0.4,  -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},        // 2
                {{0.4,  0.4,  0.0}, {1.0f, 0.0f, 0.0f}},        // 3
        };

        std::vector<Vertex> meshVertices2 = {
                {{-0.25, 0.6,  0.0}, {0.0f, 0.0f, 1.0f}},        // 0I love
                {{-0.25, -0.6, 0.0}, {0.0f, 0.0f, 1.0f}},        // 1
                {{0.25,  -0.6, 0.0}, {0.0f, 0.0f, 1.0f}},        // 2
                {{0.25,  0.6,  0.0}, {0.0f, 0.0f, 1.0f}},        // 3
        };


        // Index Data
        std::vector<uint32_t> meshIndices = {
                0, 1, 2,                   // First triangle
                2, 3, 0                    // Second triangle
        };
        Mesh firstMesh = Mesh(mainDevice.physicalDevice,
                              mainDevice.logicalDevice,
                              graphicsQueue,
                              graphicsCommandPool,
                              &meshVertices,
                              &meshIndices);

        Mesh secondMesh = Mesh(mainDevice.physicalDevice,
                               mainDevice.logicalDevice,
                               graphicsQueue,
                               graphicsCommandPool,
                               &meshVertices2,
                               &meshIndices);

        meshList.push_back(firstMesh);
        meshList.push_back(secondMesh);

        glm::mat4 meshModelMatrix = meshList[0].getModel().model;
        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        meshList[0].setModel(meshModelMatrix);

        createCommandBuffers();
        //allocateDynamicBufferTransferSpace();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createSynchronisation();

    } catch (const std::runtime_error &e) {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void VulkanRenderer::draw() {
    // 1. Get next available image to draw to and set something to signal and set something to signal when we're finished with the image (a semaphore)
    // 2. Submit command buffer to queue for execution, making sure it waits for the image to be signalled as available before drawing
    // and signals when it has finished rendering
    // 3. Present image to screen when it has signaled finished rendering

    // Wait for fences before we continue submitting to the GPU
    vkWaitForFences(mainDevice.logicalDevice,
                    1,
                    &drawFences[currentFrame],
                    VK_TRUE,
                    std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

    // -- GET NEXT IMAGE --
    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mainDevice.logicalDevice,
                          swapchain,
                          std::numeric_limits<uint64_t>::max(),
                          imageAvailable[currentFrame],
                          VK_NULL_HANDLE,
                          &imageIndex);

    // Update vpUniformBuffer
    recordCommands(imageIndex);
    updateUniformBuffers(imageIndex);


    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                          // Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];               // List of semaphores to wait on
    VkPipelineStageFlags waitStages[]{
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;                  // Stages to check semaphores at
    submitInfo.commandBufferCount = 1;                          // Number of command buffers to submit
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];   // Command buffer to submit
    submitInfo.signalSemaphoreCount = 1;                        // Number of semaphore to signal
    submitInfo.pSignalSemaphores = &renderFinished[currentFrame];             // Semaphores to signal when cmd buffer finishes

    // Submit command buffer to queue
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit command buffer to Queue");
    // Now our images has been drawn, it actually has data

    // -- PRESENT RENDERED IMAGE TO SCREEN/SURFACE --
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                         // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &renderFinished[currentFrame];              // Semaphores to wait on
    presentInfo.swapchainCount = 1;                             // Number of swapchains to present to/from
    presentInfo.pSwapchains = &swapchain;                       // Swapchain to present images to
    presentInfo.pImageIndices = &imageIndex;                    // Index of images in swapchain to present

    // Present image
    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present image to screen");

    // Get next frame (use % MAX_FRAME_DRAWS to keep value belo MAX_FRAME_DRAWS)
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}


void VulkanRenderer::createDescriptorSetLayout() {

    // VP binding info Create descriptor layout bindings
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor(uniform, dynamic uniform, image sampler, etc..)
    vpLayoutBinding.descriptorCount = 1;                                   // Number of descriptors for binding
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;               // Shader stage to bind to
    vpLayoutBinding.pImmutableSamplers = nullptr;                          // For texture: can make sampler data unchangeable (immutable) by specyfing layout but the imageView it samples from can still be changed

    /*
    // Model binding info
    VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;
*/
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
            vpLayoutBinding
    };
    // Create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());     // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                               // Array of binding infos

    // Create descriptor set layout
    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr,
                                                  &descriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor set layout");

}


void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(mainDevice.logicalDevice);
    //free(modelTransferSpace);


    for (size_t i = 0; i < textureImages.size(); ++i) {
        vkDestroyImage(mainDevice.logicalDevice, textureImages[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, textureImageMemory[i], nullptr);
    }

    vkDestroyImageView(mainDevice.logicalDevice, depthBufferImageView, nullptr);
    vkDestroyImage(mainDevice.logicalDevice, depthBufferImage, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, depthBufferImageMemory, nullptr);

    vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);

        //vkDestroyBuffer(mainDevice.logicalDevice, modelDynamicUniformBuffer[i], nullptr);
        //vkFreeMemory(mainDevice.logicalDevice, modelDynamicUniformBufferMemory[i], nullptr);
    }

    for (auto &i : meshList) {
        i.destroyBuffers();
    }

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i) {
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
    }

    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

    for (auto image :swapChainImages) {
        vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
    }
    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);

    //if (enableValidationLayers)
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    vkDestroyDevice(mainDevice.logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);
}


void VulkanRenderer::createInstance() {
    //  Setup debug messenger
    if (//enableValidationLayers &&
            !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // Info about the app itself mostly used for developer convenience
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan app";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine name";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    // Info for the VkInstance itself
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Create list to hold instance extensions

    std::vector<const char *> instanceExtensions = std::vector<const char *>();

    // Set up extensions instance will use
    uint32_t glfwExtensionCount = 0;                // GLFW may require multiple extensions
    const char **glfwExtensions;                    // Extensions passed as array of cstrings, so need pointer (the array) to pointer to (c_string)

    // Get GLFW extensions
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extensions to list of extensions
    for (size_t i = 0; i < glfwExtensionCount; i++) {
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    //if (enableValidationLayers) {
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //}

    //Check instance extensions supported...
    if (!checkInstanceExtensionSupport(&instanceExtensions)) {
        throw std::runtime_error("vkInstance does not support required extensions");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()); // .size() values are compatible with uint32_t but this explicitly ensures maybe future changes.
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();


    // if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    //} else {
    //      createInfo.enabledLayerCount = 0;
    // }

    //Create instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a vulcan instance");
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char *> *checkExtensions) {
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


void VulkanRenderer::getPhysicalDevice() {
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

    minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;


}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device) {

    /*
    //Information about the device itself (ID, name, type, vendor, etc..)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    //Info about what the device can do (geo shader, tess shader, wide lines, etc..)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */
    QueueFamilyIndices indices = getQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;

    //  If the surface is supported then we can do some checks for surface support
    if (extensionsSupported) {
        SwapChainDetails swapChainDetails = getSwapChainDetails(device);
        swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
    }
    return indices.isValid() && extensionsSupported && swapChainValid;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    //Get all queue family property info for the given device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());
    int i = 0;
    //Go through each queue family and check if it has at least one of the required types of queue.
    for (const auto &queueFamily : queueFamilyList) {
        //Check if queue family has at least 1 queue in that family
        //Queue can be multiple types defined thourgh bitfield. Need to & VK_QUEUEU_*_BIT if has required flags
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

void VulkanRenderer::createLogicalDevice() {
    //  Get the QueueFamily indices for the physical device
    QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

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
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());      //Number of enabled logical device extensions
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();                           //List of enabled logical device extensions

    //Physical Device features the logical device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};
    //  No features for now
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    //  Create the logical device (interface) for the given physical device.
    VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceInfo, nullptr, &mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Could not create Logical device");

    //  Queues are created at the same time as the device...
    // So we want a handle to queues, QueueIndex is 0 because we only create one queue.
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::setDebugMessenger() {

    //if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

bool VulkanRenderer::checkValidationLayerSupport() {
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
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

VkResult
VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkBool32 VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                       [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                                       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                       [[maybe_unused]] void *pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        // Message is important enough to show
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::createSurface() {
    //Create surface (Creating a surface create info struct that is specific to how GLFW is handling it's window)
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create surface");
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    //  Get device extension count
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    if (extensionCount == 0)
        throw std::runtime_error("No Extensions found");

    //  Populate list of extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    //Check for extension
    for (const auto &deviceExtension : deviceExtensions) {
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

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device) {
    SwapChainDetails swapChainDetails;

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

void VulkanRenderer::createSwapChain() {
    //Get details so we can pick best settings
    SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

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
    QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

    // If graphics and presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily) {
        //Queues to share between
        uint32_t queueFamilyIndices[] = {
                (uint32_t) indices.graphicsFamily,
                (uint32_t) indices.presentationFamily
        };

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
    VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);

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
        SwapchainImage swapchainImage = {};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        //Add to swapchain image list
        swapChainImages.push_back(swapchainImage);
    }
}

//  Best format is subjective but ours will be:
//  Format      :   VK_FORMAT_R8G8B8A8_UNORM
//  colorSpace  :   VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {

    //   If only 1 format is available and is undefined, then this means all formats available
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    //  Or else we want to check the ones we want the most
    for (const auto &format : formats) {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentMode(const std::vector<VkPresentModeKHR> &presentationModes) {

    //Look for mailbox presentation mode
    for (const auto &presentationMode : presentationModes) {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentationMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR; // According to the specs this one is always available, or else there is something wrong with the system
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
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

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const {
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
    VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an image view");
    return imageView;
}

void VulkanRenderer::createGraphicsPipeline() {
    auto vertexShaderCode = readFile("../shaders/vert.spv");
    auto fragmentShaderCode = readFile("../shaders/frag.spv");

    //  Build shader modules to link to graphics pipeline
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    //  -- SHADER STAGE CREATION INFORMATION --
    //  Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;                  //Shader stage name
    vertexShaderCreateInfo.module = vertexShaderModule;                         //Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                                      //Entry point in to shader

    //  Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;                  //Shader stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;                         //Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                                      //Entry point in to shader

    //  Put shader stage creation info in to array
    //  Graphics pipeline create info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                             // Can bind multiple streams of data, this defined which one
    bindingDescription.stride = sizeof(Vertex);                 // Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Draw one object at a time (VERTEX), or draw multiple object simultaneously (INSTANCING)
    // Only valid for the same object
    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    //Position Attribute
    attributeDescriptions[0].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[0].location = 0;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[0].offset = offsetof(Vertex,
                                               pos);        // We can have multiple attributes in the same data which is separated by nth index

    //Color Attribute
    attributeDescriptions[1].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[1].location = 1;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[1].offset = offsetof(Vertex,
                                               col);        // We can have multiple attributes in the same data which is separated by nth index


    //  -- VERTEX INPUT --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;                                     //List of vertex Binding descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();                          //List of vertex attribute descriptions (data format and where to bind to or from)

    //  -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;       //Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;                    //Allow overriding of "strip" topology to start new primitives

    //  -- VIEWPORT & SCISSOR --
    //  Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;                                  //x start coordinate
    viewport.y = 0.0f;                                  //y start coordinate
    viewport.width = (float) swapChainExtent.width;     //viewport width
    viewport.height = (float) swapChainExtent.height;   //viewport height
    viewport.minDepth = 0.0f;                           //min framebuffer depth
    viewport.maxDepth = 1.0f;                           //max framebuffer depth

    //  Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = {0, 0};                      //Offset to use region from
    scissor.extent = swapChainExtent;                   //Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // -- DYNAMIC STATES --
    //  Dynamic states to enable
    /*
    std::vector<VkDynamicState> dynamicStateEnable;
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_VIEWPORT);        //Dynamic viewport : can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport)
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_SCISSOR);         //Dynamic scissor  : can resize in command buffer with vkCmdsetScissor(commandbuffer, 0, 1, &scissor)
    //Make sure to destroy swapchain and recreate after you've resized, which is also the same with the depth buffer.
    //  Dynamic state creation info
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnable.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStateEnable.data();
*/

    //  -- RASTERIZER --
    //  Converts primitives into fragments
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;                       //Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;                //Stop the pipeline process here, which we wont
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;                //How we want our polygons to be filled in color between vertices.
    rasterizerCreateInfo.lineWidth = 1.0f;                                  //How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;                  //Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;               //Winding to dertermine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                        //Wether to add depth bias to fragments (Good for stopping "shadow acne")

    //  -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;                       //Enable multisample shading or not
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;         //Number of samples to user per fragment

    //  -- BLENDING --
    //  Blending decides how to blend a new color being written to a fragment, with the old value

    //  Blend attachment state (how blending is handled)
    VkPipelineColorBlendAttachmentState colorStateAttachment = {};
    colorStateAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;           //Colors to apply blending to
    colorStateAttachment.blendEnable = VK_TRUE;                               //Enable blending
    //  Blending uses equation: (srcColorBlendFactor * new color) colorblendOp (dstColorBlendfactor * old color)
    colorStateAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorStateAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorStateAttachment.colorBlendOp = VK_BLEND_OP_ADD;                      //Which mathematical operation we use to calculate color blend
    //  Replace the old alpha channel with the new one
    colorStateAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorStateAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorStateAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //  Summarised: (1 * newAlpha) + (0 * oldAlpha) = new alpha
    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;               //Alternative to calculations is to use logical operations
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorStateAttachment;

    //  -- PIPELINE LAYOUT --
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;                                    // Number of descriptor set layouts
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;                    // Descriptor set layout to bind
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr,
                                             &pipelineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline");

    //  -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;                    // Enable checking depth to determine fragment write (Wether to replace pixel)
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;                   // Can we replace our value when we place a pixel
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;          // If the new value is less then overwrite
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;             // Depth bounds test: Does the depth value exist between two bounds=
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;                 // Enable stencil test

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;                                   //Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                           //List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;       //All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;                         //Pipeline layout pipeline should use
    pipelineCreateInfo.renderPass = renderPass;                         //Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                     //Subpass of render pass to use with pipeline

    //  Pipeline derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;             //If we want to create a pipeline from an old pipeline
    pipelineCreateInfo.basePipelineIndex = -1;                          //Or index we want to derive pipeline from

    //  Create graphics pipeline
    result = vkCreateGraphicsPipelines(
            mainDevice.logicalDevice,
            VK_NULL_HANDLE,
            1,
            &pipelineCreateInfo,
            nullptr,
            &graphicsPipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
    //  Destroy modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);

}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char> &code) const {
    //  Shader module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();                                          // Size of code
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());         // pointer to code -> reinterpret_cast is to convert between pointer cast

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");

    return shaderModule;
}

void VulkanRenderer::createRenderPass() {

    // ATTACHMENTS
    //  Color attachment of render pass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;                      //Format to use for attachment
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    //Number of samples to write for multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               //Start with a blank state when we draw our image, describes what to do with attachment before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             //Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    //Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  //Describes what to do with stencil before rendering

    //  Framebuffer data will be stored as an image, but images can be given different data layouts
    //  to give optimal use for certain operations
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          //Image data layout before render pass starts
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      //Image data layout after render pass starts (to change to)

    //Depth Attachment of render pass
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = chooseSupportedFormat(
            {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //REFERENCES
    //  Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;     //Will be converted to this type after initalLayout in colorAttachment

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    //  Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies{};

    //  Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    //  Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        //Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     //Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               //Stage access mask (memory access)

    //  But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    //  Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    //  Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //  But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    // Array with attachments
    std::array<VkAttachmentDescription, 2> renderPassAttachments = {colorAttachment, depthAttachment};

    //  Create info for Render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Render pass");
}

void VulkanRenderer::createFramebuffer() {
    //Resize framebuffer count to equal swap chain image count
    swapChainFramebuffers.resize(swapChainImages.size());

    //  Create a framebuffer for each swap chain image
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {

        std::array<VkImageView, 2> attachments = {
                swapChainImages[i].imageView,
                depthBufferImageView
                //Another attachment here would also come from the second attachment in the render pass
        };
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;                                          //  Render pass layout the framebuffer will be sued with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();                                //  List of attachments (1:1 with render pass)
        framebufferCreateInfo.width = swapChainExtent.width;                                    //  Framebuffer width
        framebufferCreateInfo.height = swapChainExtent.height;                                  //  Framebuffer height
        framebufferCreateInfo.layers = 1;                                                       //  Framebuffer layers

        VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr,
                                              &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create Framebuffer");
    }
}

void VulkanRenderer::createCommandPool() {
    //  Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;      //  Queue Family type that buffers from this command pool will use

    //  Create a Graphics queue family command pool
    VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
}

void VulkanRenderer::createCommandBuffers() {
    //  Resize command buffer count to have one for each framebuffer
    commandBuffers.resize(swapChainFramebuffers.size());
    //  This is called allocated because we have already "created" it in the pool when we reserved memory
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;                                // vkCmdExecuteCommands(buffer) executing a command buffer from a command buffer. Primary means that it is executed by queue, and secondary means it is executed by a commandbuffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());      // Amount of command buffers that we are going to create

    //  Allocate command buffers and place handles in arraay of buffers
    VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-misleading-indentation"

void VulkanRenderer::recordCommands(uint32_t currentImage) {
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;     // The command buffer can be submitted while it is awaiting execution Yes it can

    // Information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;                  //  Render pass to begin
    renderPassBeginInfo.renderArea.offset = {0, 0};        // Start point of render pass in pixels
    renderPassBeginInfo.renderArea.extent = swapChainExtent;     //Size of region to run render pass on (starting at offset)

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.6f, 0.65f, 0.4f, 1.0f};
    clearValues[1].depthStencil.depth = 1.0f;

    renderPassBeginInfo.pClearValues = clearValues.data();       //List of clear values
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

    renderPassBeginInfo.framebuffer = swapChainFramebuffers[currentImage];

    // Start recording commands to command buffer
    VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to start recording a Command Buffer");

    // Begin Render Pass
    vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind Pipeline to be used in render pass
    vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    for (auto &j : meshList) {
        // Bind our vertex buffer
        VkBuffer vertexBuffers[] = {
                j.getVertexBuffer()};                                                       // Buffers to bind
        VkDeviceSize offsets[] = {
                0};                                                                                   // Offsets into buffers being bound
        vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers,
                               offsets);     // Command to bind vertex buffer before drawing

        // Bind our mesh Index buffer using the uint32 type ONLY ONE CAN BE BINDED
        vkCmdBindIndexBuffer(commandBuffers[currentImage], j.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Dynamic offset amount
        // uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

        // Bind descriptor sets
        vkCmdBindDescriptorSets(
                commandBuffers[currentImage],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                0,
                1,
                &descriptorSets[currentImage],
                0,
                nullptr);
        Model tempModel = j.getModel();
        // Pushing PushConstants
        vkCmdPushConstants(
                commandBuffers[currentImage],
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(Model),                          // Size of data being pushed
                &tempModel         // Actual data being pushed
        );

        // Execute pipeline
        vkCmdDrawIndexed(commandBuffers[currentImage], j.getIndexCount(), 1, 0, 0, 0);

    }

    // End Render Pass
    vkCmdEndRenderPass(commandBuffers[currentImage]);

    // Stop recording to command buffer
    result = vkEndCommandBuffer(commandBuffers[currentImage]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end recording a Command Buffer");

}


void VulkanRenderer::createSynchronisation() {
    imageAvailable.resize(MAX_FRAME_DRAWS);
    renderFinished.resize(MAX_FRAME_DRAWS);
    drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;       // Fence will start open flag

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
        if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) !=
            VK_SUCCESS ||
            vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)

            throw std::runtime_error("Failed to create Semaphore(s)");
    }
}

void VulkanRenderer::createUniformBuffers() {
    // ViewProjection buffer size (Will offset access)
    VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    vpUniformBuffer.resize(swapChainImages.size());
    vpUniformBufferMemory.resize(swapChainImages.size());
    modelDynamicUniformBuffer.resize(swapChainImages.size());
    modelDynamicUniformBufferMemory.resize(swapChainImages.size());

    // Create uniform buffers
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(mainDevice.physicalDevice,
                     mainDevice.logicalDevice,
                     vpBufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &vpUniformBuffer[i],
                     &vpUniformBufferMemory[i]);

        /*
        createBuffer(mainDevice.physicalDevice,
                     mainDevice.logicalDevice,
                     modelBufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &modelDynamicUniformBuffer[i],
                     &modelDynamicUniformBufferMemory[i]);

*/
    }
}

void VulkanRenderer::createDescriptorPool() {

    // Type of descriptors + how many DESCRIPTORS, not Descriptor sets (combined makes the pool size)
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

    /*
    VkDescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDynamicUniformBuffer.size());
*/
    std::vector<VkDescriptorPoolSize> poolList = {
            vpPoolSize,
    };

    // Data to create descriptor pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());       // Maximum nmber of descriptor sets that can be created from pool
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolList.size());        // Amount of pool sizes being passed
    poolCreateInfo.pPoolSizes = poolList.data();                                  // Pool sizes to create pool with

    // Create descriptor pool
    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");
}

void VulkanRenderer::createDescriptorSets() {
    // Resize descriptor set list so one for every uniform buffer
    descriptorSets.resize(swapChainImages.size());

    std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), descriptorSetLayout);

    //Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = descriptorPool;                                     // Pool to allocate descriptor set from
    setAllocateInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());  // Number of sets to allocate
    setAllocateInfo.pSetLayouts = setLayouts.data();                                     // Layouts to use to allocates sets(1:1 relationship)

    // Allocate descriptor sets (multiple)

    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocateInfo, descriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets");

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // Buffer info and data offset info
        // VIEW PROJECTION DESCRIPTOR
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = vpUniformBuffer[i];                // Buffer to get data from
        vpBufferInfo.offset = 0;                               // Offset into the data
        vpBufferInfo.range = sizeof(UboViewProjection);                      // Size of the data that is going to be bound to the descriptor set


        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = descriptorSets[i];                         // Descriptor set to update
        vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader
        vpSetWrite.dstArrayElement = 0;                                // Index in the array we want to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor we are updating
        vpSetWrite.descriptorCount = 1;                                // Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;                        // Information about buffer data to bind

        /*
        // MODEL DESCRIPTOR
        // Model Buffer Binding info
        VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelDynamicUniformBuffer[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = modelUniformAlignment;                  // Size is for each single piece of data, not the whole thing

        VkWriteDescriptorSet modelSetWrite = {};
        modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelSetWrite.dstSet = descriptorSets[i];
        modelSetWrite.dstBinding = 1;
        modelSetWrite.dstArrayElement = 0;
        modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelSetWrite.descriptorCount = 1;
        modelSetWrite.pBufferInfo = &modelBufferInfo;
*/

        std::vector<VkWriteDescriptorSet> writeDescriptorSetlist = {vpSetWrite};
        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(writeDescriptorSetlist.size()),
                               writeDescriptorSetlist.data(), 0, nullptr);
    }
}

void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex) {
    // Copy VP data
    void *data;
    vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
    memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
    vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);

    // Copy Model data
    // Only used for dynamic uniform buffer
    /*
    for (size_t i = 0; i < meshList.size(); i++) {
        // Get the address and add a offset for each model, for i = 2 the pointer will point to the address of the second object
        auto *thisModel = (Model *) ((uint64_t) modelTransferSpace + (i * modelUniformAlignment));
        *thisModel = meshList[i].getModel();
    }

    // Map the list of model data
    vkMapMemory(mainDevice.logicalDevice, modelDynamicUniformBufferMemory[imageIndex], 0,
                modelUniformAlignment * meshList.size(), 0, &data);
    memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
    vkUnmapMemory(mainDevice.logicalDevice, modelDynamicUniformBufferMemory[imageIndex]);
*/

}

void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel) {

    if (modelId >= meshList.size())return;
    meshList[modelId].setModel(newModel);

}

void VulkanRenderer::allocateDynamicBufferTransferSpace() {

    /*
// Calculate alignment of model data
    modelUniformAlignment = (sizeof(Model) + minUniformBufferOffset - 1)
                            & ~(minUniformBufferOffset - 1);

    // Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
    modelTransferSpace = (Model *) aligned_alloc(modelUniformAlignment, modelUniformAlignment * MAX_OBJECTS);
*/
}

void VulkanRenderer::createPushConstantRange() {
    // Define push constant values
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;      // Shader stage push constant will go to
    pushConstantRange.offset = 0;                                   // Offset into given data to pass to push constat
    pushConstantRange.size = sizeof(Model);                         // Size of data being passed

}

void VulkanRenderer::createDepthBufferImage() {

    // Get supported format for depth buffer
    VkFormat depthFormat = chooseSupportedFormat(
            {
                    VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    // Create depth Buffer Image
    depthBufferImage = createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &depthBufferImageMemory
    );

    // Create depth buffer image view
    depthBufferImageView = createImageView(depthBufferImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


}

VkImage VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
                                    VkDeviceMemory *imageMemory) {
    // CREATE IMAGE
    //Image creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                             // Type of image (1D, 2D or 3D)
    imageCreateInfo.extent.height = height;                                   // Height of image extent
    imageCreateInfo.extent.width = width;                                     // Width of image extent
    imageCreateInfo.extent.depth = 1;                                         // depth of image extent  (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;                                            // Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;                                          // Number of levels in image array;
    imageCreateInfo.format = format;                                          // Format type of image
    imageCreateInfo.tiling = tiling;                                          // How image data should be "tiled" (arranged for optimal reading speed)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                // Layout of iamge data on creation
    imageCreateInfo.usage = useFlags;                                         // Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                          // Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                  // Sharing mode between queues.

    VkImage image;
    VkResult result = vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an Image");

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice,
                                                             memoryRequirements.memoryTypeBits, propFlags);
    result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocateInfo, nullptr, imageMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to Allocate memory for image");

    // Connect memory to image, in imagememory we could have data for 10 images therefore we have a offset
    vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0);

    return image;

}

VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling,
                                               VkFormatFeatureFlags featureFlags) {
    // Loop through options and find compatible ones
    for (VkFormat format : formats) {
        // Get properties for given format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(mainDevice.physicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
            return format;
    }

    throw std::runtime_error("Failed to find a matching format");
}

stbi_uc *VulkanRenderer::loadTextureFile(std::string fileName, int *width, int *height, VkDeviceSize *imageSize) {
    // Number of channels image uses
    int channels;

    // Load pixel data for image
    std::string fileLoc = "../textures/" + fileName;
    stbi_uc *image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

    if (!image)
        throw std::runtime_error("Failed to load a Texture file (" + fileName + ")");

    // Calculate image size using: dimmensions * channels
    *imageSize = *width * *height * 4;

    return image;
}

int VulkanRenderer::createTexture(std::string fileName) {
    //Load in image file
    int width, height;
    VkDeviceSize imageSize;
    stbi_uc *imageData = loadTextureFile(fileName, &width, &height, &imageSize);

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer imageStagingBuffer;
    VkDeviceMemory imageStagingBufferMemory;
    createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &imageStagingBuffer,
                 &imageStagingBufferMemory
    );

    // Copy image data to staging buffer
    void *data;
    vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);

    //Free original image data memory
    stbi_image_free(imageData);

    // Create image to hold final texture
    VkImage texImage;
    VkDeviceMemory texImageMemory;
    texImage = createImage(
            width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory
    );

    // COPY DATA TO IMAGE
    // Transition image to be DST for copy operation
    transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //Copy image data
    copyImageBuffer(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width, height);
    // Transition image to be shader readable for shader usage
    transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // Add texture data to vector for reference
    textureImages.push_back(texImage);
    textureImageMemory.push_back(texImageMemory);

    // Destroy staging buffers
    vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

    //Return texture image ID
    return textureImages.size() - 1;
}


#pragma clang diagnostic pop
