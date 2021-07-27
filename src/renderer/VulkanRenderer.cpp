//
// Created by magnus on 7/4/20.
//




#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include "VulkanRenderer.hpp"
#include "../include/stbi_image_write.h"
#include "opencv2/opencv.hpp"
#include "../include/helper_functions.h"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice); // TODO Remove this. Put it to use in abstract classes
        descriptors = new Descriptors(arEngine); // TODO Remove this. Put it to use in abstract classes
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);

        textures = new Textures(images);
        vulkanCompute = new VulkanCompute(arEngine);

        createFrameBuffersAndRenderPass();


        createCommandBuffers();
        createSyncObjects();
        printf("Initiated vulkan\n");

        initComputePipeline();

        printf("Initiated compute pipeline\n\n");
        textRenderTest();

    } catch (std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arEngine.mainDevice.device); // wait for GPU to finish rendering before we clean up resources

    vulkanCompute->cleanup();

    images->cleanUp(); // Clean up depth images

    for (auto &object : objects) {
        object.cleanUp(arEngine.mainDevice.device);
    }

    // Clean descriptor sets
    for (auto &arDescriptor : arDescriptors) {
        descriptors->cleanUp(arDescriptor);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(arEngine.mainDevice.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(arEngine.mainDevice.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(arEngine.mainDevice.device, inFlightFences[i], nullptr);

    }

    vkDestroyRenderPass(arEngine.mainDevice.device, renderPass, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(arEngine.mainDevice.device, framebuffer, nullptr);
    }

    for (auto &arPipeline : arPipelines) {
        pipeline.cleanUp(arPipeline);

    }
    platform->cleanUp();
}


void VulkanRenderer::draw() {
    // 1. Get next available image to draw to and set something to signal when we're finished with the image (a semaphore)
    // 2. Submit command buffer to queue for execution, making sure it waits for the image to be signalled as available before drawing
    // and signals when it has finished rendering
    // 3. Present image to screen when it has signaled finished rendering

    vkWaitForFences(arEngine.mainDevice.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


    uint32_t imageIndex;
    vkAcquireNextImageKHR(arEngine.mainDevice.device, arEngine.swapchain, UINT64_MAX,
                          imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(arEngine.mainDevice.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // TODO check if this should be done different
    updateBuffer(imageIndex);
    if (updateDisparity)
        updateDisparityData();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // TEXTOVERLAY ADDITION cmd buffers
    std::vector<VkCommandBuffer> cmdBuffers = {
    };
    cmdBuffers.push_back(commandBuffers[imageIndex]);

    if (visible){
        cmdBuffers.push_back(cmdBuffersText[imageIndex]);
    }

    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    submitInfo.pCommandBuffers = cmdBuffers.data();

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(arEngine.mainDevice.device, 1, &inFlightFences[currentFrame]);

    VkResult result = vkQueueSubmit(arEngine.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (result != VK_SUCCESS) {
        printf("result: %d\n", result);
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {arEngine.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(arEngine.presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void VulkanRenderer::createFrameBuffersAndRenderPass() {
    // Use pipeline helper class to create RenderPass
    pipeline.createRenderPass(arEngine.mainDevice.device, images->findDepthFormat(), arEngine.swapchainFormat,
                              &renderPass);

    // Create depth image view for use in framebuffer --> Probably wont touch it again so it's fine its here..
    images->createDepthImageView(&arDepthResource.depthImageView);

    // Create FrameBuffers
    swapChainFramebuffers.resize(arEngine.swapchainImages.size());
    for (size_t i = 0; i < arEngine.swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {arEngine.swapChainImageViews[i], arDepthResource.depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = arEngine.swapchainExtent.width;
        framebufferInfo.height = arEngine.swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(arEngine.mainDevice.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = arEngine.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(arEngine.mainDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // TEXTRENDER ALSO ALLOCATE BUFFERS

    VkCommandBufferAllocateInfo allocInfo2{};
    cmdBuffersText.resize(swapChainFramebuffers.size());
    allocInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo2.commandPool = arEngine.commandPool;
    allocInfo2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo2.commandBufferCount = (uint32_t) cmdBuffersText.size();

    if (vkAllocateCommandBuffers(arEngine.mainDevice.device, &allocInfo, cmdBuffersText.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

}

void VulkanRenderer::recordCommand() {


    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = arEngine.swapchainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.10f, 0.35f, 0.15f, 1.0f}; // gray
        //clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};      // Black

        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (int j = 0; j < objects.size(); ++j) {
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipelines[j].pipeline);

            VkBuffer vertexBuffers[] = {objects[j].getVertexBuffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], objects[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    arPipelines[j].pipelineLayout, 0, arDescriptors[j].descriptorSets.size(),
                                    arDescriptors[j].descriptorSets.data(), 0,
                                    nullptr);

            vkCmdDrawIndexed(commandBuffers[i], objects[j].getIndexCount(), 1, 0, 0, 0);

        }

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to FaceAugment command buffer!");
        }
    }

}

void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(arEngine.swapchainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(arEngine.mainDevice.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(arEngine.mainDevice.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateFence(arEngine.mainDevice.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanRenderer::updateBuffer(uint32_t imageIndex) {
    for (int i = 0; i < objects.size(); ++i) {

        uboModelVar.model = objects[i].getModel();
        // Copy vertex data
        void *data;
        vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0], 0, arDescriptors[i].dataSizes[0],
                    0, &data);
        memcpy(data, &uboModelVar, arDescriptors[i].dataSizes[0]);
        vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0]);

        // If current model has two buffers (e.g. fragment shader input then continue)
        if (arDescriptors[i].bufferMemory.size() == 2) {
            void *data2;
            vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1], 0, arDescriptors[i].dataSizes[1],
                        0, &data2);
            memcpy(data2, &fragmentColor, arDescriptors[i].dataSizes[1]);
            vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1]);
        }
    }
}


void VulkanRenderer::updateCamera(glm::mat4 newView, glm::mat4 newProjection) {
    uboModelVar.view = newView;
    uboModelVar.projection = newProjection;
}


void VulkanRenderer::updateTextureImage(std::string fileName) {

    //textures->setDisparityImageTexture(disparity, &disparityTexture, &disparityTextureBuffer);
    descriptors->createDescriptorsSampler(&arDescriptors[0], disparityTexture);
    createCommandBuffers();

}

void VulkanRenderer::updateDisparityVideoTexture() {

    // Skip updating if pixels are not Ready optional: add a timeout function to this
    /*if (!disparity->pixelDataReady) {
        return;
    }
    //clock_t Start = clock();
    textures->setDisparityVideoTexture(disparity, &videoTexture, &videoTextureBuffer);

    descriptors->createDescriptorsSampler(&arDescriptors[3], videoTexture);
    recordCommand();
     */

    // printf("Texture and re-FaceAugment cmd buffers time taken: %.7fs\n", (double) (clock() - Start) / CLOCKS_PER_SEC;);
}

void VulkanRenderer::setupSceneFromFile(std::vector<std::map<std::string, std::string>> modelSettings) {
// Load in each model
    for (int i = 0; i < modelSettings.size(); ++i) {
        SceneObject object(modelSettings[i], arEngine);
        object.createPipeline(renderPass);

        // Push objects to global lists
        arDescriptors.push_back(object.getArDescriptor());
        arPipelines.push_back(object.getArPipeline());
        objects.push_back(object);

        if (objects[i].isLight()) {
            glm::mat4 newModel = objects[i].getModel();
            fragmentColor.lightPos = glm::vec4(newModel[3].x, newModel[3].y, newModel[3].z, 1.0f);
            fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);
        }

    }
    recordCommand();
}


void VulkanRenderer::initComputePipeline() {
    vulkanCompute->setupComputePipeline(buffer, descriptors, platform, pipeline);
    memP = threadSpawner.getVideoMemoryPointer();

}


void VulkanRenderer::updateDisparityData() {

    cv::Mat img = cv::imread("../left.jpg",
                             cv::IMREAD_GRAYSCALE);     // Imread a dummy image to populate the cv::Mat img object
    img.data = reinterpret_cast<uchar *>((unsigned char *) memP->imgTwo);
    cv::cvtColor(img, img,
                 cv::COLOR_GRAY2BGR);                       // cvtColor to make img a valid input for OpenCv's DNN caffe implementation
    faceDetector.detectFaceRegion(img);
    cv::Rect rect = faceDetector.getRoiFace();

    glm::vec4 roi(rect.y, rect.y + rect.height, rect.x, rect.x + rect.width);
    vulkanCompute->loadComputeData(roi,
                                   memP);                                           // Load stereo images into GPU. also copy one to be used as face recognition


    vulkanCompute->executeComputeCommandBuffer();
    //vulkanCompute->loadImagePreviewData(arCompute, buffer);
    cv::Mat disparityImg = vulkanCompute->readComputeResult();



    // -- TRANSLATION --
    std::vector<glm::vec3> pointPositions;
    disparityImg.convertTo(disparityImg, CV_8UC1, 255);
    //cv::imwrite("../disparity_output.png", disparityImg);
    int avgDisp = 0;
    int elements = 0;
    for (int i = 0; i < disparityImg.cols; ++i) {
        for (int j = 0; j < disparityImg.rows; ++j) {
            int pixel = disparityImg.at<uchar>(i, j);
            if (pixel > 0 && pixel < 200) {
                avgDisp += disparityImg.at<uchar>(i, j);
                elements++;
            }
        }
    }

    float disparity = (float) avgDisp / (float) elements;
    printf("Disparity: %f\n", disparity);
    glm::vec3 position;
    disparityToPoint(disparity, &position, faceDetector.getNosePoint2d());
    position *= glm::vec3(0, 0, 10);
    position -= glm::vec3(0, 0, 5);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    printf("Pos: %f %f %f\n", position.x, position.y, position.z);

    // -- ROTATION --
    glm::vec3 rot = faceDetector.getFaceRotVector();
    //printf("Rot: %f %f %f, length: %f\n", rot.x, rot.y, rot.z, glm::length(rot));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));

    //model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
    //model = glm::rotate(model, (glm::length(rot.y) * 2), glm::vec3(0, 1, 0)); // Yaw
    //model = glm::rotate(model, glm::length(rot.x) + 0.2f, glm::vec3(1, 0, 0));   // Pitch
    //model = glm::rotate(model, (glm::length(rot.z)), glm::vec3(0, 0, 1)); // setRoll
    float angle = glm::length(rot);
    model = glm::rotate(model, -angle, rot);

    if (glm::length(rot) < 2) {
        objects[0].setModel(model);
    }

    if (cv::waitKey(1) == 27) {
        updateDisparity = false;
        cv::destroyAllWindows();
    }

    /*
    vulkanCompute->loadComputeData(arCompute, buffer);
    vulkanComputeShaders();
    cv::waitKey(0);
*/
}

void VulkanRenderer::textRenderTest() {

    const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
    const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

    static unsigned char font24pixels[fontWidth][fontHeight];
    stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);

    dataBuffer.bufferSize = 2048 * sizeof(glm::vec4);
    dataBuffer.bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    dataBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    dataBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer->createBuffer(&dataBuffer);


    images->createImage(fontWidth, fontHeight, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory, &memoryRequirements);


    // Staging
    ArBuffer stagingBuffer{};
    stagingBuffer.bufferSize = memoryRequirements.size;
    stagingBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    stagingBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    buffer->createBuffer(&stagingBuffer);
    uint8_t *data;
    vkMapMemory(arEngine.mainDevice.device, stagingBuffer.bufferMemory, 0, stagingBuffer.bufferSize, 0,
                (void **) &data);
    memcpy(data, &font24pixels[0][0], fontWidth * fontHeight);
    vkUnmapMemory(arEngine.mainDevice.device, stagingBuffer.bufferMemory);

    // Transition image to transfer destination
    images->transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arEngine.commandPool, arEngine.graphicsQueue);

    // Copy buffer to image
    images->copyBufferToImage(stagingBuffer.buffer, image, fontWidth, fontHeight, arEngine.commandPool,
                              arEngine.graphicsQueue);


    // Transition image to shader read
    images->transitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, arEngine.commandPool,
                                  arEngine.graphicsQueue);

    // Create image view
    imageView = images->createImageView(image, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);


    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(arEngine.mainDevice.device, &samplerInfo, nullptr, &sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    // Create descriptor
    // Font uses a separate descriptor pool
    descriptorInfo.descriptorPoolCount = 1;
    descriptorInfo.descriptorCount = 1;
    descriptorInfo.descriptorSetLayoutCount = 1;
    descriptorInfo.descriptorSetCount = 1;
    std::vector<uint32_t> descriptorCounts;
    descriptorCounts.push_back(1);
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::vector<uint32_t> bindings;
    bindings.push_back(0);
    descriptorInfo.pBindings = bindings.data();
    // types
    std::vector<VkDescriptorType> types(1);
    types[0] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorInfo.pDescriptorType = types.data();
    // stages
    std::array<VkShaderStageFlags, 1> stageFlags = {VK_SHADER_STAGE_FRAGMENT_BIT};
    descriptorInfo.stageFlags = stageFlags.data();

    descriptorInfo.sampler = sampler;
    descriptorInfo.view = imageView;

    descriptors->createDescriptors(descriptorInfo, &descriptor);

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(arEngine.mainDevice.device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);


    /// --- RENDER PASS ---
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = arEngine.swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Setup to how the colorAttachment is referenced by shader
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = images->findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachmentDescriptions = {colorAttachment, depthAttachment};

    VkSubpassDependency subpassDependencies[2] = {};

    // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Transition from initial to final
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.flags = 0;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentRef;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = subpassDependencies;

    vkCreateRenderPass(arEngine.mainDevice.device, &renderPassInfo, nullptr, &textRenderPass);



    // Create PIPELINE
    ArShadersPath shadersPath{};
    shadersPath.vertexShader = "../shaders/textoverlay/textVert";
    shadersPath.fragmentShader = "../shaders/textoverlay/textFrag";

    textPipeline.device = arEngine.mainDevice.device;
    textPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    textPipeline.swapchainExtent = arEngine.swapchainExtent;
    pipeline.textRenderPipeline(textRenderPass, descriptor, shadersPath, &textPipeline, pipelineCache);


    mapped = nullptr;
    vkMapMemory(arEngine.mainDevice.device, dataBuffer.bufferMemory, 0, VK_WHOLE_SIZE, 0, (void **) &mapped);


    addText("Magnus - Vulkan Text render test", 640.0f, 360.0f, alignCenter);

    //vkUnmapMemory(arEngine.mainDevice.device, dataBuffer.bufferMemory);
    updateCommandBuffers();


}


// Add text to the current buffer
// todo : drop shadow? color attribute?
void VulkanRenderer::addText(const std::string& text, float x, float y, TextAlign align) {
    numLetters = 0;

    const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;
    assert(mapped != nullptr);

    const float charW = 1.5f / (float) viewport.WIDTH;
    const float charH = 1.5f / (float) viewport.HEIGHT;

    float fbW = (float) viewport.WIDTH;
    float fbH = (float) viewport.HEIGHT;
    x = (x / fbW * 2.0f) - 1.0f;
    y = (y / fbH * 2.0f) - 1.0f;

    // Calculate text width
    float textWidth = 0;
    for (auto letter : text) {
        stb_fontchar *charData = &stbFontData[(uint32_t) letter - firstChar];
        textWidth += charData->advance * charW;
    }

    switch (align) {
        case alignRight:
            x -= textWidth;
            break;
        case alignCenter:
            x -= textWidth / 2.0f;
            break;
    }

    // Generate a uv mapped quad per char in the new text
    for (auto letter : text) {
        stb_fontchar *charData = &stbFontData[(uint32_t)letter - firstChar];

        mapped->x = (x + (float) charData->x0 * charW);
        mapped->y = (y + (float) charData->y0 * charH);
        mapped->z = charData->s0;
        mapped->w = charData->t0;
        std::cout << mapped->x << " " << mapped->y << std::endl;
        mapped++;

        mapped->x = (x + (float) charData->x1 * charW);
        mapped->y = (y + (float) charData->y0 * charH);
        mapped->z = charData->s1;
        mapped->w = charData->t0;
        std::cout << mapped->x << " " << mapped->y  << std::endl;
        mapped++;

        mapped->x = (x + (float) charData->x0 * charW);
        mapped->y = (y + (float) charData->y1 * charH);
        mapped->z = charData->s0;
        mapped->w = charData->t1;
        std::cout << mapped->x << " " << mapped->y << std::endl;
        mapped++;

        mapped->x = (x + (float) charData->x1 * charW);
        mapped->y = (y + (float) charData->y1 * charH);
        mapped->z = charData->s1;
        mapped->w = charData->t1;
        std::cout << mapped->x << " " << mapped->y << "\n" << std::endl;
        mapped++;

        x += charData->advance * charW;

        numLetters++;
    }


}

void VulkanRenderer::updateCommandBuffers() {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkClearValue clearValues[2];
    clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = textRenderPass;
    renderPassBeginInfo.renderArea.extent.width = viewport.WIDTH;
    renderPassBeginInfo.renderArea.extent.height = viewport.HEIGHT;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < cmdBuffersText.size(); ++i) {
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];

        vkBeginCommandBuffer(cmdBuffersText[i], &beginInfo);

        vkCmdBeginRenderPass(cmdBuffersText[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmdBuffersText[i], VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline.pipeline);
        vkCmdBindDescriptorSets(cmdBuffersText[i], VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline.pipelineLayout, 0, 1, &descriptor.descriptorSets[0], 0,
                                NULL);

        VkDeviceSize offsets = 0;
        vkCmdBindVertexBuffers(cmdBuffersText[i], 0, 1, &dataBuffer.buffer, &offsets);
        vkCmdBindVertexBuffers(cmdBuffersText[i], 1, 1, &dataBuffer.buffer, &offsets);
        for (uint32_t j = 0; j < numLetters; j++) {
            vkCmdDraw(cmdBuffersText[i], 4, 1, j * 4, 0);        }


        vkCmdEndRenderPass(cmdBuffersText[i]);
        vkEndCommandBuffer(cmdBuffersText[i]);
    }
    visible = true;
}

void VulkanRenderer::testFunction() {
    float xyScale = 2.1;
    float zScale = 5;
    float scale = 5;

    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            SceneObject object(arEngine);
            object.createPipeline(renderPass);
            // Push objects to global lists
            arDescriptors.push_back(object.getArDescriptor());
            arPipelines.push_back(object.getArPipeline());

            glm::mat4 model = object.getModel();
            model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));

            glm::vec3 noiseCoordinates((float) j * xyScale, (float) i * xyScale, 0.075f * zScale);
            float noiseValue = -abs(getNoise(noiseCoordinates)) * scale;
            std::cout << noiseValue << std::endl;
            model = glm::translate(model, glm::vec3((float) j * 2, noiseValue, (float) i * 2));
            object.setModel(model);

            objects.push_back(object);
        }
    }

    recordCommand(); // TODO do in separate thread in order to speed up application

}

std::vector<SceneObject> VulkanRenderer::getSceneObjects() const {
    return objects;
}

void VulkanRenderer::startDisparityStream() {
    threadSpawner.startChildProcess();
    threadSpawner.waitForExistence();
    updateDisparity = true;
}

void VulkanRenderer::stopDisparityStream() {
    updateDisparity = false;
    cv::destroyAllWindows();
    threadSpawner.stopChildProcess();
}




