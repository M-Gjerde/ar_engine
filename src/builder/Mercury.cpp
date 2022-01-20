//
// Created by magnus on 9/5/21.
//

#include "Mercury.h"

#include <cmath>


void Mercury::setup(Base::SetupVars vars) {
    printf("Sphere setup\n");

    this->vulkanDevice = vars.device;
    b.device = vars.device;
    b.UBCount = vars.UBCount;
    b.renderPass = vars.renderPass;

    std::string fileName;
    //loadFromFile(fileName);
    model.loadFromFile(Utils::getAssetsPath() + "models/asteroid/asteroid_high_res.gltf", vars.device,
                       vars.device->transferQueue, 1.0f);

    model.setTexture(Utils::getAssetsPath() + "textures/mercury.jpg");


}


void Mercury::update() {
    mat.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    mat.model = glm::mat4(1.0f);
    float X = (std::sin(x ) * 2 - 1) * 5;
    float Z = (std::cos(x ) * 2 - 1) * 5;

    mat.model = glm::translate(mat.model, glm::vec3(X, 0.0f, Z));

    mat.model = glm::rotate(mat.model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));


    rotation += 0.01f;
    x += 0.00001f;

    if (rotation >= 360.0f)
        rotation = 0.0f;

    auto *d = (UBOMatrix *) data.matrix;
    d->model = mat.model;
}


void Mercury::onUIUpdate(UISettings uiSettings) {

}

void Mercury::prepareObject() {
    createUniformBuffers();

    createDescriptorSetLayout();
    createDescriptors(b.UBCount, Base::uniformBuffers);

    VkPipelineShaderStageCreateInfo vs = loadShader("myScene/sphere/sphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    VkPipelineShaderStageCreateInfo fs = loadShader("myScene/sphere/sphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {{vs},
                                                            {fs}};
    createPipeline(*b.renderPass, shaders);

}


void Mercury::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}


std::string Mercury::getType() {
    return type;
}