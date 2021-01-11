//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_STRUCTS_H
#define AR_ENGINE_STRUCTS_H


#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vector>
#include "triangle.h"
#include <string>
#include <array>

struct ArShadersPath {
    std::string vertexShader;
    std::string fragmentShader;
};

struct ArModel {
    VkQueue transferQueue;
    VkCommandPool transferCommandPool;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertexCount{};
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};
    uint32_t indexCount{};
    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};
    std::string modelName;
};

struct ArDescriptor {
    VkDescriptorPool pDescriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
    uint32_t descriptorSetLayoutCount;
    VkDescriptorSetLayout *pDescriptorSetLayouts;
    std::vector<uint32_t> dataSizes;
};

struct ArDescriptorInfo {
    const uint32_t *pBindings;
    VkDescriptorType *pDescriptorType;  // Array of different types 1:1 relationship
    uint32_t descriptorCount;           // Should be equal to all the numbers of pDescriptorSplitCount array summed
    uint32_t *pDescriptorSplitCount;
    VkShaderStageFlags* stageFlags;      // stageFlags for descriptors
    uint32_t descriptorSetLayoutCount;  // How many descriptor layouts
    uint32_t descriptorSetCount;
    uint32_t *dataSizes;
};


struct ArTextureImage {
    VkImageView textureImageView;
    VkImage textureImage;
    VkSampler textureSampler;
    VkDeviceMemory textureImageMemory;
    VkCommandPool transferCommandPool;
    VkQueue transferQueue;
    int width;
    int height;
    int channels;
    void *data;
};

struct ArBuffer {
    VkDeviceSize bufferSize;
    VkBufferUsageFlags bufferUsage;
    VkMemoryPropertyFlags bufferProperties;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
};


struct MainDevice {
    VkDevice device;
    VkPhysicalDevice physicalDevice;

};

struct ArEngine {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    MainDevice mainDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    VkCommandPool commandPool;
};

struct ArPipeline {
    VkDevice device;
    VkFormat swapchainImageFormat;
    VkFormat depthFormat;
    VkExtent2D swapchainExtent;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

struct ArDepthResource {
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkExtent2D swapChainExtent;
};

struct ArCompute {
    VkCommandBuffer commandBuffer;
    ArDescriptor descriptor;
};


#endif //AR_ENGINE_STRUCTS_H
