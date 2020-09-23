//
// Created by magnus on 9/20/20.
//

#ifndef AR_ENGINE_GRAPHICSPIPELINE_H
#define AR_ENGINE_GRAPHICSPIPELINE_H


#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

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


    void
    createGraphicsPipeline(Utils::MainDevice mainDevice, VkExtent2D swapchainExtent, Utils::Pipelines *pipelineObject,
                           Utils::Pipelines *secondPipeline);

    void createBoxPipeline(Utils::MainDevice mainDevice, VkExtent2D swapchainExtent, Utils::Pipelines *pipelineToBind);
    void createAnotherRenderPass(Utils::MainDevice mainDevice, VkFormat swapchainImageFormat, Utils::Pipelines *pipes);

    void createRenderPass(Utils::MainDevice mainDevice, VkFormat swapChainImageFormat, Utils::Pipelines *renderPass);


private:
    // Allocator
    VkAllocationCallbacks vkAllocationCallbacks = Allocator().operator VkAllocationCallbacks();

    // Vulkan components
    // - Descriptors
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSetLayout samplerSetLayout{};
    VkDescriptorSetLayout inputSetLayout{};
    VkPushConstantRange pushConstantRange{};


    Utils::Pipelines boxPipeline;

    void createPushConstantRange();
    void createDescriptorSetLayout(Utils::MainDevice mainDevice);
    static VkShaderModule createShaderModule(Utils::MainDevice mainDevice, const std::vector<char> &code) ;
};


#endif //AR_ENGINE_GRAPHICSPIPELINE_H
