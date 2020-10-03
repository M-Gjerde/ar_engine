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


    void
    createGraphicsPipeline(Utils::UdemyGraphicsPipeline *pipe);

    void
    createBoxPipeline(Utils::MainDevice mainDevice, VkExtent2D swapchainExtent, VkDescriptorSetLayout descriptorLayout,
                      Utils::Pipelines *pipelineToBind);
    void createAnotherRenderPass(Utils::MainDevice mainDevice, VkFormat swapchainImageFormat, Utils::Pipelines *pipes);

    void createRenderPass(Utils::MainDevice mainDevice, VkFormat swapChainImageFormat, VkRenderPass *renderPass);
    void createPushConstantRange(VkPushConstantRange *pPushConstantRange);


private:
    // Allocator
    VkAllocationCallbacks vkAllocationCallbacks = Allocator().operator VkAllocationCallbacks();

    // Vulkan components
    // - Descriptors

    static VkShaderModule createShaderModule(Utils::MainDevice mainDevice, const std::vector<char> &code) ;
};


#endif //AR_ENGINE_GRAPHICSPIPELINE_H
