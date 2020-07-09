//
// Created by magnus on 7/4/20.
//

#ifndef UDEMY_VULCAN_VULKANRENDERER_HPP
#define UDEMY_VULCAN_VULKANRENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include "utilities.h"
#include "Mesh.hpp"

class VulkanRenderer {

public:
    VulkanRenderer();

    int init(GLFWwindow *newWindow);

    void updateModel(int modelId, glm::mat4 newModel);

    void draw();
    void cleanup();

    ~VulkanRenderer();

private:

    GLFWwindow *window{};
    int currentFrame = 0;

    // Scene Objects
    std::vector<Mesh> meshList;

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

    VkImage depthBufferImage{};
    VkDeviceMemory depthBufferImageMemory{};
    VkImageView depthBufferImageView{};

    // - Descriptors
    VkDescriptorSetLayout descriptorSetLayout{};
    VkPushConstantRange pushConstantRange{};

    VkDescriptorPool descriptorPool = {};
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkBuffer> vpUniformBuffer;
    std::vector<VkDeviceMemory> vpUniformBufferMemory;

    std::vector<VkBuffer> modelDynamicUniformBuffer;
    std::vector<VkDeviceMemory> modelDynamicUniformBufferMemory;


    VkDeviceSize minUniformBufferOffset{};
    //size_t modelUniformAlignment{};
    Model * modelTransferSpace{};

    // - Pipeline
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};
    VkRenderPass renderPass{};

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
    void createDepthBufferImage();
    void createFramebuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

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
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char> &code) const;

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
