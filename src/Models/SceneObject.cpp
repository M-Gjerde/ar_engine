//
// Created by magnus on 4/25/21.
//

#include <string>
#include <map>
#include <utility>
#include <glm/ext/matrix_transform.hpp>
#include <sstream>
#include "SceneObject.h"
#include "../include/structs.h"
#include "../pipeline/MeshModel.h"

SceneObject::SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine) {
    arEngine = std::move(mArEngine);
    // Helper classes handles
    descriptors = new Descriptors(arEngine);
    buffer = new Buffer(arEngine.mainDevice);

    // Create pipeline for object
    shadersPath.vertexShader = "../shaders/" + modelSettings.at("vertex_shader");
    shadersPath.fragmentShader = "../shaders/" + modelSettings.at("fragment_shader");

    // Create mesh from model type
    createMesh(modelSettings.at("type"));
    // Get descriptorInfo from file
    getDescriptorInfo(modelSettings);
    // Get object position
    getSceneObjectPose(modelSettings);



}

void SceneObject::getSceneObjectPose(std::map<std::string, std::string> modelSettings){
    float posX = std::stof(modelSettings.at("posX"));
    float posY = std::stof(modelSettings.at("posY"));
    float posZ = std::stof(modelSettings.at("posZ"));
    float rotX = std::stof(modelSettings.at("rotX"));
    float rotY = std::stof(modelSettings.at("rotY"));
    float rotZ = std::stof(modelSettings.at("rotZ"));
    rotAngle = std::stof(modelSettings.at("rotAngle"));
    posVector = glm::vec3(posX, posY, posZ);
    rotVector = glm::vec3(rotX, rotY, rotZ);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), posVector);
    model = glm::rotate(model, rotAngle, rotVector);
    meshModel.setModel(model);

}
void SceneObject::getDescriptorInfo(std::map<std::string, std::string> modelSettings) {
    // --- READ DESCRIPTOR INFO
    descriptorInfo.descriptorSetCount = std::stoi(modelSettings.at("descriptorSetCount"));
    descriptorInfo.descriptorSetLayoutCount = std::stoi(modelSettings.at("descriptorSetLayoutsCount"));;
    descriptorInfo.descriptorCount = std::stoi(modelSettings.at("descriptorCount"));
    descriptorInfo.descriptorPoolCount = std::stoi(modelSettings.at("descriptorPoolCount"));
    // Descriptor bindings
    std::vector<uint32_t> bindings;
    getDescriptorInfoSequence("descriptorBindingSequence", &bindings, modelSettings);
    descriptorInfo.pBindings = bindings.data();
    // Buffer sizes
    std::vector<uint32_t> dataSizes;
    getDescriptorInfoSequence("descriptorDataSizeSequence", &dataSizes, modelSettings);
    descriptorInfo.dataSizes = dataSizes.data();
    // descriptor in each set
    std::vector<uint32_t> descriptorCounts;
    getDescriptorInfoSequence("DescriptorSplitCountSequence", &descriptorCounts, modelSettings);
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    // descriptor type
    std::vector<VkDescriptorType> descriptorTypes;
    std::vector<VkShaderStageFlags> stageFlags;
    getDescriptorInfoTypeAndStage(&descriptorTypes, &stageFlags, modelSettings);
    descriptorInfo.pDescriptorType = descriptorTypes.data();
    descriptorInfo.stageFlags = stageFlags.data();

    // --- DESCRIPTOR BUFFERS
    // Create descriptors
    std::vector<ArBuffer> buffers(descriptorInfo.descriptorSetCount);
    // Create and fill buffers
    for (int j = 0; j < descriptorInfo.descriptorSetCount; ++j) {
        buffers[j].bufferSize = descriptorInfo.dataSizes[j];
        buffers[j].bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffers[j].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffers[j].bufferProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        buffer->createBuffer(&buffers[j]);
        arDescriptor.buffer.push_back(buffers[j].buffer);
        arDescriptor.bufferMemory.push_back(buffers[j].bufferMemory);

        // Copy over datasizes
        arDescriptor.dataSizes.resize(descriptorInfo.descriptorSetCount);
        arDescriptor.dataSizes[j] = descriptorInfo.dataSizes[j];
    }
    descriptors->createDescriptors(descriptorInfo, &arDescriptor);
}

void SceneObject::getDescriptorInfoSequence(const std::string &type, std::vector<uint32_t> *data,
                                            std::map<std::string, std::string> modelSettings) {
    std::stringstream bindingSequence(modelSettings.at(type));
    std::string intermediate;
    // Tokenizing w.r.t. space ' '
    while (getline(bindingSequence, intermediate, ' ')) {
        if (intermediate == "uboModel") {
            data->push_back(sizeof(uboModel));
        } else {
            data->push_back(std::stoi(intermediate));

        }
    }
}


void SceneObject::getDescriptorInfoTypeAndStage(std::vector<VkDescriptorType> *descriptorTypes, std::vector<VkShaderStageFlags>* stageFlags,
                                                std::map<std::string, std::string> modelSettings) {

    std::stringstream bindingSequence(modelSettings.at("pDescriptorType"));
    std::string intermediate;
    // Tokenizing w.r.t. space ' '
    while (getline(bindingSequence, intermediate, ' ')) {
        if (intermediate == "uniformBuffer") {
            descriptorTypes->push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }
    bindingSequence = std::stringstream(modelSettings.at("descriptorState"));
    while (getline(bindingSequence, intermediate, ' ')) {
        if (intermediate == "vertex") {
            stageFlags->push_back(VK_SHADER_STAGE_VERTEX_BIT);
        }
    }

}


void SceneObject::createMesh(std::string modelType) {

    // Create Mesh
    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;

    // ModelInfo
    ArModelInfo arModelInfo;
    arModelInfo.modelName = "standard/" + modelType + ".obj";
    arModelInfo.generateNormals = false;

    meshModel.loadModel(arEngine.mainDevice, arModel, arModelInfo);


}


void SceneObject::createPipeline(Pipeline pipeline, VkRenderPass renderPass) {
    arPipeline.device = arEngine.mainDevice.device;
    arPipeline.swapchainImageFormat = arEngine.swapchainFormat;
    arPipeline.swapchainExtent = arEngine.swapchainExtent;
    pipeline.arLightPipeline(renderPass, arDescriptor, shadersPath, &arPipeline);

}

const ArDescriptor &SceneObject::getArDescriptor() const {
    return arDescriptor;
}

const ArPipeline &SceneObject::getArPipeline() const {
    return arPipeline;
}


MeshModel SceneObject::getMeshModel() {
    return meshModel;
}
