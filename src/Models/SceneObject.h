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

    glm::vec3 posVector;
    glm::vec3 rotVector;
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

public:


    SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine);
    void createDescriptors(Descriptors *descriptors, Buffer *buffer);
    void createPipeline(Pipeline pipeline, VkRenderPass renderPass);
    void createMesh(std::string modelType);

    const ArDescriptor &getArDescriptor() const;
    const ArPipeline &getArPipeline() const;
    MeshModel getMeshModel();

    void getDescriptorInfo(std::map<std::string, std::string> modelSettings);

    static void getDescriptorInfoSequence(const std::string& type, std::vector<uint32_t> *data,
                                   std::map<std::string, std::string> modelSettings);

    void
    getDescriptorInfoDescriptorType(std::vector<VkDescriptorType> *data,
                                    std::map<std::string, std::string> modelSettings);

    void getDescriptorInfoTypeAndStage(std::vector<VkDescriptorType> *descriptorTypes,
                                       std::vector<VkShaderStageFlags> *stageFlags,
                                       std::map<std::string, std::string> modelSettings);

    void getSceneObjectPose(std::map<std::string, std::string> modelSettings);
};


#endif //AR_ENGINE_SCENEOBJECT_H
