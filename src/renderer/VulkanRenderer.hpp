//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#define VK_ENABLE_BETA_EXTENSIONS
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../pipeline/Buffer.h"
#include "../Platform/Platform.h"
#include "../include/structs.h"
#include "../pipeline/Pipeline.h"
#include "../pipeline/Descriptors.h"
#include "../include/settings.h"
#include "../pipeline/Mesh.h"
#include <stdexcept>
#include <vector>
#include <iostream>

class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);

    void updateModel(glm::mat4 newModel);
    void updateCamera(glm::mat4 newView, glm::mat4 newProjection);

    void draw();
    void cleanup();

    ~VulkanRenderer();


private:

    // Vulkan components
    Platform *platform{};
    ArEngine arEngine;

    // - Pipelines
    Pipeline pipeline;
    ArPipeline arPipeline{};

    // Buffer
    Buffer *buffer{};
    std::vector<ArBuffer> uboBuffers;
    uboModel uboModelVar{};

    // Objects
    Mesh *mesh;
    StandardModel triangleModel{};

    // - Descriptors
    Descriptors *descriptors{};
    ArDescriptor arDescriptor;

    // - Drawing
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    // - Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    void createPipeline();
    void createFrameBuffers();
    void createCommandBuffers();

    void recordCommand();

    void createSyncObjects();

    void updateBuffer(uint32_t imageIndex) const;

    void createUboBuffer();

    void createVertexBuffer();
};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
