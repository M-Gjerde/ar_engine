//
// Created by magnus on 8/5/21.
//

#ifndef AR_ENGINE_COMMANDBUFFERS_H
#define AR_ENGINE_COMMANDBUFFERS_H


#include "../include/structs.h"
#include "../Models/SceneObject.h"

class CommandBuffers {
public:

    // Secondary scene command buffers used to store backdrop and user interface
    struct SecondaryCommandBuffers {
        VkCommandBuffer objects;
        VkCommandBuffer ui;
    } secondaryCommandBuffers;


    static VkCommandBufferAllocateInfo
    commandBufferAllocateInfo(VkCommandPool pool, VkCommandBufferLevel level, uint32_t count);

    static VkCommandPoolCreateInfo commandPoolCreateInfo();

    void cleanUp();

    static VkCommandBufferBeginInfo beginInfo();
    static VkCommandBufferInheritanceInfo inheritanceInfo();

private:

    VkCommandBuffer primaryCommandBuffer;


};


#endif //AR_ENGINE_COMMANDBUFFERS_H
