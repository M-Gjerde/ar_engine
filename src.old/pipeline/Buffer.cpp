//
// Created by magnus on 10/12/20.
//

#include <cstring>
#include "Buffer.h"


Buffer::Buffer(MainDevice mainDevice) {
    physicalDevice = mainDevice.physicalDevice;
    device = mainDevice.device;
}

void Buffer::cleanUp(ArBuffer _arBuffer) {
    vkFreeMemory(device, _arBuffer.bufferMemory, nullptr);
    vkDestroyBuffer(device, _arBuffer.buffer, nullptr);
}

void Buffer::createBuffer(ArBuffer *buffer) {

    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer->bufferSize;                                   // Size of buffer (size of 1 vertex * number of vertices) Size in memory
    bufferInfo.usage = buffer->bufferUsage;           // Types of buffer, however we want vertex buffer
    bufferInfo.sharingMode = buffer->sharingMode;             // Similar to swap chain images, can share vertex buffers
    bufferInfo.pQueueFamilyIndices = buffer->pQueueFamilyIndices;
    bufferInfo.queueFamilyIndexCount = buffer->queueFamilyIndexCount;

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

inline uint32_t Buffer::findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");

}

void Buffer::copyBuffer(const ArModel &modelInfo, ArBuffer srcBuffer, ArBuffer dstBuffer) {


    // Create buffer
    VkCommandBuffer transferCommandBuffer = beginCommandBuffer(modelInfo.transferCommandPool);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = srcBuffer.bufferSize;

    // Command to copy src.old buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &bufferCopyRegion);

    endAndSubmitCommandBuffer(transferCommandBuffer, modelInfo.transferQueue, modelInfo.transferCommandPool);


}

VkCommandBuffer Buffer::beginCommandBuffer(VkCommandPool commandPool) {

    // Command buffer details
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;

    // Command buffer to hold transfer commands
    VkCommandBuffer commandBuffer;
    // Allocate command buffer from pool
    vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

    // Information to begin the command buffer FaceAugment
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;      // We're only using the command buffer once, so set up for one time submit

    // Begin recording transfer commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void
Buffer::endAndSubmitCommandBuffer(VkCommandBuffer transferCommandBuffer, VkQueue queue, VkCommandPool commandPool) {

    // End commands
    vkEndCommandBuffer(transferCommandBuffer);

    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    // Submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    // Wait until this queue is done, then continue Not optimal for thousands of meshes
    vkQueueWaitIdle(queue);
    // Free temporary command buffer back to pool
    vkFreeCommandBuffers(device, commandPool, 1, &transferCommandBuffer);
}

/**
* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
*
* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
* @param offset (Optional) Byte offset from beginning
*
* @return VkResult of the buffer mapping call
*/
VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(device, bufferMemory, offset, size, 0, &mapped);
}

/**
* Unmap a mapped memory range
*
* @note Does not return a result as vkUnmapMemory can't fail
*/
void Buffer::unmap() {
    if (mapped) {
        vkUnmapMemory(device, bufferMemory);
        mapped = nullptr;
    }
}

/**
* Attach the allocated memory block to the buffer
*
* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
*
* @return VkResult of the bindBufferMemory call
*/
VkResult Buffer::bind(VkDeviceSize offset) {
    return vkBindBufferMemory(device, buffer, bufferMemory, offset);
}

/**
* Setup the default descriptor for this buffer
*
* @param size (Optional) Size of the memory range of the descriptor
* @param offset (Optional) Byte offset from beginning
*
*/
void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
    descriptor.offset = offset;
    descriptor.buffer = buffer;
    descriptor.range = size;
}

/**
* Copies the specified data to the mapped buffer
*
* @param data Pointer to the data to copy
* @param size Size of the data to copy in machine units
*
*/
void Buffer::copyTo(void *data, VkDeviceSize size) {
    assert(mapped);
    memcpy(mapped, data, size);
}

/**
* Flush a memory range of the buffer to make it visible to the device
*
* @note Only required for non-coherent memory
*
* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
* @param offset (Optional) Byte offset from beginning
*
* @return VkResult of the flush call
*/
VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = bufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

/**
* Invalidate a memory range of the buffer to make it visible to the host
*
* @note Only required for non-coherent memory
*
* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
* @param offset (Optional) Byte offset from beginning
*
* @return VkResult of the invalidate call
*/
VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = bufferMemory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
}

/**
* Release all Vulkan resources held by this buffer
*/
void Buffer::destroy() {
    if (buffer) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (bufferMemory) {
        vkFreeMemory(device, bufferMemory, nullptr);
    }
}

void Buffer::createBuffer() {

    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;                                   // Size of buffer (size of 1 vertex * number of vertices) Size in memory
    bufferInfo.usage = bufferUsage;           // Types of buffer, however we want vertex buffer
    bufferInfo.sharingMode = sharingMode;             // Similar to swap chain images, can share vertex buffers
    bufferInfo.pQueueFamilyIndices = pQueueFamilyIndices;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount;

    // Create buffer
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(
            (uint32_t)memRequirements.memoryTypeBits,              //Index of memory type on physical device that has required bit flags
            bufferProperties);                        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : CPU can interact without cache capabilities

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(device, buffer, bufferMemory, 0);

}

Buffer::Buffer() = default;

Buffer::~Buffer() = default;
