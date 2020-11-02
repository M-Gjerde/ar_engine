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

    void copyBuffer(ArModel modelInfo, ArBuffer srcBuffer, ArBuffer dstBuffer);

    uint32_t findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties);

    VkCommandBuffer beginCommandBuffer(VkCommandPool commandPool);
    void endAndSubmitCommandBuffer(VkCommandBuffer transferCommandBuffer, VkQueue queue, VkCommandPool commandPool);

private:

    VkDevice device;
    VkPhysicalDevice physicalDevice;


};



#endif //AR_ENGINE_BUFFER_H
