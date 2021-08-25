//
// Created by magnus on 7/4/20.
//




#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <ar_engine/src/include/settings.h>
#include "VulkanRenderer.hpp"
#include "../include/stbi_image_write.h"
#include "opencv2/opencv.hpp"
#include "../include/helper_functions.h"
#include "GUI.h"

VulkanRenderer::VulkanRenderer() {
    // Get number of max. concurrent threads
    numThreads = std::thread::hardware_concurrency() / 6;
    assert(numThreads > 0);
    threadPool.setThreadCount(numThreads);
    numObjectsPerThread = 6 / numThreads;
    rndEngine.seed((uint32_t) 666);
}

int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        int a = 0;
        auto k = &a;
        // CLass handles with helper functions. Can be passed arbitrarily
        platform = new ar::Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice);
        descriptors = new Descriptors(arEngine);
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);
        //textures = new Textures(images);
        //vulkanCompute = new VulkanCompute(arEngine);
        gui = new GUI(arEngine);

        createFrameBuffersAndRenderPass();
        createCommandBuffers();
        createSyncObjects();
        printf("Initiated vulkan\n");

        // Init GUI

        gui->init(arEngine.swapchainExtent.width, arEngine.swapchainExtent.height);
        gui->initResources(renderPass);
        printf("Initiated imGUI\n");
        //initComputePipeline();
        printf("Initiated compute pipeline\n\n");

        // Commandbuffers x2
        prepareMultiThreadedRenderer();

        // Initialize MeshRenderer Settings
        meshGenerator = new MeshGenerator(arEngine, renderPass);

        // Update global list of settings lastly
        settings = meshGenerator->settings;
        // Set the settings
        gui->setSettings(settings);
        //textRenderTest();

    } catch (std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    return 0;
}


float VulkanRenderer::rnd(float range) {
    std::uniform_real_distribution<float> rndDist(0.0f, range);
    return rndDist(rndEngine);
}

void VulkanRenderer::prepareMultiThreadedRenderer() {
    // Allocate commandBuffers

    VkCommandBufferAllocateInfo cmdAllocateInfo = CommandBuffers::commandBufferAllocateInfo(arEngine.commandPool,
                                                                                            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                                            1);

    VkResult result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &cmdAllocateInfo, &primaryBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate primary cmd buffer");
    // Allocate secondary commandbuffers

    VkCommandBufferAllocateInfo secondaryAllocateInfo = CommandBuffers::commandBufferAllocateInfo(arEngine.commandPool,
                                                                                                  VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                                                                                  1);

    result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &secondaryAllocateInfo, &secondaryCmdBuffers.objects);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate secondary cmd buffer");
    result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &secondaryAllocateInfo, &secondaryCmdBuffers.ui);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate secondary cmd buffer");
    // Create one command pool for each thread
    threadData.resize(numThreads);


    for (int i = 0; i < numThreads; ++i) {
        ThreadData *thread = &threadData[i];

        VkCommandPoolCreateInfo cmdPoolInfo = CommandBuffers::commandPoolCreateInfo();
        cmdPoolInfo.queueFamilyIndex = ar::Platform::findQueueFamilies(arEngine.mainDevice.physicalDevice,
                                                                       arEngine.surface).graphicsFamily.value();
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        result = vkCreateCommandPool(arEngine.mainDevice.device, &cmdPoolInfo, nullptr, &thread->commandPool);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool");

        // one secondary cmd buffer per object per thread
        thread->commandBuffer.resize(numObjectsPerThread);
        // Generate cmdbuffers
        VkCommandBufferAllocateInfo secondaryCmdBufferAllocateInfo = CommandBuffers::commandBufferAllocateInfo(
                thread->commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, thread->commandBuffer.size());

        result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &secondaryCmdBufferAllocateInfo,
                                          thread->commandBuffer.data());
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate secondary cmdbuffers");

        thread->objectData.resize(numObjectsPerThread);

        for (int j = 0; j < numObjectsPerThread; ++j) {

            thread->objectData[j].createDefaultSceneObject(arEngine);
            thread->objectData[j].createPipeline(renderPass);

            float theta = 2.0f * float(M_PI) * rnd(1.0f);
            float phi = acos(1.0f - 2.0f * rnd(1.0f));
            glm::vec3 pos(sin(phi) * cos(theta), 0.0f, cos(phi) * 35.0f);

            glm::vec3 scale(0.1f, 0.1f, 0.1f);

            glm::mat4 model = thread->objectData[j].getModel();

            glm::vec3 position(0.0f, 0.0f, 0.0f);

            position.x = (float) numThreads / 2;
            position.z = (float) numObjectsPerThread / 4;

            model = glm::translate(model, pos);
            model = glm::scale(model, scale);
            thread->objectData[j].setModel(model);


        }

    }
}

