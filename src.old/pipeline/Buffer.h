//
// Created by magnus on 10/12/20.
//

#ifndef AR_ENGINE_BUFFER_H
#define AR_ENGINE_BUFFER_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdexcept>
#include "../include/structs.h"
#include "Pipeline.h"

class Buffer {

public:
    explicit Buffer(MainDevice mainDevice);

    Buffer();
    ~Buffer();

    void cleanUp(ArBuffer _arBuffer);
    virtual void createBuffer(ArBuffer *buffer);
    void createBuffer();
    void copyBuffer(const ArModel &modelInfo, ArBuffer srcBuffer, ArBuffer dstBuffer);
    uint32_t findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer beginCommandBuffer(VkCommandPool commandPool);
    void endAndSubmitCommandBuffer(VkCommandBuffer transferCommandBuffer, VkQueue queue, VkCommandPool commandPool);

    // ADDED FROM SASCHA WILLEMS BUFFER STRUCT
    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();
    VkResult bind(VkDeviceSize offset = 0);
    void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void copyTo(void *data, VkDeviceSize size);
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void destroy();

    // Unpacked ArBuffer struct
    VkDeviceSize bufferSize{};
    VkBufferUsageFlags bufferUsage{};
    VkMemoryPropertyFlags bufferProperties{};
    VkBuffer buffer{};
    VkDeviceMemory bufferMemory{};
    void* mapped = nullptr;
    VkSharingMode sharingMode{};
    uint32_t queueFamilyIndexCount{};
    const uint32_t *pQueueFamilyIndices{};

private:
    VkDevice device{};
    VkPhysicalDevice physicalDevice{};

    VkDescriptorBufferInfo descriptor;
};


#endif //AR_ENGINE_BUFFER_H
