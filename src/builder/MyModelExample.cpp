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


    vars.ui->dropDownItems.emplace_back("Grayscale");
    vars.ui->dropDownItems.emplace_back("Albedo");
    vars.ui->dropDownItems.emplace_back("Albedo + Normal");
}

void MyModelExample::update() {

}

void MyModelExample::onUIUpdate(UISettings uiSettings) {

    if (uiSettings.selectedDropDown == NULL)
        return;

    if (strcmp(uiSettings.selectedDropDown, "Grayscale") == 0){
        selection = (void *) "0";
    }
    if (strcmp(uiSettings.selectedDropDown, "Albedo") == 0){
        selection = (void *) "1";
    }
    if (strcmp(uiSettings.selectedDropDown, "Albedo + Normal") == 0){
        selection = (void *) "2";
    }

    printf("Selection %s\n", (char *) selection);
}

void MyModelExample::prepareObject(prepareVars vars) {
    prepareUniformBuffers(vars.UBCount);
    createDescriptorSetLayout();
    createDescriptors(vars.UBCount);
    createPipeline(*vars.renderPass, *vars.shaders);

}

void MyModelExample::updateUniformBufferData(uint32_t index, void *params, void *matrix) {
    glTFModel::updateUniformBufferData(index, params, matrix, selection);

}

void MyModelExample::draw(VkCommandBuffer commandBuffer, uint32_t i) {
    glTFModel::draw(commandBuffer, i);

}

std::string MyModelExample::getType() {
    return type;
}
