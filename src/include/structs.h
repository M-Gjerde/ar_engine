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

struct ArDescriptor{
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<size_t> dataSizes;


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
    void* data;
};

struct ArBuffer {
    VkDeviceSize bufferSize;
    VkBufferUsageFlags bufferUsage;
    VkMemoryPropertyFlags bufferProperties;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
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


#endif //AR_ENGINE_STRUCTS_H
