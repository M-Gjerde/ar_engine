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
        uint32_t UBCount = 0;
        VkRenderPass *renderPass{};

    } b;

    virtual ~Base() = default;

    virtual void update() = 0;
    virtual void setup(SetupVars vars) = 0;
    virtual void onUIUpdate(UISettings uiSettings) = 0;
    virtual std::string getType() {return type;}

    /**@brief Render Commands **/
    virtual void prepareObject(){};
    virtual void updateUniformBufferData(uint32_t index, void *params, void *matrix) {};
    virtual void draw(VkCommandBuffer commandBuffer, uint32_t i){};

    [[nodiscard]] VkPipelineShaderStageCreateInfo loadShader(const std::string& fileName, VkShaderStageFlagBits stage) const {
        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = stage;
        shaderStage.module = Utils::loadShader( (Utils::getShadersPath() + fileName).c_str(), b.device->logicalDevice);
        shaderStage.pName = "main";
        assert(shaderStage.module != VK_NULL_HANDLE);
        // TODO CLEANUP SHADERMODULES WHEN UNUSED
        return shaderStage;
    }

    std::string type = "None";
};

#endif //AR_ENGINE_BASE_H
