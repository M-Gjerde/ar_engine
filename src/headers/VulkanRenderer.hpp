//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cstdlib>
#include <vector>
#include <cstring>
#include <iostream>
#include <array>
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

#include "../libs/Utils.h"
#include "../include/utilities.h"
#include "Mesh.hpp"
#include "MeshModel.h"
#include "PlayerController.h"
#include "../include/stb_image.h"
#include "Vfd.h"
#include "../libs/TextureLoading.h"
#include "../libs/GraphicsPipeline.h"


class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);

    int createMeshModel(std::string modelFile);
    void updateModel(int modelId, glm::mat4 newModel);

    void draw();
    void cleanup();

    ~VulkanRenderer();

    // Scene settings
    PlayerController::UboViewProjection getPlayerController();
    void setPlayerController(PlayerController::UboViewProjection newUboViewProjection);

private:

    GLFWwindow *window{};
    int currentFrame = 0;

    // Create class handles
    Vfd vfd;
    TextureLoading textureLoading;
    GraphicsPipeline myPipe;

    // Scene Objects
    std::vector<MeshModel> modelList;

    // Scene Settings
    PlayerController playerController;

    //  Vulcan Components
    //  - Main
    VkInstance instance{};
    VkQueue graphicsQueue{};
    VkQueue presentationQueue{};
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkSurfaceKHR surface{};
    VkSwapchainKHR swapchain{};

    Utils::MainDevice mainDevice{};


    //  Following 3 handles are 1:1 connected
    std::vector<Utils::SwapchainImage> swapChainImages;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkImage> colorBufferImage{};
    std::vector<VkDeviceMemory> colorBufferImageMemory{};
    std::vector<VkImageView> colorBufferImageView{};

    std::vector<VkImage> depthBufferImage{};
    std::vector<VkDeviceMemory> depthBufferImageMemory{};
    std::vector<VkImageView> depthBufferImageView{};

    VkSampler textureSampler{};

    // - Descriptors
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayout samplerSetLayout{};
    VkDescriptorSetLayout inputSetLayout{};
    VkPushConstantRange pushConstantRange{};

    VkDescriptorPool descriptorPool = {};
    VkDescriptorPool samplerDescriptorPool{};
    VkDescriptorPool inputDescriptorPool{};
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> samplerDescriptorSets;     // Not chained to swapchain images
    std::vector<VkDescriptorSet> inputDescriptorSets;

    std::vector<VkBuffer> vpUniformBuffer;
    std::vector<VkDeviceMemory> vpUniformBufferMemory;

    std::vector<VkBuffer> modelDynamicUniformBuffer;
    std::vector<VkDeviceMemory> modelDynamicUniformBufferMemory;


    // - Assets
    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImageMemory;
    std::vector<VkImageView> textureImageViews;

    // - Pipeline
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};
    VkRenderPass renderPass{};

    VkPipeline secondPipeline{};
    VkPipelineLayout secondPipelineLayout{};

    //  - Pools
    VkCommandPool graphicsCommandPool{};

    //  - Utility
    VkFormat  swapChainImageFormat;
    VkExtent2D swapChainExtent{};

    // - Synchronisation
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> drawFences;


    //  Vulcan functions
    //  - Create functions
    void setDebugMessenger();
    void getLogicalDevice();
    void getQueues();
    void getSurface();
    void getSwapchain();
    void getRenderPass();
    void getDescriptorSetLayout();
    void getPushConstantRange();
    void getGraphicsPipeline();
    void createGraphicsPipeline();
    void createColorBufferImage();
    void createDepthBufferImage();
    void createFramebuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();
    void getTextureSampler();

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createInputDescriptorSets();

    void updateUniformBuffers(uint32_t imageIndex);

    //  - Record functions
    void recordCommands(uint32_t currentImage);

    //  - Get functions
    void getPhysicalDevice();
    void getVulkanInstance();

    // - Allocate functions
    void allocateDynamicBufferTransferSpace();

    //  - Support functions

    //  -- Getter functions
    Utils::QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

    //  -- Choose functions
    VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const;

    //  -- Create functions
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char> &code) const;

    int createTexture(std::string fileName);

};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
