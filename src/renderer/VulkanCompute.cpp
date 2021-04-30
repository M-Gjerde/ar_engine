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

    setupFaceDetector();

    //int width = 1280, height = 720;
    //int width = 427, height = 370;
    int width = 640, height = 480;
    //int width = 256, height = 256;
    //int width = 1282, height = 1110;
    //int width = 450, height = 375;

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

    ArBuffer ROIInput{};
    ROIInput.bufferSize = sizeof(glm::vec4);
    ROIInput.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    ROIInput.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ROIInput.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    pBuffer->createBuffer(&ROIInput);

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
    arDescriptor.buffer.push_back(ROIInput.buffer);
    arDescriptor.bufferMemory.push_back(ROIInput.bufferMemory);
    arDescriptor.buffer.push_back(outputBuffer.buffer);
    arDescriptor.bufferMemory.push_back(outputBuffer.bufferMemory);


    // Create descriptor sets
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorCount = 4;
    std::array<uint32_t, 4> descriptorCounts = {3, 1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 4> bindings = {0, 1, 2, 0};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 4> types = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::vector<VkShaderStageFlags> stageFlags(4, VK_SHADER_STAGE_COMPUTE_BIT);
    descriptorInfo.stageFlags = stageFlags.data();
    descriptorInfo.descriptorPoolCount = 1;

    descriptorInfo.descriptorSetLayoutCount = 2;
    descriptorInfo.descriptorSetCount = 2;
    std::array<uint32_t, 4> dataSizes = {bufferSize, bufferSize, sizeof(glm::vec4), bufferSize};
    descriptorInfo.dataSizes = dataSizes.data();
    // Create descriptor set two

    pDescriptors->createDescriptors(descriptorInfo, &arDescriptor);


    computePipeline.device = arEngine.mainDevice.device;
    ArShadersPath arShaderPath;
    //arShaderPath.computeShader = "../shaders/experimental/computeShader";
    arShaderPath.computeShader = "../shaders/experimental/computeDisparity";
    pipeline.computePipeline(arDescriptor, arShaderPath, &computePipeline);


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
    vkCmdDispatch(commandBuffer, (uint32_t) 4800, (uint32_t) 1, 1);

    result = vkEndCommandBuffer(commandBuffer); // end recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffers");

    // If Vulkan setup finished retrieve pointer to memory for camera data IPC.
    memP = threadSpawner.getVideoMemoryPointer();

    ArCompute arCompute;
    arCompute.commandBuffer = commandBuffer;
    arCompute.descriptor = arDescriptor;
    return arCompute;
}

void VulkanCompute::loadImagePreviewData(ArCompute arCompute, Buffer *pBuffer) const {


    int texWidth, texHeight, texChannels;
    std::string leftImageFilePath = "../left15.png";
    std::string rightImageFilePath = "../right15.png";

    stbi_uc *imageOne = stbi_load(leftImageFilePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
    if (!imageOne) throw std::runtime_error("failed to load texture image: " + leftImageFilePath);
    stbi_uc *imageTwo = stbi_load(rightImageFilePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_grey);
    if (!imageTwo) throw std::runtime_error("failed to load texture image: " + rightImageFilePath);

    //stbi_write_png("../textures/test_input/test3/rightoutgrey.png", 1280, 720, 1, imageTwo, 1280);
    //stbi_write_png("../textures/test_input/test3/leftoutgrey.png", 1280, 720, 1, imageOne, 1280);

    //int width = 256, height = 256;
    //int width = 427, height = 370;
    //int width = 1280, height = 720;
    //int width = 1282, height = 1110;
    //int width = 450, height = 375;
    int width = 640, height = 480;

    int imageSize = width * height;

    auto *pixelValue = new glm::vec4[imageSize];
    auto *imgOnePixel = new glm::vec4[imageSize];
    auto *imgTwoPixel = new glm::vec4[imageSize];

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


    void *data;
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

int loopp = 0;

void VulkanCompute::loadComputeData(ArCompute arCompute, Buffer *pBuffer) {


    //int imageSize = memP->imgLen1 / 2;
    int imageSize = 640 * 480;
    int width = 640, height = 480;

    auto *imgOnePixel = new float[imageSize];
    auto *imgTwoPixel = new float[imageSize];

    auto origOne = imgOnePixel;
    auto origTwo = imgTwoPixel;

    auto memPixelOne = (unsigned char *) memP->imgOne;
    auto memPixelTwo = (unsigned char *) memP->imgTwo;


    cv::Mat img1(height, width, CV_8UC1);
    cv::Mat img2(height, width, CV_8UC1);

    // memcpy(imgOnePixel, memPixelOne, imageSize/sizeof(float)*sizeof(float));
    //memcpy(imgTwoPixel, memPixelTwo, imageSize/sizeof(float)*sizeof(float));

    //const auto *memPixelOne = reinterpret_cast<const float*>(memP->imgOne);

    img1.data = reinterpret_cast<uchar *>(memPixelOne);
    img2.data = memPixelTwo;

    //glm::vec4 roi(0, 0, 480, 640);

    // OpenCV Face detection

    std::vector<cv::Rect> objects;
    classifier.detectMultiScale(img1, objects, 1.1, 8);

    for (auto & object : objects) {
        cv::rectangle(img1, object, cv::Scalar(0, 255, 0));
    }
    int yMax = 0, xMax = 0;
    ROI.active = false;
    int nRoi = 50 ;// variable to increase ROI window size;
    glm::vec4 roi(0, 0, 0, 0);
    if (!objects.empty()) {
        yMax = objects[0].y + objects[0].height + nRoi;
        xMax = objects[0].x + objects[0].width + nRoi;
        roi = glm::vec4(objects[0].y - nRoi, yMax, objects[0].x - nRoi , xMax);

        ROI.y = objects[0].y -nRoi;
        ROI.x = objects[0].x -nRoi;
        ROI.width = objects[0].width + nRoi;
        ROI.height =  objects[0].height + nRoi;
        ROI.active = true;
    }

    // Put region of interest into vec4 for shader compatability

    cv::imshow("left", img1);
    //cv::imshow("right", img2);

    if (takePhoto) {
        cv::imwrite("../left" + std::to_string(loopp) + ".png", img1);
        cv::imwrite("../right" + std::to_string(loopp) + ".png", img2);
        takePhoto = false;
        loopp++;
    }
    cv::imwrite("../left.png", img1);


    for (int i = 0; i < imageSize; ++i) {
        *imgOnePixel= *memPixelOne;
        *imgTwoPixel = *memPixelTwo;

        imgTwoPixel++;
        imgOnePixel++;
        memPixelOne++;
        memPixelTwo++;
    }

    imgOnePixel = origOne;
    imgTwoPixel = origTwo;

    void *data;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0], 0, imageSize * sizeof(float), 0,
                &data);

    memcpy(data, imgOnePixel, imageSize * sizeof(float));
    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[0]);

    data = nullptr;
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1], 0, imageSize * sizeof(float), 0,
                &data);

    memcpy(data, imgTwoPixel, imageSize * sizeof(float));
    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[1]);

    data = nullptr;

    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[2], 0, sizeof(glm::vec4), 0,
                &data);

    memcpy(data, &roi, sizeof(glm::vec4));
    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[2]);

    // Cleanup
    delete[](imgOnePixel);
    delete[](imgTwoPixel);

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
    //threadSpawner.waitForExistence();

}

const ArROI &VulkanCompute::getRoi() const {
    return ROI;
}

void VulkanCompute::setupFaceDetector() {
    classifier = cv::CascadeClassifier("../user/haarcascade_frontalface_default.xml");

}
