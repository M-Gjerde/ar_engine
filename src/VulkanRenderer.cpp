//
// Created by magnus on 8/28/21.
//


#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer(bool enableValidation) {
    settings.validation = enableValidation;

    // Create window instance
    // boilerplate stuff (ie. basic window setup, initialize OpenGL) occurs in abstract class
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, VulkanRenderer::keyCallback);
    glfwSetWindowSizeCallback(window, VulkanRenderer::resizeCallback);


    initVulkan();

}

VkResult VulkanRenderer::createInstance(bool enableValidation) {
    settings.validation = enableValidation;


    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = name.c_str();
    appInfo.pEngineName = name.c_str();
    appInfo.apiVersion = apiVersion;

    // Get extensions supported by the instance
    enabledInstanceExtensions = Validation::getRequiredExtensions(settings.validation);

    // Check if extensions are supported
    if (!Validation::checkInstanceExtensionSupport(enabledInstanceExtensions))
        throw std::runtime_error("Instance Extensions not supported");


    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    if (!enabledInstanceExtensions.empty()) {
        if (settings.validation) {
            enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t) enabledInstanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    if (settings.validation) {
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

        // Check if this layer is available at instance level
        if (Validation::checkValidationLayerSupport(validationLayers)) {
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        } else {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled\n";
        }

        return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);


    }


}

bool VulkanRenderer::initVulkan() {
    // Create Instance
    VkResult err = createInstance(settings.validation);
    if (err) {
        throw std::runtime_error("Could not create Vulkan instance");
    }
    // If requested, we enable the default validation layers for debugging
// If requested, we enable the default validation layers for debugging
    if (settings.validation) {
        // The report flags determine what type of messages for the layers will be displayed
        // For validating (debugging) an application the error and warning bits should suffice
        // Additional flags include performance info, loader and layer debug messages, etc.
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Validation::populateDebugMessengerCreateInfo(createInfo);

        if (Validation::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugUtilsMessenger) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // Get list of devices and capabilities of each device
    uint32_t gpuCount = 0;
    err = vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    if (err != VK_SUCCESS or gpuCount == 0) {
        throw std::runtime_error("No device with vulkan support found");
    }

    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Could not enumerate physical devices");
    }
    // Select physical device to be used for the Vulkan example
    // Defaults to the first device unless anything else specified
    uint32_t selectedDevice = 0;
    physicalDevice = physicalDevices[selectedDevice];



    // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

    // Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
    getEnabledFeatures();
    // Vulkan device creation
    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device
    vulkanDevice = new VulkanDevice(physicalDevice);
    err = vulkanDevice->createLogicalDevice(deviceFeatures, enabledDeviceExtensions, nullptr);
    if (err != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device");

    device = vulkanDevice->logicalDevice;
    // Get a graphics queue from the device
    vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);
    // Find a suitable depth format
    depthFormat = Utils::findDepthFormat(physicalDevice);

    swapchain.connect(instance, physicalDevice, device);
    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo = Populate::semaphoreCreateInfo();
    // Create a semaphore used to synchronize image presentation
    // Ensures that the image is displayed before we start submitting new commands to the queue
    err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete);
    if (err)
        throw std::runtime_error("Failed to create semaphore");
    // Create a semaphore used to synchronize command submission
    // Ensures that the image is not presented until all commands have been submitted and executed
    err = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete);
    if (err)
        throw std::runtime_error("Failed to create semaphore");
    // Set up submit info structure
    // Semaphores will stay the same during application lifetime
    // Command buffer submission info is set by each example
    submitInfo = Populate::submitInfo();
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderComplete;

    return true;
}

VulkanRenderer::~VulkanRenderer() {
    // CleanUP all vulkan resources
    swapchain.cleanup();

    //vkDestroyCommandPool(device, cmdPool, nullptr);

    vkDestroySemaphore(device, semaphores.presentComplete, nullptr);
    vkDestroySemaphore(device, semaphores.renderComplete, nullptr);

    if (settings.validation)
        Validation::DestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);

    delete vulkanDevice;

    vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::cleanUp() {

}

void VulkanRenderer::getEnabledFeatures() {

}

void VulkanRenderer::viewChanged() {

}

void VulkanRenderer::keyPressed(uint32_t) {

}

void VulkanRenderer::mouseMoved(double x, double y, bool &handled) {

}

void VulkanRenderer::windowResized() {

}

void VulkanRenderer::buildCommandBuffers() {

}

void VulkanRenderer::setupDepthStencil() {
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = { width, height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkResult result = vkCreateImage(device, &imageCI, nullptr, &depthStencil.image);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create depth image");

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    result = vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to allocate depth image memory");
    result = vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to bind depth image memory");

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = depthStencil.image;
    imageViewCI.format = depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    result = vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create depth image view");

}

void VulkanRenderer::setupFrameBuffer() {
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthStencil.view;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;

    // Create frame buffers for every swap chain image
    frameBuffers.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        attachments[0] = swapchain.buffers[i].view;
        VkResult result = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create framebuffer");

    }
}

void VulkanRenderer::setupRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = swapchain.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkResult result = (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create render pass");

}

void VulkanRenderer::createCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = swapchain.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult result = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create command pool");

}

void VulkanRenderer::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    drawCmdBuffers.resize(swapchain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            Populate::commandBufferAllocateInfo(
                    cmdPool,
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    static_cast<uint32_t>(drawCmdBuffers.size()));

    VkResult result = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data());
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create command pool");


}

void VulkanRenderer::createSynchronizationPrimitives()
{
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = Populate::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    waitFences.resize(drawCmdBuffers.size());
    for (auto& fence : waitFences) {
        VkResult result =vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
        if (result != VK_SUCCESS) throw std::runtime_error("Failed to create command pool");

    }
}
void VulkanRenderer::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult result =vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
    if (result != VK_SUCCESS) throw std::runtime_error("Failed to create command pool");
}



void VulkanRenderer::prepare() {
    swapchain.initSurface(window);
    createCommandPool();
    swapchain.create(&width, &height, settings.vsync);
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();

}


void VulkanRenderer::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    printf("GLFW Key: %d\n", key);

    if (key == GLFW_KEY_ESCAPE and action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);

    }

}

void VulkanRenderer::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void VulkanRenderer::windowResize(uint32_t width, uint32_t height) {
    if (!prepared)
    {
        return;
    }
    prepared = false;
    resized = true;

    // Ensure all operations on the device have been finished before destroying resources
    vkDeviceWaitIdle(device);

    // Recreate swap chain
    swapchain.create(&width, &height);

    // Recreate the frame buffers
    vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.mem, nullptr);
    setupDepthStencil();
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
    }
    setupFrameBuffer();

    // Maybe resize overlay too
    // --- Missing code snippet ---

    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    destroyCommandBuffers();
    createCommandBuffers();
    buildCommandBuffers();

    vkDeviceWaitIdle(device);


    // Notify derived class
    windowResized();
    viewChanged();

    prepared = true;
}

void foobar() {

}

void VulkanRenderer::resizeCallback(GLFWwindow *window, int width, int height) {

    auto* myApp = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    myApp->windowResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));


}
