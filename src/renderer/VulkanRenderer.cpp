//
// Created by magnus on 7/4/20.
//



#include <array>
#include "VulkanRenderer.hpp"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice);
        descriptors = new Descriptors(arEngine);
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);

        createVertexBuffer();
        createUboBuffer();


        createPipeline();
        createFrameBuffers();

        createCommandBuffers();
        createSyncObjects();
        printf("Initiated vulkan\n");

    } catch (std::runtime_error &err) {
        printf("Error: %s\n", err.what());
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arPipeline.device); // wait for GPU to finish rendering before we clean up resources

    images->cleanUp();

    for (auto &mesh : meshes) {
        mesh.cleanUp();
    }

    descriptors->cleanUp(arDescriptor);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(arPipeline.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(arPipeline.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(arPipeline.device, inFlightFences[i], nullptr);
    }

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(arEngine.mainDevice.device, framebuffer, nullptr);
    }
    pipeline.cleanUp();
    platform->cleanUp();
}


void VulkanRenderer::draw() {
    // 1. Get next available image to draw to and set something to signal when we're finished with the image (a semaphore)
    // 2. Submit command buffer to queue for execution, making sure it waits for the image to be signalled as available before drawing
    // and signals when it has finished rendering
    // 3. Present image to screen when it has signaled finished rendering

    vkWaitForFences(arPipeline.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


    uint32_t imageIndex;
    vkAcquireNextImageKHR(arPipeline.device, arEngine.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(arPipeline.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
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

    vkResetFences(arPipeline.device, 1, &inFlightFences[currentFrame]);

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


void VulkanRenderer::createPipeline() {
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;
    arPipeline.depthFormat = images->findDepthFormat();
    images->createDepthImageView(&arDepthResource.depthImageView);       // Create depth image view

    pipeline.createGraphicsPipeline(&arPipeline, arDescriptor.descriptorSetLayout);

}

void VulkanRenderer::createFrameBuffers() {
    swapChainFramebuffers.resize(arEngine.swapchainImages.size());
    for (size_t i = 0; i < arEngine.swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {arEngine.swapChainImageViews[i], arDepthResource.depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = arPipeline.renderPass;
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

    if (vkAllocateCommandBuffers(arPipeline.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    recordCommand();
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
        renderPassInfo.renderPass = arPipeline.renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = arPipeline.swapchainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.05f, 0.25f, 0.05f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkClearValue clearColor = {0.05f, 0.25f, 0.05f, 1.0f};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipeline);

        for (int j = 0; j < triangleModels.size(); ++j) {
            VkBuffer vertexBuffers[] = {triangleModels[j].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);


            vkCmdBindIndexBuffer(commandBuffers[i], triangleModels[j].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipelineLayout, 0, 1,
                                    &arDescriptor.descriptorSets[j], 0, nullptr);

            vkCmdDrawIndexed(commandBuffers[i], triangleModels[j].indexCount, 1, 0, 0, 0);

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
        if (vkCreateSemaphore(arPipeline.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(arPipeline.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(arPipeline.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanRenderer::updateBuffer(uint32_t imageIndex) {


    uboModelVar.model = meshes[0].getModel();

    void *data2;
    vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0], 0, sizeof(uboModel), 0, &data2);
    memcpy(data2, &uboModelVar, sizeof(uboModel));
    vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0]);

    uboModelVar.model = meshes[1].getModel();
    // Copy VP data
    void *data;
    vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[1], 0, sizeof(uboModel), 0, &data);
    memcpy(data, &uboModelVar, sizeof(uboModel));
    vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[1]);


}


void VulkanRenderer::createUboBuffer() {

    // UBO buffer
    arDescriptor.descriptorSets.resize(meshes.size());
    uboBuffers.resize(arDescriptor.descriptorSets.size());
    arDescriptor.buffer.resize(arDescriptor.descriptorSets.size());
    arDescriptor.bufferMemory.resize(arDescriptor.descriptorSets.size());

    VkDeviceSize vpBufferSize = sizeof(uboModel);

    for (int i = 0; i < arDescriptor.descriptorSets.size(); ++i) {
        uboBuffers[i].bufferSize = vpBufferSize;
        uboBuffers[i].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uboBuffers[i].bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        buffer->createBuffer(&uboBuffers[i]);

        arDescriptor.buffer[i] = uboBuffers[i].buffer;
        arDescriptor.bufferMemory[i] = uboBuffers[i].bufferMemory;
    }
    descriptors->createDescriptors(&arDescriptor);
}


void VulkanRenderer::createVertexBuffer() {

    std::vector<ArBuffer> modelBuffers;
    modelBuffers.resize(2);
    modelBuffers[0].bufferSize = sizeof(meshVertices[0]) * meshVertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].bufferSize = sizeof(meshIndices[0]) * meshIndices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    triangleModels.resize(2);

    for (int i = 0; i < triangleModels.size(); ++i) {
        triangleModels[i].indices = meshIndices;
        triangleModels[i].vertices = meshVertices;
        triangleModels[i].transferCommandPool = arEngine.commandPool;
        triangleModels[i].transferQueue = arEngine.graphicsQueue;
        meshes.emplace_back(Mesh(arEngine.mainDevice, &triangleModels[i], modelBuffers));

    }



}


void VulkanRenderer::updateCamera(glm::mat4 newView, glm::mat4 newProjection) {

    uboModelVar.view = newView;
    uboModelVar.projection = newProjection;
}

void VulkanRenderer::updateModel(glm::mat4 newModel, int index) {

    meshes[index].setModel(newModel);

}