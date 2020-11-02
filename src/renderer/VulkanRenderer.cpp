//
// Created by magnus on 7/4/20.
//




#include <array>
#include <utility>
#include <glm/ext/matrix_transform.hpp>
#include "VulkanRenderer.hpp"
#include "../pipeline/MeshModel.h"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice);
        descriptors = new Descriptors(arEngine);
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);

        textures = new Textures(images);


        //createSimpleMesh();
        // TODO REWRITE

        createFrameBuffersAndRenderPass();



        createCommandBuffers();
        createSyncObjects();
        printf("Initiated vulkan\n");

    } catch (std::runtime_error &err) {
        printf("Error: %s\n", err.what());
        return -1;
    }
    return 0;
}

void VulkanRenderer::cleanup() {
    vkDeviceWaitIdle(arPipeline.device); // wait for GPU to finish rendering before we clean up resources

    textures->cleanUp(arTextureSampler, textureImageBuffer); // TODO This could be vectorized
    //textures->cleanUp(disparityTexture, disparityTextureBuffer);
    //textures->cleanUp(videoTexture, videoTextureBuffer);
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

    vkDestroyRenderPass(arEngine.mainDevice.device, renderPass, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(arEngine.mainDevice.device, framebuffer, nullptr);
    }

    pipeline.cleanUp(arPipeline);
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

    // TODO check if this should be done different
    updateBuffer(imageIndex);
    if (textureUpdateToggle)
        updateDisparityVideoTexture();

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

        for (int j = 0; j < models.size(); ++j) {
            VkBuffer vertexBuffers[] = {models[j].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);


            vkCmdBindIndexBuffer(commandBuffers[i], models[j].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipelineLayout, 0, 1,
                                    &arDescriptor.descriptorSets[j], 0, nullptr);

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
        if (vkCreateSemaphore(arEngine.mainDevice.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(arEngine.mainDevice.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(arEngine.mainDevice.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanRenderer::updateBuffer(uint32_t imageIndex) {


    for (int i = 0; i < meshes.size(); ++i) {

        uboModelVar.model = meshes[i].getModel();
        // Copy VP data
        void *data;
        vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i], 0, sizeof(uboModel), 0, &data);
        memcpy(data, &uboModelVar, sizeof(uboModel));
        vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i]);

    }


}




void VulkanRenderer::createSimpleMesh() {

    // Create memory and buffers for vertices
    std::vector<ArBuffer> modelBuffers;
    modelBuffers.resize(2);
    modelBuffers[0].bufferSize = sizeof(meshVertices[0]) * meshVertices.size();
    modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    modelBuffers[1].bufferSize = sizeof(meshIndices[0]) * meshIndices.size();
    modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    triangleModels.resize(0);

    for (int i = 0; i < triangleModels.size(); ++i) {
        triangleModels[i].indices = meshIndices;
        triangleModels[i].vertices = meshVertices;
        triangleModels[i].transferCommandPool = arEngine.commandPool;
        triangleModels[i].transferQueue = arEngine.graphicsQueue;
        meshes.emplace_back(Mesh(arEngine.mainDevice, &triangleModels[i], modelBuffers));

    }

    // Create UBO
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
    // Create descriptors
    descriptors->createDescriptors(&arDescriptor);

    // Create Texture descriptors
    arTextureSampler.transferQueue = arEngine.graphicsQueue;
    arTextureSampler.transferCommandPool = arEngine.commandPool;

    textureImageBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    textureImageBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // Create initial texture for visual pleasantries
    textures->createTexture("viking_room.png", &arTextureSampler, &textureImageBuffer);
    descriptors->createDescriptorsSampler(&arDescriptor, arTextureSampler);

    // initialize texture objects for disparity IMAGE
    disparityTexture.transferQueue = arEngine.graphicsQueue;
    disparityTexture.transferCommandPool = arEngine.commandPool;
    disparityTexture.width = 640;
    disparityTexture.height = 550;
    disparityTexture.channels = 1;
    disparityTextureBuffer.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    disparityTextureBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    textures->createTextureImage(&disparityTexture, &disparityTextureBuffer);

    // Initialize texture objects for disparity VIDEO

    videoTexture.transferQueue = arEngine.graphicsQueue;
    videoTexture.transferCommandPool = arEngine.commandPool;
    videoTexture.width = 1242;
    videoTexture.height = 375;
    videoTexture.channels = 1;

    videoTextureBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    videoTextureBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    textures->createTextureImage(&videoTexture, &videoTextureBuffer);

}


void VulkanRenderer::updateCamera(glm::mat4 newView, glm::mat4 newProjection) {

    uboModelVar.view = newView;
    uboModelVar.projection = newProjection;
}

void VulkanRenderer::updateModel(glm::mat4 newModel, int index) {

    meshes[index].setModel(newModel);

}

void VulkanRenderer::updateTextureImage(std::string fileName) {

    textures->setDisparityImageTexture(disparity, &disparityTexture, &disparityTextureBuffer);
    descriptors->createDescriptorsSampler(&arDescriptor, disparityTexture);
    createCommandBuffers();

}

void VulkanRenderer::updateDisparityVideoTexture() {

    // Skip updating if pixels are not Ready optional: add a timeout function to this
    if (!disparity->pixelDataReady) {
        return;
    }
    //clock_t Start = clock();
    textures->setDisparityVideoTexture(disparity, &videoTexture, &videoTextureBuffer);

    descriptors->createDescriptorsSampler(&arDescriptor, videoTexture);
    recordCommand();

    // printf("Texture and re-record cmd buffers time taken: %.7fs\n", (double) (clock() - Start) / CLOCKS_PER_SEC;);
}

void VulkanRenderer::drawScene(std::vector<std::map<std::string, std::string>> modelSettings) {
    models.resize(modelSettings.size()); // Number of objects


    for (int i = 0; i < modelSettings.size(); ++i) {
        ArModel arModel;
        arModel.transferCommandPool = arEngine.commandPool;
        arModel.transferQueue = arEngine.graphicsQueue;
        arModel.modelName = "standard/" + modelSettings[i].at("type") + ".obj";
        MeshModel meshModel;

        meshes.push_back(meshModel.loadModel(arEngine.mainDevice, &arModel));
        models[i] = arModel;
    }

    // Create descriptors for objects
    createUBODescriptors();

    // Create initial texture
    arTextureSampler.transferQueue = arEngine.graphicsQueue;
    arTextureSampler.transferCommandPool = arEngine.commandPool;
    textureImageBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    textureImageBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    textures->createTexture("default.jpg", &arTextureSampler, &textureImageBuffer);
    // Create Texture descriptors
    descriptors->createDescriptorsSampler(&arDescriptor, arTextureSampler);

    // Should be called for specific objects during loading
    // Initialize pipeline struct
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;
    // Create pipeline
    pipeline.createGraphicsPipeline(renderPass, arDescriptor.descriptorSetLayout, &arPipeline);


    // update command buffers
    recordCommand();

    // Translate models..
    for (int i = 0; i < models.size(); ++i) {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f + static_cast<float>(i), 0.0f, -8.0f));
        updateModel(trans, i);

        if (modelSettings[i].at("type") == "tree"){
            trans = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f + static_cast<float>(i), 5.0f, -20.0f));
            trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            updateModel(trans, i);
        }
    }

}

void VulkanRenderer::createUBODescriptors() {

    // Create UBO for each Mesh
    arDescriptor.descriptorSets.resize(3);
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
    // Create descriptors
    descriptors->createDescriptors(&arDescriptor);


}