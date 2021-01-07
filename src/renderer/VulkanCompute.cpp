//
// Created by magnus on 1/6/21.
//

#include "VulkanCompute.h"
#include "../include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stbi_image_write.h"

#include <utility>


VulkanCompute::VulkanCompute(ArEngine mArEngine) {
    arEngine = std::move(mArEngine);
}


void VulkanCompute::cleanup() {
    // Clean up

    for (int i = 0; i < arDescriptor.buffer.size(); ++i) {
        vkFreeMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i], nullptr);
        vkDestroyBuffer(arEngine.mainDevice.device, arDescriptor.buffer[i], nullptr);

    }

    for (int i = 0; i < arDescriptor.descriptorSetLayouts.size(); ++i) {
        vkDestroyDescriptorSetLayout(arEngine.mainDevice.device, arDescriptor.descriptorSetLayouts[i], nullptr);
    }

    vkDestroyPipelineLayout(arEngine.mainDevice.device, computePipeline.pipelineLayout, nullptr);
    vkDestroyPipeline(arEngine.mainDevice.device, computePipeline.pipeline, nullptr);
    vkDestroyDescriptorPool(arEngine.mainDevice.device, arDescriptor.descriptorPool, nullptr);
    vkDestroyCommandPool(arEngine.mainDevice.device, commandPool, nullptr);

}

ArCompute VulkanCompute::setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, Platform *pPlatform,
                                              Pipeline pipeline) {

    // Create DescriptorBuffers
    ArBuffer inputBufferOne{};
    inputBufferOne.bufferSize = 1282 * 1110 * sizeof(glm::vec4);
    inputBufferOne.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    inputBufferOne.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    inputBufferOne.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&inputBufferOne);

    ArBuffer inputBufferTwo{};
    inputBufferTwo.bufferSize = 1282 * 1110 * sizeof(glm::vec4);
    inputBufferTwo.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    inputBufferTwo.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    inputBufferTwo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&inputBufferTwo);

    ArBuffer outputBuffer{};
    outputBuffer.bufferSize = 1282 * 1110 * sizeof(glm::vec4);
    outputBuffer.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    outputBuffer.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    outputBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&outputBuffer);

    // Create DescriptorSets
    std::vector<ArDescriptorInfo> inputDescriptorInfo(3);
    inputDescriptorInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputDescriptorInfo[0].descriptorCount = 1;
    inputDescriptorInfo[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputDescriptorInfo[0].binding = 0;
    inputDescriptorInfo[0].bindingCount = 1;

    inputDescriptorInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputDescriptorInfo[1].descriptorCount = 1;
    inputDescriptorInfo[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputDescriptorInfo[1].binding = 1;
    inputDescriptorInfo[1].bindingCount = 1;

    inputDescriptorInfo[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputDescriptorInfo[2].descriptorCount = 1;
    inputDescriptorInfo[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputDescriptorInfo[2].binding = 0;
    inputDescriptorInfo[2].bindingCount = 1;

    arDescriptor.descriptorSets.resize(2);
    arDescriptor.descriptorSetLayouts.resize(2);
    arDescriptor.dataSizes.resize(1);
    // Bind buffers to descriptors
    arDescriptor.buffer.push_back(inputBufferOne.buffer);
    arDescriptor.bufferMemory.push_back(inputBufferOne.bufferMemory);
    arDescriptor.buffer.push_back(inputBufferTwo.buffer);
    arDescriptor.bufferMemory.push_back(inputBufferTwo.bufferMemory);
    arDescriptor.buffer.push_back(outputBuffer.buffer);
    arDescriptor.bufferMemory.push_back(outputBuffer.bufferMemory);

    arDescriptor.dataSizes[0] = 1282 * 1110 * sizeof(glm::vec4);

    pDescriptors->createDescriptors(inputDescriptorInfo, &arDescriptor);

    computePipeline.device = arEngine.mainDevice.device;
    pipeline.computePipeline(arDescriptor.descriptorSetLayouts, ArShadersPath(), &computePipeline);

    VkCommandBuffer commandBuffer;
    pPlatform->createCommandPool(&commandPool,
                                 pPlatform->findQueueFamilies(arEngine.mainDevice.physicalDevice).computeFamily.value());


    /*
    Now allocate a command buffer from the command pool.
    */
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool; // specify the command pool to allocate from.
    // if the command buffer is primary, it can be directly submitted to queues.
    // A secondary buffer has to be called from some primary command buffer, and cannot be directly
    // submitted to a queue. To keep things simple, we use a primary command buffer.
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1; // allocate a single command buffer.
    VkResult result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &commandBufferAllocateInfo,
                                               &commandBuffer); // allocate command buffer.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");

    // ---- Begin Record Command buffers ----
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // the buffer is only submitted and used once in this application.
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo); // start recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffers");
    /*
    We need to bind a pipeline, AND a descriptor set before we dispatch.
    The validation layer will NOT give warnings if you forget these, so be very careful not to forget them.
    */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0, arDescriptor.descriptorSets.size(),
                            arDescriptor.descriptorSets.data(), 0, NULL);

    /*
    Calling vkCmdDispatch basically starts the compute pipeline, and executes the compute shader.
    The number of workgroups is specified in the arguments.
    If you are already familiar with compute shaders from OpenGL, this should be nothing new to you.
    */
    vkCmdDispatch(commandBuffer, (uint32_t) 5559, (uint32_t) 1, 1);

    result = vkEndCommandBuffer(commandBuffer); // end recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffers");

    ArCompute arCompute;
    arCompute.commandBuffer = commandBuffer;
    arCompute.descriptor = arDescriptor;
    return arCompute;
}


void VulkanCompute::loadComputeData(ArCompute arCompute) {

    // Load image using stb_image.h
    int texWidth, texHeight, texChannels;
    std::string filePath = "../textures/Aloe/view1.png";

    imageOne = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
    if (!imageOne) throw std::runtime_error("failed to load texture image: view1.png");

    int width = 1282, height = 1110;
    int imageSize = width * height;

    auto * pixelValue = new glm::vec4[imageSize];

    auto orig = pixelValue;
    for (int i = 0; i < imageSize; ++i) {
        pixelValue->x = *imageOne;
        if (i > imageSize - 10)
            printf("Pixel value: %f\n", pixelValue->x);
        imageOne++;
        pixelValue++;
    }
    pixelValue = orig;
    printf("\n");


    void* data;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0], 0, VK_WHOLE_SIZE, 0,
                &data);

    memcpy(data, pixelValue, imageSize * sizeof(glm::vec4));

    auto *copiedMem = (glm::vec4  *) data;
    for (int i = 0; i < imageSize; ++i) {
        if (i > imageSize - 10)
            printf("Pixel value: %f\n", copiedMem->x);
        copiedMem++;
    }

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0]);

}

void VulkanCompute::loadImageData() {



}
