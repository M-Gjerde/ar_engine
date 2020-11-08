//
// Created by magnus on 7/4/20.
//




#include <array>
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
    vkDeviceWaitIdle(arEngine.mainDevice.device); // wait for GPU to finish rendering before we clean up resources

    for (int i = 0; i < arTextureSampler.size(); ++i) {
        textures->cleanUp(arTextureSampler[i], textureImageBuffer[i]); // TODO This could be vectorized
    }

    //textures->cleanUp(disparityTexture, disparityTextureBuffer);
    //textures->cleanUp(videoTexture, videoTextureBuffer);
    images->cleanUp();

    for (auto &mesh : meshes) {
        mesh.cleanUp();
    }

    // Clean descriptor sets
    for (auto &arDescriptor : arDescriptors) {
        descriptors->cleanUp(arDescriptor);
    }
    // TODO REWRITE
    vkDestroyDescriptorSetLayout(arEngine.mainDevice.device, arDescriptors[1].descriptorSetLayout2, nullptr);

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
        renderPassInfo.renderArea.extent = arEngine.swapchainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.05f, 0.25f, 0.05f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkClearValue clearColor = {0.05f, 0.25f, 0.05f, 1.0f};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (int j = 0; j < arPipelines.size(); ++j) {
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, arPipelines[j].pipeline);

            VkBuffer vertexBuffers[] = {models[j].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], models[j].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // TODO rework
            if (j == 1 || j == 0) {
                /*vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        arPipelines[j].pipelineLayout, 0, 1, &arDescriptors[j].descriptorSets[1], 0,
                                        nullptr);*/
                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        arPipelines[j].pipelineLayout, 0, 2, arDescriptors[j].descriptorSets.data(), 0,
                                        nullptr);
            } else

                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        arPipelines[j].pipelineLayout, 0, 1, &arDescriptors[j].descriptorSets[0], 0,
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
        // Copy VP data TODO REMOVE
        if (i == 1 || i == 0) {
            // Copy color data
            void *data;
            vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0], 0, sizeof(uboModel), 0, &data);
            memcpy(data, &uboModelVar, sizeof(uboModel));
            vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0]);

            void *data2;
            vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1], 0, sizeof(FragmentColor), 0,
                        &data2);
            memcpy(data2, &fragmentColor, sizeof(FragmentColor));
            vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[1]);

        } else {
            void *data;
            vkMapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0], 0, sizeof(uboModel), 0, &data);
            memcpy(data, &uboModelVar, sizeof(uboModel));
            vkUnmapMemory(arEngine.mainDevice.device, arDescriptors[i].bufferMemory[0]);
        }

    }


}


void VulkanRenderer::updateCamera(glm::mat4 newView, glm::mat4 newProjection) {
    uboModelVar.view = newView;
    uboModelVar.projection = newProjection;
}

void VulkanRenderer::updateModel(glm::mat4 newModel, int index) {
    meshes[index].setModel(newModel);
}

