//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#include "stb_image.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include "utilities.h"
#include "Mesh.hpp"
#include "MeshModel.h"


class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);

    int createMeshModel(std::string modelFile);
    void updateModel(int modelId, glm::mat4 newModel);

    void draw();
    void cleanup();

    ~VulkanRenderer();

private:

    GLFWwindow *window{};
    int currentFrame = 0;

    // Scene Objects
    std::vector<MeshModel> modelList;


    // Scene Settings
    struct UboViewProjection {
        glm::mat4 projection;
        glm::mat4 view;
    }uboViewProjection
    {};

    //  Vulcan Components
    //  - Main
    VkInstance instance{};
    VkQueue graphicsQueue{};
    VkQueue presentationQueue{};
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkSurfaceKHR surface{};
    VkSwapchainKHR swapchain{};

    struct {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
    } mainDevice{};


    //  Following 3 handles are 1:1 connected
    std::vector<SwapchainImage> swapChainImages;
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


    VkDeviceSize minUniformBufferOffset{};
    //size_t modelUniformAlignment{};
    Model * modelTransferSpace{};

    // - Assets
    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImageMemory;
    std::vector<VkImageView> textureImageViews;

    // - Pipeline
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};
    VkRenderPass renderPass{};

    VkPipeline secondPipeline;
    VkPipelineLayout secondPipelineLayout;

    //  - Pools
    VkCommandPool graphicsCommandPool{};

    //  - Utility
    VkFormat  swapChainImageFormat;
    VkExtent2D swapChainExtent{};



    // - Synchronisation
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> drawFences;

    //  - Validation Layers
    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    //  Vulcan functions
    //  - Create functions
    void createInstance();
    void setDebugMessenger();
    void createLogicalDevice();
    void createSurface();
    void createSwapChain();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createPushConstantRange();
    void createGraphicsPipeline();
    void createColorBufferImage();
    void createDepthBufferImage();
    void createFramebuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();
    void createTextureSampler();

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createInputDescriptorSets();

    void updateUniformBuffers(uint32_t imageIndex);

    //  - Record functions
    void recordCommands(uint32_t currentImage);

    //  - Get functions
    void getPhysicalDevice();

    // - Allocate functions
    void allocateDynamicBufferTransferSpace();

    //  - Support functions
    //  -- Checker functions
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    static bool checkInstanceExtensionSupport(std::vector<const char *> *checkExtensions);
    bool checkDeviceSuitable(VkPhysicalDevice device);
    bool checkValidationLayerSupport();

    //  -- Getter functions
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

    //  -- Choose functions
    static VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
    static VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
    VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

    //  -- Create functions
    VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    static stbi_uc * loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);

    int createTextureImage(std::string fileName);
    int createTexture(std::string fileName);
    int createTextureDescriptor(VkImageView textureImage);


    //  -- Debugger functions
    static VkResult
    CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                 const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, [[maybe_unused]] void *pUserData);


};


#endif //UDEMY_VULCAN_VULKANRENDERER_HPP
