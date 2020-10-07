//
// Created by magnus on 10/3/20.
//

#ifndef AR_ENGINE_STRUCTS_H
#define AR_ENGINE_STRUCTS_H


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

struct ArDescriptor{
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
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
    VkExtent2D swapchainExtent;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkRenderPass renderPass;
};


#endif //AR_ENGINE_STRUCTS_H
