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
    float rotAngle;
    ArShadersPath shadersPath;

    ArEngine arEngine;
    ArDescriptor arDescriptor;
    ArPipeline arPipeline{};
    MeshModel meshModel;


public:


    SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine);
    void createDescriptors(Descriptors *descriptors, Buffer *buffer);
    void createPipeline(Pipeline pipeline, VkRenderPass renderPass);
    void createMesh();

    const ArDescriptor &getArDescriptor() const;
    const ArPipeline &getArPipeline() const;
    const MeshModel &getMeshModel() const;

};


#endif //AR_ENGINE_SCENEOBJECT_H
