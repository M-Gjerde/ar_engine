//
// Created by magnus on 9/5/21.
//

#include "Sphere.h"


void Sphere::update() {
}

std::string Sphere::getType() {
    return type;
}

void Sphere::setup(Base::SetupVars vars) {
    printf("Sphere setup\n");

    this->vulkanDevice = vars.device;
    b.device = vars.device;
    b.UBCount = vars.UBCount;
    b.renderPass = vars.renderPass;

    std::string fileName;
    //loadFromFile(fileName);
    model.loadFromFile(Utils::getAssetsPath() + "models/sphere/sphere.gltf", vars.device,
                       vars.device->transferQueue, 1.0f);

}

void Sphere::onUIUpdate(UISettings uiSettings) {
}

void Sphere::prepareObject() {
    prepareUniformBuffers(b.UBCount);
    createDescriptorSetLayout();
    createDescriptors(b.UBCount);

    VkPipelineShaderStageCreateInfo vs = loadShader("myScene/sphere/sphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineShaderStageCreateInfo fs = loadShader("myScene/sphere/sphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {{vs}, {fs}};
    createPipeline(*b.renderPass, shaders);

}

void Sphere::updateUniformBufferData(uint32_t index, void *params, void *matrix) {

    glTFModel::updateUniformBufferData(index, params, matrix, selection);

}

void Sphere::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}