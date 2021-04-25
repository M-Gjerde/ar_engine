//
// Created by magnus on 4/25/21.
//

#include <string>
#include <map>
#include <utility>
#include <glm/ext/matrix_transform.hpp>
#include "SceneObject.h"
#include "../include/structs.h"
#include "../pipeline/MeshModel.h"

SceneObject::SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine) {
    arEngine = std::move(mArEngine);
    // Create pipeline for object
    shadersPath.vertexShader = "../shaders/" + modelSettings.at("vertex_shader");
    shadersPath.fragmentShader = "../shaders/" + modelSettings.at("fragment_shader");

    createMesh();
}


void SceneObject::createMesh() {

    // Create Mesh
    ArModel arModel;
    arModel.transferCommandPool = arEngine.commandPool;
    arModel.transferQueue = arEngine.graphicsQueue;

    // ModelInfo
    ArModelInfo arModelInfo;
    arModelInfo.modelName = "standard/cube.obj";
    arModelInfo.generateNormals = false;

    meshModel.loadModel(arEngine.mainDevice, arModel, arModelInfo);

}

void SceneObject::createDescriptors(Descriptors *descriptors, Buffer *buffer) {

    // descriptor info
    ArDescriptorInfo descriptorInfo{};
    descriptorInfo.descriptorSetCount = 1;
    descriptorInfo.descriptorSetLayoutCount = 1;
    descriptorInfo.descriptorPoolCount = 1;
    std::array<uint32_t, 1> sizes = {sizeof(uboModel)};
    descriptorInfo.dataSizes = sizes.data();

    descriptorInfo.descriptorCount = 1;
    std::array<uint32_t, 1> descriptorCounts = {1};
    descriptorInfo.pDescriptorSplitCount = descriptorCounts.data();
    std::array<uint32_t, 1> bindings = {0};
    descriptorInfo.pBindings = bindings.data();
    std::array<VkDescriptorType, 1> types = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    descriptorInfo.pDescriptorType = types.data();
    std::array<VkShaderStageFlags, 1> stageFlags = {VK_SHADER_STAGE_VERTEX_BIT};
    descriptorInfo.stageFlags = stageFlags.data();

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

const MeshModel &SceneObject::getMeshModel() const {
    return meshModel;
}


