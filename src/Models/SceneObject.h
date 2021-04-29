//
// Created by magnus on 4/25/21.
//

#ifndef AR_ENGINE_SCENEOBJECT_H
#define AR_ENGINE_SCENEOBJECT_H


#include <vector>
#include "glm/glm.hpp"
#include "../include/structs.h"
#include "../pipeline/Descriptors.h"
#include "../pipeline/Buffer.h"
#include "../pipeline/MeshModel.h"

class SceneObject {

private:
    glm::mat4 model;
    glm::vec3 posVector;
    glm::vec3 rotVector;
    glm::vec3 scaleVector;

private:
    float rotAngle;
    ArShadersPath shadersPath;

    // Descriptors
    ArDescriptorInfo descriptorInfo{};


    ArEngine arEngine;
    ArDescriptor arDescriptor;
    ArPipeline arPipeline{};
    MeshModel meshModel;

    // Helper classes handles
    Descriptors *descriptors{};
    Buffer *buffer{};
    Pipeline pipeline;


public:

    const glm::mat4 &getModel() const;

    void setModel(const glm::mat4 &model);

    SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine);

    void createPipeline(VkRenderPass renderPass);

    void createMesh(std::map<std::string, std::string> modelSettings);

    const ArDescriptor &getArDescriptor() const;

    const ArPipeline &getArPipeline() const;

    MeshModel getMeshModel();

    void getDescriptorInfo(std::map<std::string, std::string> modelSettings);

    static void getDescriptorInfoSequence(const std::string &type, std::vector<uint32_t> *data,
                                          std::map<std::string, std::string> modelSettings);


    void getDescriptorInfoTypeAndStage(std::vector<VkDescriptorType> *descriptorTypes,
                                       std::vector<VkShaderStageFlags> *stageFlags,
                                       std::map<std::string, std::string> modelSettings);

    void getSceneObjectPose(std::map<std::string, std::string> modelSettings);

    VkBuffer getVertexBuffer() const;
    VkBuffer getIndexBuffer() const;
    uint32_t getIndexCount() const;
    const glm::vec3 &getScaleVector() const;

    void cleanUp(VkDevice device);
};


#endif //AR_ENGINE_SCENEOBJECT_H