void VulkanRenderer::updateLightPos(glm::vec3 newPos, glm::mat4 transMat, int index) {


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
    models.resize(modelSettings.size()); // Number of objects
    arPipelines.resize(modelSettings.size());
    arDescriptors.resize(modelSettings.size());
// Load in each model
    for (int i = 0; i < modelSettings.size(); ++i) {
        // Create Mesh
        ArModel arModel;
        arModel.transferCommandPool = arEngine.commandPool;
        arModel.transferQueue = arEngine.graphicsQueue;
        arModel.modelName = "standard/" + modelSettings[i].at("type") + ".obj";
        MeshModel meshModel;

        meshes.push_back(meshModel.loadModel(arEngine.mainDevice, &arModel, true));
        models[i] = arModel;

        // Create UBO for each Mesh
        arDescriptors[i].descriptorSets.resize(2);
        //TODO Create descriptor sets according to how many sets the object/shader needs
        if (i > 1) arDescriptors[i].descriptorSets.resize(1);
        uboBuffers.resize(arDescriptors[i].descriptorSets.size());
        arDescriptors[i].buffer.resize(arDescriptors[i].descriptorSets.size());
        arDescriptors[i].bufferMemory.resize(arDescriptors[i].descriptorSets.size());

        VkDeviceSize vpBufferSize = sizeof(uboModel);

        // Create buffers
        for (int j = 0; j < arDescriptors[i].descriptorSets.size(); ++j) {
            if (j == 1)
                vpBufferSize = sizeof(FragmentColor);
            uboBuffers[j].bufferSize = vpBufferSize;
            uboBuffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            uboBuffers[j].bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            buffer->createBuffer(&uboBuffers[j]);

            arDescriptors[i].buffer[j] = uboBuffers[j].buffer;
            arDescriptors[i].bufferMemory[j] = uboBuffers[j].bufferMemory;
        }

        // Create descriptors
        if (i == 1 || i == 0) { // light descriptor
            descriptors->lightDescriptors(&arDescriptors[i]);
        } else { // Default descriptor
            descriptors->createDescriptors(&arDescriptors[i]);
            // Create initial texture
            arTextureSampler.resize(3);
            textureImageBuffer.resize(3);
            arTextureSampler[i].transferQueue = arEngine.graphicsQueue;
            arTextureSampler[i].transferCommandPool = arEngine.commandPool;
            textureImageBuffer[i].bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            textureImageBuffer[i].bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            videoTexture.transferQueue = arEngine.graphicsQueue;
            videoTexture.transferCommandPool = arEngine.commandPool;
            videoTextureBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            videoTextureBuffer.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            videoTexture.width = 1242;
            videoTexture.height = 375;
            videoTexture.channels = 1;
            // Create texture image
            textures->createTextureImage(&videoTexture, &videoTextureBuffer);

            textures->createTexture("default.jpg", &arTextureSampler[i], &textureImageBuffer[i]);
            // Create Texture sampler and add to descriptor set
            descriptors->createDescriptorsSampler(&arDescriptors[i], arTextureSampler[i]);
        }
        // Should be called for specific objects during loading
        // Initialize pipeline structs

        arPipelines[i].device = arEngine.mainDevice.device;
        arPipelines[i].swapchainImageFormat = arEngine.swapchainFormat;
        arPipelines[i].swapchainExtent = arEngine.swapchainExtent;
        // Create pipeline
        if (i == 1 || i == 0) {
            ArShadersPath shadersPath;
            shadersPath.fragmentShader = "../shaders/phongLightFrag";
            shadersPath.vertexShader = "../shaders/defaultVert";
            std::vector<VkDescriptorSetLayout> layouts = {arDescriptors[i].descriptorSetLayout,
                                                          arDescriptors[i].descriptorSetLayout2};
            pipeline.arLightPipeline(renderPass, layouts, shadersPath, &arPipelines[i]);
            fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);

        } else if (i == 2) {
            ArShadersPath shadersPath;
            shadersPath.fragmentShader = "../shaders/lampFrag";
            shadersPath.vertexShader = "../shaders/lampVert";
            std::vector<VkDescriptorSetLayout> layouts = {arDescriptors[i].descriptorSetLayout};
            pipeline.arLightPipeline(renderPass, layouts, shadersPath, &arPipelines[i]);
        } else {
            ArShadersPath shadersPath;
            shadersPath.fragmentShader = "../shaders/defaultFrag";
            shadersPath.vertexShader = "../shaders/defaultVert";
            std::vector<VkDescriptorSetLayout> layouts = {arDescriptors[i].descriptorSetLayout};
            pipeline.arLightPipeline(renderPass, layouts, shadersPath, &arPipelines[i]);

        }
    }
    // update command buffers
    recordCommand();

    // Translate models..
    for (int i = 0; i < models.size(); ++i) {
        glm::mat4 trans(1.0f);
        updateModel(glm::translate(trans, glm::vec3(i * 2, 0.0f, 0.0f)), i);

        if (i == 0){
            updateModel(glm::translate(trans, glm::vec3(0.0f, 0.0f, -5)), i);

        }

        if (i == 3){
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(i * 2, 0.0f, -4.0f));
            model = glm::scale(model, glm::vec3(10.0f, 3.0f, 1.0f));
            updateModel(model, i);

        }

        if (i == 2) {
            updateLightPos(glm::vec3(5.0f, -2.0f, 3.0f), trans, 2);

        }
    }
}

