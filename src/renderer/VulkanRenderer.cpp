//
// Created by magnus on 7/4/20.
//




#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanRenderer.hpp"
#include "../pipeline/MeshModel.h"
#include "../include/stbi_image_write.h"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice);
        descriptors = new Descriptors(arEngine);
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);

        textures = new Textures(images);
        vulkanCompute = new VulkanCompute(arEngine);

        createFrameBuffersAndRenderPass();


        createCommandBuffers();
        createSyncObjects();
        printf("Initiated vulkan\n");

        initComputePipeline();
        loadComputeData();
        printf("Initiated compute pipeline\n");

    } catch (std::runtime_error &err) {
        printf("Error: %s\n", err.what());
        return -1;
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arEngine.mainDevice.device); // wait for GPU to finish rendering before we clean up resources

    vulkanCompute->cleanup();
    vkDestroyFence(arEngine.mainDevice.device, computeFence, NULL);

    images->cleanUp(); // Clean up depth images

    for (auto &mesh : meshes) {
        mesh.cleanUp();
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

    if (vkQueueSubmit(arEngine.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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
        clearValues[0].color = {0.05f, 0.25f, 0.05f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (int j = 0; j < meshes.size(); ++j) {
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipelines[j].pipeline);

            VkBuffer vertexBuffers[] = {models[j].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], models[j].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    arPipelines[j].pipelineLayout, 0, arDescriptors[j].descriptorSets.size(),
                                    arDescriptors[j].descriptorSets.data(), 0,
                                    nullptr);


            vkCmdDrawIndexed(commandBuffers[i], models[j].indexCount, 1, 0, 0, 0);

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
    for (int i = 0; i < meshes.size(); ++i) {

        uboModelVar.model = meshes[i].getModel();
        // Copy vertex data
        void *data;
        vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0], 0, arDescriptors[i].dataSizes[0],
                    0, &data);
        memcpy(data, &uboModelVar, arDescriptors[i].dataSizes[0]);
        vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0]);



        if (arDescriptors[i].bufferMemory.size() == 1) break; // TODO clean up this to be done dynamically
        void *data2;
        vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1], 0, arDescriptors[i].dataSizes[1],
                    0, &data2);
        memcpy(data2, &fragmentColor, arDescriptors[i].dataSizes[1]);
        vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1]);

    }
}


void VulkanRenderer::updateCamera(glm::mat4 newView, glm::mat4 newProjection) {
    uboModelVar.view = newView;
    uboModelVar.projection = newProjection;
}

// TODO can be combined with updateLightPos if a wrapper is written around those two
void VulkanRenderer::updateModel(glm::mat4 newModel, int index) {
    meshes[index].setModel(newModel);
    printf("Update model passing index %d\n", index);

}

void VulkanRenderer::updateLightPos(glm::vec3 newPos, glm::mat4 transMat, int index) {
    printf("Update light passing index %d\n", index);

    meshes[index].setModel(glm::translate(transMat, newPos));
    fragmentColor.lightPos = glm::vec4(newPos, 1.0f);
    fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);

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

void VulkanRenderer::drawScene(std::vector<std::map<std::string, std::string>> modelSettings) {
// Load in each model
    for (int i = 0; i < modelSettings.size(); ++i) {

        if (modelSettings[i].at("type") == "sphere")
            loadTypeOneObject();
        else if (modelSettings[i].at("type") == "cube")
            loadTypeTwoObject();
            printf("");
    }

    updateScene();
}

void VulkanRenderer::updateScene() {
    // update command buffers
    recordCommand();

    // --- Translate models ---
    glm::mat4 trans(1.0f);
    // Glasses
    for (int i = 0; i < meshes.size(); ++i) {
        updateLightPos(glm::vec3(0.0f, 0.20f, i * -3.0f), trans, i);
    }
    // lightbox
}

void VulkanRenderer::deleteLastObject() {
    meshes.pop_back();
    // TODO Clean up resources used with this object as well
}


void VulkanRenderer::loadTypeOneObject() {
    // Create Mesh
    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;
    arModel.modelName = "standard/sphere.obj";
    MeshModel meshModel;
    meshes.push_back(meshModel.loadModel(arEngine.mainDevice, &arModel, true));
    models.push_back(arModel);

    // Create descriptors
    ArDescriptor arDescriptor;
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorSetCount = 2;
    descriptorInfo.descriptorSetLayoutCount = 2;
    std::array<uint32_t , 2> sizes = {sizeof(uboModel), sizeof(FragmentColor)};
    descriptorInfo.dataSizes = sizes.data();

    // TODO Rewrite usage of vectors like this
    arDescriptor.dataSizes.resize(2);
    arDescriptor.dataSizes[0] = sizeof(uboModel);
    arDescriptor.dataSizes[1] = sizeof(FragmentColor);

    descriptorInfo.descriptorCount = 2;
    std::array<uint32_t , 2> descriptorCounts = {1, 1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 2> bindings = {0, 1};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 2> types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::array<VkShaderStageFlags, 2> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    descriptorInfo.stageFlags = stageFlags.data();


    std::vector<ArBuffer> buffers(2);
    // Create and fill buffers
    for (int j = 0; j < 2; ++j) {
        buffers[j].bufferSize = sizes[j];
        buffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffers[j].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffers[j].bufferProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        buffer->createBuffer(&buffers[j]);
        arDescriptor.buffer.push_back(buffers[j].buffer);
        arDescriptor.bufferMemory.push_back(buffers[j].bufferMemory);
    }

    descriptors->createDescriptors(descriptorInfo, &arDescriptor);

    ArPipeline arPipeline{};
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;

    // Create pipeline
    ArShadersPath shadersPath;
    shadersPath.vertexShader = "../shaders/defaultVert";
    shadersPath.fragmentShader = "../shaders/phongLightFrag";
    pipeline.arLightPipeline(renderPass, arDescriptor, shadersPath, &arPipeline);
    fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);

    arDescriptors.push_back(arDescriptor);
    arPipelines.push_back(arPipeline);

}

