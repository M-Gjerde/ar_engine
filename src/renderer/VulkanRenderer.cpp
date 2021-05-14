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

    } catch (std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arEngine.mainDevice.device); // wait for GPU to finish rendering before we clean up resources

    vulkanCompute->cleanup();
    vkDestroyFence(arEngine.mainDevice.device, computeFence, NULL);

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

    for (int i = 0; i < arPipelines.size(); ++i) {
        pipeline.cleanUp(arPipelines[i]);

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

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

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
        clearValues[0].color = {0.25f, 0.25f, 0.25f, 1.0f};
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
            throw std::runtime_error("failed to record command buffer!");
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

// TODO can be combined with updateLightPos if a wrapper is written around those two
void VulkanRenderer::updateModel(glm::mat4 newModel, int index, bool isLight) {
    objects[index].setModel(newModel);
    if (isLight) {

        fragmentColor.lightPos = glm::vec4(newModel[3].x, newModel[3].y, newModel[3].z, 1.0f);
        fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
        fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);
    }
}

void VulkanRenderer::updateLightPos(glm::vec3 newPos, glm::mat4 transMat, int index) {
    objects[index].setModel(glm::translate(transMat, newPos));
    fragmentColor.lightPos = glm::vec4(newPos, 1.0f);


}

void VulkanRenderer::updateSpecularLightCamera(glm::vec3 newPos) {
    fragmentColor.viewPos = glm::vec4(newPos, 1.0f);
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

    // printf("Texture and re-record cmd buffers time taken: %.7fs\n", (double) (clock() - Start) / CLOCKS_PER_SEC;);
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

    }
    recordCommand();
}

void VulkanRenderer::deleteLastObject() {
    objects.pop_back();
    // TODO Clean up resources used with this object as well
}


