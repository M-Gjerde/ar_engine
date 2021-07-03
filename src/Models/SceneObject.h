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
#include "MeshModel.h"

class SceneObject {

public:
    [[nodiscard]] const glm::mat4 &getModel() const;
    void setModel(const glm::mat4 &model);
    SceneObject(std::map<std::string, std::string> modelSettings, ArEngine mArEngine);
    void createPipeline(VkRenderPass renderPass);
    void createMesh(std::map<std::string, std::string> modelSettings);
    [[nodiscard]] const ArDescriptor &getArDescriptor() const;
    [[nodiscard]] const ArPipeline &getArPipeline() const;
    [[nodiscard]] bool refresh() const;
    bool isLight() const;
    void setRefresh(bool state);
    [[nodiscard]] VkBuffer getVertexBuffer() const;
    [[nodiscard]] VkBuffer getIndexBuffer() const;
    [[nodiscard]] uint32_t getIndexCount() const;
    void cleanUp(VkDevice device);

private:
    // Models params
    glm::mat4 model{};
    glm::vec3 posVector{};
    glm::vec3 rotVector{};
    glm::vec3 scaleVector{};
    float rotAngle{};
    bool refreshModel = true;
    bool lightSource = false;
    // Setup
    ArShadersPath shadersPath;



    // Descriptors
    ArDescriptorInfo descriptorInfo{};


    // Helper classes handles
    Descriptors *descriptors{};
    Buffer *buffer{};
    Pipeline pipeline;
    ArEngine arEngine;
    ArDescriptor arDescriptor;
    ArPipeline arPipeline{};
    MeshModel meshModel;



    void getSceneObjectPose(std::map<std::string, std::string> modelSettings);
    void getMiscProperties(std::map<std::string, std::string> map);
    void getDescriptorInfo(std::map<std::string, std::string> modelSettings);

    static void getDescriptorInfoSequence(const std::string &type, std::vector<uint32_t> *data,
                                          std::map<std::string, std::string> modelSettings);


    static void getDescriptorInfoTypeAndStage(std::vector<VkDescriptorType> *descriptorTypes,
                                       std::vector<VkShaderStageFlags> *stageFlags,
                                       std::map<std::string, std::string> modelSettings);

};


#endif //AR_ENGINE_SCENEOBJECT_H
