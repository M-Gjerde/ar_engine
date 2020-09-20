#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by magnus on 7/4/20.
//
#define STB_IMAGE_IMPLEMENTATION


#include "../headers/VulkanRenderer.hpp"


VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;

int VulkanRenderer::init(GLFWwindow *newWindow) {
    window = newWindow;
    try {
        initiateVulkan.init(window);
        getVulkanInstance();
        getSurface();
        getPhysicalDevice();
        setDebugMessenger();
        getLogicalDevice();
        getQueues();
        getSwapchain();
        createRenderPass();
        createDescriptorSetLayout();
        createPushConstantRange();
        createGraphicsPipeline();
        createColorBufferImage();
        createDepthBufferImage();
        createFramebuffer();
        createCommandPool();

        createCommandBuffers();
        createTextureSampler();
        //allocateDynamicBufferTransferSpace();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createInputDescriptorSets();
        createSynchronisation();


        playerController.uboViewProjection.projection = glm::perspective(glm::radians(45.0f),
                                                                         (float) swapChainExtent.width /
                                                                         (float) swapChainExtent.height, 0.1f, 100.0f);
        playerController.uboViewProjection.view = glm::lookAt(glm::vec3(5.0f, 0.0f, 20.0f),
                                                              glm::vec3(0.0f, 0.0f, -5.0f),
                                                              glm::vec3(0.0f, 1.0f, 0.0f));

        playerController.uboViewProjection.projection[1][1] *= -1; // Flip the y-axis because Vulkan and OpenGL are different and glm was initially made for openGL

        createTexture("../textures/landscape.jpg");


    } catch (const std::runtime_error &e) {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void VulkanRenderer::getVulkanInstance() {
    initiateVulkan.getInstance(&instance);
}

void VulkanRenderer::setPlayerController(PlayerController::UboViewProjection newUboViewProjection) {
    playerController.uboViewProjection = newUboViewProjection;
}

void VulkanRenderer::getLogicalDevice() {
    initiateVulkan.getLogicalDevice(&mainDevice.logicalDevice);
}

void VulkanRenderer::getQueues() {
    initiateVulkan.getGraphicsQueue(&graphicsQueue);
    initiateVulkan.getPresentationQueue(&presentationQueue);
}

void VulkanRenderer::getSwapchain() {
    initiateVulkan.getSwapchain(&swapchain);
    initiateVulkan.getSwapchainImages(&swapChainImages);
    initiateVulkan.getSwapchainImageFormat(&swapChainImageFormat);
    initiateVulkan.getSwapchainExtent(&swapChainExtent);
    initiateVulkan.getSwapchainFramebuffers(&swapChainFramebuffers);
}


void VulkanRenderer::draw() {
    // 1. Get next available image to draw to and set something to signal when we're finished with the image (a semaphore)
    // 2. Submit command buffer to queue for execution, making sure it waits for the image to be signalled as available before drawing
    // and signals when it has finished rendering
    // 3. Present image to screen when it has signaled finished rendering

    // Wait for fences before we continue submitting to the GPU
    vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE,
                    std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

    // -- GET NEXT IMAGE --
    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(),
                          imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Update vpUniformBuffer
    recordCommands(imageIndex);
    updateUniformBuffers(imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                          // Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];               // List of semaphores to wait on
    VkPipelineStageFlags waitStages[]{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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

    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
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
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {vpLayoutBinding};
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

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create a Descriptor Set Layout wit given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor set layout
    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr,
                                         &samplerSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor Set Layout");

    // CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
    // Color input binding
    VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
    colorInputLayoutBinding.binding = 0;
    colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding.descriptorCount = 1;
    colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth input binding
    VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
    depthInputLayoutBinding.binding = 1;
    depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputLayoutBinding.descriptorCount = 1;
    depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Array of input attachment bindings
    std::vector<VkDescriptorSetLayoutBinding> inputBindings = {colorInputLayoutBinding, depthInputLayoutBinding};

    // Create a Descriptor Set Layout for input attachments
    VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
    inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
    inputLayoutCreateInfo.pBindings = inputBindings.data();

    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &inputLayoutCreateInfo, nullptr, &inputSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Descriptor Set Layout");


}


void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(mainDevice.logicalDevice);
    //free(modelTransferSpace);

    for (auto &i : modelList) {
        i.destroyMeshModel();
    }

    vkDestroyDescriptorPool(mainDevice.logicalDevice, inputDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, inputSetLayout, nullptr);

    vkDestroyDescriptorPool(mainDevice.logicalDevice, samplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, samplerSetLayout, nullptr);

    vkDestroySampler(mainDevice.logicalDevice, textureSampler, nullptr);


    for (size_t i = 0; i < textureImages.size(); ++i) {
        vkDestroyImageView(mainDevice.logicalDevice, textureImageViews[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, textureImages[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, textureImageMemory[i], nullptr);
    }

    for (size_t i = 0; i < depthBufferImage.size(); i++) {
        vkDestroyImageView(mainDevice.logicalDevice, depthBufferImageView[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, depthBufferImage[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, depthBufferImageMemory[i], nullptr);
    }

    for (size_t i = 0; i < depthBufferImage.size(); i++) {
        vkDestroyImageView(mainDevice.logicalDevice, colorBufferImageView[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, colorBufferImage[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, colorBufferImageMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);

        //vkDestroyBuffer(mainDevice.logicalDevice, modelDynamicUniformBuffer[i], nullptr);
        //vkFreeMemory(mainDevice.logicalDevice, modelDynamicUniformBufferMemory[i], nullptr);
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
    vkDestroyPipeline(mainDevice.logicalDevice, secondPipeline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, secondPipelineLayout, nullptr);

    vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

    initiateVulkan.cleanUp();
}


void VulkanRenderer::getPhysicalDevice() {
    initiateVulkan.getSelectedPhysicalDevice(&mainDevice.physicalDevice);
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


void VulkanRenderer::setDebugMessenger() {
    initiateVulkan.getSetDebugMessenger(&debugMessenger);
}

void VulkanRenderer::getSurface() {
    initiateVulkan.getSurface(&surface);
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
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    // Position Attribute
    attributeDescriptions[0].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[0].location = 0;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[0].offset = offsetof(Vertex,
                                               pos);                // We can have multiple attributes in the same data which is separated by nth index

    // Color Attribute
    attributeDescriptions[1].binding = 0;                           // Binding is a value in the shader next to location
    attributeDescriptions[1].location = 1;                          // Location that we want to bind our attributes to. Location is set in the shader aswell
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;   // 32bit RGB values which is what a vec3 is made up of, also helps define size of data
    attributeDescriptions[1].offset = offsetof(Vertex,
                                               col);                // We can have multiple attributes in the same data which is separated by nth index

    // Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);


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
    colorStateAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT |
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
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {descriptorSetLayout, samplerSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());         // Number of descriptor set layouts
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();                                   // Descriptor set layout to bind
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
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                       &graphicsPipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
    //  Destroy modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);

    // CREATE SECOND PASS PIPELINE
    // Second pass shaders
    auto secondVertexShaderCode = readFile("../shaders/second_vert.spv");
    auto secondFragmentShaderCode = readFile("../shaders/second_frag.spv");

    // Build shaders
    VkShaderModule secondVertexShaderModule = createShaderModule(secondVertexShaderCode);
    VkShaderModule secondFragmentShaderModule = createShaderModule(secondFragmentShaderCode);

    // Set new shaders
    vertexShaderCreateInfo.module = secondVertexShaderModule;
    fragmentShaderCreateInfo.module = secondFragmentShaderModule;

    VkPipelineShaderStageCreateInfo secondShaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    // No vertex data for second pass
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Dont want to write to depth buffer
    depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

    // Create new pipeline layout
    VkPipelineLayoutCreateInfo secondPipelineCreateInfo = {};
    secondPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    secondPipelineCreateInfo.setLayoutCount = 1;
    secondPipelineCreateInfo.pSetLayouts = &inputSetLayout;
    secondPipelineCreateInfo.pushConstantRangeCount = 0;
    secondPipelineCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(mainDevice.logicalDevice, &secondPipelineCreateInfo, nullptr,
                                    &secondPipelineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create second pipeline");

    pipelineCreateInfo.pStages = secondShaderStages;    // Update second shader stage list
    pipelineCreateInfo.layout = secondPipelineLayout;   // Change pipeline layout for input attachment descriptor sets
    pipelineCreateInfo.subpass = 1;                     // Use second subpass

    //  Create graphics pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                       &secondPipeline);

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create second pipeline");

    //  Destroy second shader modules, no longer needed after pipeline created
    vkDestroyShaderModule(mainDevice.logicalDevice, secondVertexShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, secondFragmentShaderModule, nullptr);

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

    // Array of our subpasses
    std::array<VkSubpassDescription, 2> subpasses{};

    // ATTACHMENTS
    // SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

    // Color Attachment (Input)
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = chooseSupportedFormat({VK_FORMAT_R8G8B8A8_UNORM}, VK_IMAGE_TILING_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //Depth Attachment (Input)
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

    // Color attachment (Input) Reference
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 1;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment (Input) Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 2;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set up subpass 1
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorAttachmentReference;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;


    // SUBPASS 2 ATTACHMENT + REFERENCES

    // SwapChain Color Attachment of render pass
    VkAttachmentDescription swapChainColorAttachment = {};
    swapChainColorAttachment.format = swapChainImageFormat;                      //Format to use for attachment
    swapChainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                    //Number of samples to write for multisampling
    swapChainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               //Start with a blank state when we draw our image, describes what to do with attachment before rendering
    swapChainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             //Describes what to do with attachment after rendering
    swapChainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    //Describes what to do with stencil before rendering
    swapChainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  //Describes what to do with stencil before rendering

    //  outdated after subpass: Framebuffer data will be stored as an image, but images can be given different data layouts
    //  to give optimal use for certain operations
    swapChainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          //Image data layout before render pass starts
    swapChainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      //Image data layout after render pass starts (to change to)

    //  Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference swapChainColorAttachmentReference = {};
    swapChainColorAttachmentReference.attachment = 0;
    swapChainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;     //Will be converted to this type after initalLayout in colorAttachment

    // References to attachments that subpass will take input from
    std::array<VkAttachmentReference, 2> inputReferences{};
    inputReferences[0].attachment = 1;
    inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputReferences[1].attachment = 2;
    inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Set up subpass 2
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &swapChainColorAttachmentReference;
    subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
    subpasses[1].pInputAttachments = inputReferences.data();


    // SUBPASS DEPENDENCIES

    //  Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 3> subpassDependencies{};

    //  Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    //  Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        //Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     //Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               //Stage access mask (memory access)

    //  But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;
    // Subpass 1 layout (Color/depth) to Subpass 2 Layout (shader read)
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstSubpass = 1;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    //  Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    //  Transition must happen after...
    subpassDependencies[2].srcSubpass = 0;
    subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[2].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //  But must happen before...
    subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[2].dependencyFlags = 0;

    // Array with attachments
    std::array<VkAttachmentDescription, 3> renderPassAttachments = {swapChainColorAttachment, colorAttachment,
                                                                    depthAttachment};

    //  Create info for Render pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassCreateInfo.pSubpasses = subpasses.data();
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

        std::array<VkImageView, 3> attachments = {swapChainImages[i].imageView, colorBufferImageView[i],
                                                  depthBufferImageView[i]
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

    std::array<VkClearValue, 3> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].color = {0.6f, 0.65f, 0.4f, 1.0f};
    clearValues[2].depthStencil.depth = 1.0f;

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

    for (size_t j = 0; j < modelList.size(); j++) {
        // Pushing PushConstants
        MeshModel thisModel = modelList[j];
        glm::mat4 tempModel = thisModel.getModel();
        vkCmdPushConstants(commandBuffers[currentImage], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(Model),                          // Size of data being pushed
                           &tempModel                               // Actual data being pushed
        );

        for (size_t k = 0; k < thisModel.getMeshCount(); k++) {

            // Bind our vertex buffer
            VkBuffer vertexBuffers[] = {thisModel.getMesh(k)->getVertexBuffer()};   // Buffers to bind
            VkDeviceSize offsets[] = {0};                                           // Offsets into buffers being bound
            vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers,
                                   offsets);     // Command to bind vertex buffer before drawing

            // Bind our mesh Index buffer using the uint32 type ONLY ONE CAN BE BINDED
            vkCmdBindIndexBuffer(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexBuffer(), 0,
                                 VK_INDEX_TYPE_UINT32);

            // Dynamic offset amount
            // uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;



            std::array<VkDescriptorSet, 2> descriptorSetGroup = {descriptorSets[currentImage],
                                                                 samplerDescriptorSets[thisModel.getMesh(
                                                                         k)->getTexId()]};

            // Bind descriptor sets
            vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                                    static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0,
                                    nullptr);

            // Execute pipeline
            vkCmdDrawIndexed(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexCount(), 1, 0, 0, 0);
        }
    }

    // Start second subpass
    vkCmdNextSubpass(commandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipeline);
    vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipelineLayout, 0, 1,
                            &inputDescriptorSets[currentImage], 0, nullptr);

    vkCmdDraw(commandBuffers[currentImage], 3, 1, 0, 0);


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
    VkDeviceSize vpBufferSize = sizeof(playerController.uboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    vpUniformBuffer.resize(swapChainImages.size());
    vpUniformBufferMemory.resize(swapChainImages.size());
    modelDynamicUniformBuffer.resize(swapChainImages.size());
    modelDynamicUniformBufferMemory.resize(swapChainImages.size());

    // Create uniform buffers
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, vpBufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i],
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

    // CREATE UNIFORM DESCRIPTOR POOL
    // Type of descriptors + how many DESCRIPTORS, not Descriptor sets (combined makes the pool size)
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

    /*
    VkDescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDynamicUniformBuffer.size());
*/
    std::vector<VkDescriptorPoolSize> poolList = {vpPoolSize,};

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

    // CREATE SAMPLER DESCRIPTOR POOL
    // Texture sampler pool
    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = MAX_OBJECTS;

    VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
    samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
    samplerPoolCreateInfo.poolSizeCount = 1;
    samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

    result = vkCreateDescriptorPool(mainDevice.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Descriptor Pool");

    // CREATE INPUT ATTACHMENT DESCRIPTOR POOL
    // One descriptor per swapchain image
    // Color Attachment pool size
    VkDescriptorPoolSize colorInputPoolSize = {};
    colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputPoolSize.descriptorCount = static_cast<uint32_t>(colorBufferImageView.size());

    // Depth attachment Pool size
    VkDescriptorPoolSize depthInputPoolSize = {};
    depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputPoolSize.descriptorCount = static_cast<uint32_t>(depthBufferImageView.size());

    std::vector<VkDescriptorPoolSize> inputPoolSizes = {colorInputPoolSize, depthInputPoolSize};

    // Create input attachment pool
    VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
    inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    inputPoolCreateInfo.maxSets = swapChainImages.size();
    inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
    inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

    result = vkCreateDescriptorPool(mainDevice.logicalDevice, &inputPoolCreateInfo, nullptr, &inputDescriptorPool);
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
        vpBufferInfo.range = sizeof(playerController.uboViewProjection);       // Size of the data that is going to be bound to the descriptor set


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
    vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0,
                sizeof(playerController.uboViewProjection), 0, &data);
    memcpy(data, &playerController.uboViewProjection, sizeof(playerController.uboViewProjection));
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
    if (modelId >= modelList.size()) return;

    modelList[modelId].setModel(newModel);
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
    depthBufferImage.resize(swapChainImages.size());
    depthBufferImageMemory.resize(swapChainImages.size());
    depthBufferImageView.resize(swapChainImages.size());

    // Get supported format for depth buffer
    VkFormat depthFormat = chooseSupportedFormat(
            {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    for (size_t i = 0; i < swapChainImages.size(); i++) {


        // Create depth Buffer Image
        depthBufferImage[i] = createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
                                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory[i]);

        // Create depth buffer image view
        depthBufferImageView[i] = createImageView(depthBufferImage[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    }

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
    const char *const res = stbi_failure_reason();
    if (!image)
        throw std::runtime_error("Failed to load a Texture file (" + fileName + ")");

    // Calculate image size using: dimmensions * channels
    *imageSize = *width * *height * 4;

    return image;
}

int VulkanRenderer::createTextureImage(std::string fileName) {
    //Load in image file
    int width, height;
    VkDeviceSize imageSize;
    stbi_uc *imageData = loadTextureFile(fileName, &width, &height, &imageSize);

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer imageStagingBuffer;
    VkDeviceMemory imageStagingBufferMemory;
    createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageStagingBuffer,
                 &imageStagingBufferMemory);

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
    texImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

    // COPY DATA TO IMAGE
    // Transition image to be DST for copy operation
    transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //Copy image data
    copyImageBuffer(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width,
                    height);
    // Transition image to be shader readable for shader usage
    transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // Add texture data to vector for reference
    textureImages.push_back(texImage);
    textureImageMemory.push_back(texImageMemory);

    // Destroy staging buffers
    vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

    //Return texture image ID
    return textureImages.size() - 1;
}

int VulkanRenderer::createTexture(std::string fileName) {
    // Create Texture Image and get its location in array
    int textureImageLoc = createTextureImage(fileName);

    // Create image view and add to list
    VkImageView imageView = createImageView(textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_ASPECT_COLOR_BIT);
    textureImageViews.push_back(imageView);

    // Create texture descriptor
    int descriptorLoc = createTextureDescriptor(imageView);

    // return location of set with texture
    return descriptorLoc;
}

void VulkanRenderer::createTextureSampler() {

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;                     // How to render when image is magnified on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;                     // How to render when image is minified on screen
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in W (z) direction, 3D textures
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;   // Set border color to black (If we have enabled border) Only works on border clamp
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;               // Whether coords should be normalized between or not
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;       // Mipmap interpolation mode
    samplerCreateInfo.mipLodBias = 0.0f;                                // Mipmap level bias (Level of detail)
    samplerCreateInfo.minLod = 0.0f;                                    // Minimum level of detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;                                    // Maximum level of Detail to pick mip level
    samplerCreateInfo.anisotropyEnable = VK_TRUE;                       // Enable Anisotropy
    samplerCreateInfo.maxAnisotropy = 16;                               // Anisotropy sample level

    VkResult result = vkCreateSampler(mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a texture sampler");


}

int VulkanRenderer::createTextureDescriptor(VkImageView textureImage) {
    VkDescriptorSet descriptorSet;

    // Descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocateInfo = {};
    setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocateInfo.descriptorPool = samplerDescriptorPool;
    setAllocateInfo.descriptorSetCount = 1;
    setAllocateInfo.pSetLayouts = &samplerSetLayout;

    // Allocate Descriptor sets
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocateInfo, &descriptorSet);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate Texture Descriptor Set");

    // Texture Image info
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;           // Image layout when in use
    imageInfo.imageView = textureImage;                                         // Image to bind to se
    imageInfo.sampler = textureSampler;                                         // Sampler to use for set

    // Descriptor Write info
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;                                 // New set so we are starting from 0
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    // Update new descriptor set
    vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

    // Add descriptor set to list
    samplerDescriptorSets.push_back(descriptorSet);

    // return descriptor set location
    return samplerDescriptorSets.size() - 1;

}

int VulkanRenderer::createMeshModel(std::string modelFile) {
    // Import model "scene"
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                        aiProcess_JoinIdenticalVertices);

    if (!scene)
        throw std::runtime_error("Failed to load model (" + modelFile + ")");

    // Get vector of all materials with 1:1 ID placement
    std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

    // Conversion from the materials list ID's to our Descriptor Array IDs
    std::vector<int> matToTex(textureNames.size());

    // Loop over textureNames and create textures for them
    for (size_t i = 0; i < textureNames.size(); i++) {
        // If material has no textures, set "0" to indicate no texture, texture 0 will be reserved for a default texture
        if (textureNames[i].empty()) {
            matToTex[i] = 0;
        } else {
            // Otherwise, create texture and set value to index of new texture
            matToTex[i] = createTexture(textureNames[i]);
        }
    }

    // Load in all our meshes
    std::vector<Mesh> modelMeshes = MeshModel::LoadNode(mainDevice.physicalDevice, mainDevice.logicalDevice,
                                                        graphicsQueue, graphicsCommandPool, scene->mRootNode, scene,
                                                        matToTex);

    // Create mesh model and add to list
    MeshModel meshModel = MeshModel(modelMeshes);
    modelList.push_back(meshModel);

    return modelList.size() - 1;
}

void VulkanRenderer::createColorBufferImage() {

    // Resize supported format for color attachment
    colorBufferImage.resize(swapChainImages.size());
    colorBufferImageMemory.resize(swapChainImages.size());
    colorBufferImageView.resize(swapChainImages.size());

    // Get supported format for color attachment
    VkFormat colorFormat = chooseSupportedFormat({VK_FORMAT_R8G8B8A8_UNORM}, VK_IMAGE_TILING_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // Create color buffer image
        colorBufferImage[i] = createImage(swapChainExtent.width, swapChainExtent.height, colorFormat,
                                          VK_IMAGE_TILING_OPTIMAL,
                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &colorBufferImageMemory[i]);

        colorBufferImageView[i] = createImageView(colorBufferImage[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::createInputDescriptorSets() {
    // Resize array to hold descriptor set for each swap chain image
    inputDescriptorSets.resize(swapChainImages.size());

    // fill array of layouts ready for set creation
    std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), inputSetLayout);

    // Input Attachment descriptor set allocation info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = inputDescriptorPool;
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    setAllocInfo.pSetLayouts = setLayouts.data();

    // Allocate Descriptor sets
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, inputDescriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate input attachment descriptor sets");

    // Update each descriptor set with input attachment
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // Color attachment descriptor
        VkDescriptorImageInfo colorAttachmentDescriptor = {};
        colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorAttachmentDescriptor.imageView = colorBufferImageView[i];
        colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

        // Color attachment descriptor write
        VkWriteDescriptorSet colorWrite = {};
        colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorWrite.dstSet = inputDescriptorSets[i];
        colorWrite.dstBinding = 0;
        colorWrite.dstArrayElement = 0;
        colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        colorWrite.descriptorCount = 1;
        colorWrite.pImageInfo = &colorAttachmentDescriptor;

        // Depth attachment descriptor
        VkDescriptorImageInfo depthAttachmentDescriptor = {};
        depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthAttachmentDescriptor.imageView = depthBufferImageView[i];
        depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

        // Depth attachment descriptor write
        VkWriteDescriptorSet depthWrite = {};
        depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depthWrite.dstSet = inputDescriptorSets[i];
        depthWrite.dstBinding = 1;
        depthWrite.dstArrayElement = 0;
        depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        depthWrite.descriptorCount = 1;
        depthWrite.pImageInfo = &depthAttachmentDescriptor;

        // List of input descriptor set writes
        std::vector<VkWriteDescriptorSet> setWrites = {colorWrite, depthWrite};
        vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0,
                               nullptr);
    }
}

PlayerController::UboViewProjection VulkanRenderer::getPlayerController() {
    return playerController.uboViewProjection;
}



#pragma clang diagnostic pop
