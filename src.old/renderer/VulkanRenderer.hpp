//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP



#include "GUI.h"
#include "VulkanCompute.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>
#include <random>
#include <ar_engine/src/Platform/threadPool.h>
#include <ar_engine/src/Platform/Platform.h>
#include <ar_engine/src/FaceAugment/FaceDetector.h>
#include <ar_engine/src/pipeline/Textures.h>
#include <ar_engine/src/include/MeshGenerator.h>

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

    void updateUI(UISettings uiSettings);

private:
    // Number of animated objects to be renderer
    // by using threads and secondary command buffers
    uint32_t numObjectsPerThread;

    // Multi threaded stuff
    // Max. number of concurrent threads
    uint32_t numThreads;

    struct ThreadData {
        VkCommandPool commandPool;
        // One command buffer per render object
        std::vector<VkCommandBuffer> commandBuffer;
        // One push constant block per render object
        std::vector<ThreadPushConstantBlock> pushConstBlock;
        // Per object information (position, rotation, etc.)
        std::vector<SceneObject> objectData;
    };
    std::vector<ThreadData> threadData;
    ar::ThreadPool threadPool;
    VkCommandBuffer primaryBuffer{};

    CommandBuffers::SecondaryCommandBuffers secondaryCmdBuffers{};

    std::default_random_engine rndEngine;

    // Vulkan components
    ar::Platform *platform{};
    ArEngine arEngine;

    // Pipelines and drawing stuff
    Pipeline pipeline;
    std::vector<ArPipeline> arPipelines{};
    Images *images{};
    ArDepthResource arDepthResource{};
    VkRenderPass renderPass{};
    VkRenderPass textRenderPass{};
    std::vector<VkCommandBuffer> commandBuffers;

    // UI components
    bool visible = false;
    GUI *gui{};
    uint32_t frameNumber = 0;
    std::vector<ArMeshInfoUI> settings;
    MeshGenerator *meshGenerator{};

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
    //std::vector<VkCommandBuffer> commandBuffers;
    //CommandBuffers* commandBuffers{};

    // - Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    void createFrameBuffersAndRenderPass();
    void createCommandBuffers();
    void createSyncObjects();
    void updateBuffer(uint32_t imageIndex);
    void initComputePipeline();


    void recordCommands();

    void prepareMultiThreadedRenderer();

    float rnd(float range);

    void
    threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

    void updateSecondaryCommandBuffers(VkCommandBufferInheritanceInfo inheritanceInfo);

    void updateCommandBuffers(VkFramebuffer frameBuffer);

    void onUIUpdate();
};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