void VulkanRenderer::threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex,
                                      VkCommandBufferInheritanceInfo inheritanceInfo) {
    ThreadData *thread = &threadData[threadIndex];
    SceneObject *sceneObject = &thread->objectData[cmdBufferIndex];


    VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBuffers::beginInfo();
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

    VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

    VkResult result = vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer");


    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneObject->getArPipeline().pipeline);


    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            sceneObject->getArPipeline().pipelineLayout, 0,
                            sceneObject->getArDescriptor().descriptorSets.size(),
                            sceneObject->getArDescriptor().descriptorSets.data(), 0,
                            nullptr);


    VkDeviceSize offsets[1] = {0};
    VkBuffer vertexBuffer = sceneObject->getVertexBuffer();
    VkBuffer indexBuffer = sceneObject->getIndexBuffer();
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, sceneObject->getIndexCount(), 1, 0, 0, 0);
    // Check visibility against view frustum using a simple sphere check based on the radius of the mesh
    //objectData->visible = frustum.checkSphere(objectData->pos, models.ufo.dimensions.radius * 0.5f);

    result = vkEndCommandBuffer(cmdBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffer");

    // Also update descriptor

    uboModelVar.model = sceneObject->getModel();


}

void VulkanRenderer::updateSecondaryCommandBuffers(VkCommandBufferInheritanceInfo inheritanceInfo) {

    // Secondary command buffer for the sky sphere
    VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBuffers::beginInfo();
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
    // Objects
    VkResult result = vkBeginCommandBuffer(secondaryCmdBuffers.objects, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to start recording objects cmdbuffer");

    for (int j = 0; j < objects.size(); ++j) {
        vkCmdBindPipeline(secondaryCmdBuffers.objects, VK_PIPELINE_BIND_POINT_GRAPHICS, arPipelines[j].pipeline);

        VkBuffer vertexBuffers[] = {objects[j].getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(secondaryCmdBuffers.objects, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(secondaryCmdBuffers.objects, objects[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(secondaryCmdBuffers.objects, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                arPipelines[j].pipelineLayout, 0, arDescriptors[j].descriptorSets.size(),
                                arDescriptors[j].descriptorSets.data(), 0,
                                nullptr);

        vkCmdDrawIndexed(secondaryCmdBuffers.objects, objects[j].getIndexCount(), 1, 0, 0, 0);

    }


    result = vkEndCommandBuffer(secondaryCmdBuffers.objects);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to start recording objects cmdbuffer");


    // UI
    result = vkBeginCommandBuffer(secondaryCmdBuffers.ui, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to start recording ui cmdbuffer");

    // Have to skip some frames apparently.
    for (int i = 0; i < 2; ++i) {
        gui->newFrame(frameNumber == 0);
    }
    gui->updateBuffers();

    gui->drawNewFrame(secondaryCmdBuffers.ui);

    result = vkEndCommandBuffer(secondaryCmdBuffers.ui);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to stop recording ui cmdbuffer");


}

void VulkanRenderer::updateCommandBuffers(VkFramebuffer frameBuffer) {
    // Contains the list of secondary command buffers to be submitted
    std::vector<VkCommandBuffer> cmdBuffers;

    VkCommandBufferBeginInfo cmdBufInfo = CommandBuffers::beginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = {0.25f, 0.25f, 0.25f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = arEngine.swapchainExtent.width;
    renderPassBeginInfo.renderArea.extent.height = arEngine.swapchainExtent.height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = frameBuffer;

    VkResult result = vkBeginCommandBuffer(primaryBuffer, &cmdBufInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin primary command buffer");

    // The primary command buffer does not contain any rendering commands
    // These are stored (and retrieved) from the secondary command buffers
    vkCmdBeginRenderPass(primaryBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);


    // Inheritance info for the secondary command buffers
    VkCommandBufferInheritanceInfo inheritanceInfo = CommandBuffers::inheritanceInfo();
    inheritanceInfo.renderPass = renderPass;
    // Secondary command buffer also use the currently active framebuffer
    inheritanceInfo.framebuffer = frameBuffer;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.occlusionQueryEnable = VK_FALSE;
    inheritanceInfo.queryFlags = 0;


    // Update secondary scene command buffers
    updateSecondaryCommandBuffers(inheritanceInfo);

    cmdBuffers.push_back(secondaryCmdBuffers.objects);


    // Add a job to the thread's queue for each object to be rendered
    for (uint32_t t = 0; t < numThreads; t++) {
        for (uint32_t i = 0; i < numObjectsPerThread; i++) {
            threadPool.threads[t]->addJob([=] { threadRenderCode(t, i, inheritanceInfo); });
        }
    }
    threadPool.wait();

    // Only submit if object is within the current view frustum
    for (uint32_t t = 0; t < numThreads; t++) {
        for (uint32_t i = 0; i < numObjectsPerThread; i++) {
            if (threadData[t].objectData[i].visible) {
                cmdBuffers.push_back(threadData[t].commandBuffer[i]);
            }
        }
    }

    cmdBuffers.push_back(secondaryCmdBuffers.ui);


    // Execute render commands from the secondary command buffer
    vkCmdExecuteCommands(primaryBuffer, cmdBuffers.size(), cmdBuffers.data());

    vkCmdEndRenderPass(primaryBuffer);

    result = vkEndCommandBuffer(primaryBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end record of commandBuffer");


}

VulkanRenderer::~VulkanRenderer() = default;


void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arEngine.mainDevice.device); // wait for GPU to finish rendering before we clean up resources

    for (auto &thread : threadData) {
        vkFreeCommandBuffers(arEngine.mainDevice.device, thread.commandPool, thread.commandBuffer.size(),
                             thread.commandBuffer.data());
        vkDestroyCommandPool(arEngine.mainDevice.device, thread.commandPool, nullptr);

        for (auto &i : thread.objectData) {
            descriptors->cleanUp(i.getArDescriptor());
            i.cleanUp(arEngine.mainDevice.device);
            pipeline.cleanUp(i.getArPipeline());
        }
    }


    vulkanCompute->cleanup();
    //commandBuffers->cleanUp();
    gui->cleanUp();
    delete gui;
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

    vkDeviceWaitIdle(arEngine.mainDevice.device);


    vkWaitForFences(arEngine.mainDevice.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(arEngine.mainDevice.device, arEngine.swapchain, UINT64_MAX,
                          imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //recordCommands();
    updateCommandBuffers(swapChainFramebuffers[imageIndex]);


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
    submitInfo.pCommandBuffers = &primaryBuffer;

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
    frameNumber++;
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
    /*
    commandBuffers = new CommandBuffers(arEngine);
    commandBuffers->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers->allocateCommandBuffers(swapChainFramebuffers.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
*/
    commandBuffers.resize(swapChainFramebuffers.size());

    // TEXTRENDER ALSO ALLOCATE BUFFERS
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = arEngine.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(arEngine.mainDevice.device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
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

    for (int i = 0; i < numThreads; ++i) {
        ThreadData thread = threadData[i];
        for (const auto &sceneObject : thread.objectData) {
            uboModelVar.model = sceneObject.getModel();

            // Copy vertex data
            void *data;
            vkMapMemory(arEngine.mainDevice.device, sceneObject.getArDescriptor().bufferMemory[0], 0,
                        sceneObject.getArDescriptor().dataSizes[0],
                        0, &data);
            memcpy(data, &uboModelVar,
                   sceneObject.getArDescriptor().dataSizes[0]);
            vkUnmapMemory(arEngine.mainDevice.device, sceneObject.getArDescriptor().bufferMemory[0]);


            // If current model has two buffers (e.g. fragment shader input then continue)
            void *data2;
            vkMapMemory(arEngine.mainDevice.device, sceneObject.getArDescriptor().bufferMemory[1], 0,
                        sceneObject.getArDescriptor().dataSizes[1],
                        0, &data2);
            memcpy(data2, &fragmentColor, sceneObject.getArDescriptor().dataSizes[1]);
            vkUnmapMemory(arEngine.mainDevice.device, sceneObject.getArDescriptor().bufferMemory[1]);
        }
    }


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
    startRecordCommand();
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


void VulkanRenderer::testFunction() {

    onUIUpdate();
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

float sum = 0.0f;

void VulkanRenderer::updateUI(UISettings uiSettings_) {
    gui->uiSettings.frameTimes[frameNumber % 1000] = uiSettings_.FPS;
    gui->uiSettings.frameLimiter = uiSettings_.frameLimiter;

    sum += uiSettings_.FPS;
    if ((frameNumber % 200) == 0) {
        gui->uiSettings.average = sum / 200;
        sum = 0;
    }


    //std::rotate(gui->uiSettings.frameTimes.begin(), gui->uiSettings.frameTimes.begin() + (1),gui->uiSettings.frameTimes.end());
    //gui.cleanUp();
}

bool update = true;

void VulkanRenderer::onUIUpdate() {
    settings = gui->getSettings();


    for (auto &setting: gui->getSettings()) {
        if (setting.active && setting.update) {

            int k = 0;
            auto a = &k;

            ArShadersPath shadersPath{};
            ArModel arModel{};
            ArDescriptorInfo descriptorInfo{};

            meshGenerator->update(settings, &arModel);


            shadersPath.fragmentShader = "phongLightFrag";
            shadersPath.vertexShader = "defaultVert";


            // DescriptorInfo
            // Create descriptor
            // Font uses a separate descriptor pool
            // poolcount and descriptor set counts
            descriptorInfo.descriptorPoolCount = 1;
            descriptorInfo.descriptorCount = 2;
            descriptorInfo.descriptorSetLayoutCount = 2;
            descriptorInfo.descriptorSetCount = 2;
            std::vector<uint32_t> descriptorCounts;
            descriptorCounts.push_back(1);
            descriptorCounts.push_back(1);
            descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
            std::vector<uint32_t> bindings;
            bindings.push_back(0);
            bindings.push_back(1);
            descriptorInfo.pBindings = bindings.data();
            // dataSizes
            std::vector<uint32_t> dataSizes;
            dataSizes.push_back(sizeof(uboModel));
            dataSizes.push_back(sizeof(FragmentColor));
            descriptorInfo.dataSizes = dataSizes.data();
            // types
            std::vector<VkDescriptorType> types(2);
            types[0] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            types[1] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorInfo.pDescriptorType = types.data();
            // stages
            std::array<VkShaderStageFlags, 2> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
            descriptorInfo.stageFlags = stageFlags.data();

            SceneObject object(arEngine, shadersPath, arModel, descriptorInfo);
            object.createPipeline(renderPass);

            /*
            objects.push_back(object);
            arPipelines.push_back(object.getArPipeline());
            arDescriptors.push_back(object.getArDescriptor());
*/
            setting.update = false;
        }
    }

}




