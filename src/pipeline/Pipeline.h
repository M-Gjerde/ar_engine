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

    void createGraphicsPipeline(ArPipeline *pipeline, VkDescriptorSetLayout descriptorSetLayout);
    void cleanUp() const;
private:

    ArPipeline arPipeline{};

    VkShaderModule createShaderModule(const std::vector<char>&code);
    void createRenderPass();

};


#endif //AR_ENGINE_PIPELINE_H
