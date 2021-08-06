//
// Created by magnus on 10/6/20.
//

#ifndef AR_ENGINE_PIPELINE_H
#define AR_ENGINE_PIPELINE_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <fstream>
#include <vector>
#include "../include/load_file.h"
#include "../include/structs.h"

class Pipeline {

public:
    struct PushConstBlock {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstBlock;


    Pipeline();

    ~Pipeline();

    void createRenderPass(VkDevice device, VkFormat depthFormat, VkFormat colorFormat, VkRenderPass *pRenderPass);

    void arLightPipeline(VkRenderPass renderPass, ArDescriptor arDescriptor,
                         const ArShadersPath &shaderPath, ArPipeline *pipeline);

    void cleanUp(ArPipeline arPipeline) const;

    void textRenderPipeline(VkRenderPass renderPass, ArDescriptor arDescriptor, const ArShadersPath &shaderPath,
                            ArPipeline *pipeline, VkPipelineCache cache);

    void
    computePipeline(ArDescriptor arDescriptor, const ArShadersPath &shaderPath, ArPipeline *pipeline);

    static VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkPolygonMode polygonMode,
                                                                               VkCullModeFlags cullMode,
                                                                               VkFrontFace frontFace,
                                                                               VkPipelineRasterizationStateCreateFlags flags = 0);

    static VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable);

    static VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo(
            VkBool32 depthTestEnable,
            VkBool32 depthWriteEnable,
            VkCompareOp depthCompareOp);

    static VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo(
            uint32_t attachmentCount,
            const VkPipelineColorBlendAttachmentState * pAttachments);

    static VkPipelineViewportStateCreateInfo
    viewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount,
                            VkPipelineViewportStateCreateFlags flags);

    static VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples,
            VkPipelineMultisampleStateCreateFlags flags = 0);

    static VkGraphicsPipelineCreateInfo createInfo(
            VkPipelineLayout layout,
            VkRenderPass renderPass,
            VkPipelineCreateFlags flags = 0);

    static VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo(
            const std::vector<VkDynamicState>& pDynamicStates,
            VkPipelineDynamicStateCreateFlags flags = 0);

    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);


};


#endif //AR_ENGINE_PIPELINE_H
