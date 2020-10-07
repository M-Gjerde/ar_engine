//
// Created by magnus on 10/7/20.
//

#ifndef AR_ENGINE_BUFFERCREATION_H
#define AR_ENGINE_BUFFERCREATION_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include "structs.h"
#include "../pipeline/Pipeline.h"

class Buffer {

public:
    Buffer(ArEngine engine);
    ~Buffer();
    void cleanUp(ArBuffer _arBuffer);
    void createArBuffer(ArBuffer *_arBuffer);

    void createBuffer(ArBuffer *buffer);

private:

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    uint32_t findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties);


};

Buffer::Buffer(ArEngine engine) {
    physicalDevice = engine.mainDevice.physicalDevice;
    device = engine.mainDevice.device;
}

void Buffer::cleanUp(ArBuffer _arBuffer) {
    vkFreeMemory(device, _arBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(device, _arBuffer.buffer, nullptr);
}

void Buffer::createArBuffer(ArBuffer *_arBuffer) {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &_arBuffer->buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, _arBuffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_arBuffer->bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, _arBuffer->buffer, _arBuffer->bufferMemory, 0);


}

void Buffer::createBuffer(ArBuffer *buffer) {


    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer->bufferSize;                                   // Size of buffer (size of 1 vertex * number of vertices) Size in memory
    bufferInfo.usage = buffer->bufferUsage;           // Types of buffer, however we want vertex buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;             // Similar to swap chain images, can share vertex buffers

    // Create buffer
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->buffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(device, buffer->buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(
            memRequirements.memoryTypeBits,              //Index of memory type on physical device that has required bit flags
            buffer->bufferProperties);                        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : CPU can interact without cache capabilities

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &buffer->bufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(device, buffer->buffer, buffer->bufferMemory, 0);

}

uint32_t Buffer::findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");

}

Buffer::~Buffer() = default;


#endif //AR_ENGINE_BUFFERCREATION_H
