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

    void recordCommand(VkRenderPass renderPass, std::vector<VkFramebuffer> framebuffers);
    void endRecord();
    void bindResources(std::vector<SceneObject> objects, std::vector<ArPipeline> pipelines,
                       std::vector<ArDescriptor> descriptors);

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
