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
#include "../include/triangle.h"

class Pipeline {

public:
    Pipeline();
    ~Pipeline();

    void createRenderPass(VkDevice device, VkFormat depthFormat, VkFormat colorFormat, VkRenderPass *pRenderPass);
    void
    createGraphicsPipeline(VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, ArPipeline *pipeline);
    void createRayTracingPipeline(ArPipeline *pipeline);
    void cleanUp(ArPipeline arPipeline) const;
private:

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

};


#endif //AR_ENGINE_PIPELINE_H
