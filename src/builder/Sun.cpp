//
// Created by magnus on 1/12/22.
//

#include "Sun.h"

void Sun::setup(Base::SetupVars vars) {
    printf("Cube setup\n");
    b.device = vars.device;
    b.UBCount = vars.UBCount;
    b.renderPass = vars.renderPass;
    vulkanDevice = vars.device;

    model.loadFromFile(Utils::getAssetsPath() + "models/sphere/sphere.gltf", vars.device,vars.device->transferQueue, 1.0f);

}


void Sun::update() {
    mat.model = glm::mat4(1.0f);
    mat.model = glm::scale(mat.model, glm::vec3(0.2f, 0.2f, 0.2f));
    mat.model = glm::rotate(mat.model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    rotation += 0.01f;

    if (rotation >= 360.0f)
        rotation = 0.0f;

    auto *d = (UBOMatrix *) data.matrix;
    d->model = mat.model;

}

std::string Sun::getType() {
    return type;
}


void Sun::onUIUpdate(UISettings uiSettings) {
}

void Sun::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}


void Sun::prepareObject() {
    createUniformBuffers();
    createDescriptorSetLayout();
    createDescriptors(b.UBCount, uniformBuffers);

    VkPipelineShaderStageCreateInfo vs = loadShader("myScene/spv/box.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineShaderStageCreateInfo fs = loadShader("myScene/spv/box.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {{vs}, {fs}};
    createPipeline(*b.renderPass, shaders);
}
