//
// Created by magnus on 4/25/21.
//

#include <string>
#include <map>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include "SceneObject.h"
#include "../include/structs.h"
#include "MeshModel.h"
#include "../Platform/LoadSettings.h"

// Create sceneobjects from file
SceneObject::SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine) {
    arEngine = std::move(mArEngine);
    // Helper classes handles
    descriptors = new Descriptors(arEngine);
    buffer = new Buffer(arEngine.mainDevice);

    // Create pipeline for object
    shadersPath.vertexShader = "../shaders/" + modelSettings.at("vertex_shader");
    shadersPath.fragmentShader = "../shaders/" + modelSettings.at("fragment_shader");

    // Create mesh from model type
    createMesh(modelSettings);
    // Get descriptorInfo from file
    getDescriptorInfo(modelSettings);
    // Get object position
    getSceneObjectPose(modelSettings);
    // misc properties (lightSource etc..)
    getMiscProperties(modelSettings);

    // Free pointer memory
    delete descriptors;
    delete buffer;


}

// Create template sceneobject
SceneObject::SceneObject(ArEngine mArEngine) {
    LoadSettings loadSettings("templates");

    auto modelSettings = loadSettings.getSceneObjects()[0];

    arEngine = std::move(mArEngine);
    // Helper classes handles
    descriptors = new Descriptors(arEngine);
    buffer = new Buffer(arEngine.mainDevice);

    // Create pipeline for object
    shadersPath.vertexShader = "../shaders/" + modelSettings.at("vertex_shader");
    shadersPath.fragmentShader = "../shaders/" + modelSettings.at("fragment_shader");

    // Create mesh from model type
    createMesh(modelSettings);
    // Get descriptorInfo from file
    getDescriptorInfo(modelSettings);
    // Get object position
    getSceneObjectPose(modelSettings);
    // misc properties (lightSource etc..)
    getMiscProperties(modelSettings);

    // Free pointer memory
    delete descriptors;
    delete buffer;


}

void SceneObject::getSceneObjectPose(std::map<std::string, std::string> modelSettings){
    // pose
    float posX = std::stof(modelSettings.at("posX"));
    float posY = std::stof(modelSettings.at("posY"));
    float posZ = std::stof(modelSettings.at("posZ"));
    float rotX = std::stof(modelSettings.at("rotX"));
    float rotY = std::stof(modelSettings.at("rotY"));
    float rotZ = std::stof(modelSettings.at("rotZ"));

    // Scaling
    float scaleX = std::stof(modelSettings.at("scaleX"));
    float scaleY = std::stof(modelSettings.at("scaleY"));
    float scaleZ = std::stof(modelSettings.at("scaleZ"));
    posVector = glm::vec3(posX, posY, posZ);
    rotVector = glm::vec3(rotX, rotY, rotZ);
    scaleVector = glm::vec3(scaleX, scaleY, scaleZ);
    rotAngle = std::stof(modelSettings.at("rotAngle"));
    model = glm::mat4(1.0f);
    model = glm::translate(model, posVector);
    model = glm::rotate(model, glm::radians(rotAngle), rotVector);
    model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

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
        } else if (intermediate == "FragmentColor"){
            data->push_back(sizeof(FragmentColor));
        }
        else {
            data->push_back(std::stoi(intermediate));

        }
    }
}


void SceneObject::getDescriptorInfoTypeAndStage(std::vector<VkDescriptorType> *descriptorTypes, std::vector<VkShaderStageFlags>* stageFlags,
                                                std::map<std::string, std::string> modelSettings) {

    std::stringstream bindingSequence(modelSettings.at("pDescriptorType"));
    std::string intermediate;
    // Descriptor types
    while (getline(bindingSequence, intermediate, ' ')) {
        if (intermediate == "uniformBuffer") {
            descriptorTypes->push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }
    // Descriptor binding stages
    bindingSequence = std::stringstream(modelSettings.at("descriptorState"));
    while (getline(bindingSequence, intermediate, ' ')) {
        if (intermediate == "vertex") {
            stageFlags->push_back(VK_SHADER_STAGE_VERTEX_BIT);
        } else if(intermediate == "fragment"){
            stageFlags->push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
        }
    }

}

void SceneObject::getMiscProperties(std::map<std::string, std::string> map) {
    std::string lightProperty = map.at("lightProperty");
    if (lightProperty == "lightSource")
       lightSource = true;

}

void SceneObject::createMesh(std::map<std::string, std::string> modelSettings) {

    // Create Mesh
    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;

    // ModelInfo
    ArModelInfo arModelInfo;
    arModelInfo.modelName = "standard/" + modelSettings.at("type") + ".obj";
    arModelInfo.generateNormals = (modelSettings.at("normals") == "true");

    meshModel.loadModel(arEngine.mainDevice, arModel, arModelInfo);


}


void SceneObject::createPipeline(VkRenderPass renderPass) {
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


const glm::mat4 &SceneObject::getModel() const {
    return model;
}

void SceneObject::setModel(const glm::mat4 &newModel) {
    model = newModel;
    setRefresh(true);
}

void SceneObject::setRefresh(bool state) {
    refreshModel = state;
}

VkBuffer SceneObject::getVertexBuffer() const {
    return meshModel.getVertexBuffer();
}

VkBuffer SceneObject::getIndexBuffer() const {
    return meshModel.getIndexBuffer();
}

uint32_t SceneObject::getIndexCount() const {
    return meshModel.indexCount;
}

void SceneObject::cleanUp(VkDevice device){
    meshModel.cleanUp(device);
}

bool SceneObject::refresh() const {
    return refreshModel;
}

bool SceneObject::isLight() const {
    return lightSource;
}





