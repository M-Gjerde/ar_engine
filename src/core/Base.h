//
// Created by magnus on 11/27/21.
//

#ifndef AR_ENGINE_BASE_H
#define AR_ENGINE_BASE_H

#include <ar_engine/src/core/MyModel.h>
#include "ar_engine/src/imgui/UISettings.h"
class Base {
public:

    struct SetupVars {
        VulkanDevice* device{};
        UISettings* ui{};
    };

    struct prepareVars {
        uint32_t UBCount = 0;
        std::vector<VkPipelineShaderStageCreateInfo> *shaders{};
        std::vector<VkPipelineShaderStageCreateInfo> *shaders2{};
        VkRenderPass *renderPass{};
    };
    virtual ~Base() = default;

    virtual void update() = 0;
    virtual void setup(SetupVars vars) = 0;
    virtual void onUIUpdate(UISettings uiSettings) = 0;
    virtual std::string getType() {return type;}

    /**@brief Render Commands **/
    virtual void prepareObject(prepareVars vars){};
    virtual void updateUniformBufferData(uint32_t index, void *params, void *matrix){};
    virtual void draw(VkCommandBuffer commandBuffer, uint32_t i){};
    std::string type = "None";
};

#endif //AR_ENGINE_BASE_H
