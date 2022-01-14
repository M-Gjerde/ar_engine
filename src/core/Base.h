//
// Created by magnus on 11/27/21.
//

#ifndef AR_ENGINE_BASE_H
#define AR_ENGINE_BASE_H

#include <ar_engine/src/core/MyModel.h>
#include "ar_engine/src/imgui/UISettings.h"
#include "Camera.h"

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
    virtual void draw(VkCommandBuffer commandBuffer, uint32_t i){};
    virtual void updateUniformBufferData(uint32_t index, void *params, void *matrix, Camera* camera) {};

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

    /**@brief A standard set of uniform buffers */
    struct UniformBufferSet {
        Buffer vert;
        Buffer frag;
        Buffer fragSelect;
    };

protected:
    std::vector<UniformBufferSet> uniformBuffers;
    void createUniformBuffers(){
        uniformBuffers.resize(b.UBCount);

        for (auto &uniformBuffer: uniformBuffers) {

            b.device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffer.vert, sizeof(UBOMatrix));

            b.device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffer.frag, sizeof(FragShaderParams));

            b.device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffer.fragSelect, sizeof(float));

        }

    }

    void updateUniformBufferData(uint32_t index, void *params, void *matrix, void *selection) {

        // If initialized
        if(uniformBuffers.empty())
            return;

        UniformBufferSet currentUB = uniformBuffers[index];

        // TODO unceesarry mapping and unmapping occuring here maybe.
        currentUB.vert.map();
        memcpy(currentUB.vert.mapped, matrix, sizeof(UBOMatrix));
        currentUB.vert.unmap();

        currentUB.frag.map();
        memcpy(currentUB.frag.mapped, params, sizeof(FragShaderParams));
        currentUB.frag.unmap();

        currentUB.fragSelect.map();

        char *val = static_cast<char *>(selection);
        float f = (float) atoi(val);
        memcpy(currentUB.fragSelect.mapped, &f, sizeof(float));
        currentUB.fragSelect.unmap();

    }


};

#endif //AR_ENGINE_BASE_H
