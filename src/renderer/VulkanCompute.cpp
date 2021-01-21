//
// Created by magnus on 1/6/21.
//

#include "VulkanCompute.h"
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../include/stbi_image_write.h"
#include "../objects/BufferObject.h"

#include <utility>
#include <array>
#include <opencv2/opencv.hpp>


VulkanCompute::VulkanCompute(ArEngine mArEngine) {
    arEngine = std::move(mArEngine);
}


void VulkanCompute::cleanup() {
    // Clean up

    for (int i = 0; i < arDescriptor.buffer.size(); ++i) {
        vkFreeMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i], nullptr);
        vkDestroyBuffer(arEngine.mainDevice.device, arDescriptor.buffer[i], nullptr);

    }

    for (int i = 0; i < arDescriptor.descriptorSetLayoutCount; ++i) {
        vkDestroyDescriptorSetLayout(arEngine.mainDevice.device, arDescriptor.pDescriptorSetLayouts[i], nullptr);
    }

    vkDestroyPipelineLayout(arEngine.mainDevice.device, computePipeline.pipelineLayout, nullptr);
    vkDestroyPipeline(arEngine.mainDevice.device, computePipeline.pipeline, nullptr);

    vkDestroyDescriptorPool(arEngine.mainDevice.device, arDescriptor.pDescriptorPool, nullptr);

    vkDestroyCommandPool(arEngine.mainDevice.device, commandPool, nullptr);

}

ArCompute VulkanCompute::setupComputePipeline(Buffer *pBuffer, Descriptors *pDescriptors, Platform *pPlatform,
                                              Pipeline pipeline) {
    int width = 1280, height = 720;
    int imageSize = (width * height);
    // Buffer size
    uint32_t bufferSize = (imageSize * sizeof(glm::vec4));
    // Create DescriptorBuffers
    ArBuffer inputBufferOne{};

    inputBufferOne.bufferSize = bufferSize;
    inputBufferOne.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    inputBufferOne.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;;
    inputBufferOne.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&inputBufferOne);


    ArBuffer inputBufferTwo{};
    inputBufferTwo.bufferSize = bufferSize;
    inputBufferTwo.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    inputBufferTwo.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    inputBufferTwo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&inputBufferTwo);

    ArBuffer outputBuffer{};
    outputBuffer.bufferSize = bufferSize;
    outputBuffer.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    outputBuffer.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    outputBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&outputBuffer);

    arDescriptor.bufferObject.push_back(inputBufferOne);
    arDescriptor.buffer.push_back(inputBufferOne.buffer);
    arDescriptor.bufferMemory.push_back(inputBufferOne.bufferMemory);
    arDescriptor.buffer.push_back(inputBufferTwo.buffer);
    arDescriptor.bufferMemory.push_back(inputBufferTwo.bufferMemory);
    arDescriptor.buffer.push_back(outputBuffer.buffer);
    arDescriptor.bufferMemory.push_back(outputBuffer.bufferMemory);


    // Create descriptor sets
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorCount = 3;
    std::array<uint32_t, 3> descriptorCounts = {2, 1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 3> bindings = {0, 1, 0};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 3> types = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::vector<VkShaderStageFlags> stageFlags(3, VK_SHADER_STAGE_COMPUTE_BIT);
    descriptorInfo.stageFlags = stageFlags.data();
    descriptorInfo.descriptorPoolCount = 1;

    descriptorInfo.descriptorSetLayoutCount = 2;
    descriptorInfo.descriptorSetCount = 2;
    std::array<uint32_t, 3> dataSizes = {bufferSize, bufferSize, bufferSize};
    descriptorInfo.dataSizes = dataSizes.data();
    // Create descriptor set two

    pDescriptors->createDescriptors(descriptorInfo, &arDescriptor);


    computePipeline.device = arEngine.mainDevice.device;
    pipeline.computePipeline(arDescriptor, ArShadersPath(), &computePipeline);


    VkCommandBuffer commandBuffer;
    pPlatform->createCommandPool(&commandPool,
                                 pPlatform->findQueueFamilies(
                                         arEngine.mainDevice.physicalDevice).computeFamily.value());


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
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo); // imgOne recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffers");
    /*
    We need to bind a pipeline, AND a descriptor set before we dispatch.
    The validation layer will NOT give warnings if you forget these, so be very careful not to forget them.
    */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0,
                            arDescriptor.descriptorSets.size(),
                            arDescriptor.descriptorSets.data(), 0, NULL);

    /*
    Calling vkCmdDispatch basically starts the compute pipeline, and executes the compute shader.
    The number of workgroups is specified in the arguments.
    If you are already familiar with compute shaders from OpenGL, this should be nothing new to you.
    */
    vkCmdDispatch(commandBuffer, (uint32_t) 6500, (uint32_t) 1, 1);

    result = vkEndCommandBuffer(commandBuffer); // end recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffers");

    ArCompute arCompute;
    arCompute.commandBuffer = commandBuffer;
    arCompute.descriptor = arDescriptor;
    return arCompute;
}

