//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_GRAPHICSPIPELINE_H
#define AR_ENGINE_GRAPHICSPIPELINE_H


#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

#include "../headers/PlayerController.h"

#include "Utils.h"


class GraphicsPipeline {
public:
    GraphicsPipeline();
    virtual ~GraphicsPipeline();

    void getRenderPass(Utils::MainDevice mainDevice, VkRenderPass *pRenderPass, VkFormat swapChainImageFormat);
    void getPushConstantRange(VkPushConstantRange* pPushConstantRange);
    void getDescriptorSetLayout(Utils::MainDevice mainDevice, VkDescriptorSetLayout *pDescriptorSetLayout,
                                VkDescriptorSetLayout *pDescriptorSetLayout1,
                                VkDescriptorSetLayout *pDescriptorSetLayout2);

    void getGraphicsPipeline(Utils::MainDevice mainDevice, VkExtent2D swapChainExtent, VkPipelineLayout * pPipelineLayout, VkPipeline * pPipeline, VkPipelineLayout * pPipelineLayout2, VkPipeline * pPipeline2 );

private:
    // Vulkan components
    // - Descriptors
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayout samplerSetLayout{};
    VkDescriptorSetLayout inputSetLayout{};
    VkPushConstantRange pushConstantRange{};

    // - Pipeline
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};
    VkRenderPass renderPass{};

    VkPipeline secondPipeline{};
    VkPipelineLayout secondPipelineLayout{};

    void createRenderPass(Utils::MainDevice mainDevice, VkFormat swapChainImageFormat);
    void createPushConstantRange();
    void createDescriptorSetLayout(Utils::MainDevice mainDevice);
    void createGraphicsPipeline(Utils::MainDevice mainDevice, VkExtent2D swapChainExtent);
    static VkShaderModule createShaderModule(Utils::MainDevice mainDevice, const std::vector<char> &code) ;
};


#endif //AR_ENGINE_GRAPHICSPIPELINE_H
