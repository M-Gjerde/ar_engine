//
// Created by magnus on 8/5/21.
//

#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <utility>
#include "CommandBuffers.h"
#include "../Platform/Platform.h"
#include "../Models/SceneObject.h"

void CommandBuffers::createCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex) const {

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(arEngine.mainDevice.device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandBuffers::allocateCommandBuffers(std::vector<VkCommandBuffer> *cmdBuffers, uint32_t count) {
    cmdBuffers->resize(count);

    // TEXTRENDER ALSO ALLOCATE BUFFERS
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = arEngine.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = (uint32_t) cmdBuffers->size();

    if (vkAllocateCommandBuffers(arEngine.mainDevice.device, &allocateInfo, cmdBuffers->data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void CommandBuffers::allocateCommandBuffers(uint32_t count, VkCommandBufferLevel level) {
    commandBuffers.resize(count);

    // TEXTRENDER ALSO ALLOCATE BUFFERS
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = arEngine.commandPool;
    allocateInfo.level = level;
    allocateInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(arEngine.mainDevice.device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}


CommandBuffers::CommandBuffers(ArEngine arEngine_) {
    arEngine = std::move(arEngine_);

}

void CommandBuffers::createCommandPool(VkCommandPoolCreateFlagBits flags) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = Platform::findQueueFamilies(arEngine.mainDevice.physicalDevice,
                                                            arEngine.surface).graphicsFamily.value();
    poolInfo.flags = flags; // Optional

    if (vkCreateCommandPool(arEngine.mainDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandBuffers::startRecordCommand(VkRenderPass renderPass, std::vector<VkFramebuffer> framebuffers) {

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = arEngine.swapchainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.10f, 0.35f, 0.15f, 1.0f}; // green
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    }
}

void CommandBuffers::bindDescriptorSets(VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet){
    for (auto & commandBuffer : commandBuffers) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }
}

void CommandBuffers::bindPipeline(VkPipeline pipeline){
    for (auto & commandBuffer : commandBuffers) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }
}

void CommandBuffers::pushConstants(VkPipelineLayout pipelineLayout, uint32_t size, PushConstBlock pushConstBlock){
    for (auto & commandBuffer : commandBuffers) {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
    }
}

void CommandBuffers::vertexBuffer(VkBuffer buffer, VkDeviceSize* offsets){
    for (auto & commandBuffer : commandBuffers) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, offsets);
    }
}

void CommandBuffers::indexBuffer(VkBuffer buffer) {
    for (auto & commandBuffer : commandBuffers) {
        vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT16);

    }
}

void CommandBuffers::drawIndexed(uint32_t elements, uint32_t indexOffset, uint32_t vertexOffset) {
    for (auto & commandBuffer : commandBuffers) {
        vkCmdDrawIndexed(commandBuffer,elements, 1, indexOffset, vertexOffset, 0);

    }
}

void CommandBuffers::setScissor(uint32_t x,uint32_t y, uint32_t z, uint32_t w) {
    for (auto & commandBuffer : commandBuffers) {
        VkRect2D scissorRect;
        scissorRect.offset.x = std::max((int32_t)(x), 0);
        scissorRect.offset.y = std::max((int32_t)(y), 0);
        scissorRect.extent.width = (uint32_t)(z - x);
        scissorRect.extent.height = (uint32_t)(w - y);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
    }
}

// Bind resources for Scene objects which typically indlues a pipeline and descriptor bind
void CommandBuffers::bindResources(std::vector<SceneObject> objects, std::vector<ArPipeline> pipelines,
                                   std::vector<ArDescriptor> descriptors) {
    for (size_t i = 0; i < commandBuffers.size(); i++) {

        for (int j = 0; j < objects.size(); ++j) {
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[j].pipeline);

            VkBuffer vertexBuffers[] = {objects[j].getVertexBuffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], objects[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelines[j].pipelineLayout, 0, descriptors[j].descriptorSets.size(),
                                    descriptors[j].descriptorSets.data(), 0,
                                    nullptr);

            vkCmdDrawIndexed(commandBuffers[i], objects[j].getIndexCount(), 1, 0, 0, 0);

        }


    }
}

void CommandBuffers::endRecord() {
    for (size_t i = 0; i < commandBuffers.size(); i++) {

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to FaceAugment command buffer!");
        }
    }
}

const std::vector<VkCommandBuffer> &CommandBuffers::getCommandBuffers() const {
    return commandBuffers;
}

void CommandBuffers::addCommandBuffer(VkCommandBuffer addBuffer) {
    commandBuffers.push_back(addBuffer);
}

void CommandBuffers::removeCommandBuffer(uint32_t index){
    commandBuffers.erase(commandBuffers.begin() + index);
}

void CommandBuffers::cleanUp() {
    vkDestroyCommandPool(arEngine.mainDevice.device, commandPool, nullptr);

}