void VulkanCompute::previewVideoStreams() {

    /*
    ArSharedMemory *memP = threadSpawner.readMemory();

    cv::namedWindow("window2", cv::WINDOW_FREERATIO);
    cv::namedWindow("window", cv::WINDOW_FREERATIO);


    printf("img 1 size: %zu\n", memP->imgLen1);
    printf("img 1 data: %d\n", *(uint16_t *) memP->imgOne);

    printf("img 2 size: %zu\n", memP->imgLen2);
    printf("img 2 data: %d\n", *(uint16_t *) memP->imgTwo);


    cv::Mat outputImage(720, 1280, CV_8U);
    cv::Mat outputImage2(720, 1280, CV_8U);

    size_t imgSize = 720 * 1280;
    while (true) {


        auto *d = (uint16_t *) memP->imgOne;
        auto *d2 = (uint16_t *) memP->imgTwo;
        std::vector<uchar> newPixels(imgSize);
        std::vector<uchar> newPixels2(imgSize);
        std::vector<uchar> rawPixels(imgSize);
        std::vector<uchar> rawPixels2(imgSize);

        int pixMax = 255, pixMin = 50;
        for (int j = 0; j < 720 * 1280; ++j) {
            uchar newVal = (255 - 0) / (pixMax - pixMin) * (*d - pixMax) + 255;
            uchar newVal2 = (255 - 0) / (pixMax - pixMin) * (*d2 - pixMax) + 255;
            rawPixels.at(j) = *d ;
            rawPixels2.at(j) = *d2;
            newPixels.at(j) = newVal;
            newPixels2.at(j) = newVal2;
            d++;
            d2++;
        }

        stbi_write_png("../stbpng.png", 1280, 720, 1, rawPixels.data(), 1280);
        stbi_write_png("../stbpng2.png", 1280, 720, 1, rawPixels2.data(), 1280);


        outputImage.data = newPixels.data();
        outputImage2.data = newPixels2.data();


        cv::imshow("window2", outputImage2);
        cv::imshow("window", outputImage);

        if (cv::waitKey(30) == 27) break;

    }
     */
}


void VulkanCompute::loadComputeData(ArCompute arCompute, Buffer *pBuffer) {

    ArSharedMemory *memP = threadSpawner.readMemory();

    printf("img 1 size: %zu\n", memP->imgLen1);
    printf("img 1 data: %d\n", *(uint16_t *) memP->imgOne);

    printf("img 2 size: %zu\n", memP->imgLen2);
    printf("img 2 data: %d\n", *(uint16_t *) memP->imgTwo);

    uint16_t imageSize = memP->imgLen1 / 2;


    // Load image using stb_image.h
    int texWidth, texHeight, texChannels;
    std::string filePath = "../textures/Aloe_thirdsize/view1.png";


    int width = 427, height = 370;
    //int imageSize = (width * height);
    //int width = 1282, height = 1110;



    auto *imgOnePixel = new glm::vec4[imageSize];
    auto *imgTwoPixel = new glm::vec4[imageSize];

    auto origOne = imgOnePixel;
    auto origTwo = imgTwoPixel;

    auto memPixelOne = (uint16_t *) memP->imgOne;
    auto memPixelTwo = (uint16_t *) memP->imgTwo;

    for (int i = 0; i < imageSize; ++i) {
        imgOnePixel->x = *memPixelOne;
        imgTwoPixel->x = *memPixelTwo;
        memPixelOne++;
        memPixelTwo++;
        imgOnePixel++;
        imgTwoPixel++;

    }
    imgOnePixel = origOne;
    imgTwoPixel = origTwo;

    printf("\n");


    void *data;

    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0], 0, imageSize * sizeof(glm::vec4), 0,
                &data);

    memcpy(data, imgOnePixel, imageSize * sizeof(glm::vec4));

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0]);

    data = nullptr;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1], 0, imageSize * sizeof(glm::vec4), 0,
                &data);

    memcpy(data, imgTwoPixel, imageSize * sizeof(glm::vec4));

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1]);
// TODO RESEARCH DEVICE LOCAL GPU MEMORY
/*
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferSize = imageSize * sizeof(glm::vec4);
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    stagingBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&stagingBuffer);

    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;

    pBuffer->copyBuffer(arModel, stagingBuffer, arCompute.descriptor.bufferObject[0]);

    vkFreeMemory(arEngine.mainDevice.device, stagingBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(arEngine.mainDevice.device, stagingBuffer.buffer, nullptr);

*/


}

void VulkanCompute::stopDisparityStream() {
    threadSpawner.stopChildProcess();

}

void VulkanCompute::startDisparityStream() {
    threadSpawner.startChildProcess();
}
