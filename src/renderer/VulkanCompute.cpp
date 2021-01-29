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
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>


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
    //int width = 1280, height = 720;
    int width = 427, height = 370;
    //int width = 640, height = 480;
    //int width = 256, height = 256;

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
    std::array<uint32_t , 3> descriptorCounts = {2, 1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 3> bindings = {0,1, 0};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 3> types = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::vector<VkShaderStageFlags> stageFlags(3, VK_SHADER_STAGE_COMPUTE_BIT);
    descriptorInfo.stageFlags = stageFlags.data();
    descriptorInfo.descriptorPoolCount = 1;

    descriptorInfo.descriptorSetLayoutCount = 2;
    descriptorInfo.descriptorSetCount = 2;
    std::array<uint32_t , 3> dataSizes = {bufferSize, bufferSize, bufferSize};
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
    vkCmdDispatch(commandBuffer, (uint32_t) 3000, (uint32_t) 1, 1);

    result = vkEndCommandBuffer(commandBuffer); // end recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffers");

    ArCompute arCompute;
    arCompute.commandBuffer = commandBuffer;
    arCompute.descriptor = arDescriptor;
    return arCompute;
}

void VulkanCompute::loadImagePreviewData(ArCompute arCompute, Buffer *pBuffer) const{


    int texWidth, texHeight, texChannels;
    std::string filePath = "../textures/Aloe_thirdsize/view1.png";

    stbi_uc* imageOne = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
    if (!imageOne) throw std::runtime_error("failed to load texture image: view1.png");
    //filePath = "../textures/Aloe_thirdsize/view5.png";
    filePath = "../textures/Aloe_thirdsize/view5.png";
    stbi_uc* imageTwo = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
    if (!imageTwo) throw std::runtime_error("failed to load texture image: view5.png");


    //int width = 256, height = 256;
    int width = 427, height = 370;

    int imageSize = width * height;

    auto * pixelValue = new glm::vec4[imageSize];
    auto * imgOnePixel = new glm::vec4[imageSize];
    auto * imgTwoPixel = new glm::vec4[imageSize];

    auto origOne = imgOnePixel;
    auto origTwo = imgTwoPixel;

    auto orig = pixelValue;
    for (int i = 0; i < imageSize; ++i) {
        pixelValue->x = *imageOne;
        imgOnePixel->x = *imageOne;
        imgTwoPixel->x = *imageTwo;
        imageOne++;
        pixelValue++;
        imageTwo++;
        imgTwoPixel++;
        imgOnePixel++;
    }
    pixelValue = orig;
    imgOnePixel = origOne;
    imgTwoPixel = origTwo;


    void* data;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0], 0, VK_WHOLE_SIZE, 0,
                &data);

    memcpy(data, imgOnePixel, imageSize * sizeof(glm::vec4));

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0]);

    data = nullptr;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1], 0, VK_WHOLE_SIZE, 0,
                &data);

    memcpy(data, imgTwoPixel, imageSize * sizeof(glm::vec4));

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1]);

}


void VulkanCompute::loadComputeData(ArCompute arCompute, Buffer *pBuffer) {

    ArSharedMemory *memP = threadSpawner.getVideoMemoryPointer();


    int imageSize = memP->imgLen1 / 2;


    auto *imgOnePixel = new glm::vec4[256 * 256];
    auto *imgTwoPixel = new glm::vec4[256 * 256];

    auto origOne = imgOnePixel;
    auto origTwo = imgTwoPixel;

    auto memPixelOne = (uint16_t *) memP->imgOne;
    auto memPixelTwo = (uint16_t *) memP->imgTwo;


    cv::Mat img1(256, 256, CV_8UC1);
    cv::Mat img2(256, 256, CV_8UC1);


    for (int i = 0; i < imageSize; ++i) {

        int x = i % 1280;
        int y = i / 1280;

        if (x >= 512 && x < 768 && y >= 232 && y < 488){
            imgOnePixel->x = *memPixelOne;
            imgTwoPixel->x = *memPixelTwo;
            img1.at<uchar>(y - 232, x - 512) = imgOnePixel->x;
            img2.at<uchar>(y - 232, x - 512) = imgTwoPixel->x;
            imgTwoPixel++;
            imgOnePixel++;

        }

        memPixelOne++;
        memPixelTwo++;
    }
    imgOnePixel = origOne;
    imgTwoPixel = origTwo;

    cv::imshow("window", img1);
    cv::imshow("window2", img2);

    cv::imwrite("../textures/test_images/test1.png", img1);
    cv::imwrite("../textures/test_images/test2.png", img2);

    imageSize = 256 * 256;

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
    threadSpawner.waitForExistence();

}
