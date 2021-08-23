//
// Created by magnus on 8/2/21.
//

#ifndef AR_ENGINE_GUI_H
#define AR_ENGINE_GUI_H

#include <vulkan/vulkan_core.h>
#include <ar_engine/src/pipeline/CommandBuffers.h>
#include "imgui.h"

#include "../pipeline/Images.h"
#include "../pipeline/Descriptors.h"


class GUI {
public:
    CommandBuffers *commandBuffers;
    UISettings uiSettings;
    std::vector<ArGuiSliderMeshGenerator> settings;

    explicit GUI(ArEngine mArEngine);

    ~GUI();

    void setSettings(std::vector<ArGuiSliderMeshGenerator> settings);
    std::vector<ArGuiSliderMeshGenerator> getSettings();

    void initResources(VkRenderPass renderPass);

    void updateBuffers();

    void cleanUp();

    void init(uint32_t width, uint32_t height);

    void newFrame(bool updateFrameGraph);

    void drawNewFrame(VkRenderPass renderPass, std::vector<VkFramebuffer> framebuffers);

    void drawNewFrame(VkCommandBuffer commandBuffer);

private:
    PushConstBlock pushConstBlock;


    VkDevice device;
    ArEngine arEngine;
    Images *images;
    Buffer *buffer;
    Descriptors *descriptors;
    Pipeline *pipeline;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
    VkMemoryRequirements memReqs;
    VkSampler sampler;

    Buffer *vertexBuffer;
    Buffer *indexBuffer;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;


    ArDescriptor descriptor;
    ArPipeline arPipeline;


};


#endif //AR_ENGINE_GUI_H
