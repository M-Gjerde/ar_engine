//
// Created by magnus on 1/12/22.
//

#include "Cube.h"

void Cube::update() {
}

std::string Cube::getType() {
    return type;
}

void Cube::setup(Base::SetupVars vars) {
    printf("Cube setup\n");
    b.device = vars.device;
    b.UBCount = vars.UBCount;
    b.renderPass = vars.renderPass;
    vulkanDevice = vars.device;

    model.loadFromFile(Utils::getAssetsPath() + "models/Box/glTF-Embedded/Box.gltf", vars.device,vars.device->transferQueue, 1.0f);

}

void Cube::onUIUpdate(UISettings uiSettings) {
}

void Cube::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}

void Cube::updateUniformBufferData(uint32_t index, void *params, void *matrix) {
    UBOMatrixLight mat{};

    memcpy(&mat, matrix, sizeof(UBOMatrixLight));
    mat.model = glm::translate(mat.model, glm::vec3(0.0f, -1.0f, -3.0f));

    glTFModel::updateUniformBufferData(index, params, &mat, selection);

}

void Cube::prepareObject() {
    prepareUniformBuffers(b.UBCount);
    createDescriptorSetLayout();
    createDescriptors(b.UBCount);

    VkPipelineShaderStageCreateInfo vs = loadShader("myScene/box.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineShaderStageCreateInfo fs = loadShader("myScene/box.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {{vs}, {fs}};
    createPipeline(*b.renderPass, shaders);
}
