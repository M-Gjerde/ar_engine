//
// Created by magnus on 7/4/20.
//



#include <glm/gtc/matrix_transform.hpp>
#include "../headers/VulkanRenderer.hpp"
#include "../include/settings.h"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);

        buffer = new Buffer(arEngine);
        descriptors = new Descriptors(arEngine);

        createUboBuffer();

        uboModelVar.projection = glm::perspective(glm::radians(45.0f), (float)viewport.WIDTH/(float)viewport.HEIGHT, 0.1f, 100.0f);

        uboModelVar.projection[1][1] *= -1;


        uboModelVar.model = glm::mat4(1.0f);
        uboModelVar.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(0.0f, 1.0f, 0.0f));

        uboModelVar.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        createPipeline();
        createFrameBuffers();

        // TODO create a queueSubmit and recording class
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

    descriptors->cleanUp(arDescriptor);
    buffer->cleanUp(arBuffer);

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
    vkAcquireNextImageKHR(arPipeline.device, arEngine.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
    pipeline.createGraphicsPipeline(&arPipeline, arDescriptor.descriptorSetLayout);

}

void VulkanRenderer::createFrameBuffers() {
    swapChainFramebuffers.resize(arEngine.swapchainImages.size());
    for (size_t i = 0; i < arEngine.swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {arEngine.swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = arPipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
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

        VkClearValue clearColor = {0.05f, 0.25f, 0.05f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipeline);

        VkBuffer vertexBuffers[] = {arBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipelineLayout, 0, 1, &arDescriptor.descriptorSets[i], 0, nullptr);

        vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

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

void VulkanRenderer::updateBuffer(uint32_t imageIndex) const {

    // Copy VP data
    void *data2;
    vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[imageIndex], 0, sizeof(uboModel), 0, &data2);
    memcpy(data2, &uboModelVar, sizeof(uboModel));
    vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[imageIndex]);



}

void VulkanRenderer::createUboBuffer() {
// Triangle
    buffer->createArBuffer(&arBuffer);
    void *data;
    vkMapMemory(arEngine.mainDevice.device, arBuffer.bufferMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(vertices[0]) * vertices.size());
    vkUnmapMemory(arEngine.mainDevice.device, arBuffer.bufferMemory);

    // UBO buffer
    uboBuffers.resize(arEngine.swapchainImages.size());
    arDescriptor.descriptorSets.resize(arEngine.swapchainImages.size());
    arDescriptor.buffer.resize(arEngine.swapchainImages.size());
    arDescriptor.bufferMemory.resize(arEngine.swapchainImages.size());

    VkDeviceSize vpBufferSize = sizeof(uboModel);

    for (int i = 0; i < arEngine.swapchainImages.size(); ++i) {
        uboBuffers[i].bufferSize = vpBufferSize;
        uboBuffers[i].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uboBuffers[i].bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        buffer->createBuffer(&uboBuffers[i]);

        arDescriptor.buffer[i] = uboBuffers[i].buffer;
        arDescriptor.bufferMemory[i] = uboBuffers[i].bufferMemory;
    }

    descriptors->createDescriptors(&arDescriptor);




}

