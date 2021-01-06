//
// Created by magnus on 7/4/20.
//




#include <array>
#include <glm/gtc/matrix_transform.hpp>
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
        else
            loadTypeTwoObject();

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

    // Create number of descriptors according to scene file
    ArDescriptor arDescriptor;
    arDescriptor.dataSizes.resize(2);
    arDescriptor.dataSizes[0] = sizeof(uboModel);
    arDescriptor.dataSizes[1] = sizeof(FragmentColor);

    arDescriptor.descriptorSets.resize(2);
    arDescriptor.descriptorSetLayouts.resize(2);

    std::vector<ArBuffer> buffers(2);

    // Create and fill buffers
    for (int j = 0; j < 2; ++j) {
        buffers[j].bufferSize = arDescriptor.dataSizes[j];
        buffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffers[j].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffers[j].bufferProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        buffer->createBuffer(&buffers[j]);
        arDescriptor.buffer.push_back(buffers[j].buffer);
        arDescriptor.bufferMemory.push_back(buffers[j].bufferMemory);
    }

    descriptors->lightDescriptors(&arDescriptor);

    ArPipeline arPipeline{};
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;

    // Create pipeline
    ArShadersPath shadersPath;
    shadersPath.vertexShader = "../shaders/defaultVert";
    shadersPath.fragmentShader = "../shaders/phongLightFrag";
    pipeline.arLightPipeline(renderPass, arDescriptor.descriptorSetLayouts, shadersPath, &arPipeline);
    fragmentColor.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    fragmentColor.objectColor = glm::vec4(0.5f, 0.5f, 0.31f, 0.0f);

    arDescriptors.push_back(arDescriptor);
    arPipelines.push_back(arPipeline);

}

void VulkanRenderer::loadTypeTwoObject() {

}

#define NUM_PARTICLES (1024*1024) // total number of particles to move
#define NUM_WORK_ITEMS_PER_GROUP 64 // # work-items per work-group
#define NUM_X_WORK_GROUPS ( NUM_PARTICLES / NUM_WORK_ITEMS_PER_GROUP )
struct pos {
    glm::vec4 pos; // positions
};
struct vel {
    glm::vec4 vel; // velocities
};
struct col {
    glm::vec4 col; // colors
};

#define TOP 2147483647. // 2^31 - 1

float
Ranf(float low, float high) {
    auto r = (float) rand();
    return low + r * (high - low) / (float) RAND_MAX;
}

void VulkanRenderer::vulkanComputeShaders() {

    // Create DescriptorBuffers
    ArBuffer posBuffer{};
    posBuffer.bufferSize = NUM_PARTICLES * sizeof(glm::vec4);
    posBuffer.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    posBuffer.bufferProperties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    posBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer->createBuffer(&posBuffer);

    // Create VulkanDataBuffer
    ArBuffer newBuffer{};
    newBuffer.bufferSize = NUM_PARTICLES * sizeof(glm::vec4);
    newBuffer.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    newBuffer.bufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    newBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer->createBuffer(&newBuffer);

    // Create DescriptorSets

    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorInfo.descriptorCount = 1;
    descriptorInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorInfo.binding = 0;
    descriptorInfo.bindingCount = 1;

    ArDescriptor arDescriptor{};
    arDescriptor.descriptorSets.resize(1);
    arDescriptor.descriptorSetLayouts.resize(1);
    arDescriptor.buffer.push_back(posBuffer.buffer);
    arDescriptor.bufferMemory.push_back(posBuffer.bufferMemory);

    descriptors->createDescriptors(descriptorInfo, &arDescriptor);

    ArPipeline computePipeline{};
    computePipeline.device = arEngine.mainDevice.device;

    pipeline.computePipeline(arDescriptor.descriptorSetLayouts, ArShadersPath(), &computePipeline);


    struct pos *positions{};
    vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0], 0, VK_WHOLE_SIZE, 0,
                reinterpret_cast<void **>(&positions));
    for (int i = 0; i < NUM_PARTICLES; i++) {
        positions[i].pos.x = 10;
        positions[i].pos.y = Ranf(0, 1);
        positions[i].pos.z = Ranf(0, 1);
        positions[i].pos.w = 1.;
    }
    vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0]);

    // --------- COMMAND BUFFERS ----------

    /*
     We are getting closer to the end. In order to send commands to the device(GPU),
     we must first record commands into a command buffer.
     To allocate a command buffer, we must first create a command pool. So let us do that.
     */
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = 0;
    // the queue family of this command pool. All command buffers allocated from this command pool,
    // must be submitted to queues of this family ONLY.
    commandPoolCreateInfo.queueFamilyIndex = 0;
    VkResult result = vkCreateCommandPool(arEngine.mainDevice.device, &commandPoolCreateInfo, NULL, &commandPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
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
    result = vkAllocateCommandBuffers(arEngine.mainDevice.device, &commandBufferAllocateInfo,
                                      &commandBuffer); // allocate command buffer.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
    /*
    Now we shall start recording commands into the newly allocated command buffer.
    */
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // the buffer is only submitted and used once in this application.
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo); // start recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffers");
    /*
    We need to bind a pipeline, AND a descriptor set before we dispatch.
    The validation layer will NOT give warnings if you forget these, so be very careful not to forget them.
    */
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0, 1,
                            &arDescriptor.descriptorSets[0], 0, NULL);

#define WIDTH 3200
#define HEIGHT 2400
#define WORKGROUP_SIZE 32

    /*
    Calling vkCmdDispatch basically starts the compute pipeline, and executes the compute shader.
    The number of workgroups is specified in the arguments.
    If you are already familiar with compute shaders from OpenGL, this should be nothing new to you.
    */
    vkCmdDispatch(commandBuffer, (uint32_t) ceil(WIDTH / float(WORKGROUP_SIZE)),
                  (uint32_t) ceil(HEIGHT / float(WORKGROUP_SIZE)), 1);

    result = vkEndCommandBuffer(commandBuffer); // end recording commands.
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffers");

    /*
Now we shall finally submit the recorded command buffer to a queue.
*/

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1; // submit a single command buffer
    submitInfo.pCommandBuffers = &commandBuffer; // the command buffer to submit.

    /*
      We create a fence.
    */
    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    result = vkCreateFence(arEngine.mainDevice.device, &fenceCreateInfo, NULL, &fence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create fence");

    /*
    We submit the command buffer on the queue, at the same time giving a fence.
    */
    result = vkQueueSubmit(arEngine.graphicsQueue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit queue");

    /*
    The command will not have finished executing until the fence is signalled.
    So we wait here.
    We will directly after this read our buffer from the GPU,
    and we will not be sure that the command has finished executing unless we wait for the fence.
    Hence, we use a fence here.
    */
    result = vkWaitForFences(arEngine.mainDevice.device, 1, &fence, VK_TRUE, 100000000000);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to wait for fence");

    vkDestroyFence(arEngine.mainDevice.device, fence, NULL);


    // ----------- Retrieve data from compute pipeline

    void* mappedMemory = NULL;
    // Map the buffer memory, so that we can read from it on the CPU.
    vkMapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0], 0, VK_WHOLE_SIZE, 0, &mappedMemory);
    pos * pmappedMemory = (pos *)mappedMemory;
    // Get the color data from the buffer, and cast it to bytes.
    // We save the data to a vector.
    std::vector<double> image;
    image.reserve(WIDTH * HEIGHT * 4);
    image.push_back(pmappedMemory[0].pos.x);

    // Done reading, so unmap.
    vkUnmapMemory(arEngine.mainDevice.device, arDescriptor.bufferMemory[0]);

}


