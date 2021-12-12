//
// Created by magnus on 12/10/21.
//

#include "MyModelExample.h"

void MyModelExample::setup(Base::SetupVars vars) {
    printf("MyModelExample setup\n");
    this->device = vars.device;

    std::string fileName;
    //loadFromFile(fileName);
    model.loadFromFile(Utils::getAssetsPath() + "models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf", vars.device,
                       vars.device->transferQueue, 1.0f);

}

void MyModelExample::update() {

}

void MyModelExample::onUIUpdate(UISettings uiSettings) {

}

void MyModelExample::prepareObject(prepareVars vars) {
    prepareUniformBuffers(vars.UBCount);
    createDescriptorSetLayout();
    createDescriptors(vars.UBCount);
    createPipeline(*vars.renderPass, *vars.shaders);

}

void MyModelExample::updateUniformBufferData(uint32_t index, void *params, void *matrix) {
    glTFModel::updateUniformBufferData(index, params, matrix);

}

void MyModelExample::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}

std::string MyModelExample::getType() {
    return type;
}
