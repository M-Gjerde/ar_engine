//
// Created by magnus on 8/2/21.
//

#ifndef AR_ENGINE_GUI_H
#define AR_ENGINE_GUI_H

#include <vulkan/vulkan_core.h>
#include "../../external/imgui/imgui.h"
#include "../pipeline/Images.h"
#include "../pipeline/Descriptors.h"


class GUI {
public:

    explicit GUI(ArEngine mArEngine);
    ~GUI();

    void initResources(VkRenderPass renderPass);
    void cleanUp();

private:
    // Options and values to display/toggle from the UI
    struct UISettings {
        bool displayModels = true;
        bool displayLogos = true;
        bool displayBackground = true;
        bool animateLight = false;
        float lightSpeed = 0.25f;
        std::array<float, 50> frameTimes{};
        float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
        float lightTimer = 0.0f;
    } uiSettings;
    // UI params are set via push constants
    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;

    VkDevice device;
    ArEngine arEngine;
    Images* images;
    Buffer* buffer;
    Descriptors *descriptors;
    Pipeline* pipeline;

    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
    VkMemoryRequirements memReqs;
    VkSampler sampler;

    ArDescriptor descriptor;
    ArPipeline arPipeline;

    void init(int width, int height);

    void newFrame(bool updateFrameGraph);
};


#endif //AR_ENGINE_GUI_H
