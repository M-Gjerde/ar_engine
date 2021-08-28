//
// Created by magnus on 8/5/21.
//

#include "CommandBuffers.h"

VkCommandBufferAllocateInfo
CommandBuffers::commandBufferAllocateInfo(VkCommandPool pool, VkCommandBufferLevel level, uint32_t count) {
    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level = level;
    info.commandPool = pool;
    info.commandBufferCount = count;

    return info;
}

VkCommandPoolCreateInfo CommandBuffers::commandPoolCreateInfo(){
    VkCommandPoolCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;

    return info;
}

VkCommandBufferBeginInfo CommandBuffers::beginInfo(){
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;
    return info;
}

VkCommandBufferInheritanceInfo CommandBuffers::inheritanceInfo() {
    VkCommandBufferInheritanceInfo info;
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    info.pNext = nullptr;
    info.pipelineStatistics = 0;
    return info;
}
