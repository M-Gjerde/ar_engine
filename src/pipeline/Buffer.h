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
    Buffer(ArEngine engine);

    Buffer();

    ~Buffer();
    void cleanUp(ArBuffer _arBuffer);

    void createBuffer(ArBuffer *buffer);

    void copyBuffer(StandardModel modelInfo, ArBuffer srcBuffer, ArBuffer dstBuffer);

private:

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    uint32_t findMemoryTypeIndex(int32_t typeFilter, VkMemoryPropertyFlags properties);


    VkCommandBuffer beginCommandBuffer(VkCommandPool commandPool);

    void endAndSubmitCommandBuffer(StandardModel sModel, VkCommandBuffer transferCommandBuffer);
};



#endif //AR_ENGINE_BUFFER_H
