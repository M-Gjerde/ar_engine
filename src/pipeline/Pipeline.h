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
    Pipeline();

    ~Pipeline();

    void createRenderPass(VkDevice device, VkFormat depthFormat, VkFormat colorFormat, VkRenderPass *pRenderPass);

    void arLightPipeline(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
                         const ArShadersPath &shaderPath, ArPipeline *pipeline);

    void cleanUp(ArPipeline arPipeline) const;


    void
    computePipeline(ArDescriptor arDescriptor, const ArShadersPath &shaderPath, ArPipeline *pipeline);

private:

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);


};


#endif //AR_ENGINE_PIPELINE_H
