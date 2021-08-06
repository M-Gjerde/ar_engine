//
// Created by magnus on 8/5/21.
//

#ifndef AR_ENGINE_COMMANDBUFFERS_H
#define AR_ENGINE_COMMANDBUFFERS_H


#include "../include/structs.h"
#include "../Models/SceneObject.h"

class CommandBuffers {
public:
    explicit CommandBuffers(ArEngine arEngine);

    void allocateCommandBuffers(uint32_t count, VkCommandBufferLevel level);

    void createCommandPool(VkCommandPoolCreateFlagBits flags);

    void startRecordCommand(VkRenderPass renderPass, std::vector<VkFramebuffer> framebuffers);

    void bindResources(std::vector<SceneObject> objects, std::vector<ArPipeline> pipelines,
                       std::vector<ArDescriptor> descriptors);

    void bindDescriptorSets(VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet);

    void bindPipeline(VkPipeline pipeline);
    void vertexBuffer(VkBuffer buffer, VkDeviceSize *offsets);

    void indexBuffer(VkBuffer buffer);
    void pushConstants(VkPipelineLayout pipelineLayout, uint32_t size, PushConstBlock pushConstBlock);
    void drawIndexed(uint32_t elements, uint32_t indexOffset, uint32_t vertexOffset);
    void setScissor(uint32_t x, uint32_t y, uint32_t z, uint32_t w);

    void endRecord();

    [[nodiscard]] const std::vector<VkCommandBuffer> &getCommandBuffers() const;

    void addCommandBuffer(VkCommandBuffer addBuffer);
    void removeCommandBuffer(uint32_t index);

    void cleanUp();
private:
    ArEngine arEngine;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandPool commandPool{};

    void createCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex) const;
    void allocateCommandBuffers(std::vector<VkCommandBuffer> *cmdBuffers, uint32_t count);


};


#endif //AR_ENGINE_COMMANDBUFFERS_H
