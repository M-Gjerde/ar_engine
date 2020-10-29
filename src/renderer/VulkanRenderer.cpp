//
// Created by magnus on 7/4/20.
//


#define TINYOBJLOADER_IMPLEMENTATION

#include "../../external/tinyobj/tiny_obj_loader.h"

#include <array>
#include <utility>
#include "VulkanRenderer.hpp"

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() = default;


int VulkanRenderer::init(GLFWwindow *newWindow) {
    try {
        platform = new Platform(newWindow, &arEngine);
        buffer = new Buffer(arEngine.mainDevice);
        descriptors = new Descriptors(arEngine);
        images = new Images(arEngine.mainDevice, arEngine.swapchainExtent);

        textures = new Textures(images);

        loadModel();
        createSimpleMesh();
        // TODO REWRITE


        createPipeline();
        createFrameBuffers();

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
    textures->cleanUp(disparityTexture, disparityTextureBuffer);
    textures->cleanUp(videoTexture, videoTextureBuffer);
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

        for (int j = 0; j < loadModels.size(); ++j) {
            VkBuffer vertexBuffers[] = {loadModels[j].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);


            vkCmdBindIndexBuffer(commandBuffers[i], loadModels[j].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipeline.pipelineLayout, 0, 1,
                                    &arDescriptor.descriptorSets[j], 0, nullptr);

            vkCmdDrawIndexed(commandBuffers[i], loadModels[j].indexCount, 1, 0, 0, 0);

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


    for (int i = 0; i < meshes.size(); ++i) {

        uboModelVar.model = meshes[i].getModel();
        // Copy VP data
        void *data;
        vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i], 0, sizeof(uboModel), 0, &data);
        memcpy(data, &uboModelVar, sizeof(uboModel));
        vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[i]);

    }


}

void VulkanRenderer::loadModel() {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string MODEL_PATH = "../objects/viking_room.obj";

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<ArBuffer> modelBuffers;
    modelBuffers.resize(2);
    loadModels.resize(1);

    std::map<Vertex, uint32_t> uniqueVertices{};


    for (int i = 0; i < shapes.size(); ++i) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); ++j) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                          attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]};

            vertex.texCoord = {attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 0],
                               1.0f - attrib.texcoords[2 * shapes[i].mesh.indices[j].texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            loadModels[0].vertices.push_back(vertex);

            loadModels[0].indices.push_back(loadModels[0].indices.size());
        }
    }

        modelBuffers[0].bufferSize = sizeof(loadModels[0].vertices[0]) * loadModels[0].vertices.size();
        modelBuffers[0].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        modelBuffers[0].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        modelBuffers[1].bufferSize = sizeof(loadModels[0].indices[0]) * loadModels[0].indices.size();
        modelBuffers[1].bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        modelBuffers[1].bufferProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        loadModels[0].indexCount = loadModels[0].indices.size();
        loadModels[0].transferCommandPool = arEngine.commandPool;
        loadModels[0].transferQueue = arEngine.graphicsQueue;
        meshes.emplace_back(Mesh(arEngine.mainDevice, &loadModels[0], modelBuffers));

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
    textures->createTexture("wallpaper.png", &arTextureSampler, &textureImageBuffer);
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