void VulkanRenderer::initComputePipeline() {
    arCompute = vulkanCompute->setupComputePipeline(buffer, descriptors, platform, pipeline);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkResult result = vkCreateFence(arEngine.mainDevice.device, &fenceCreateInfo, NULL, &computeFence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create fence");

    vkResetFences(arEngine.mainDevice.device, 1, &computeFence);
}

void VulkanRenderer::updateDisparityData() {
    //cv::namedWindow("window", cv::WINDOW_FREERATIO);
    //cv::namedWindow("window2", cv::WINDOW_FREERATIO);
    //cv::namedWindow("Disparity image", cv::WINDOW_FREERATIO);

    vulkanCompute->loadComputeData(arCompute, buffer);
    //vulkanCompute->loadImagePreviewData(arCompute, buffer);
    vulkanComputeShaders();

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

void VulkanRenderer::startDisparityStream() {
    vulkanCompute->startDisparityStream();
    updateDisparity = true;
}

void VulkanRenderer::stopDisparityStream() {
    vulkanCompute->stopDisparityStream();
}


int loop = 0;

void VulkanRenderer::vulkanComputeShaders() {


    auto start = std::chrono::high_resolution_clock::now();
    vkResetFences(arEngine.mainDevice.device, 1, &computeFence);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1; // submit a single command buffer
    submitInfo.pCommandBuffers = &arCompute.commandBuffer; // the command buffer to submit.

    VkResult result = vkQueueSubmit(arEngine.computeQueue, 1, &submitInfo, computeFence);
    if (result != VK_SUCCESS) {
        printf("result: %d\n", result);
        throw std::runtime_error("Failed to wait for fence");
    }

    //printf("Waiting for fences\n");
    result = vkWaitForFences(arEngine.mainDevice.device, 1, &computeFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        printf("result: %d\n", result);
        throw std::runtime_error("Failed to wait for fence");
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto endTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //printf("Queue submit Time taken: %ld ms\n", endTime.count() / 1000);


    // --- Retrieve data from compute pipeline ---
    //int width = 1282, height = 1110;
    //int width = 1280, height = 720;
    //int width = 427, height = 370;
    int width = 640, height = 480;
    //int width = 450, height = 375;
    //int width = 256, height = 256;

    int imageSize = (width * height);

    void *mappedMemory = nullptr;
    // Map the buffer memory, so that we can read from it on the CPU.
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[3], 0, imageSize * sizeof(float), 0,
                &mappedMemory);
    auto *pmappedMemory = (float *) mappedMemory;




    // Create float type image at fill it with data from gpu
    cv::Mat img(height, width, CV_32FC1);
    img.data = reinterpret_cast<uchar *>(pmappedMemory);

    // convert image to 16 bit uc1
    img.convertTo(img, CV_16UC1, 65535);

    // Remove salt 'n pepper noise - must be a 8 or 16 bit type.
    cv::medianBlur(img, img, 5);
    cv::Mat kernel = cv::Mat::ones(5, 5, CV_32F);
    cv::dilate(img, img, kernel);

    // Convert back to float format to save and display
    img.convertTo(img, CV_32FC1, (float) 1 / 65535);

    ArROI roi = vulkanCompute->getRoi();
    std::vector<glm::vec3> pointPositions;
    if (roi.active) {
        disparityToPoint(img, roi, &pointPositions);
        disparityToPointWriteToFile(img, roi, &pointPositions);

        int k = pointPositions.size() / 2;
        if (pointPositions[k].x > -10 && pointPositions[k].x < 10) {

            float sumX = 0;
            float sumY = 0;
            float sumZ = 0;
            for (int i = 0; i < 10; ++i) {
                sumX += pointPositions[k + i - 5].x;
                sumY += pointPositions[k + i - 5].y;
                sumZ += pointPositions[k + i - 5].z;

            }

            glm::vec3 trans(sumX / 10, sumY / 10, -sumZ / 10);
            glm::mat4 model = glm::translate(glm::mat4(2.0f), trans);
            model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
            updateModel(model, 1, true);
            printf("cube_pos: %f, %f, %f\n", pointPositions[k].x, pointPositions[k].y, pointPositions[k].z);

        }

        //disparityToPointWriteToFile(img, roi, &pointPositions);

    }
    if (takePhoto) {
        cv::imwrite("../home_disparity" + std::to_string(loop) + ".exr", img);
        vulkanCompute->takePhoto = true;

    }
    img.convertTo(img, CV_8UC1, 255);

    stop = std::chrono::high_resolution_clock::now();
    endTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //printf("Filtering and converting: %ld ms\n", endTime.count() / 1000);

    // -- CALCULATE POINT CLOUD
    //createPointCloudWriteToPCD(img, "/home/magnus/CLionProjects/bachelor_project/pcl_data/3d_points"+std::to_string(loop)+".pcd");


    //cv::normalize(img, img, 0, 1, cv::NORM_MINMAX);
    cv::equalizeHist(img, img);

    cv::imshow("Raw disparity", img);


    uint32_t roiSize = roi.width * roi.height;

    //boost::numeric::ublas::matrix<float> pointMat(4, roiSize);


    //printf("avg disp: %f\n", (float) disparitySum/roiSize);
    stop = std::chrono::high_resolution_clock::now();
    endTime = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //printf("summing ROI and updating pos: %ld ms\n", endTime.count() / 1000);
    //cv::imwrite("../output.png", img);


    /*
    // -- VISUALIZE
    cv::Mat bwImg = img;
    cv::Mat jetmapImage;
    cv::equalizeHist(img, img);

    cv::applyColorMap(img, jetmapImage, cv::COLORMAP_JET);

    //cv::circle(jetmapImage,cv::Point(320, 240),2,cv::Scalar(255, 255, 255),cv::FILLED,cv::LINE_8);
    //cv::circle(bwImg,cv::Point(320, 240),5,cv::Scalar(255, 255, 255),cv::FILLED,cv::LINE_8);
    if (takePhoto) {
        cv::imwrite("../home_disparity_jet" + std::to_string(loop) + ".png", jetmapImage);
        loop++;
        takePhoto = false;

    }

    cv::imshow("Jet Disparity image", jetmapImage);
    cv::imshow("BW Disparity image", bwImg);

*/
    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[3]);

    //stbi_write_png("../stbpng.png", width, height, 1, pixels, width * 1);


}

void VulkanRenderer::resetScene() {

}

std::vector<SceneObject> VulkanRenderer::getSceneObjects() const {
    return objects;
}




