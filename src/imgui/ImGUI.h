//
// Created by magnus on 9/5/21.
//

#ifndef AR_ENGINE_IMGUI_H
#define AR_ENGINE_IMGUI_H


#include <vulkan/vulkan.h>
#include <ar_engine/src/core/VulkanDevice.h>
#include <ar_engine/src/core/VulkanRenderer.h>
#include <ar_engine/src/core/Buffer.h>

class ImGUI {
private:
    // Vulkan resources for rendering the UI
    VkSampler sampler;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    int32_t vertexCount = 0;
    int32_t indexCount = 0;
    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
    VkImage fontImage = VK_NULL_HANDLE;
    VkImageView fontView = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    VulkanDevice *device;
    VulkanRenderer *example;

};


#endif //AR_ENGINE_IMGUI_H
