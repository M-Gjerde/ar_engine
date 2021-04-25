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
#include "../pipeline/Mesh.h"
#include "../pipeline/Images.h"
#include "../pipeline/Textures.h"
#include "VulkanCompute.h"
#include "../record/ThreadSpawner.h"
#include "../pipeline/MeshModel.h"
#include "../Models/SceneObject.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>

class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);

    void updateModel(glm::mat4 newModel, int index);
    void updateLightPos(glm::vec3 newPos, glm::mat4 transMat, int index);
    void updateSpecularLightCamera(glm::vec3 newPos);

    void updateCamera(glm::mat4 newView, glm::mat4 newProjection);

    void updateTextureImage(std::string fileName); // TODO TEST METHOD
    void updateDisparityVideoTexture(); // TODO TEST METHOD
    void vulkanComputeShaders(); //TODO Test method

    void loadSphereObjects(); // TODO Test Method

    void draw();

    void cleanup();

    ~VulkanRenderer();

    void setupSceneFromFile(std::vector<std::map<std::string, std::string>> modelSettings);

    void updateScene();

    void deleteLastObject();

    void updateDisparityData();

    void startDisparityStream();
    void stopDisparityStream();

    bool takePhoto = false;

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

    // Compute pipeline
    VulkanCompute *vulkanCompute{};
    ArCompute arCompute;
    VkFence computeFence{};
    bool updateDisparity = false;

    // Buffer
    Buffer *buffer{};


    // Objects
    std::vector<MeshModel> models{};
    std::vector<SceneObject> cubes;

    // Descriptors
    Descriptors *descriptors{};
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


    void loadCubeObjects();

    void initComputePipeline();

};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