void VulkanRenderer::loadTypeTwoObject() {

    // Create Mesh
    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;
    arModel.modelName = "standard/cube.obj";
    MeshModel meshModel;
    meshes.push_back(meshModel.loadModel(arEngine.mainDevice, &arModel, true));
    models.push_back(arModel);

    // Create descriptors
    ArDescriptor arDescriptor;
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorSetCount = 1;
    descriptorInfo.descriptorSetLayoutCount = 1;
    std::array<uint32_t , 1> sizes = {sizeof(uboModel)};
    descriptorInfo.dataSizes = sizes.data();

    // TODO Rewrite usage of vectors like this
    arDescriptor.dataSizes.resize(1);
    arDescriptor.dataSizes[0] = sizeof(uboModel);

    descriptorInfo.descriptorCount = 1;
    std::array<uint32_t , 1> descriptorCounts = {1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 1> bindings = {0};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 1> types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::array<VkShaderStageFlags, 1> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT};
    descriptorInfo.stageFlags = stageFlags.data();


    std::vector<ArBuffer> buffers(1);
    // Create and fill buffers
    for (int j = 0; j < 1; ++j) {
        buffers[j].bufferSize = sizes[j];
        buffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffers[j].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffers[j].bufferProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        buffer->createBuffer(&buffers[j]);
        arDescriptor.buffer.push_back(buffers[j].buffer);
        arDescriptor.bufferMemory.push_back(buffers[j].bufferMemory);
    }

    descriptors->createDescriptors(descriptorInfo, &arDescriptor);


    ArPipeline arPipeline{};
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;

    // Create pipeline
    ArShadersPath shadersPath;
    shadersPath.vertexShader = "../shaders/lampVert";
    shadersPath.fragmentShader = "../shaders/lampFrag";
    pipeline.arLightPipeline(renderPass, arDescriptor, shadersPath, &arPipeline);

    arDescriptors.push_back(arDescriptor);
    arPipelines.push_back(arPipeline);

}


void VulkanRenderer::initComputePipeline() {
    arCompute = vulkanCompute->setupComputePipeline(buffer, descriptors, platform, pipeline);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkResult result = vkCreateFence(arEngine.mainDevice.device, &fenceCreateInfo, NULL, &computeFence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create fence");

}

void VulkanRenderer::loadComputeData() {
    vulkanCompute->loadComputeData(arCompute);

}

void VulkanRenderer::vulkanComputeShaders() {

    vkResetFences(arEngine.mainDevice.device, 1, &computeFence);

    clock_t Start = clock();
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1; // submit a single command buffer
    submitInfo.pCommandBuffers = &arCompute.commandBuffer; // the command buffer to submit.


    VkResult result = vkQueueSubmit(arEngine.graphicsQueue, 1, &submitInfo, computeFence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit queue");

    result = vkWaitForFences(arEngine.mainDevice.device, 1, &computeFence, VK_TRUE, 100000000000);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to wait for fence");

    auto endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
    printf("Time taken: %.7fs\n", endTime);

    // --- Retrieve data from compute pipeline ---
    int width = 1282, height = 1110;
    int imageSize = width * height;
    void *mappedMemory = nullptr;
    // Map the buffer memory, so that we can read from it on the CPU.
    vkMapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[2], 0, imageSize * sizeof(glm::vec4), 0, &mappedMemory);
    auto *pmappedMemory = (glm::vec4 *) mappedMemory;

    auto* pixels = new stbi_uc[imageSize];
    auto original = pixels;
    for (int i = 0; i < imageSize; ++i) {
        *pixels = pmappedMemory->x;
        if (i > imageSize - 10) printf("Result: %d\n", *pixels);
        pixels++;
        pmappedMemory++;
    }
    pixels = original;

    stbi_write_png("../stbpng.png", width, height, 1, pixels, width * 1);

    vkUnmapMemory(arEngine.mainDevice.device, arCompute.descriptor.bufferMemory[2]);


    endTime = (double) (clock() - Start) / CLOCKS_PER_SEC;
    printf("Time taken: %.7fs\n", endTime);


}


