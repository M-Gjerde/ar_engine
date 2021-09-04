//
// Created by magnus on 9/4/21.
//

#ifndef AR_ENGINE_POPULATE_H
#define AR_ENGINE_POPULATE_H

namespace Populate {

    inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
    {
        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = flags;
        return fenceCreateInfo;
    }

    inline VkSemaphoreCreateInfo semaphoreCreateInfo() {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        return semaphoreCreateInfo;
    }

    inline VkSubmitInfo submitInfo()
    {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        return submitInfo;
    }

    inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
            VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t bufferCount)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = level;
        commandBufferAllocateInfo.commandBufferCount = bufferCount;
        return commandBufferAllocateInfo;
    }
}


#endif //AR_ENGINE_POPULATE_H
