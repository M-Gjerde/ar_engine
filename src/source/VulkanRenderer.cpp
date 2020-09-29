#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by magnus on 7/4/20.
//
#define STB_IMAGE_IMPLEMENTATION


#include "../headers/VulkanRenderer.hpp"
#include "../libs/Descriptors.h"


VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;

int VulkanRenderer::init(GLFWwindow *newWindow) {
    window = newWindow;
    try {
        // Static init
        vfd.init(window);

        getVulkanInstance();
        getSurface();
        getPhysicalDevice();
        setDebugMessenger();
        getLogicalDevice();
        getQueues();
        getSwapchain();
        getRenderPass();
        getDescriptorSetLayout();
        getPushConstantRange();
        getGraphicsPipeline();

        // Get descriptors
        descriptors = new Descriptors(mainDevice);


        createColorBufferImage();
        createDepthBufferImage();
        createTriangle();

        createFramebuffer();
        getCommandPool();
        getCommandBuffers();

        getTextureSampler();
        //allocateDynamicBufferTransferSpace();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createInputDescriptorSets();
        createSynchronisation();


        triangleModel.projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
        triangleModel.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        triangleModel.model = glm::mat4(1.0f);

        triangleModel.projection[1][1] *= -1;

        // Create a mesh
        // Vertex Data
        std::vector<TriangleVertex> triangleVertices = {
                { { -0.1, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
                { { -0.1, 0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
                { { -0.9, 0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
        };

        Mesh triangleMesh = Mesh(mainDevice, graphicsQueue, graphicsCommandPool, &triangleVertices);

        meshList.push_back(triangleMesh);



        createTexture("../textures/landscape.jpg");

    } catch (const std::runtime_error &e) {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    VkAllocationCallbacks tempAllocator = vfd.getAllocator(); // Cannot take reference of temporary object, used for passing ref to pAllocator in vk commands

    vkDeviceWaitIdle(mainDevice.logicalDevice);
    //free(modelTransferSpace);

    for (auto &i : modelList) {
        i.destroyMeshModel();
    }

    descriptors->clean(triangle);

    for (int i = 0; i < meshList.size(); ++i) {
        meshList[i].destroyBuffers();
    }

    vkDestroyDescriptorPool(mainDevice.logicalDevice, inputDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, inputSetLayout, nullptr);

    vkDestroyDescriptorPool(mainDevice.logicalDevice, samplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, samplerSetLayout, nullptr);

    vkDestroySampler(mainDevice.logicalDevice, textureSampler, &tempAllocator);

    for (int i = 0; i < swapChainImages.size(); ++i) {
        vkFreeMemory(mainDevice.logicalDevice, triangle.bufferMemory[i], nullptr);
        vkDestroyBuffer(mainDevice.logicalDevice, triangle.buffer[i], nullptr);
    }


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

    for (size_t i = 0; i < Utils::MAX_FRAME_DRAWS; ++i) {
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
    }

    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(mainDevice.logicalDevice, secondPipeline.pipeline, &vkAllocationCallbacks);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, secondPipeline.pipelineLayout, &vkAllocationCallbacks);

    vkDestroyPipeline(mainDevice.logicalDevice, modelPipeline.pipeline, &vkAllocationCallbacks);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, modelPipeline.pipelineLayout, &vkAllocationCallbacks);
    vkDestroyRenderPass(mainDevice.logicalDevice, modelPipeline.renderPass, &vkAllocationCallbacks);

    vkDestroyPipeline(mainDevice.logicalDevice, boxPipeline.pipeline, &vkAllocationCallbacks);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, boxPipeline.pipelineLayout, &vkAllocationCallbacks);
    vkDestroyRenderPass(mainDevice.logicalDevice, boxPipeline.renderPass, &vkAllocationCallbacks);

    vfd.cleanUp();
}

void VulkanRenderer::getVulkanInstance() {
    vfd.getInstance(&instance);
}

void VulkanRenderer::getPhysicalDevice() {
    vfd.getSelectedPhysicalDevice(&mainDevice.physicalDevice);
}

void VulkanRenderer::getLogicalDevice() {
    vfd.getLogicalDevice(&mainDevice.logicalDevice);
}

void VulkanRenderer::setDebugMessenger() {
    vfd.getSetDebugMessenger(&debugMessenger);
}

void VulkanRenderer::getSurface() {
    vfd.getSurface(&surface);
}

void VulkanRenderer::getQueues() {
    vfd.getGraphicsQueue(&graphicsQueue);
    vfd.getPresentationQueue(&presentationQueue);
}

void VulkanRenderer::getSwapchain() {
    vfd.getSwapchain(&swapchain);
    vfd.getSwapchainImages(&swapChainImages);
    vfd.getSwapchainImageFormat(&swapChainImageFormat);
    vfd.getSwapchainExtent(&swapChainExtent);
    vfd.getSwapchainFramebuffers(&swapChainFramebuffers);
}

void VulkanRenderer::getCommandPool() {
    vfd.getCommandPool(&graphicsCommandPool);
}

void VulkanRenderer::getCommandBuffers() {
    //  Resize command buffer count to have one for each framebuffer
    commandBuffers.resize(swapChainFramebuffers.size());
    for (int i = 0; i < swapChainFramebuffers.size(); ++i) {
        vfd.getCommandBuffer(&commandBuffers[i], graphicsCommandPool);
    }

}

void VulkanRenderer::getRenderPass() {
    myPipe.createRenderPass(mainDevice, swapChainImageFormat, &modelPipeline);

}

void VulkanRenderer::getGraphicsPipeline() {
    myPipe.createGraphicsPipeline(mainDevice, swapChainExtent, &modelPipeline, &secondPipeline);

}

void VulkanRenderer::setCamera(Camera newCamera) {
    camera = newCamera;
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
        framebufferCreateInfo.renderPass = modelPipeline.renderPass;                                          //  Render pass layout the framebuffer will be sued with
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



void VulkanRenderer::createSynchronisation() {
    imageAvailable.resize(Utils::MAX_FRAME_DRAWS);
    renderFinished.resize(Utils::MAX_FRAME_DRAWS);
    drawFences.resize(Utils::MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;       // Fence will start open flag

    for (size_t i = 0; i < Utils::MAX_FRAME_DRAWS; i++) {
        if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) !=
            VK_SUCCESS ||
            vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)

            throw std::runtime_error("Failed to create Semaphore(s)");
    }
}

void VulkanRenderer::getDescriptorSetLayout() {
    myPipe.getDescriptorSetLayout(mainDevice, &descriptorSetLayout, &samplerSetLayout, &inputSetLayout);

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
    std::vector <VkDescriptorPoolSize> poolList = {vpPoolSize};

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
    samplerPoolSize.descriptorCount = Utils::MAX_OBJECTS;

    VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
    samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolCreateInfo.maxSets = Utils::MAX_OBJECTS;
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

    std::vector <VkDescriptorPoolSize> inputPoolSizes = {colorInputPoolSize, depthInputPoolSize};

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
        vpBufferInfo.range = sizeof(Camera::UboViewProjection);       // Size of the data that is going to be bound to the descriptor set


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

void VulkanRenderer::createInputDescriptorSets() {
    // Resize array to hold descriptor set for each swap chain image
    inputDescriptorSets.resize(swapChainImages.size());

    // fill array of layouts ready for set creation
    std::vector <VkDescriptorSetLayout> setLayouts(swapChainImages.size(), inputSetLayout);

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
        std::vector <VkWriteDescriptorSet> setWrites = {colorWrite, depthWrite};
        vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0,
                               nullptr);
    }
}

void VulkanRenderer::createUniformBuffers() {
    // ViewProjection buffer size (Will offset access)
    VkDeviceSize vpBufferSize = sizeof(Camera::UboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    vpUniformBuffer.resize(swapChainImages.size());
    vpUniformBufferMemory.resize(swapChainImages.size());
    modelDynamicUniformBuffer.resize(swapChainImages.size());
    modelDynamicUniformBufferMemory.resize(swapChainImages.size());

    // Create uniform buffers
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        Utils::createBuffer(mainDevice, vpBufferSize,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

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

void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex) {
    // Copy VP data
    void *data;
    if (camera.viewProjectionEnable) {
        vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(Camera::UboViewProjection),
                    0, &data);
        memcpy(data, &camera.uboViewProjection, sizeof(Camera::UboViewProjection));
        vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);
    }

    uint32_t size = sizeof(TriangleModel);

    void* data2;
    vkMapMemory(mainDevice.logicalDevice, triangle.bufferMemory[imageIndex], 0, size, 0, &data2);
    memcpy(data2, &triangleModel, (size_t) size);
    vkUnmapMemory(mainDevice.logicalDevice, triangle.bufferMemory[imageIndex]);



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


void VulkanRenderer::getPushConstantRange() {
    myPipe.getPushConstantRange(&pushConstantRange);
}



VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling,
                                               VkFormatFeatureFlags featureFlags) const {
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

int VulkanRenderer::createTexture(std::string fileName) {
    // Create Texture Image and get its location in array
    int textureImageLoc = TextureLoading::getTextureImageLoc(mainDevice, fileName, graphicsQueue, graphicsCommandPool,
                                                             &textureImages, &textureImageMemory);
    // Create image view and add to list
    VkImageView imageView = Utils::createImageView(mainDevice, textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM,
                                                   VK_IMAGE_ASPECT_COLOR_BIT);
    textureImageViews.push_back(imageView);

    // Create texture descriptor
    int descriptorLoc = textureLoading.createTextureDescriptor(mainDevice, imageView, samplerDescriptorPool,
                                                               samplerSetLayout, &samplerDescriptorSets);
    // return location of set with texture
    return descriptorLoc;
}

void VulkanRenderer::getTextureSampler() {
    textureLoading.getSampler(mainDevice, &textureSampler, vfd.getAllocator());

}

int VulkanRenderer::createMeshModel(std::string modelFile) {
    // Import model "scene"
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                        aiProcess_JoinIdenticalVertices);

    if (!scene)
        throw std::runtime_error("Failed to load model (" + modelFile + ")");


    // Get vector of all materials with 1:1 ID placement
    std::vector <std::string> textureNames = MeshModel::LoadMaterials(scene);


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
    std::vector <Mesh> modelMeshes = MeshModel::LoadNode(mainDevice,
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
        colorBufferImage[i] = Utils::createImage(mainDevice, swapChainExtent.width, swapChainExtent.height, colorFormat,
                                                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &colorBufferImageMemory[i]);

        colorBufferImageView[i] = Utils::createImageView(mainDevice, colorBufferImage[i], colorFormat,
                                                         VK_IMAGE_ASPECT_COLOR_BIT);
    }
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
        depthBufferImage[i] = Utils::createImage(mainDevice, swapChainExtent.width, swapChainExtent.height, depthFormat,
                                                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory[i]);

        // Create depth buffer image view
        depthBufferImageView[i] = Utils::createImageView(mainDevice, depthBufferImage[i], depthFormat,
                                                         VK_IMAGE_ASPECT_DEPTH_BIT);

    }

}



void VulkanRenderer::createTriangle() {

    triangle.buffer.resize(swapChainImages.size());
    triangle.bufferMemory.resize(swapChainImages.size());
    triangle.descriptorSets.resize(swapChainImages.size());

    uint32_t size = sizeof(TriangleModel);


    for (int i = 0; i < swapChainImages.size(); ++i) {


        Utils::createBuffer(mainDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &triangle.buffer[i],
                            &triangle.bufferMemory[i]);
    }

    // Create Descriptor sets
    Descriptors triangleDescriptors(mainDevice);

    triangleDescriptors.createDescriptorSetLayout(&triangle.descriptorSetLayout);

    //myPipe.createAnotherRenderPass(mainDevice, swapChainImageFormat, &boxPipeline);
    // retrieve render pass
    myPipe.createRenderPass(mainDevice, swapChainImageFormat, &boxPipeline);
    // retrieve graphics pipeline
    myPipe.createBoxPipeline(mainDevice, swapChainExtent, triangle.descriptorSetLayout, &boxPipeline);


    // Get descriptor pool
    triangleDescriptors.createDescriptorSetPool(&triangle.descriptorPool, swapChainImages.size());

    // Get descriptor sets
    for (int i = 0; i < swapChainImages.size(); ++i) {
        triangleDescriptors.createDescriptorSet(triangle.descriptorSetLayout, triangle.descriptorPool, triangle.buffer[i], &triangle.descriptorSets[i], sizeof(TriangleModel));
    }
}

void VulkanRenderer::updateTriangle(glm::mat4 newModel) {
        triangleModel.model = newModel;
}

void VulkanRenderer::updateViewTriangle(glm::mat4 newModel) {
    triangleModel.view = newModel;
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
    currentFrame = (currentFrame + 1) % Utils::MAX_FRAME_DRAWS;
}

void VulkanRenderer::recordCommands(uint32_t currentImage) {
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;     // The command buffer can be submitted while it is awaiting execution Yes it can


    // Start recording commands to command buffer
    VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to start recording a Command Buffer");


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = modelPipeline.renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[currentImage];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 3> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].color = {0.6f, 0.65f, 0.4f, 1.0f};
    clearValues[2].depthStencil.depth = 1.0f;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, boxPipeline.pipeline);

    for (int i = 0; i < meshList.size(); ++i) {

        VkBuffer vertexBuffer[] = {meshList[i].getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffer, offsets);


        vkCmdBindDescriptorSets(commandBuffers[currentImage],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                boxPipeline.pipelineLayout,
                                0,
                                1,
                                &triangle.descriptorSets[currentImage],
                                0,
                                nullptr);


        vkCmdDraw(commandBuffers[currentImage], static_cast<uint32_t>(meshList[i].getVertexCount()), 1, 0, 0);

    }

    // Bind Pipeline to be used in render pass
    vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipeline);

    for (size_t j = 0; j < modelList.size(); j++) {
        // Pushing PushConstants
        MeshModel thisModel = modelList[j];
        glm::mat4 tempModel = thisModel.getModel();
        vkCmdPushConstants(commandBuffers[currentImage], modelPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
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
            vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipelineLayout, 0,
                                    static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0,
                                    nullptr);

            // Execute pipeline
            vkCmdDrawIndexed(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexCount(), 1, 0, 0, 0);
        }
    }

    // Subpass 2
    vkCmdNextSubpass(commandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipeline.pipelineLayout, 0, 1,
                            &inputDescriptorSets[currentImage], 0, nullptr);

    vkCmdDraw(commandBuffers[currentImage], 3, 1, 0, 0);



    vkCmdEndRenderPass(commandBuffers[currentImage]);

    // Stop recording to command buffer
    result = vkEndCommandBuffer(commandBuffers[currentImage]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end recording a Command Buffer");

}

#pragma clang diagnostic pop
