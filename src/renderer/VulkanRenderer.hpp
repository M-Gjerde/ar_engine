//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP


#include "../pipeline/Buffer.h"
#include "../Platform/Platform.h"
#include "../include/structs.h"
#include "../pipeline/Pipeline.h"
#include "../pipeline/Descriptors.h"
#include "../include/settings.h"
#include "../Models/Mesh.h"
#include "../pipeline/Images.h"
#include "../pipeline/Textures.h"
#include "VulkanCompute.h"
#include "../FaceAugment/ThreadSpawner.h"
#include "../Models/MeshModel.h"
#include "../Models/SceneObject.h"
#include "../FaceAugment/FaceDetector.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>

class VulkanRenderer {

public:
    std::vector<SceneObject> objects;



    VulkanRenderer();
    int init(GLFWwindow *newWindow);
    void updateCamera(glm::mat4 newView, glm::mat4 newProjection);
    void updateTextureImage(std::string fileName); // TODO TEST METHOD
    void updateDisparityVideoTexture(); // TODO TEST METHOD
    void draw();
    void cleanup();
    ~VulkanRenderer();
    void setupSceneFromFile(std::vector<std::map<std::string, std::string>> modelSettings);
    [[nodiscard]] std::vector<SceneObject> getSceneObjects() const;
    void updateDisparityData();
    void startDisparityStream();
    void stopDisparityStream();
    void testFunction(); // TODO implement function

private:
    // Vulkan components
    Platform *platform{};
    ArEngine arEngine;

    // Pipelines and drawing stuff
    Pipeline pipeline;
    std::vector<ArPipeline> arPipelines{};
    Images *images{};
    ArDepthResource arDepthResource{};
    VkRenderPass renderPass{};
    VkRenderPass textRenderPass{};

    // Compute pipeline
    VulkanCompute *vulkanCompute{};
    VkFence computeFence{};
    bool updateDisparity = false;
    FaceDetector faceDetector;
    ThreadSpawner threadSpawner;
    ArSharedMemory *memP{};

    // Buffer
    Buffer *buffer{}; // TODO To be removed

    // Objects
    //std::vector<MeshModel> models{};

    // Descriptors
    Descriptors *descriptors{}; // TODO To be removed
    std::vector<ArDescriptor> arDescriptors;

    // - Data structures for descriptors
    uboModel uboModelVar{};
    FragmentColor fragmentColor{};

    // Textures
    Textures *textures{};
    std::vector<ArTextureImage> arTextureSampler{};
    std::vector<ArBuffer> textureImageBuffer{};

    ArTextureImage disparityTexture{};
    ArBuffer disparityTextureBuffer{};

    ArTextureImage videoTexture{};
    ArBuffer videoTextureBuffer{};

    // - Drawing
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    // - Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    void createFrameBuffersAndRenderPass();
    void createCommandBuffers();
    void recordCommand();
    void createSyncObjects();
    void updateBuffer(uint32_t imageIndex);
    void initComputePipeline();

    void textRenderTest();
};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
